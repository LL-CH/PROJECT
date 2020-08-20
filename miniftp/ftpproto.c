#include"ftpproto.h"
#include"ftpcodes.h"
#include"str.h"
#include"sysutil.h"
#include"privsock.h"
#include"tunable.h"

session_t *p_sess;


void ftp_reply(session_t* sess,int code,const char* text){
	char buf[MAX_BUFFER_SIZE]={0};
	sprintf(buf,"%d %s\r\n",code,text);
	send(sess->ctrl_fd,buf,strlen(buf),0);
}

//命令映射机制////////////////////////////////////////////////////////
static void do_user(session_t *sess);
static void do_pass(session_t *sess);
static void do_syst(session_t *sess);
static void do_feat(session_t *sess);
static void do_pwd(session_t *sess);
static void do_type(session_t *sess);
static void do_port(session_t *sess);
static void do_pasv(session_t *sess);
static void do_list(session_t *sess);
static void do_cwd (session_t *sess);
static void do_mkd (session_t *sess);
static void do_rmd (session_t *sess);
static void do_rnfr(session_t *sess);
static void do_rnto(session_t *sess);
static void do_size(session_t *sess);
static void do_dele(session_t *sess);
static void do_stor(session_t *sess);
static void do_retr(session_t *sess);
static void do_rest(session_t *sess);
static void do_quit(session_t *sess);

//ftpcmd命令结构体
typedef struct ftpcmd{
	const char *cmd;//命令
	void(*cmd_handler)(session_t *sess);//命令执行函数
}ftpcmd_t;

//命令映射表
static ftpcmd_t ctrl_cmds[] = 
{
	{"USER",	do_user},
	{"PASS",	do_pass},
	{"SYST",	do_syst},
	{"FEAT",	do_feat},
	{"PWD",		do_pwd},
	{"TYPE",	do_type},
	{"PORT",	do_port},
	{"PASV",	do_pasv},
	{"LIST",	do_list},
	{"CWD" ,	do_cwd },
	{"MKD",		do_mkd },
	{"RMD",		do_rmd },
	{"RNFR",	do_rnfr},
	{"RNTO",	do_rnto},
	{"SIZE",	do_size},
	{"DELE",	do_dele},
	{"STOR",	do_stor},
	{"RETR",	do_retr},
	{"REST",	do_rest},
	{"QUIT",	do_quit}
};

////////////////////////////////////////////////////////////////////////

//空闲断开
void start_data_alarm();

//控制连接超时
void handle_ctrl_timeout(int sig)
{
	shutdown(p_sess->ctrl_fd, SHUT_RD);
	ftp_reply(p_sess, FTP_IDLE_TIMEOUT, "Timeout.");
	shutdown(p_sess->ctrl_fd, SHUT_WR);
	exit(EXIT_SUCCESS);
	//close(p_sess->ctrl_fd);
}

//开始连接计时
void start_cmdio_alarm()
{
	//设定了会话超时时间
	if(tunable_idle_session_timeout > 0)
	{
		//处理超时信号
		signal(SIGALRM, handle_ctrl_timeout);
		alarm(tunable_idle_session_timeout); //启动闹钟
	}
}

//数据连接超时
void handle_data_timeout(int sig)
{
	if(!p_sess->data_process)
	{
		ftp_reply(p_sess, FTP_DATA_TIMEOUT, "Data timeout. Reconnect Sorry.");
		exit(EXIT_FAILURE);
	}
	p_sess->data_process = 0;
	start_data_alarm();
}

//数据连接计时
void start_data_alarm()
{
	if(tunable_data_connection_timeout > 0)
	{
		signal(SIGALRM, handle_data_timeout);
		alarm(tunable_data_connection_timeout);  //启动闹钟
	}
	else if(tunable_idle_session_timeout > 0)
		alarm(0);
}

////////////////////////////////////////////////////////////////////////

