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

int Work(FILE* fp, int sockfd, struct sockaddr* pcliaddr, socklen_t clilen);

int main()
{
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	
	struct sockaddr_in servaddr;
	servaddr.sin_family = AF_INET;
	//servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
	servaddr.sin_port = htons(SERV_PORT);
	
	int ret = Work(stdin, sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	if (ret < 0)
	{
		return -1;
	}
	
	return 0;
}

int Work(FILE* fp, int sockfd, struct sockaddr* pcliaddr, socklen_t clilen)
{
	char send[MAXLINE], recv[MAXLINE];

	while(fgets(send, MAXLINE, fp) != NULL)
	{
		int n = sendto(sockfd, send, strlen(send), 0, pcliaddr, clilen);
		if (n < 0)
		{
			printf("sendto error\n");
			return -1;
		}

		n = recvfrom(sockfd, recv, MAXLINE, 0, NULL, NULL);
		if (n < 0)
                {
                        printf("recvfrom error\n");
                        return -1;
                }

                printf("===%s====%s===", send, recv);
	}	
	
	return 0;
}


