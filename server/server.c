/*************************************************************************
    > File Name: server.c
    > Author: racle
    > Mail: racleray@qq.com
    > Created Time:
 ************************************************************************/

#include "./src/head.h"

const char* conf = "./chatbox_server.conf";

struct chatbox_user *users;
int epollfd, subefd1, subefd2, cur_max_fds = 5;
char *data[2000];

int main(int argc, char** argv) {

    /*====================================================================*/
    // Read cammand line
    //
    int opt, port = 0;
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
        case 'p':
            port = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Usage: %s -p PORT.\n", argv[0]);
            exit(1);
        }
    }

    /*====================================================================*/
    // Load config
    //
    // check config file
    if (access(conf, R_OK)) {
        fprintf(stderr, RED "config file error\n" NONE);
        exit(1);
    }

    // read config File
    char config[512] = {0};
    get_config(conf, "PORT", config);
    if (!port) {
        // Error
        port = atoi(config);
        DBG(GREEN "port is %d\n" NONE, port);
    }

    int server_listen, sockfd;
    if ((server_listen = socket_create(port)) < 0) {
        perror("socket create");
        exit(1);
    }

    DBG(YELLOW "listen port: %d. \n", port);

    /*====================================================================*/
    // reactor resourses allcate
    struct itimerval itv;
    itv.it_interval.tv_sec = 10;
    itv.it_interval.tv_usec = 0;
    itv.it_value.tv_sec = 10;
    itv.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &itv, NULL);

    signal(SIGALRM, heart_beat);

    /*====================================================================*/
    // reactor resourses allcate
    //
    users = (chatbox_user*)calloc(MAX_USERS, sizeof(chatbox_user));
    if ((epollfd = epoll_create(1)) < 0) {
        perror("epoll create");
        exit(2);
    }
    if ((subefd1 = epoll_create(1)) < 0) {
        perror("epoll create");
        exit(2);
    }
    if ((subefd2 = epoll_create(1)) < 0) {
        perror("epoll create");
        exit(2);
    }

    DBG(YELLOW ">>> Main reactor and two sub reactors created \n" NONE);

    pthread_t tid1, tid2;
    pthread_create(&tid1, NULL, sub_reactor, (void*)&subefd1);
    pthread_create(&tid2, NULL, sub_reactor, (void*)&subefd2);

    usleep(100000);
    DBG(YELLOW ">>> Sub reactor threads created.\n" NONE);


    /*====================================================================*/
    // main reactor running
    // - listen to client connections
    struct epoll_event events[MAX_EVENTS], ev;
    ev.data.fd = server_listen;
    ev.events  = EPOLLIN;

    epoll_ctl(epollfd, EPOLL_CTL_ADD, server_listen, &ev);

    for (;;) {
        sigset_t sigset;
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGALRM);

        // epoll_pwait
        int nfds = epoll_pwait(epollfd, events, MAX_EVENTS, -1, &sigset);
        if (nfds < 0) {
            perror("epoll wait");
            exit(1);
        }

        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;
            if (fd == server_listen && events[i].events & EPOLLIN) {
                if ((sockfd = accept(server_listen, NULL, NULL)) < 0) {
                    perror("accept");
                    exit(1);
                }
                // acceptor works
                ev.data.fd = sockfd;
                ev.events  = EPOLLIN;
                epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev);
            }
            else {
                // receive data and check
                chatbox_msg msg;
                bzero(&msg, sizeof(msg));
                int ret = recv(fd, (void*)&msg, sizeof(msg), 0);

                if (ret <= 0) {
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                    continue;
                }
                if (ret != sizeof(msg)) {
                    DBG(RED "<MSG Error>" NONE ": msg size error!\n");
                    continue;
                }
                
                // check message type
                if (msg.type & CHAT_SIGNUP) {
                    // sign up: success or not.
                    msg.type = CHAT_ACK;
                    send(fd, (void*)&msg, sizeof(msg), 0);
                }
                else if (msg.type & CHAT_SIGNIN) {
                    // sign in
                    msg.type = CHAT_ACK;
                    strcpy(msg.msg, "你已登录成功!\n");
                    send(fd, (void*)&msg, sizeof(msg), 0);

                    msg.type = CHAT_SYS;
                    sprintf(msg.msg, " 你的好友 %s 上线了 ", msg.from);
                    strcpy(users[fd].name, msg.from);
                    users[fd].fd       = fd;
                    users[fd].isOnline = 5;
                    users[fd].sex      = msg.sex;

                    cur_max_fds = cur_max_fds < fd ? fd : cur_max_fds;

                    // load balance (simple): by gender
                    int which = msg.sex ? subefd1 : subefd2;
                    // remove fd from acceptor epoll.
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
                    // add to sub reactor to deal with client.
                    add_to_reactor(which, fd);
                    send_all_not_me(&msg);
                }
            }
        }
    }

    return 0;
}