//ftp 服务进程
void handle_child(session_t *sess){
	ftp_reply(sess,FTP_GREET,"(miniftp 1.0)");
	
	int ret;
	while(1){
		//等待客户端的命令并进行处理
		memset(sess->cmdline, 0, MAX_COMMAND_LINE);
		memset(sess->cmd, 0, MAX_COMMAND);
		memset(sess->arg, 0, MAX_ARG);
		
		//开始计时
		start_cmdio_alarm();

		ret = recv(sess->ctrl_fd, sess->cmdline, MAX_COMMAND_LINE, 0);
		if(ret == -1)
			ERR_EXIT("readline");
		else if(ret == 0)
			exit(EXIT_SUCCESS);
		//处理\r\n	
		str_trim_crlf(sess->cmdline);
		//以空格为标志将str分为左右两部分
		str_split(sess->cmdline, sess->cmd, sess->arg, ' ');
		
		//已有命令的数量
		int table_size=sizeof(ctrl_cmds)/sizeof(ftpcmd_t);
		//遍历查找所得到的命令，并执行
		int i=0;
		for(i;i<table_size;i++){
			if(strcmp(sess->cmd,ctrl_cmds[i].cmd)==0){
				//命令的执行函数存在 则执行
				if(ctrl_cmds[i].cmd_handler!=NULL)
					ctrl_cmds[i].cmd_handler(sess);
				else
					ftp_reply(sess,FTP_COMMANDNOTIMPL,"Unimplement command.");
				break;
			}
		}

		if(i>=table_size)
			ftp_reply(sess, FTP_BADCMD, "Unknown command.");
	}
}

//登录时 USER ab
static void do_user(session_t *sess)
{
	struct passwd *pwd = getpwnam(sess->arg);
	if(pwd != NULL)
		sess->uid = pwd->pw_uid;

	ftp_reply(sess, FTP_GIVEPWORD, "Please specify the password.");
}

//PASS 123abc
static void do_pass(session_t *sess)
{
	//鉴权 获取登录密码进行比对
	//返回指向包含在密码数据库中记录与用户ID uid匹配的记录
	struct passwd *pwd = getpwuid(sess->uid);
	if(pwd == NULL)
	{
		ftp_reply(sess, FTP_LOGINERR, "Login incorrect.");
		return;
	}
	//返回本地密码数据库与用户名匹配
	struct spwd *spd = getspnam(pwd->pw_name);
	if(spd == NULL)
	{
		ftp_reply(sess, FTP_LOGINERR, "Login incorrect.");
		return;
	}
	//进行对比
	//crypt 密码加密函数
	char *encry_pwd = crypt(sess->arg, spd->sp_pwdp);
	if(strcmp(encry_pwd, spd->sp_pwdp) != 0)
	{
		ftp_reply(sess, FTP_LOGINERR, "Login incorrect.");
		return;
	}
	
	setegid(pwd->pw_gid);
	seteuid(pwd->pw_uid);
	chdir(pwd->pw_dir);

	ftp_reply(sess, FTP_LOGINOK, "Login successful.");
}

static void do_syst(session_t *sess)
{
	ftp_reply(sess, FTP_SYSTOK, "UNIX Type: L8");
}

//返回功能列表
static void do_feat(session_t *sess){
	send(sess->ctrl_fd, "211-Features:\r\n" ,strlen("211-Features:\r\n"), 0);
	send(sess->ctrl_fd, " EPRT\r\n", strlen(" EPRT\r\n"), 0);
	send(sess->ctrl_fd, " EPSV\r\n", strlen(" EPSV\r\n"), 0);
	send(sess->ctrl_fd, " MDTM\r\n", strlen(" MDTM\r\n"), 0);
	send(sess->ctrl_fd, " PASV\r\n", strlen(" PASV\r\n"), 0);
	send(sess->ctrl_fd, " REST STREAM\r\n", strlen(" REST STREAM\r\n"), 0);
	send(sess->ctrl_fd, " SIZE\r\n", strlen(" SIZE\r\n"), 0);
	send(sess->ctrl_fd, " TVFS\r\n", strlen(" TVFS\r\n"), 0);
	send(sess->ctrl_fd, " UTF8\r\n", strlen(" UTF8\r\n"), 0);
	send(sess->ctrl_fd, "211 End\r\n", strlen("211 End\r\n"), 0);
}

//返回用户当前所在目录
static void do_pwd (session_t *sess){
	char buf[MAX_BUFFER_SIZE]={0};
	//获取当前目录列表  /home/ll
	getcwd(buf,MAX_BUFFER_SIZE);
	char ret[MAX_BUFFER_SIZE]={0};
	//输出格式  "/home/ll"
	sprintf(ret,"\"%s\"",buf);
	ftp_reply(sess,FTP_PWDOK,ret);
}

//确定文件传输方式 'A'ascii码  'I'二进制
static void do_type(session_t *sess){
	//TYPE A  /  TYPE I
	if(strcmp(sess->arg,"A")==0){
		sess->is_ascii=1;
		ftp_reply(sess, FTP_TYPEOK, "Switching to ASCII mode.");
	}
	else if(strcmp(sess->arg, "I") == 0){
		sess->is_ascii = 0;
		ftp_reply(sess, FTP_TYPEOK, "Switching to Binary mode.");
	}
	else{
		ftp_reply(sess, FTP_BADCMD, "Unrecognised TYPE command.");
	}
}

