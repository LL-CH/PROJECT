#include"privparent.h"
#include"privsock.h"
#include"session.h"

//获取主动模式数据连接套接字
static void privop_pasv_get_data_sock(session_t *sess); 

//判断是否处于被动模式的激活状态
static void privop_pasv_active(session_t *sess); 

//获取被动模式下的监听端口
static void privop_pasv_listen(session_t *sess);

//获取被动模式下的数据连接套接字
static void privop_pasv_accept(session_t *sess); 

//提升权限
static void minimize_privilege(){
	//把root进程更改进程名为nobody
	struct passwd *pw = getpwnam("nobody");
	if(pw == NULL)
		ERR_EXIT("getpwname");
	if(setegid(pw->pw_gid) < 0)
		ERR_EXIT("setegid");
	if(seteuid(pw->pw_uid) < 0)
		ERR_EXIT("seteuid");
	
	struct __user_cap_header_struct cap_header;
	struct __user_cap_data_struct   cap_data;
	memset(&cap_header, 0, sizeof(cap_header));
	memset(&cap_data, 0, sizeof(cap_data));
	//64位系统使用_LINUX_CAPABILITY_VERSION_2
	cap_header.version = _LINUX_CAPABILITY_VERSION_2;
	cap_header.pid = 0;

	unsigned int cap_mask = 0;
	//CAP_NET_BIND_SERVICE 允许绑定1024一下端口权限
	cap_mask |= (1 << CAP_NET_BIND_SERVICE);  //0000 0000 0000 0000 0001 0000 0100 0000

	cap_data.effective = cap_data.permitted = cap_mask;
	cap_data.inheritable = 0; // 不继承权限
	
	capset(&cap_header, &cap_data);//用于设置权限能力
}


//nobody 进程
void handle_parent(session_t *sess){
	//提升权限 可以绑定20端口
	minimize_privilege();
	char cmd;
	while(1){
		//等待ftp进程的消息并处理
		cmd = priv_sock_get_cmd(sess->parent_fd);
		switch(cmd)
		{
		case PRIV_SOCK_GET_DATA_SOCK:
			privop_pasv_get_data_sock(sess);
			break;
		case PRIV_SOCK_PASV_ACTIVE:
			privop_pasv_active(sess);
			break;
		case PRIV_SOCK_PASV_LISTEN:
			privop_pasv_listen(sess);
			break;
		case PRIV_SOCK_PASV_ACCEPT:
			privop_pasv_accept(sess);
			break;
		}
	}
}

//获取主动模式数据连接套接字
static void privop_pasv_get_data_sock(session_t *sess){
	//获取主动连接发送的端口号
	unsigned short port=(unsigned short)priv_sock_get_int(sess->parent_fd);
	char ip[16]={0};
	//获取ip
	priv_sock_recv_buf(sess->parent_fd,ip,sizeof(ip));

	struct sockaddr_in addr;
	addr.sin_family=AF_INET;
	//htons 转化为网络字节序
	addr.sin_port=htons(port);
	addr.sin_addr.s_addr=inet_addr(ip);
	
	//绑定20端口
	int fd=tcp_client(20);
	if(fd == -1)
	{
		priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_BAD);
		return;
	}
	//进行连接
	//连接失败则返回
	if(connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
	{
		close(fd);
		priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_BAD);
		return;
	}
	//连接成功，发送结果给ftp进程
	priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_OK);
	//发送fd
	priv_sock_send_fd(sess->parent_fd,  fd);
	close(fd);
}


//判断是否处于被动模式的激活状态
static void privop_pasv_active(session_t *sess){
	int active;
	if(sess->pasv_listen_fd != -1)
		active = 1;
	else
		active = 0;
	priv_sock_send_int(sess->parent_fd, active);
}

//获取被动模式下的监听端口
static void privop_pasv_listen(session_t *sess){
	char ip[16]={0};
	//获取ip地址
	getlocalip(ip);
	sess->pasv_listen_fd = tcp_server(ip, 0); //传端口0表示绑定临时端口
	
	struct sockaddr_in address;
	socklen_t addrlen = sizeof(struct sockaddr);
	if(getsockname(sess->pasv_listen_fd, (struct sockaddr*)&address, &addrlen) < 0)
		ERR_EXIT("getsockname");

	unsigned short port = ntohs(address.sin_port);
	//发送端口号给ftp进程
	priv_sock_send_int(sess->parent_fd, (int)port);
}

//获取被动模式下的数据连接套接字
static void privop_pasv_accept(session_t *sess){
	int fd = accept(sess->pasv_listen_fd, 0, 0);
	close(sess->pasv_listen_fd);
	sess->pasv_listen_fd = -1;

	if(fd == -1)
	{
		priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_BAD);
		return;
	}
	//发送结果和fd
	priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_OK);
	priv_sock_send_fd(sess->parent_fd, fd);
	close(fd);
}