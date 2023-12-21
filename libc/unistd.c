#include <unistd.h>
#include <priv/util.h>

int fork() {
	int r = 0;
	SYSCALL(3, r);
	return r;
}

int write(int fd, void* buffer, size_t len) {
	int r = 0;
	asm volatile("int $0x80": "=a"(r) : "a"(0x04), "b"(fd), "c"(buffer), "d"(len));
	return r;
}

_Noreturn void exit(int status) {
	while (1)
		asm volatile("int $0x80":: "a"(0x01), "b"(status));
}