//主动连接方式
static void do_port(session_t *sess){
	//PORT 192,168,174,1,31,120
	unsigned int v[6]={0};
	sscanf(sess->arg,"%u,%u,%u,%u,%u,%u", &v[0],&v[1],&v[2],&v[3],&v[4],&v[5]);
	
	//开辟数据连接地址空间
	sess->port_addr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
	//取数据连接地址的端口号 7,34
	unsigned char *p = (unsigned char *)&sess->port_addr->sin_port;
	p[0] = v[4];
	p[1] = v[5];
	//取ip地址 192,168,232,1
	p = (unsigned char *)&sess->port_addr->sin_addr;
	p[0] = v[0];
	p[1] = v[1];
	p[2] = v[2];
	p[3] = v[3];

	sess->port_addr->sin_family = AF_INET;
	ftp_reply(sess, FTP_PORTOK, "command successful. Consider using PASV.");
}

static void do_pasv(session_t* sess)
{
	char ip[16] = "192.168.174.128"; //服务器的IP
	priv_sock_send_cmd(sess->child_fd, PRIV_SOCK_PASV_LISTEN);
	unsigned short port = (unsigned short)priv_sock_get_int(sess->child_fd);

	int v[4] = {0};
	sscanf(ip, "%u.%u.%u.%u", &v[0], &v[1], &v[2], &v[3]);
	char msg[MAX_BUFFER_SIZE] = {0};
	sprintf(msg, "Entering Passive Mode (%u,%u,%u,%u,%u,%u).", v[0],v[1],v[2],v[3], port>>8, port&0x00ff);
	ftp_reply(sess, FTP_PASVOK, msg);
}

//判断主动连接是否激活
int port_active(session_t* sess){
	if(sess->port_addr){
		if(pasv_active(sess)){
			fprintf(stderr, "both port and pasv are active.");
			exit(EXIT_FAILURE);
		}
		return 1;
	}
	return 0;
}

//判断被动连接是否激活
int pasv_active(session_t *sess)
{
	//发送判断激活命令给nobody进程
	priv_sock_send_cmd(sess->child_fd, PRIV_SOCK_PASV_ACTIVE);
	//接收结果，如果激活了
	if(priv_sock_get_int(sess->child_fd))
	{
		//判断主动连接是否关闭，没有则报错
		if(port_active(sess))
		{
			fprintf(stderr, "both port and pasv are active.");
			exit(EXIT_FAILURE);
		}
		return 1;
	}
	return 0; 
}

//获取主动连接的fd
static int port_get_fd(session_t* sess){
	int ret = 1;
	//ftp 向 nobody 发送获取主动连接套接字命令
	priv_sock_send_cmd(sess->child_fd, PRIV_SOCK_GET_DATA_SOCK);
	//转换端口号和ip地址
	unsigned short port = ntohs(sess->port_addr->sin_port);
	char *ip = inet_ntoa(sess->port_addr->sin_addr);

	//发送port 和 ip
	priv_sock_send_int(sess->child_fd, (int)port);
	priv_sock_send_buf(sess->child_fd, ip, strlen(ip));
	
	//接收处理结果
	char res = priv_sock_get_result(sess->child_fd);
	//失败
	if(res == PRIV_SOCK_RESULT_BAD)
		ret = 0;
	//成功
	else if(res == PRIV_SOCK_RESULT_OK)
		//接收创建好的套接字
		sess->data_fd = priv_sock_recv_fd(sess->child_fd);
	return ret;
}

//获取被动连接的fd
static int pasv_get_fd(session_t* sess){
	int ret = 1;
	//发送获取被动连接接收套接字命令
	priv_sock_send_cmd(sess->child_fd, PRIV_SOCK_PASV_ACCEPT);
	//接收命令
	char res = priv_sock_get_result(sess->child_fd);
	if(res == PRIV_SOCK_RESULT_BAD)
		ret = 0;
	else if(res == PRIV_SOCK_RESULT_OK)
		//接收套接字
		sess->data_fd = priv_sock_recv_fd(sess->child_fd);
	
	return ret;
}

