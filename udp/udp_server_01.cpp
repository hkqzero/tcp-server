#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <cstdlib>
#include <iostream>

#define SERV_PORT 9999
#define MAXLINE 1024

int Work(int sockfd, struct sockaddr* pcliaddr, socklen_t clilen);

int main()
{
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	
	struct sockaddr_in servaddr, cliaddr;
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);
	
	if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
	{
		printf("bind error\n");
		return 0;
	}
	
	int ret = Work(sockfd, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
	if (ret < 0)
	{
		return -1;
	}
	
	return 0;
}

int Work(int sockfd, struct sockaddr* pcliaddr, socklen_t clilen)
{
	char mesg[MAXLINE];

	for ( ; ; )
	{
		int n = recvfrom(sockfd, mesg, MAXLINE, 0, pcliaddr, &clilen);
		if (n < 0)
		{
			printf("recvfrom error\n");
			return -1;
		}
		
		printf("recv: %s", mesg);

		n = sendto(sockfd, mesg, n, 0, pcliaddr, clilen);
		if (n < 0)
		{
			printf("sendto error\n");
			return -1;
		}
		memset(mesg, 0, strlen(mesg));
	}	
	
	return 0;
}


