#include"parseconf.h"
#include"tunable.h"

//bool值配置项
static struct parseconf_bool_setting
{
	const char *p_setting_name; //配置项的名字
	int        *p_variable;     //配置项的值
} 
parseconf_bool_array[] = 
{
	{"pasv_enable", &tunable_pasv_enable},
	{"port_enable", &tunable_port_enable},
	{NULL, NULL}
};

//int型配置项
static struct parseconf_uint_setting
{
	const char   *p_setting_name; //配置项的名字
	unsigned int *p_variable;     //配置项的值
} 
parseconf_uint_array[] =
{
	{"listen_port", &tunable_listen_port},
	{"max_clients", &tunable_max_clients},
	{"max_per_ip",  &tunable_max_per_ip},
	{"accept_timeout", &tunable_accept_timeout},
	{"connect_timeout", &tunable_connect_timeout},
	{"idle_session_timeout", &tunable_idle_session_timeout},
	{"data_connection_timeout", &tunable_data_connection_timeout},
	{"local_umask", &tunable_local_umask},
	{"upload_max_rate", &tunable_upload_max_rate},
	{"download_max_rate", &tunable_download_max_rate},
	{NULL, NULL}
};

//字符型配置项
static struct parseconf_str_setting
{
	const char   *p_setting_name; //配置项的名字
	const char  **p_variable;     //配置项的值
} 
parseconf_str_array[] = 
{
	{"listen_address", &tunable_listen_address},
	{NULL, NULL}
};

//读取配置文件信息
void parseconf_load_file(const char *path){
	//以只读的方式打开文件
	FILE *fp = fopen(path, "r");
	if(NULL == fp)
		ERR_EXIT("parseconf_load_file");

	char setting_line[MAX_SETTING_LINE] = {0};
	//从fp中读取数据
	while(fgets(setting_line, MAX_SETTING_LINE, fp) != NULL){
		//注释文件和默认选项跳过
		if(strlen(setting_line)==0 || setting_line[0]=='#')
			continue;
		str_trim_crlf(setting_line);
		parseconf_load_setting(setting_line);
		memset(setting_line, 0, MAX_SETTING_LINE);
	}

	fclose(fp);
}

//pasv_enable=1 解析配置文件信息
void parseconf_load_setting(const char *setting){
	char key[MAX_KEY_VALUE_SIZE] = {0};
	char value[MAX_KEY_VALUE_SIZE] = {0};
	//分割字符串
	str_split(setting, key, value, '=');

	//解析字符型
	const struct parseconf_str_setting *p_str_setting = parseconf_str_array;
	while(p_str_setting->p_setting_name != NULL){
		//找到了配置文件
		if(strcmp(key, p_str_setting->p_setting_name) == 0){
			//获取配置的值
			const char **p_cur_setting = p_str_setting->p_variable;

			if(*p_cur_setting)
				free((char*)*p_cur_setting);
			//复制value的值 自动开辟空间
			*p_cur_setting = strdup(value);
			return;
		}
		p_str_setting++;
	}

	//解析bool型
	const struct parseconf_bool_setting *p_bool_setting = parseconf_bool_array;
	while(p_bool_setting->p_setting_name != NULL){
		if(strcmp(key, p_bool_setting->p_setting_name) == 0){
			//字母转换为小写
			str_upper(value);
			if(strcmp(value, "YES") == 0)
				*(p_bool_setting->p_variable) = 1;
			else if(strcmp(value, "NO") == 0)
				*(p_bool_setting->p_variable) = 0;
			else{
				fprintf(stderr, "bad bool value in config file for : %s\n", key);
				exit(EXIT_FAILURE);
			}
			return;
		}
		p_bool_setting++;
	}
	//解析int型
	const struct parseconf_uint_setting *p_uint_setting = parseconf_uint_array;
	while(p_uint_setting->p_setting_name != NULL){
		if(strcmp(key, p_uint_setting->p_setting_name) == 0){
			//不为默认值
			if(value[0] != '0')
				*(p_uint_setting->p_variable) = atoi(value);
			return;
		}
		p_uint_setting++;
	}
}
