#include <epoll_manager.h>

EpollManager::EpollManager()
{
	
}

EpollManager::~EpollManager()
{
	
}

int EpollManager::EpollCreate()
{
	epfd_ = epoll_create(MAX_EPFDS);
	if (-1 == epfd_)
	{
		return -1;
	}
	
	return 0;
}

int EpollManager::EpollCtlAdd(int fd, uint32_t events)
{
	struct epoll_event event;
	event.data.fd = fd;
	event.events = events;
	
	if (0 != epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &event))
	{
		return -1;
	}
	
	return 0;
}

int EpollManager::EpollCtlDel(int fd, uint32_t events)
{
	struct epoll_event event;
	event.data.fd = fd;
	event.events = events;
	
	if (0 != epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, &event))
	{
		return -1;
	}
	
	return 0;
}

int EpollManager::EpollCtlMod(int fd, uint32_t events)
{
	struct epoll_event event;
	event.data.fd = fd;
	event.events = events;
	
	if (0 != epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &event))
	{
		return -1;
	}
	
	return 0;
}

int EpollManager::EpollWait()
{
	return epoll_wait(epfd_, events_, EVENTS_SIZE, 5000);	
}
