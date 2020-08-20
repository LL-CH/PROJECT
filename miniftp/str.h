#ifndef _STR_H_
#define _STR_H_

#include"common.h"
//处理接收命令：去除\r\n
void str_trim_crlf(char *str);
//分解命令 用户名
void str_split(const char *str, char *left, char *right, char c);
//大小写转换
void str_upper(char *str);

#endif /* _STR_H_ */