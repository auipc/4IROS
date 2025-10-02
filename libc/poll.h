#ifndef _LIBC_POLL_H
#define _LIBC_POLL_H
#include <stddef.h>

#define POLLIN (1 << 0)

struct pollfd {
	int fd;
	short events;
	short revents;
};

typedef size_t nfds_t;
int poll(struct pollfd *fds, nfds_t nfds, int timeout);
#endif
