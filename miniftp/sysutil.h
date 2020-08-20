#ifndef _SEESION_H_
#define _SEESION_H_

#include"common.h"

//���������
int tcp_server(const char *host,unsigned short port);
//�����ͻ���
int tcp_client();

//��ȡ�ļ�Ȩ��
const char* statbuf_get_perms(struct stat* sbuf);
//��ȡ�ļ�ʱ��
const char* statbuf_get_date(struct stat* sbuf);

//����fd
void send_fd(int sock_fd, int fd);
//����fd
int recv_fd(const int sock_fd);

//��ȡ����ip��ַ
void getlocalip(char *ip);

//ȡ��
long get_time_sec();
//΢��
long get_time_usec();

//˯��
void nano_sleep(double sleep_time);
#endif /*_SEESION_H_*/