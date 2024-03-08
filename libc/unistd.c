#include <priv/common.h>
#include <priv/util.h>
#include <unistd.h>

int fork() {
	int r = 0;
	SYSCALL(SYS_FORK, r);
	return r;
}

int write(int fd, void *buffer, size_t len) {
	int r = 0;
	asm volatile("int $0x80"
				 : "=a"(r)
				 : "a"(SYS_WRITE), "b"(fd), "c"(buffer), "d"(len));
	return r;
}

_Noreturn void exit(int status) {
	while (1)
		asm volatile("int $0x80" ::"a"(SYS_EXIT), "b"(status));
}
