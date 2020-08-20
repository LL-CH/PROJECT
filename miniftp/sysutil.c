#include"sysutil.h"

//建立服务端
int  tcp_server(const char *host,unsigned short port){
	int listenfd;
	if((listenfd=socket(AF_INET,SOCK_STREAM,0))<0){
		ERR_EXIT("tcp_server");
	}

	struct sockaddr_in addrSer;
	addrSer.sin_family=AF_INET;
	addrSer.sin_port=htons(port);
	addrSer.sin_addr.s_addr=inet_addr(host);

	int on=1;
	//为应用程序提供控制套接字行为的方法
	if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
		ERR_EXIT("setsockopt");

	if(bind(listenfd, (struct sockaddr*)&addrSer, sizeof(addrSer)) < 0)
		ERR_EXIT("bind");

	if(listen(listenfd, SOMAXCONN) < 0)
		ERR_EXIT("listen");

	return listenfd;

}
//客户端建立
int tcp_client(int port)
{
	int sock;
	if(( sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		ERR_EXIT("tcp_client");

	if(port > 0)
	{
		int on = 1;
		//设置端口可以复用
		if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
			ERR_EXIT("setsockopt");

		struct sockaddr_in address;
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons(port);
		if(bind(sock, (struct sockaddr*)&address, sizeof(struct sockaddr)) < 0)
			ERR_EXIT("bind 20");
	}
	return sock;
}

//获取文件权限
const char* statbuf_get_perms(struct stat* sbuf){
	//- --- --- --- 10位
	static char perms[] = "----------";
	//mode 文件类型和模式
	mode_t mode = sbuf->st_mode;
	//判断文件类型
	switch(mode & S_IFMT)
	{
	case S_IFSOCK:
		perms[0] = 's';
		break;
	case S_IFLNK:
		perms[0] = 'l';
		break;
	case S_IFREG:
		perms[0] = '-';
		break;
	case S_IFBLK:
		perms[0] = 'b';
		break;
	case S_IFDIR:
		perms[0] = 'd';
		break;
	case S_IFCHR:
		perms[0] = 'c';
		break;
	case S_IFIFO:
		perms[0] = 'p';
		break;
	}
	//判断文件权限
	if(mode & S_IRUSR)
		perms[1] = 'r';
	if(mode & S_IWUSR)
		perms[2] = 'w';
	if(mode & S_IXUSR)
		perms[3] = 'x';

	if(mode & S_IRGRP)
		perms[4] = 'r';
	if(mode & S_IWGRP)
		perms[5] = 'w';
	if(mode & S_IXGRP)
		perms[6] = 'x';

	if(mode & S_IROTH)
		perms[7] = 'r';
	if(mode & S_IWOTH)
		perms[8] = 'w';
	if(mode & S_IXOTH)
		perms[9] = 'x';

	return perms;
}
//获取文件时间
const char* statbuf_get_date(struct stat* sbuf){
	static char datebuf[64] = {0};
	//st_mtime 文件上一次的修改时间
	time_t file_time = sbuf->st_mtime;
	//函数作用：将日历时间转换为分解的时间表示形式，表示为相对于用户指定的时区
	struct tm *ptm = localtime(&file_time);
	//函数的作用是：转换日历时间；strftime（）函数根据格式规范格式格式化分解后的时间ptm并将结果放入最大大小的字符数组datebuf中
	strftime(datebuf, 64, "%b %e %H:%M",  ptm);
	return datebuf;
}

//发送fd
void send_fd(int sock_fd, int fd){
	int ret;
	struct msghdr msg;
	struct cmsghdr *p_cmsg;
	struct iovec vec;
	char cmsgbuf[CMSG_SPACE(sizeof(fd))];
	int *p_fds;
	char sendchar = 0;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);
	p_cmsg = CMSG_FIRSTHDR(&msg);
	p_cmsg->cmsg_level = SOL_SOCKET;
	p_cmsg->cmsg_type = SCM_RIGHTS;
	p_cmsg->cmsg_len = CMSG_LEN(sizeof(fd));
	p_fds = (int*)CMSG_DATA(p_cmsg);
	*p_fds = fd;

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;
	msg.msg_flags = 0;

	vec.iov_base = &sendchar;
	vec.iov_len = sizeof(sendchar);
	ret = sendmsg(sock_fd, &msg, 0);
	if (ret != 1)
		ERR_EXIT("sendmsg");
}
//接收fd
int recv_fd(const int sock_fd){
	int ret;
	struct msghdr msg;
	char recvchar;
	struct iovec vec;
	int recv_fd;
	char cmsgbuf[CMSG_SPACE(sizeof(recv_fd))];
	struct cmsghdr *p_cmsg;
	int *p_fd;
	vec.iov_base = &recvchar;
	vec.iov_len = sizeof(recvchar);
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);
	msg.msg_flags = 0;

	p_fd = (int*)CMSG_DATA(CMSG_FIRSTHDR(&msg));
	*p_fd = -1;  
	ret = recvmsg(sock_fd, &msg, 0);
	if (ret != 1)
		ERR_EXIT("recvmsg");

	p_cmsg = CMSG_FIRSTHDR(&msg);
	if (p_cmsg == NULL)
		ERR_EXIT("no passed fd");


	p_fd = (int*)CMSG_DATA(p_cmsg);
	recv_fd = *p_fd;
	if (recv_fd == -1)
		ERR_EXIT("no passed fd");

	return recv_fd;
}

//获取本地ip地址
void getlocalip(char *ip)
{
	char host[MAX_HOST_NAME_SIZE] = {0};
	//获取主机名
	if(gethostname(host, sizeof(host)) < 0)
		ERR_EXIT("gethostname");
	//printf("host name = %s\n", host);
	
	struct hostent *ph;
	//获取ip地址
	if((ph = gethostbyname(host)) == NULL)
		ERR_EXIT("gethostbyname");

	strcpy(ip, inet_ntoa(*(struct in_addr*)ph->h_addr));
}


static struct timeval s_cur_time;

long get_time_sec(){
	if(gettimeofday(&s_cur_time, NULL) < 0)
		ERR_EXIT("gettimeofday");
	return s_cur_time.tv_sec;
}

long get_time_usec(){
	return s_cur_time.tv_usec;
}


//睡眠
void nano_sleep(double sleep_time){
	//秒
	time_t sec =  (time_t)sleep_time;
	//微秒
	double decimal = sleep_time - (double)sec;

	struct timespec ts;
	ts.tv_sec = sec;
	//转为纳秒
	ts.tv_nsec = (long)(decimal * 1000000000);

	int ret;
	do{
		ret = nanosleep(&ts, &ts);
	}while(ret==-1 && errno==EINTR);  //循环用于预防休眠被信号中断
}