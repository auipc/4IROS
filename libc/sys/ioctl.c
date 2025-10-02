#include <errno.h>
#include <priv/common.h>
#include <sys/ioctl.h>

int ioctl(int fd, unsigned long op, char *argp) {
	int r = 0;
	asm volatile("syscall"
				 : "=a"(r), "=b"(_errno)
				 : "a"(SYS_IOCTL), "b"(fd), "d"(op), "D"(argp)
				 : "rcx", "r11", "memory");
	return r;
}
