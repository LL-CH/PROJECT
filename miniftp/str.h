#ifndef _STR_H_
#define _STR_H_

#include"common.h"
//����������ȥ��\r\n
void str_trim_crlf(char *str);
//�ֽ����� �û���
void str_split(const char *str, char *left, char *right, char c);
//��Сдת��
void str_upper(char *str);

#endif /* _STR_H_ */