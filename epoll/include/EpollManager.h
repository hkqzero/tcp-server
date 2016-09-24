/*
 * =====================================================================================
 *
 * @Filename: EpollManager.h
 * @Description: Epoll Manager 
 * @Version: 4.0
 * @Created time: 2016-06-30
 * @Update time: 2016-06-30
 * @Author: limeng31
 *
 * =====================================================================================
 */

#ifndef __EPOLL_MANAGER_H__
#define __EPOLL_MANAGER_H__

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/prctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <list>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/epoll.h>

class EpollManager
{
public:
    EpollManager();
	~EpollManager();
	int EpollCreate();
	int EpollCtlAdd(int fd, uint32_t events);
	int EpollCtlDel(int fd, uint32_t events);
	int EpollCtlMod(int fd, uint32_t events);
	int EpollWait();
	struct epoll_event* EpollEvents(){return events_;}
	int handle(){return epfd_;}	

private:
	#define MAX_EPFDS 1024
	#define EVENTS_SIZE 20
	
	struct epoll_event events_[EVENTS_SIZE];

    int epfd_;
};

#endif

