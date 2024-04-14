#include <priv/common.h>
#include <priv/util.h>
#include <unistd.h>

int fork() {
	int r = 0;
	SYSCALL(SYS_FORK, r);
	return r;
}

int open(const char *path, int flags) {
	int r = 0;
#ifdef __i386__
	asm volatile("int $0x80" : "=a"(r) : "a"(SYS_OPEN), "b"(path), "c"(flags));
#else
#warning Unsupported architecture
#endif
	return r;
}

int read(int fd, void *buf, size_t count) {
	int r = 0;
#ifdef __i386__
	asm volatile("int $0x80"
				 : "=a"(r)
				 : "a"(SYS_READ), "b"(fd), "c"(buf), "d"(count));
#else
#warning Unsupported architecture
#endif
	return r;
}

int write(int fd, void *buf, size_t len) {
	int r = 0;
#ifdef __i386__
	asm volatile("int $0x80"
				 : "=a"(r)
				 : "a"(SYS_WRITE), "b"(fd), "c"(buf), "d"(len));
#else
#warning Unsupported architecture
#endif
	return r;
}

_Noreturn void exit(int status) {
	int r = 0;
#ifdef __i386__
	while (1)
		asm volatile("int $0x80" ::"a"(SYS_EXIT), "b"(status));
#else
#warning Unsupported architecture
	while (1)
		;
#endif
}

int exec(const char *path) {
	int r = 0;
#ifdef __i386__
	asm volatile("int $0x80" : "=a"(r) : "a"(SYS_EXEC), "b"(path));
#else
#warning Unsupported architecture
	while (1)
		;
#endif
	return r;
}

int waitpid(pid_t pid, int *wstatus, int options) {
	int r = 0;
#ifdef __i386__
	asm volatile("int $0x80"
				 : "=a"(r)
				 : "a"(SYS_WAITPID), "b"(pid), "c"(wstatus), "d"(options));
#else
#warning Unsupported architecture
	while (1)
		;
#endif
	return r;
}
