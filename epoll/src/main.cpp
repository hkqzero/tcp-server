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

int main(int argc, char* argv[])
{
	signal(SIGPIPE, SIG_IGN);

	int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	sk_set_nonblock(listen_fd);
	sk_resue_addr(listen_fd);

	if (sk_bind_listen(listen_fd, gserver.my_port) != 0) {
		SCREEN(SCREEN_RED, stderr, "bind(2): %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	EpollManager epoll_manager;
	int ret = epoll_manager.EpollCreate();
	if (ret < 0)
	{
		printf("EpollCreate error\n");
		return -1;
	}
	ret = epoll_manager.EpollCtlAdd(listen_fd, EPOLLIN|EPOLLET);
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
		int n;
		char line[1024];
		for (int i=0; i<nfds; ++i)
		{
			int sfd = events[i]->data.fd;
			if (sfd == listen_fd)//如果新监测到一个SOCKET用户连接到了绑定的SOCKET端口，建立新的连接。
			{
				int connfd = accept(listen_fd, (sockaddr *)&clientaddr, &clilen);
				if (connfd < 0)
				{
					perror("connfd<0");
					exit(1);
				}
				//setnonblocking(connfd);

				char *str = inet_ntoa(clientaddr.sin_addr);
				cout << "accapt a connection from " << str << endl;
			
				//设置用于读操作的文件描述符
				ret = epoll_manager.EpollCtlAdd(connfd, EPOLLIN|EPOLLET);
				if (ret < 0)
				{
					printf("EpollCtlAdd error: %d\n", connfd);
					return -1;
				}
			}
			else if (sfd & EPOLLIN)//如果是已经连接的用户，并且收到数据，那么进行读入。
			{

				cout << "EPOLLIN" << endl;
				if (sfd < 0)
					continue;
				if ((n = read(sfd, line, 1024)) < 0) {
					if (errno == ECONNRESET) {
						close(sfd);
						sfd = -1;
					} else
						cout<<"readline error"<<std::endl;
				} else if (n == 0) {
					close(sfd);
					sfd = -1;
				}
				line[n] = '/0';
				cout << "read " << line << endl;

				ret = epoll_manager.EpollCtlMod(sfd, EPOLLOUT|EPOLLET);
				if (ret < 0)
				{
					printf("EpollCtlAdd error: %d\n", sfd);
					return -1;
				}
			}
			else if (sfd & EPOLLOUT) // 如果有数据发送
			{
				write(sfd, line, n);

				ret = epoll_manager.EpollCtlMod(sfd, EPOLLIN|EPOLLET);
				if (ret < 0)
				{
					printf("EpollCtlMod error: %d\n", sfd);
					return -1;
				}
			}
		}
	}


	return 0;
}
