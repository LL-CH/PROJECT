#include"str.h"

//ȥ������ĩβ��\r\n
void str_trim_crlf(char* str){
	assert(str!=NULL);
	char* p=str+(strlen(str)-1);
	while(*p=='\n' || *p=='\r')
		*p--='\0';
}

//ͨ���ַ�c��str��Ϊ����������
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

//��Сдת��
void str_upper(char *str)
{
	while(*str)
	{
		if(*str>='a' && *str<='z')
			*str -= 32;
		str++;
	}
}