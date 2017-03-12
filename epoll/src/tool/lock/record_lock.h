#ifndef _RECORD_LOCK_H_
#define _RECORD_LOCK_H_

#include <fcntl.h>

#define read_record_lock(fd, whence, start, len) \
	record_lock((fd), F_SETLK, F_RDLCK, (whence), (start), (len))
#define read_record_lockw(fd, whence, start, len) \
	record_lock((fd), F_SETLKW, F_RDLCK, (whence), (start), (len))
#define write_record_lock(fd, whence, start, len) \
	record_lock((fd), F_SETLK, F_WRLCK, (whence), (start), (len))
#define write_record_lockw(fd, whence, start, len) \
	record_lock((fd), F_SETLK, F_WRLCKW, (whence), (start), (len))
#define un_record_lock(fd, whence, start, len) \
	record_lock((fd), F_SETLK, F_UNLCK, (whence), (start), (len))

static int record_lock(int fd, int cmd, int type, int whence, off_t start, off_t len);

#endif

