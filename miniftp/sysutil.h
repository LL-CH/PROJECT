#ifndef _SEESION_H_
#define _SEESION_H_

#include"common.h"

//创建服务端
int tcp_server(const char *host,unsigned short port);
//创建客户端
int tcp_client();

//获取文件权限
const char* statbuf_get_perms(struct stat* sbuf);
//获取文件时间
const char* statbuf_get_date(struct stat* sbuf);

//发送fd
void send_fd(int sock_fd, int fd);
//接收fd
int recv_fd(const int sock_fd);

//获取本地ip地址
void getlocalip(char *ip);

//取秒
long get_time_sec();
//微秒
long get_time_usec();

//睡眠
void nano_sleep(double sleep_time);
#endif /*_SEESION_H_*/