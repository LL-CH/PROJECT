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


//������������
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
//����ip�Ͽ�����
void drop_ip_count(void *ip)
{
	//ͨ��IP��ַ��ȡ��Ӧ������
	unsigned int *p_count = hash_lookup_entry(s_ip_count_hash, ip, sizeof(unsigned int));
	if(p_count == NULL)
		return;
	int count = *p_count;
	--count;
	*p_count = count;
	//ip��ַ���������ͷŵ�
	if(count == 0)
		hash_free_entry(s_ip_count_hash, ip, sizeof(unsigned int));
}

//�ӽ����˳��źŴ�������
void handle_sigchld(int sig)
{
	pid_t pid;
	while((pid = waitpid(-1, NULL, WNOHANG)) > 0)
	{
		//�����������һ
		--s_children;
		//��pidֵ���ҽڵ�ip
		unsigned int *ip = hash_lookup_entry(s_pid_ip_hash, &pid, sizeof(pid));
		if(ip == NULL)
			continue;
		//ip��������һ
		drop_ip_count(ip);
		hash_free_entry(s_pid_ip_hash, &pid, sizeof(pid));
	}
}

//�������������±�
unsigned int hash_func(unsigned int buckets, void *key)
{
	return (*(unsigned int*)key % buckets);
}

//����ip������������
unsigned int handle_ip_count(void *ip)
{
	int count = 0;
	//ͨ��ip��ȡ������
	unsigned int *p_count = hash_lookup_entry(s_ip_count_hash, ip, sizeof(unsigned int));
	//û�����IP˵����һ������
	if(p_count == NULL)
	{
		count = 1;
		//���ӽڵ�
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
	//��ȡ�����ļ�
	parseconf_load_file("miniftp.conf");

	if(getuid() != 0)
	{
		printf("miniftp : must be started as root.\n");
		exit(EXIT_FAILURE);
	}

	session_t sess = 
	{
		/* �������� */
		-1,-1,"","","",

		//��������
		NULL,-1,-1,0,

		//ftp �ļ�����Э���־
		0,NULL,0,0,0,

		//���ӽ���ͨ��
		-1,-1,
		
		//����
		0,0,0,0
	};
	p_sess = &sess;

	//���������ٶ�
	sess.upload_max_rate = tunable_upload_max_rate;
	sess.download_max_rate = tunable_download_max_rate;

	//����ip��count��Ӧ�Ĺ�ϣ��
	s_ip_count_hash = hash_alloc(MAX_BUCKET_SIZE, hash_func);
	//����pid��ip��Ӧ�Ĺ�ϣ��
	s_pid_ip_hash = hash_alloc(MAX_BUCKET_SIZE, hash_func);

	int listenfd =  tcp_server(tunable_listen_address, tunable_listen_port);
	
	//�����ӽ����˳��ź�
	signal(SIGCHLD, handle_sigchld);

	int sockConn;
	struct sockaddr_in addrCli;
	socklen_t addrlen;
	while(1)
	{
		if((sockConn = accept(listenfd, (struct sockaddr*)&addrCli, &addrlen)) < 0)
			ERR_EXIT("accept");
		
		//���������
		++s_children;
		sess.num_clients = s_children;

		//ÿip����������
		unsigned int ip = addrCli.sin_addr.s_addr;
		sess.num_per_ip = handle_ip_count(&ip);

		pid_t pid = fork();
		if(pid == -1){
			//���Ӳ��ɹ�����������һ
			--s_children;
			ERR_EXIT("fork");
		}
		if(pid == 0)
		{
			//Child Process
			close(listenfd);
			
			//�Ự
			sess.ctrl_fd = sockConn;

			//�����������������
			check_limit(&sess);

			begin_session(&sess);
			exit(EXIT_SUCCESS);
		}
		else 
		{
			//Parent Process
			//�Ǽ�ÿ����������Ӧ��ip
			hash_add_entry(s_pid_ip_hash, &pid, sizeof(pid), &ip, sizeof(ip));
			close(sockConn);
		}
	}

	close(listenfd);
	return 0;
}