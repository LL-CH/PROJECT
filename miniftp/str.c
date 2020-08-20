#include"str.h"

//去除命令末尾的\r\n
void str_trim_crlf(char* str){
	assert(str!=NULL);
	char* p=str+(strlen(str)-1);
	while(*p=='\n' || *p=='\r')
		*p--='\0';
}

//通过字符c将str分为左右两部分
void str_split(const char *str, char *left, char *right, char c){
	assert(str!=NULL);
	char* pos=strchr(str,c);
	if(pos==NULL)
		strcpy(left,str);
	else{
		strncpy(left,str,pos-str);
		strcpy(right,pos+1);
	}
}

//大小写转换
void str_upper(char *str)
{
	while(*str)
	{
		if(*str>='a' && *str<='z')
			*str -= 32;
		str++;
	}
}