//返回建立数据连接的状态
int get_transfer_fd(session_t *sess)
{
	//主动连接与被动连接都没有激活
	if(!port_active(sess) && !pasv_active(sess))
	{
		ftp_reply(sess, FTP_BADSENDCONN,"Use PORT or PASV first.");
		return 0;
	}
	
	int ret = 1;
	//port 主动连接激活
	if(port_active(sess))
	{
		//失败
		if(!port_get_fd(sess))
			ret=0;
	}
	//pasv 被动连接激活
	if(pasv_active(sess))
	{
		//失败
		if(!pasv_get_fd(sess))
			ret = 0;	
	}
	
	//释放空间，防止内存泄漏
	if(sess->port_addr)
	{
		free(sess->port_addr);
		sess->port_addr = NULL;
	}
	
	//开启数据空闲断开的闹钟
	if(ret)
		start_data_alarm();


	return ret;
}

//显示当前用户文件下的文件列表
static void list_common(session_t *sess)
{
	DIR *dir = opendir(".");//打开当前目录
	if(dir == NULL)
		return;

	//drwxr-xr-x    3 1000     1000           30 Sep 09  2019 Desktop
	char buf[MAX_BUFFER_SIZE] = {0};

	struct stat sbuf; //用于保存文件的属性
	struct dirent *dt; 
	//readdir 读取文件目录项
	while((dt = readdir(dir)) != NULL)
	{
		//d_name 文件名
		//stat（）检索路径名指向的文件的信息
		if(stat(dt->d_name, &sbuf) < 0)
			continue;
		if(dt->d_name[0] == '.')  //过滤掉隐藏文件
			continue;

		memset(buf, MAX_BUFFER_SIZE, 0);
		//先组合权限
		const char *perms = (const char*)statbuf_get_perms(&sbuf);  //drwxr-xr-x
		int offset = 0;
		offset += sprintf(buf, "%s", perms);
		offset += sprintf(buf+offset, "%3d %-8d %-8d %8u ", sbuf.st_nlink, sbuf.st_uid, sbuf.st_gid, sbuf.st_size);
		
		//后组合时间日期
		const char *pdate = (const char*)statbuf_get_date(&sbuf);   //Sep 09  2019 
		offset += sprintf(buf+offset, "%s ", pdate);
		sprintf(buf+offset, "%s\r\n", dt->d_name);

		//发送数据
		send(sess->data_fd, buf, strlen(buf), 0);
	}
}

//文件列表显示
static void do_list(session_t *sess){
	//1 建立数据连接
	if(get_transfer_fd(sess) == 0)
		return;

	//2 回复 150 建立成功
	ftp_reply(sess, FTP_DATACONN ,"Here comes the directory listing.");

	//3 显示列表
	list_common(sess);

	//4 关闭连接
	close(sess->data_fd);
	sess->data_fd = -1;

	//5 回复 226
	ftp_reply(sess, FTP_TRANSFEROK, "Directory send OK.");
}

//改变当前路径
static void do_cwd (session_t *sess){
	//路径不存在则失败
	if(chdir(sess->arg) < 0)
	{
		//550 Failed to change directory.
		ftp_reply(sess, FTP_NOPERM, "Failed to change directory.");
		return;
	}
	//250 Directory successfully changed.
	ftp_reply(sess, FTP_CWDOK, "Directory successfully changed.");
}

//创建目录
static void do_mkd (session_t *sess){
	//创建目录
	if(mkdir(sess->arg, 0777) < 0)
	{
		//550 Create directory operation failed.
		ftp_reply(sess, FTP_NOPERM, "Create directory operation failed.");
		return;
	}
	//257 "/home/ll/text" created
	//返回信息
	char buf[MAX_BUFFER_SIZE] = {0};
	sprintf(buf, "\"%s\" created", sess->arg);
	ftp_reply(sess, FTP_MKDIROK, buf);
}

//删除目录
static void do_rmd (session_t *sess){
	//删除失败
	if(rmdir(sess->arg) < 0)
	{
		// 550 Remove directory operation failed.
		ftp_reply(sess, FTP_NOPERM, "Remove directory operation failed.");
		return;
	}
	// 250 Remove directory operation successful.
	ftp_reply(sess, FTP_RMDIROK, "Remove directory operation successful.");
}

//保存当前文件路径名
static void do_rnfr(session_t *sess){
	sess->rnfr_name = (char*)malloc(strlen(sess->arg)+1);
	memset(sess->rnfr_name, 0, strlen(sess->arg)+1);
	strcpy(sess->rnfr_name, sess->arg);
	ftp_reply(sess, FTP_RNFROK, "Ready for RNTO.");
}

