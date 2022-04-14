/*************************************************************************
    > File Name: chatbox.c
    > Author: racle
    > Mail: racleray@qq.com
    > Created Time: 
 ************************************************************************/


#include "head.h"

#ifndef _CLIENT

extern struct chatbox_user *users;
extern int subefd1, subefd2;

void heart_beat(int signum) {
    struct chatbox_msg msg;
    msg.type = CHAT_HEART;
    DBG(GREEN"HEART BEAT SIGALRM\n"NONE);

    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].isOnline) {
            send(users[i].fd, (void *)&msg, sizeof(msg), 0);
            users[i].isOnline--;

            if (users[i].isOnline == 0) {
                int tmp_fd = users[i].sex ? subefd1 : subefd2;
                epoll_ctl(tmp_fd, EPOLL_CTL_DEL, users[i].fd, NULL);
                close(users[i].fd);
                DBG(RED"<Heart Beat Dead>"NONE" %s is removed because of heart beat error.\n", users[i].name);
            }
        }
    }
}

int add_to_reactor(int epollfd, int fd){
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET;
    make_nonblock(fd);

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        return -1;
    }
    return 0;
}

void send_all(struct chatbox_msg *msg) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].isOnline){
            send(users[i].fd, msg, sizeof(struct chatbox_msg), 0);
        }
    }
    return ;
}

void send_all_not_me(struct chatbox_msg *msg) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].isOnline && strcmp(users[i].name, msg->from)){
            send(users[i].fd, msg, sizeof(struct chatbox_msg), 0);
        }
    }
    return ;
}

void send_to(struct chatbox_msg *msg) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].isOnline && !strcmp(users[i].name, msg->to)) {
            send(users[i].fd, msg, sizeof(struct chatbox_msg), 0);
            break;
        }
    }
}

/*
 * Server sub reactor main logic
 * */
void *sub_reactor(void *arg) {
    int subfd = *(int *)arg;
    DBG(L_RED"<Sub Reactor>"NONE" : in sub reactor %d.\n", subfd);

    struct epoll_event ev, events[MAX_EVENTS];
    for (;;) {
        DBG(YELLOW"<in sub reactor loop: start>\n"NONE);

        sigset_t sigset;
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGALRM);

        int nfds = epoll_pwait(subfd, events, MAX_EVENTS, -1, &sigset);

        if (nfds < 0) {
            DBG(L_RED"<Sub Reactor Err> : sub reactor error %d.\n"NONE, subfd);
            continue;
        }
        for (int i = 0; i < nfds; i++) {
            
            int fd = events[i].data.fd;
            struct chatbox_msg msg;
            bzero(&msg, sizeof(msg));
            DBG(YELLOW"<in sub reactor loop: event before recv> : <%d, %d>\n"NONE, i, fd);
            
            int ret = recv(fd, (void *)&msg, sizeof(msg), 0);
            DBG(YELLOW"<in sub reactor loop: event after recv>\n"NONE);

            if (ret < 0 &&  !(errno & EAGAIN)) {
                close(fd);
                epoll_ctl(subfd, EPOLL_CTL_DEL, fd, NULL);
                users[fd].isOnline = 0;
                DBG(L_RED"<Sub Reactor Err> : connecttion of %d on %d is closed.\n"NONE, fd, subfd);
                continue;
            }

            if (ret != sizeof(msg)) {
                DBG(L_RED"<Sub Reactor Err> : msg size err <%ld, %d>.\n"NONE, sizeof(msg), ret);
                perror("recv");
                continue;
            }

            // active connecttion will reset isOnline state, used by heart beat test.
            users[fd].isOnline = 5;

            /* ======================================================================== */
            // actions for different message types.
            //
            if (msg.type & CHAT_WALL) {  // public chat
                show_msg(&msg);
                DBG(BLUE"%s : %s\n"NONE, msg.from, msg.msg);
                send_all(&msg);
            } else if((msg.type & CHAT_HEART) && (msg.type & CHAT_ACK)) {
                DBG(RED"  [Client response for heart beat test success.]\n"NONE);  // do nothing
            } else if (msg.type & CHAT_FIN) {
                msg.type = CHAT_SYS;
                sprintf(msg.msg, "你的好友 %s 下线了", msg.from);
                send_all_not_me(&msg);
                epoll_ctl(subfd, EPOLL_CTL_DEL, fd, NULL);
                close(fd);
                users[fd].isOnline = 0;
            } else if (msg.type & CHAT_MSG) {  // private chat
                send_to(&msg);
            } else {  // Other functions to be completed.
                DBG(PINK"%s : %s\n"NONE, msg.from, msg.msg);
            }
        }
    }
}

#endif


/*
 * client main logic
 * */
void *client_recv(void *arg) {
    int sockfd = *(int *)arg;
    struct chatbox_msg msg;
    while (1) {
        DBG("IN client_recv <%d>\n", sockfd);

        bzero(&msg, sizeof(msg));
        int ret = recv(sockfd, &msg, sizeof(msg), 0);
        DBG("After recv\n");

        if (ret <= 0) {
            DBG(RED"<Err>"NONE" : server closed connecttion.\n");
            perror("recv");
            exit(1);
        }

        // Heart beat from server.
        if (msg.type & CHAT_HEART) {
            struct chatbox_msg ack;
            ack.type = CHAT_ACK | CHAT_HEART;
            send(sockfd, (void *)&ack, sizeof(ack), 0);
            strcpy(msg.msg, "[Heart beat from Server]");
            show_msg(&msg);
        } else {
            show_msg(&msg);
        }
    }
}
