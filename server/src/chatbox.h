/*************************************************************************
    > File Name: chatbox.h
    > Author: racle
    > Mail: racleray@qq.com
    > Created Time:
 ************************************************************************/

#ifndef _CHATBOX_H
#define _CHATBOX_H

#define MAX_EVENTS 5
#define MAX_USERS 1024

#define CHAT_SIGNUP 0x01
#define CHAT_SIGNIN 0x02
#define CHAT_SIGNOUT 0x04

#define CHAT_ACK 0x08
#define CHAT_NAK 0x10
#define CHAT_SYS 0x20
#define CHAT_WALL 0x40
#define CHAT_MSG 0x80
#define CHAT_FIN 0x100

#define CHAT_HEART 0x200

typedef struct chatbox_user {
    char name[50];
    char passwd[20];
    int  sex;
    int  fd;
    int  isOnline;
} chatbox_user;

typedef struct chatbox_msg {
    int  type;
    int  sex;
    char from[50];
    char to[50];
    char msg[512];
} chatbox_msg;

void  heart_beat(int signum);
void* sub_reactor(void* arg);
void* client_recv(void* arg);
int   add_to_reactor(int efd, int fd);
void  send_all(struct chatbox_msg*);
void  send_all_not_me(struct chatbox_msg* msg);
void  send_to(struct chatbox_msg* msg);

#endif