//文件重命名
static void do_rnto(session_t *sess){
	//没有这个文件
	if(sess->rnfr_name == NULL)
	{
		//503 RNFR required first.
		ftp_reply(sess, FTP_NEEDRNFR, "RNFR required first.");
		return;
	}
	//改名失败
	if(rename(sess->rnfr_name, sess->arg) < 0)
	{
		ftp_reply(sess, FTP_NOPERM, "Rename failed.");
		return;
	}
	
	//释放存储未更改文件路径的空间
	free(sess->rnfr_name);
	//重置rnrf_name状态
	sess->rnfr_name = NULL;
	
	ftp_reply(sess, FTP_RENAMEOK, "Rename successful.");
}

//求文件大小
static void do_size(session_t *sess){
	struct stat sbuf;
	//stat函数返回buf指向的缓冲区中文件的相关信息
	if(stat(sess->arg, &sbuf) < 0)
	{
		//550 Could not get file size.
		ftp_reply(sess, FTP_FILEFAIL, "Could not get file size.");
		return;
	}
	//判断文件类型是否为普通文件
	if(!S_ISREG(sbuf.st_mode))
	{
		ftp_reply(sess, FTP_FILEFAIL, "Could not get file size.");
		return;
	}
	
	char buf[MAX_BUFFER_SIZE] = {0};
	sprintf(buf, "%d", sbuf.st_size);
	ftp_reply(sess, FTP_SIZEOK, buf);
}

//删除文件
static void do_dele(session_t *sess){
	//unlink 删除文件
	if(unlink(sess->arg) < 0)
	{
		ftp_reply(sess, FTP_NOPERM, "Delete operation failed.");
		return;
	}
	//250 Delete operation successful.
	ftp_reply(sess, FTP_DELEOK, "Delete operation successful.");
}

//限速
static void limit_rate(session_t *sess, int transfered_bytes, int isupload){
	//获取时间
	long cur_sec = get_time_sec();
	long cur_usec = get_time_usec();
	
	//转化为秒
	double pass_time = (double)(cur_sec - sess->transfer_start_sec);
	pass_time += (double)((cur_usec - sess->transfer_start_usec) / (double)1000000);

	//当前的传输速度 bit/s
	unsigned int cur_rate = (unsigned int)((double)transfered_bytes / pass_time);

	double rate_ratio; //速率
	//上传
	if(isupload){
		//传输速度小于设定值
		if(cur_rate <= sess->upload_max_rate){
			//重置时间
			sess->transfer_start_sec = cur_sec;
			sess->transfer_start_usec = cur_usec;
			return;
		}
		//求速率
		rate_ratio = cur_rate / sess->upload_max_rate;
	}
	//下载
	else{
		if(cur_rate <= sess->download_max_rate){
			sess->transfer_start_sec = cur_sec;
			sess->transfer_start_usec = cur_usec;
			return;
		}
		rate_ratio = cur_rate / sess->download_max_rate;
	}
	//睡眠时间
	double sleep_time = (rate_ratio - 1) * pass_time;
	//睡眠
	nano_sleep(sleep_time);
	//重置时间
	sess->transfer_start_sec = get_time_sec();
	sess->transfer_start_usec = get_time_usec();
}

//上传文件
static void do_stor(session_t *sess){
	//建立数据连接
	if(get_transfer_fd(sess) == 0)
		return;

	//open()通常用于将路径名转换为一个文件描述符
	//O_WRONLY 文件以只写的方式打开 O_WRONLY 若文件不存在将创建一个新文件
	int fd = open(sess->arg,  O_CREAT|O_WRONLY, 0755);
	if(fd == -1)
	{
		ftp_reply(sess, FTP_FILEFAIL, "Failed to open file.");
		return;
	}
	
	ftp_reply(sess, FTP_DATACONN,"Ok to send data.");
	
	//断点续传
	long long offset = sess->restart_pos;
	sess->restart_pos = 0;
	//设置偏移量
	if(lseek(fd, offset, SEEK_SET) < 0){
		ftp_reply(sess, FTP_UPLOADFAIL, "Could not create file.");
		return;
	}

	
	char buf[MAX_BUFFER_SIZE] = {0};
	int ret;

	//限速登记时间
	sess->transfer_start_sec = get_time_sec(); 
	sess->transfer_start_usec = get_time_usec();

	while(1)
	{
		//从对端data_fd接收文件
		ret = recv(sess->data_fd, buf, MAX_BUFFER_SIZE, 0);
		if(ret == -1)
		{
			ftp_reply(sess, FTP_BADSENDFILE, "Failure reading from local file.");
			break;
		}
		if(ret == 0)
		{
			//226 Transfer complete.
			ftp_reply(sess, FTP_TRANSFEROK, "Transfer complete.");
			break;
		}
		
		//设置空闲断开状态
		sess->data_process = 1;

		//限速
		if(sess->upload_max_rate != 0)
			limit_rate(sess, ret, 1);

		//将数据写入fd
		if(write(fd, buf, ret) != ret)
		{
			ftp_reply(sess, FTP_BADSENDFILE, "Failure writting to network stream.");
			break;
		}
	}

	close(fd);
	close(sess->data_fd);
	sess->data_fd = -1;

	//重新启动控制连接断开
	start_cmdio_alarm();
}

