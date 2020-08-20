#ifndef _PRIV_SOCK_H_
#define _PRIV_SOCK_H_

#include"common.h"
#include"session.h"

//FTP服务进程向nobody进程请求的命令
#define PRIV_SOCK_GET_DATA_SOCK 1
#define PRIV_SOCK_PASV_ACTIVE 2
#define PRIV_SOCK_PASV_LISTEN 3
#define PRIV_SOCK_PASV_ACCEPT 4

//nobody 进程对FTP服务进程的应答
#define PRIV_SOCK_RESULT_OK 1
#define PRIV_SOCK_RESULT_BAD 2

//初始化套接字
void priv_sock_init(session_t *sess);
//关闭套接字
void priv_sock_close(session_t *sess);
//保存父进程上下文环境
void priv_sock_set_parent_context(session_t *sess);
//保存子进程上下文环境
void priv_sock_set_child_context(session_t *sess);
//发送字符命令
void priv_sock_send_cmd(int fd, char cmd);
//接收字符命令
char priv_sock_get_cmd(int fd);
//发送结果
void priv_sock_send_result(int fd, char res);
//获取结果
char priv_sock_get_result(int fd);
//发送整数命令
void priv_sock_send_int(int fd, int the_int);
//接收整数命令
int priv_sock_get_int(int fd);
//发送数据
void priv_sock_send_buf(int fd, const char *buf, unsigned int len);
//接收数据
void priv_sock_recv_buf(int fd, char *buf, unsigned int len);
//发送fd
void priv_sock_send_fd(int sock_fd, int fd);
//接收fd
int priv_sock_recv_fd(int sock_fd);

#endif /* _PRIV_SOCK_H_ */
