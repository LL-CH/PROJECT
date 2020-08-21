#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(void){
	//绑定套接字
	int fd=socket(AF_INET,SOCK_STREAM,0);
	//定义端口地址
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(9898);
	inet_aton("127.0.0.1", &addr.sin_addr);
	//进行连接
	int r = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
	if (r == -1) 
		perror("connect"), exit(1);
	//发送数据
	char buf[1024] = {};
	while (fgets(buf, 1024, stdin) != NULL) {
		write(fd, buf, strlen(buf));
		memset(buf, 0x00, sizeof(buf));
		int r = read(fd, buf, 1024);
		if (r <= 0) 
			break;
		printf("=> %s\n", buf);
	}
}	
