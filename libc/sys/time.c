#include <errno.h>
#include <priv/common.h>
#include <stdlib.h>
#include <sys/time.h>

int gettimeofday(struct timeval *tv, void *tz) {
	int r = 0;
	asm volatile("syscall"
				 : "=a"(r), "=b"(_errno)
				 : "a"(SYS_GETTIMEOFDAY), "b"(tv), "d"(tz)
				 : "rcx", "r11", "memory");
	return r;
}
