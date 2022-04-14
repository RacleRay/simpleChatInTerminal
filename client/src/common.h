/*************************************************************************
    > File Name: common.h
    > Author: racle
    > Mail: racleray@qq.com
    > Created Time:
 ************************************************************************/

#ifndef _COMMON_H
#define _COMMON_H

int socket_create(int port);
int socket_connect(const char* ip, int port);
int make_nonblock(int fd);
int make_block(int fd);
int get_config(const char* file, const char* key, char* config);

#endif
