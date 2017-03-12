#include "record_lock.h"

int record_lock(int fd, int cmd, int type, int whence, off_t start, off_t len)
{
	struct flock lock;
	lock.l_type = type;
	lock.l_whence = whence;
	lock.l_start = start;
	lock.l_len = len;

	return (fcntl(fd, cmd, &lock));
}

int main()
{
	return 0;
}

