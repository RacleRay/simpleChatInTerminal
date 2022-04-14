/*************************************************************************
    > File Name: client.c
    > Author: racle
    > Mail: racleray@qq.com
    > Created Time: 
 ************************************************************************/


#include "./src/head.h"
#include <strings.h>

const char *config = "./chatbox_client.conf";
WINDOW *msg_win, *sub_msg_win, *info_win, *sub_info_win, *input_win, *sub_input_win;
int sockfd;
char name[50];

// SIGINT handler
void logout(int signum) {
    struct chatbox_msg msg;
    msg.type = CHAT_FIN;
    strcpy(msg.from, name);
    send(sockfd, (void *)&msg, sizeof(msg), 0);
    close(sockfd);
    printf(YELLOW"Bye!\n"NONE);
    exit(0);
}

int main(int argc, char **argv) {
    int opt, server_port = 0, sex = -1, mode = 0;
    char server_ip[20] = {0};
    while ((opt = getopt(argc, argv, "p:h:s:n:m:")) != -1) {
        switch (opt) {
            case 'p':
                server_port = atoi(optarg);
                break;
            case 's':
                sex = atoi(optarg);
                break;
            case 'm':
                mode = atoi(optarg);
                break;
            case 'h':
                strcpy(server_ip, optarg);
                break;
            case 'n':
                strcpy(name, optarg);
                break;
            default:
                fprintf(stderr, "Usage : %s -p port -n name -s sex -h server_ip -m [1 for sign in, 0 for signup].\n", argv[0]);
                exit(1);
        }
    }

    if (access(config, R_OK)) {
        fprintf(stderr, RED"<Error>"NONE" : config file error!\n");
        exit(1);
    }

    char tmp[512] = {0};
    if (!strlen(server_ip)){
        get_config(config, "SERVER_IP", tmp);
        strcpy(server_ip, tmp);
        bzero(tmp, sizeof(tmp));
    }
    if (!server_port) {
        get_config(config, "SERVER_PORT", tmp);
        server_port = atoi(tmp);
        bzero(tmp, sizeof(tmp));
    }
    if (sex < 0) {
        get_config(config, "SEX", tmp);
        sex = atoi(tmp);
        bzero(tmp, sizeof(tmp));
    }
    if (!strlen(name)) {
        get_config(config, "NAME", tmp);
        strcpy(name, tmp);
        bzero(tmp, sizeof(tmp));
    } 
    DBG(YELLOW"<D>"NONE" : read config success.\n");

    #ifdef UI
    setlocale(LC_ALL, "");
    init_ui();
    #endif

    DBG(BLUE"<D>"NONE" : server_ip = %s\n", server_ip);
    DBG(BLUE"<D>"NONE" : server_port = %d\n", server_port);

    if ((sockfd = socket_connect(server_ip, server_port)) < 0) {
        perror("socket_connect");
        exit(1);
    }
    DBG(YELLOW"<D>"NONE" : connecttion to server %s:%d sockfd <%d> successfully.\n", server_ip, server_port, sockfd);

    signal(SIGINT, logout);

    struct chatbox_msg msg;
    bzero(&msg, sizeof(msg));
    strcpy(msg.from, name);
    msg.sex = sex;

    if (mode == 0) {
        msg.type = CHAT_SIGNUP;
    } else {
        msg.type = CHAT_SIGNIN;
    }
    send(sockfd, (void *)&msg, sizeof(msg), 0);


    // check connecttion timeout
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(sockfd, &rfds);
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    if (select(sockfd + 1, &rfds, NULL, NULL, &tv) <= 0) {
        fprintf(stderr, RED"<SystemErr>"NONE" : server is out of service,\n");
        exit(1);
    }

    bzero(&msg, sizeof(msg));
    int ret = recv(sockfd, (void *)&msg, sizeof(msg), 0);
    if (ret <= 0) {
        fprintf(stderr, RED"<SystemErr>"NONE" : Server closed connecttion.\n");
        exit(1);
    }
    if (msg.type & CHAT_ACK) {
        DBG(GREEN"<Success>"NONE" : server return a success.\n");
        if (!mode) {
            printf(GREEN"Please login after this.\n"NONE);
            exit(0);
        }
    } else {
        DBG(RED"<Failure>"NONE" : server return a failure.\n");
        exit(1);
    }

    DBG(BLUE"<D>"NONE" : login success!\n");

    pthread_t tid;
    pthread_create(&tid, NULL, client_recv, (void *)&sockfd);

    while (1) {
        bzero(&msg.msg, sizeof(msg.msg));
        
        // ncurse
        echo();
        nocbreak();
        mvwscanw(input_win, 2, 1, "%[^\n]", msg.msg);

        msg.type = CHAT_WALL;

        // simple private chat check
        if (msg.msg[0] == '@') {
            if (!strstr(msg.msg, " ")) {
                strcpy(msg.msg, "输入格式有误!");
                show_msg(&msg);
                continue;
            }
            msg.type = CHAT_MSG;
            // extract the name you want send to
            strncpy(msg.to, msg.msg + 1, strchr(msg.msg, ' ') - msg.msg -1);
        }

        if (!strlen(msg.msg)) continue;

        send(sockfd, (void *)&msg, sizeof(msg), 0);

        wclear(input_win);
        box(input_win, 0, 0);
        noecho();
        cbreak();
    }

    return 0;
}
