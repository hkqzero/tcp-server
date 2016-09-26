#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <time.h>

#include <stack>
#include <iostream>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "epoll_manager.h"
#include "Socket.h"
#include "log_interface.h"

#define SERV_PORT 9999
#define MAXLINE 1024

//init
int init();

int main(int argc, char* argv[])
{
	int ret = init();
	if (ret < 0)
	{
		printf("init error\n");
		return -1;
	}

	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	printf("sock_fd: %d\n", sock_fd);
	sk_set_nonblock(sock_fd);
	sk_resue_addr(sock_fd);

	if (sk_bind_listen(sock_fd, SERV_PORT) != 0) 
	{
		//SCREEN(SCREEN_RED, stderr, "bind(2): %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	EpollManager epoll_manager;
	ret = epoll_manager.EpollCreate();
	if (ret < 0)
	{
		printf("EpollCreate error\n");
		return -1;
	}
	ret = epoll_manager.EpollCtlAdd(sock_fd, EPOLLIN|EPOLLET);
	if (ret < 0)
	{
		printf("EpollCtlAdd error\n");
		return -1;
	}

	socklen_t clilen;
	struct sockaddr_in clientaddr;

	for ( ; ; ) 
	{
		int nfds = epoll_manager.EpollWait();
		struct epoll_event* events = epoll_manager.EpollEvents();
		
		for (int i=0; i < nfds; ++i)
		{
			int sfd = events[i].data.fd;
			printf("sfd: %d\n", sfd);
			if (sfd == sock_fd)//如果新监测到一个SOCKET用户连接到了绑定的SOCKET端口，建立新的连接。
			{
				int conn_fd = accept(sock_fd, (sockaddr *)&clientaddr, &clilen);
				if (conn_fd < 0)
				{
					perror("conn_fd<0");
					return -1;
				}
				//setnonblocking(conn_fd);

				char *str = inet_ntoa(clientaddr.sin_addr);
				printf("accapt a connection from %s\n", str);
			
				//设置用于读操作的文件描述符
				ret = epoll_manager.EpollCtlAdd(conn_fd, EPOLLIN | EPOLLET);
				if (ret < 0)
				{
					printf("EpollCtlAdd error: %d\n", conn_fd);
					return -1;
				}
			}
			else if (sfd & EPOLLIN)//如果是已经连接的用户，并且收到数据，那么进行读入。
			{
				printf("epollin, sfd: %d\n", sfd);
				if (sfd < 0)
				{
					continue;
				}
				
				char line[MAXLINE];
				char* head = line;
				int recvNum = 0;
				int count = 0;
				bool bReadOk = false;
				while (true)
				{
					recvNum = recv(sfd, head + count, MAXLINE, 0);
					if (recvNum < 0)
					{
						if (errno == EAGAIN)
						{
							bReadOk = true;
							break;
						}
						else if (errno == ECONNRESET)
						{
							close(sfd);
							events[i].data.fd = -1;
							printf("ECONNRESET\n");
							break;
						}
						else if (errno == EINTR)
						{
							continue;
						}
						else
						{
							close(sfd);
							events[i].data.fd = -1;
							printf("other\n");
							break;
						}
					}
					else if (recvNum == 0)
					{
						close(sfd);
						events[i].data.fd = -1;
						printf("recvNum ok\n");
						break;
					}
					count += recvNum;
					if (recvNum == MAXLINE)
					{
						continue;
					}
					else 
					{
						bReadOk = true;
						break;
					}
				}

				if (bReadOk == true)
				{
					line[count] = '\0';
					printf("read from client: %s\n", line);

					ret = epoll_manager.EpollCtlMod(sfd, EPOLLOUT|EPOLLET);
					if (ret < 0)
					{
						printf("EpollCtlAdd error: %d\n", sfd);
						return -1;
					}
				}
			}
			else if (sfd & EPOLLOUT) // 如果有数据发送
			{
				const char str[] = "hello from epoll : this is a long string which may be cut by the net\n";
				
				char line[MAXLINE];
				memcpy(line, str, sizeof(str));
				printf("write line: %s\n", line);
				
				bool bWritten = false;
				int writenLen = 0;
				int count = 0;
				char* head = line;

				while (1)
				{
					writenLen = send(sfd, head + count, MAXLINE, 0);
					if (writenLen == -1)
					{
						if (errno == EAGAIN)
						{
							bWritten = true;
							break;
						}
						else if (errno == ECONNRESET)
						{
							close(sfd);
							events[i].data.fd = -1;
							printf("ECONNRESET\n");
							break;
						}
						else if (errno == EINTR)
						{
							continue;
						}
						else
						{
							close(sfd);
							events[i].data.fd = -1;
							printf("other\n");
							break;
						}
					}
					
					if (writenLen == 0)
					{
						close(sfd);
						events[i].data.fd = -1;
						printf("writenLen ok\n");
						break;
					}
					count += writenLen;
					if (writenLen == MAXLINE)
					{
						continue;
					}
					else 
					{
						bWritten = true;
						break;
					}
				}
				if (bWritten == true)
				{
					ret = epoll_manager.EpollCtlMod(sfd, EPOLLIN|EPOLLET);
					if (ret < 0)
					{
						printf("EpollCtlMod error: %d\n", sfd);
						return -1;
					}
				}
			}
		}
	}

	return 0;
}

int init()
{
	signal(SIGPIPE, SIG_IGN);

	int ret = ILog::instance()->setLogPara("../log/err_log.log", 0, LOG_TFORMAT_0, LOG_TYPE_DAY, 50 << 20);
	if (ret != 0) {
		printf("[%u]set error log config failed\n", getpid());
		return ret;
	}

	ret = IFlowLog::instance()->setLogPara("../log/flow_log.log", 4, LOG_TFORMAT_0, LOG_TYPE_DAY, 50 << 20); 
	if (ret != 0) 
	{
		ERROR_LOG("[%u]set flowing log config failed\n", getpid());
		return ret;
	}

	return 0;
}