//下载文件
static void do_retr(session_t *sess){
	//建立数据连接
	if(get_transfer_fd(sess) == 0)
		return;
	//文件只读
	int fd = open(sess->arg, O_RDONLY);
	if(fd == -1)
	{
		ftp_reply(sess, FTP_FILEFAIL, "Failed to open file.");
		return;
	}

	struct stat sbuf;
	//返回fd的相关信息
	fstat(fd, &sbuf);

	//断点续载
	long long offset = sess->restart_pos;
	sess->restart_pos = 0;
	//如果已传输文件大小等于文件大小，传输完成
	if(offset >= sbuf.st_size)
	{
		ftp_reply(sess, FTP_TRANSFEROK, "Transfer complete.");
	}
	//从offset位置开始传输
	else{
		char msg[MAX_BUFFER_SIZE] = {0};
		//判断二进制还是ascii和文件大小
		if(sess->is_ascii)
			sprintf(msg, "Opening ASCII mode data connection for %s (%d bytes).", sess->arg, (long long)sbuf.st_size);
		else
			sprintf(msg, "Opening BINARY mode data connection for %s (%d bytes).", sess->arg, (long long)sbuf.st_size);
		// 150 Opening ASCII mode data connection for /home/bss/mytt/abc/test.cpp (70 bytes).
		ftp_reply(sess, FTP_DATACONN, msg);
		
		//偏移量设置为偏移字节
		//将fd文件的偏移量设为offset
		if(lseek(fd, offset, SEEK_SET) < 0){
			ftp_reply(sess, FTP_UPLOADFAIL, "Could not create file.");
			return;
		}

		char buf[MAX_BUFFER_SIZE] = {0};
		long long read_total_bytes = sbuf.st_size;
		int read_count = 0;
		int ret;
		
		//限速
		//登记时间
		sess->transfer_start_sec = get_time_sec(); //秒
		sess->transfer_start_usec = get_time_usec();//微秒

		while(1)
		{
			read_count = read_total_bytes > MAX_BUFFER_SIZE ? MAX_BUFFER_SIZE : read_total_bytes;
			//从文件描述符 fd 中读取 read_count 字节的数据并放入buf 开始的缓冲区中
			ret = read(fd, buf, read_count);
			if(ret==-1 || ret!=read_count)
			{
				ftp_reply(sess, FTP_BADSENDFILE, "Failure reading from local file.");
				break;
			}
			if(ret == 0)
			{
				// 226 Transfer complete.
				ftp_reply(sess, FTP_TRANSFEROK, "Transfer complete.");
				break;
			}

			//设置空闲断开状态
			sess->data_process = 1;
			
			//限速
			if(sess->download_max_rate != 0)
				limit_rate(sess, read_count, 0);

			//发送到对端
			if(send(sess->data_fd, buf, ret, 0) != ret)
			{
				ftp_reply(sess, FTP_BADSENDFILE, "Failure writting to network stream.");
				break;
			}
			read_total_bytes -= read_count;
		}
	}
	close(fd);
	close(sess->data_fd);
	sess->data_fd = -1;

	//重新启动控制连接断开
	start_cmdio_alarm();
}


//断点续传
static void do_rest(session_t *sess){
	//转为long long类型
	sess->restart_pos = (long long)atoll(sess->arg);

	//350 Restart position accepted (1612906496).
	char msg[MAX_BUFFER_SIZE] = {0};
	sprintf(msg, "Restart position accepted (%lld).", sess->restart_pos);
	ftp_reply(sess, FTP_RESTOK, msg);
}

//退出
static void do_quit(session_t *sess){
	ftp_reply(sess, FTP_GOODBYE, "Goodbye.");
}