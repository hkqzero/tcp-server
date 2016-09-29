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
#include <string>
#include <iostream>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "epoll_manager.h"
#include "Socket.h"
#include "log_interface.h"

using namespace std;

#define SERV_PORT 9999
#define MAXLINE 10

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
		printf("bind(2): %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	EpollManager epoll_manager;
	ret = epoll_manager.EpollCreate();
	if (ret < 0)
	{
		printf("EpollCreate error\n");
		return -1;
	}
	ret = epoll_manager.EpollCtlAdd(sock_fd, EPOLLIN);
	if (ret < 0)
	{
		printf("EpollCtlAdd error\n");
		return -1;
	}

	for ( ; ; ) 
	{
		int nfds = epoll_manager.EpollWait();
		printf("就绪事件数: %d\n", nfds);
		struct epoll_event* events = epoll_manager.EpollEvents();

		for (int i = 0; i < nfds; ++i)
		{
			int sfd = events[i].data.fd;
			printf("sfd: %d\n", sfd);
			if (sfd == sock_fd)//如果新监测到一个SOCKET用户连接到了绑定的SOCKET端口，建立新的连接。
			{
				socklen_t clilen;
				struct sockaddr_in clientaddr;
				int conn_fd = accept(sock_fd, (sockaddr *)&clientaddr, &clilen);
				if (conn_fd < 0)
				{
					printf("conn_fd<0\n");
					return -1;
				}
				if (sk_set_nonblock(conn_fd) < 0)
				{
					printf("conn_fd sk_set_nonblock error\n");
				}

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
			else if (events[i].events & EPOLLIN)//如果是已经连接的用户，并且收到数据，那么进行读入。
			{
				printf("epollin, sfd: %d\n", sfd);
				if (sfd < 0)
				{
					continue;
				}

				bool bReadOk = false;
				string sInput;
				while (true)
				{
					char line[MAXLINE];

					int iRecvNum = recv(sfd, line, MAXLINE, 0);
					if (iRecvNum < 0)
					{
						if (errno == EINTR)
						{
							printf("EINTR\n");
							continue;
						}
						else if (errno == EWOULDBLOCK || errno == EAGAIN)
						{
							printf("EWOULDBLOCK or EAGAIN\n");
							bReadOk = true;
							break;
						}
						else
						{
							close(sfd);
							printf("other error\n");
							break;
						}
					}
					else if (iRecvNum == 0)
					{
						printf("client close request\n");
						close(sfd);
						break;
					}
					else
					{
						if (iRecvNum == MAXLINE)
						{
							sInput.append(line);
							continue;
						}
						else 
						{
							sInput.append(line);
							bReadOk = true;
							break;
						}
					}
				}

				if (bReadOk == true)
				{
					printf("read from client: %s\n", sInput.c_str());

					ret = epoll_manager.EpollCtlMod(sfd, EPOLLOUT|EPOLLET);
					if (ret < 0)
					{
						printf("EpollCtlAdd error: %d\n", sfd);
						return -1;
					}
				}
			}
			else if (events[i].events & EPOLLOUT) // 如果有数据发送
			{
				const char str[] = "hello from epoll : this is a long string which may be cut by the net\n";

				char line[MAXLINE];
				memcpy(line, str, sizeof(str));
				printf("write line: %s\n", line);

				bool bWritten = false;
				int writenLen = 0;
				char* head = line;
				while (1)
				{
					writenLen = send(sfd, head, MAXLINE, 0);
					printf("writenLen: %d\n", writenLen);
					if (writenLen < 0)
					{
						if (errno == EINTR)
						{
							printf("EINTR\n");
							continue;
						}
						else if (errno == EWOULDBLOCK || errno == EAGAIN)
						{
							printf("[send]EWOULDBLOCK or EAGAINs\n");
							bWritten = true;
							break;
						}
						else
						{
							close(sfd);
							printf("other error\n");
							break;
						}
					}
					else if (writenLen == 0)
					{
						close(sfd);
						bWritten = true;
						printf("writenLen ok\n");
						break;
					}
					else
					{
						if (writenLen == MAXLINE)
						{
							head += writenLen;
							continue;
						}
						else 
						{
							bWritten = true;
							break;
						}
					}
				}

				if (bWritten == true)
				{
					ret = epoll_manager.EpollCtlMod(sfd, EPOLLIN | EPOLLET);
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

