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
		DEBUG_FLOWLOG("init error");
		return -1;
	}

	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	DEBUG_FLOWLOG("sock_fd: %d", sock_fd);

	sk_set_nonblock(sock_fd);
	sk_resue_addr(sock_fd);

	if (sk_bind_listen(sock_fd, SERV_PORT) != 0) 
	{
		DEBUG_FLOWLOG("bind(2): %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	//for (int i = 0; i < 3; ++i)
	//{
	//	pid_t pid = fork();
	//	if (pid > 0)
	//	{
	//		continue;
	//	}
	//}

	EpollManager epoll_manager;
	ret = epoll_manager.EpollCreate();
	if (ret < 0)
	{
		DEBUG_FLOWLOG("EpollCreate error");
		return -1;
	}
	ret = epoll_manager.EpollCtlAdd(sock_fd, EPOLLIN);
	if (ret < 0)
	{
		DEBUG_FLOWLOG("EpollCtlAdd error");
		return -1;
	}

	for ( ; ; ) 
	{
		int nfds = epoll_manager.EpollWait();
		DEBUG_FLOWLOG("[%u]就绪事件数: %d", getpid(), nfds);
		struct epoll_event* events = epoll_manager.EpollEvents();

		for (int i = 0; i < nfds; ++i)
		{
			int sfd = events[i].data.fd;
			DEBUG_FLOWLOG("sfd: %d", sfd);
			if (sfd == sock_fd)//如果新监测到一个SOCKET用户连接到了绑定的SOCKET端口，建立新的连接。
			{
				socklen_t clilen;
				struct sockaddr_in clientaddr;
				int conn_fd = accept(sock_fd, (sockaddr *)&clientaddr, &clilen);
				if (conn_fd < 0)
				{
					DEBUG_FLOWLOG("conn_fd<0");
					return -1;
				}
				if (sk_set_nonblock(conn_fd) < 0)
				{
					DEBUG_FLOWLOG("conn_fd sk_set_nonblock error");
				}

				char *str = inet_ntoa(clientaddr.sin_addr);
				DEBUG_FLOWLOG("accapt a connection from %s, conn_fd: %d", str, conn_fd);

				//设置用于读操作的文件描述符
				ret = epoll_manager.EpollCtlAdd(conn_fd, EPOLLIN | EPOLLET);
				if (ret < 0)
				{
					DEBUG_FLOWLOG("EpollCtlAdd error: %d", conn_fd);
					return -1;
				}
			}
			else if (events[i].events & EPOLLIN)//如果是已经连接的用户，并且收到数据，那么进行读入。
			{
				DEBUG_FLOWLOG("epollin, sfd: %d", sfd);
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
							DEBUG_FLOWLOG("EINTR");
							continue;
						}
						else if (errno == EWOULDBLOCK || errno == EAGAIN)
						{
							DEBUG_FLOWLOG("EWOULDBLOCK or EAGAIN");
							bReadOk = true;
							break;
						}
						else
						{
							close(sfd);
							DEBUG_FLOWLOG("other error");
							break;
						}
					}
					else if (iRecvNum == 0)
					{
						DEBUG_FLOWLOG("client close request");
						close(sfd);
						break;
					}
					else
					{
						sInput += string(line);
						if (iRecvNum == MAXLINE)
						{
							continue;
						}
						else 
						{
							bReadOk = true;
							break;
						}
					}
				}

				if (bReadOk == true)
				{
					DEBUG_FLOWLOG("recv: %d, read from client: %s", sInput.length(), sInput.c_str());
					printf("read from client: %s\n", sInput.c_str());

					ret = epoll_manager.EpollCtlMod(sfd, EPOLLOUT|EPOLLET);
					if (ret < 0)
					{
						DEBUG_FLOWLOG("EpollCtlAdd error: %d", sfd);
						return -1;
					}
				}
			}
			else if (events[i].events & EPOLLOUT) // 如果有数据发送
			{
				char sendContent[] = "HTTP/1.0 200 OK\r\nServer: jdbhttpd/0.1.0\r\nContent-Type: text/html\r\n\r\n";

				//send完成标志位
				bool bWritten = false;
				//每次send成功的字节数
				int writenLen = 0;

				//循环发送
				while (true)
				{
					writenLen = send(sfd, sendContent, sizeof(sendContent), 0);
					DEBUG_FLOWLOG("writenLen: %d, sizeof(sendContent): %d, strlen(sendContent): %d", writenLen, sizeof(sendContent), strlen(sendContent));

					if (writenLen < 0)
					{
						if (errno == EINTR)
						{
							DEBUG_FLOWLOG("EINTR");
							continue;
						}
						else if (errno == EWOULDBLOCK || errno == EAGAIN)
						{
							DEBUG_FLOWLOG("[send]EWOULDBLOCK or EAGAINs");
							bWritten = true;
							break;
						}
						else
						{
							close(sfd);
							DEBUG_FLOWLOG("other error");
							break;
						}
					}
					else if (writenLen == 0)
					{
						close(sfd);
						bWritten = true;
						DEBUG_FLOWLOG("client closed");
						break;
					}
					else
					{
						bWritten = true;
						DEBUG_FLOWLOG("writenLen ok");
						break;
					}
				}

				if (bWritten == true)
				{
					ret = epoll_manager.EpollCtlMod(sfd, EPOLLIN | EPOLLET);
					if (ret < 0)
					{
						DEBUG_FLOWLOG("EpollCtlMod error: %d", sfd);
						return -1;
					}
				}
			}
			/*************循环send的逻辑，无法判断什么时候send完成
			else if (events[i].events & EPOLLOUT) // 如果有数据发送
			{
				char sendContent[] = "hello from epoll : this is a long string which may be cut by the net";
				char* head = sendContent;

				//send完成标志位
				bool bWritten = false;
				//每次send成功的字节数
				int writenLen = 0;
				//发送的总字节数
				int count = 0;
				//循环发送
				while (true)
				{
					writenLen = send(sfd, head, MAXLINE, 0);
					//发送总节数
					count += writenLen;
					DEBUG_FLOWLOG("writenLen: %d, sizeof(sendContent): %d, strlen(sendContent): %d", writenLen, sizeof(sendContent), strlen(sendContent));
					if (writenLen < 0)
					{
						if (errno == EINTR)
						{
							DEBUG_FLOWLOG("EINTR");
							continue;
						}
						else if (errno == EWOULDBLOCK || errno == EAGAIN)
						{
							DEBUG_FLOWLOG("[send]EWOULDBLOCK or EAGAINs");
							bWritten = true;
							break;
						}
						else
						{
							close(sfd);
							DEBUG_FLOWLOG("other error");
							break;
						}
					}
					else if (writenLen == 0)
					{
						close(sfd);
						bWritten = true;
						DEBUG_FLOWLOG("writenLen ok");
						break;
					}
					else
					{
						if (count == sizeof(sendContent))
						{
							DEBUG_FLOWLOG("bWritten = true");
							bWritten = true;
							break;
						}
						else 
						{
							head += writenLen;
							DEBUG_FLOWLOG("head: %p, writenLen:%d", head, writenLen);
						}
					}
				}

				if (bWritten == true)
				{
					ret = epoll_manager.EpollCtlMod(sfd, EPOLLIN | EPOLLET);
					if (ret < 0)
					{
						DEBUG_FLOWLOG("EpollCtlMod error: %d", sfd);
						return -1;
					}
				}
			}
			*******/
		}
	}

	return 0;
}

int init()
{
	signal(SIGPIPE, SIG_IGN);

	int ret = ILog::instance()->setLogPara("../log/err_log.log", 0, LOG_TFORMAT_0, LOG_TYPE_DAY, 50 << 20);
	if (ret != 0) {
		DEBUG_FLOWLOG("[%u]set error log config failed", getpid());
		return ret;
	}

	ret = IFlowLog::instance()->setLogPara("../log/flow_log.log", 4, LOG_TFORMAT_0, LOG_TYPE_DAY, 50 << 20); 
	if (ret != 0) 
	{
		ERROR_LOG("[%u]set flowing log config failed", getpid());
		return ret;
	}

	return 0;
}

