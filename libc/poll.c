#include <errno.h>
#include <poll.h>
#include <priv/common.h>

int poll(struct pollfd *fds, nfds_t nfds, int timeout) {
	int r = 0;
	asm volatile("syscall"
				 : "=a"(r), "=b"(_errno)
				 : "a"(SYS_POLL), "b"(fds), "d"(nfds), "D"(timeout)
				 : "rcx", "r11", "memory");
	return r;
}
