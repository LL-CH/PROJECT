#include"privparent.h"
#include"privsock.h"
#include"session.h"

//��ȡ����ģʽ���������׽���
static void privop_pasv_get_data_sock(session_t *sess); 

//�ж��Ƿ��ڱ���ģʽ�ļ���״̬
static void privop_pasv_active(session_t *sess); 

//��ȡ����ģʽ�µļ����˿�
static void privop_pasv_listen(session_t *sess);

//��ȡ����ģʽ�µ����������׽���
static void privop_pasv_accept(session_t *sess); 

//����Ȩ��
static void minimize_privilege(){
	//��root���̸��Ľ�����Ϊnobody
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
	//64λϵͳʹ��_LINUX_CAPABILITY_VERSION_2
	cap_header.version = _LINUX_CAPABILITY_VERSION_2;
	cap_header.pid = 0;

	unsigned int cap_mask = 0;
	//CAP_NET_BIND_SERVICE �����1024һ�¶˿�Ȩ��
	cap_mask |= (1 << CAP_NET_BIND_SERVICE);  //0000 0000 0000 0000 0001 0000 0100 0000

	cap_data.effective = cap_data.permitted = cap_mask;
	cap_data.inheritable = 0; // ���̳�Ȩ��
	
	capset(&cap_header, &cap_data);//��������Ȩ������
}


//nobody ����
void handle_parent(session_t *sess){
	//����Ȩ�� ���԰�20�˿�
	minimize_privilege();
	char cmd;
	while(1){
		//�ȴ�ftp���̵���Ϣ������
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

//��ȡ����ģʽ���������׽���
static void privop_pasv_get_data_sock(session_t *sess){
	//��ȡ�������ӷ��͵Ķ˿ں�
	unsigned short port=(unsigned short)priv_sock_get_int(sess->parent_fd);
	char ip[16]={0};
	//��ȡip
	priv_sock_recv_buf(sess->parent_fd,ip,sizeof(ip));

	struct sockaddr_in addr;
	addr.sin_family=AF_INET;
	//htons ת��Ϊ�����ֽ���
	addr.sin_port=htons(port);
	addr.sin_addr.s_addr=inet_addr(ip);
	
	//��20�˿�
	int fd=tcp_client(20);
	if(fd == -1)
	{
		priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_BAD);
		return;
	}
	//��������
	//����ʧ���򷵻�
	if(connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
	{
		close(fd);
		priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_BAD);
		return;
	}
	//���ӳɹ������ͽ����ftp����
	priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_OK);
	//����fd
	priv_sock_send_fd(sess->parent_fd,  fd);
	close(fd);
}


//�ж��Ƿ��ڱ���ģʽ�ļ���״̬
static void privop_pasv_active(session_t *sess){
	int active;
	if(sess->pasv_listen_fd != -1)
		active = 1;
	else
		active = 0;
	priv_sock_send_int(sess->parent_fd, active);
}

//��ȡ����ģʽ�µļ����˿�
static void privop_pasv_listen(session_t *sess){
	char ip[16]={0};
	//��ȡip��ַ
	getlocalip(ip);
	sess->pasv_listen_fd = tcp_server(ip, 0); //���˿�0��ʾ����ʱ�˿�
	
	struct sockaddr_in address;
	socklen_t addrlen = sizeof(struct sockaddr);
	if(getsockname(sess->pasv_listen_fd, (struct sockaddr*)&address, &addrlen) < 0)
		ERR_EXIT("getsockname");

	unsigned short port = ntohs(address.sin_port);
	//���Ͷ˿ںŸ�ftp����
	priv_sock_send_int(sess->parent_fd, (int)port);
}

//��ȡ����ģʽ�µ����������׽���
static void privop_pasv_accept(session_t *sess){
	int fd = accept(sess->pasv_listen_fd, 0, 0);
	close(sess->pasv_listen_fd);
	sess->pasv_listen_fd = -1;

	if(fd == -1)
	{
		priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_BAD);
		return;
	}
	//���ͽ����fd
	priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_OK);
	priv_sock_send_fd(sess->parent_fd, fd);
	close(fd);
}