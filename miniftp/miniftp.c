#include"common.h"
#include"sysutil.h"
#include"session.h"
#include"tunable.h"
#include"parseconf.h"
#include"ftpcodes.h"
#include"ftpproto.h"
#include"hash.h"

extern session_t *p_sess;
static unsigned int s_children; 
static hash_t *s_ip_count_hash;
static hash_t *s_pid_ip_hash;


//检查最大连接数
static void check_limit(session_t *sess)
{
	if(tunable_max_clients!=0 && sess->num_clients>tunable_max_clients)
	{
		// 421 There are too many connected users, please try later
		ftp_reply(sess, FTP_TOO_MANY_USERS, "There are too many connected users, please try later");
		exit(EXIT_FAILURE);
	}
	if(tunable_max_per_ip!=0 && sess->num_per_ip>tunable_max_per_ip)
	{
		// 421 There are too many connections from your internet address
		ftp_reply(sess, FTP_IP_LIMIT, "There are too many connections from your internet address");
		exit(EXIT_FAILURE);
	}
}

///////////////////////////////////////////////////////////
//处理ip断开连接
void drop_ip_count(void *ip)
{
	//通过IP地址获取对应连接数
	unsigned int *p_count = hash_lookup_entry(s_ip_count_hash, ip, sizeof(unsigned int));
	if(p_count == NULL)
		return;
	int count = *p_count;
	--count;
	*p_count = count;
	//ip地址无连接则释放掉
	if(count == 0)
		hash_free_entry(s_ip_count_hash, ip, sizeof(unsigned int));
}

//子进程退出信号处理函数
void handle_sigchld(int sig)
{
	pid_t pid;
	while((pid = waitpid(-1, NULL, WNOHANG)) > 0)
	{
		//最大连接数减一
		--s_children;
		//由pid值查找节点ip
		unsigned int *ip = hash_lookup_entry(s_pid_ip_hash, &pid, sizeof(pid));
		if(ip == NULL)
			continue;
		//ip连接数减一
		drop_ip_count(ip);
		hash_free_entry(s_pid_ip_hash, &pid, sizeof(pid));
	}
}

//除留余数法求下标
unsigned int hash_func(unsigned int buckets, void *key)
{
	return (*(unsigned int*)key % buckets);
}

//处理ip连接数的增加
unsigned int handle_ip_count(void *ip)
{
	int count = 0;
	//通过ip获取连接数
	unsigned int *p_count = hash_lookup_entry(s_ip_count_hash, ip, sizeof(unsigned int));
	//没有这个IP说明第一次连接
	if(p_count == NULL)
	{
		count = 1;
		//添加节点
		hash_add_entry(s_ip_count_hash, ip, sizeof(unsigned int), &count, sizeof(unsigned int));
	}
	else
	{
		count = *p_count;
		++count;
		*p_count = count;
	}
	return count;
}



int main(int argc, char *argv[]){
	//读取配置文件
	parseconf_load_file("miniftp.conf");

	if(getuid() != 0)
	{
		printf("miniftp : must be started as root.\n");
		exit(EXIT_FAILURE);
	}

	session_t sess = 
	{
		/* 控制连接 */
		-1,-1,"","","",

		//数据连接
		NULL,-1,-1,0,

		//ftp 文件传输协议标志
		0,NULL,0,0,0,

		//父子进程通道
		-1,-1,
		
		//限速
		0,0,0,0
	};
	p_sess = &sess;

	//设置下载速度
	sess.upload_max_rate = tunable_upload_max_rate;
	sess.download_max_rate = tunable_download_max_rate;

	//申请ip和count对应的哈希表
	s_ip_count_hash = hash_alloc(MAX_BUCKET_SIZE, hash_func);
	//申请pid和ip对应的哈希表
	s_pid_ip_hash = hash_alloc(MAX_BUCKET_SIZE, hash_func);

	int listenfd =  tcp_server(tunable_listen_address, tunable_listen_port);
	
	//处理子进程退出信号
	signal(SIGCHLD, handle_sigchld);

	int sockConn;
	struct sockaddr_in addrCli;
	socklen_t addrlen;
	while(1)
	{
		if((sockConn = accept(listenfd, (struct sockaddr*)&addrCli, &addrlen)) < 0)
			ERR_EXIT("accept");
		
		//最大连接数
		++s_children;
		sess.num_clients = s_children;

		//每ip连接数限制
		unsigned int ip = addrCli.sin_addr.s_addr;
		sess.num_per_ip = handle_ip_count(&ip);

		pid_t pid = fork();
		if(pid == -1){
			//连接不成功则连接数减一
			--s_children;
			ERR_EXIT("fork");
		}
		if(pid == 0)
		{
			//Child Process
			close(listenfd);
			
			//会话
			sess.ctrl_fd = sockConn;

			//进行连接数检查限制
			check_limit(&sess);

			begin_session(&sess);
			exit(EXIT_SUCCESS);
		}
		else 
		{
			//Parent Process
			//登记每个进程所对应的ip
			hash_add_entry(s_pid_ip_hash, &pid, sizeof(pid), &ip, sizeof(ip));
			close(sockConn);
		}
	}

	close(listenfd);
	return 0;
}