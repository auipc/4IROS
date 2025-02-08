#include <errno.h>
#include <priv/common.h>
#include <priv/util.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>

int optind = 1;

int access(const char* path, int mode) {
	struct stat st;
	return stat(path, &st);
}

int isatty() { return 1; }

// Syscalls use the System V x64 calling convention (minus rcx) and rax
// specifies the syscall
int fork() {
	int r = 0;
	asm volatile("syscall"
				 : "=a"(r), "=b"(_errno)
				 : "a"(SYS_FORK)
				 : "rcx", "r11", "memory");
	return r;
}

int open(const char *path, int flags, ...) {
	int r = 0;
	asm volatile("syscall"
				 : "=a"(r), "=b"(_errno)
				 : "a"(SYS_OPEN), "b"(path), "d"(flags)
				 : "rcx", "r11", "memory");
	return r;
}

int read(int fd, void *buf, size_t count) {
	int r = 0;
	asm volatile("syscall"
				 : "=a"(r), "=b"(_errno)
				 : "a"(SYS_READ), "b"(fd), "d"(buf), "D"(count)
				 : "rcx", "r11", "memory");
	return r;
}

int write(int fd, void *buf, size_t len) {
	int r = 0;
	asm volatile("syscall"
				 : "=a"(r), "=b"(_errno)
				 : "a"(SYS_WRITE), "b"(fd), "d"(buf), "D"(len)
				 : "rcx", "r11", "memory");
	return r;
}

off_t lseek(int fd, off_t offset, int whence) {
	off_t r = 0;
	asm volatile("syscall"
				 : "=a"(r), "=b"(_errno)
				 : "a"(SYS_LSEEK), "b"(fd), "d"(offset), "D"(whence)
				 : "rcx", "r11", "memory");
	return r;
}

_Noreturn void exit(int status) {
	// Flush the big 3 stdio handles
	// however I think I'm supposed to flush everything including weird line buffered files made by the program
	fflush(stdin);
	fflush(stdout);
	fflush(stderr);
	for (;;)
		asm volatile("syscall"
					 : "=b"(_errno)
					 : "a"(SYS_EXIT), "b"((uint64_t)status)
					 : "rcx", "r11", "memory");
	// asm volatile("movq %1, %%rdi\n"
	//"syscall" :: "a"(SYS_EXIT), "b"((uint64_t)status):"rcx", "r11", "rdi");
}

int execvp(const char *path, const char **argv) {
	int r = 0;
	asm volatile("syscall"
				 : "=a"(r), "=b"(_errno)
				 : "a"(SYS_EXEC), "b"(path), "d"(argv)
				 : "rcx", "r11", "memory");
	return r;
}

int spawn(int* pid, const char *path, const char **argv) {
	int r = 0;
	asm volatile("syscall"
				 : "=a"(r), "=b"(_errno)
				 : "a"(SYS_SPAWN), "b"(pid), "d"(path), "D"(argv)
				 : "rcx", "r11", "memory");
	return r;
}

int waitpid(pid_t pid, int *wstatus, int options) {
	int r = 0;
	asm volatile("syscall"
				 : "=a"(r), "=b"(_errno)
				 : "a"(SYS_WAITPID), "b"(pid), "d"(wstatus), "D"(options)
				 : "rcx", "r11", "memory");
	return r;
}

void *mmap(void *address, size_t length) {
	void *r = 0;
	asm volatile("syscall"
				 : "=a"(r), "=b"(_errno)
				 : "a"(SYS_MMAP), "b"(address), "d"(length)
				 : "rcx", "r11", "memory");
	return r;
}

#if 0
void *mremap(void *old_address, size_t old_size, size_t new_size, int flags, void* new_address) {
	void *r = 0;
	asm volatile("syscall"
				 : "=a"(r), "=b"(_errno)
				 : "a"(SYS_MREMAP), "b"(address), "d"(length)
				 : "rcx", "r11", "memory");
	return r;
}
#endif

int sleep(unsigned int s) { return msleep(s * 1000); }

int msleep(uint64_t ms) {
	return usleep(ms*1000);
}

int usleep(uint64_t us) { 
	int r = 0;
	asm volatile("syscall"
				 : "=a"(r), "=b"(_errno)
				 : "a"(SYS_SLEEP), "b"(us)
				 : "rcx", "r11", "memory");
	return r;

}

int close(int fd) {
	int r = 0;
	asm volatile("syscall"
				 : "=a"(r), "=b"(_errno)
				 : "a"(SYS_CLOSE), "b"(fd)
				 : "rcx", "r11", "memory");
	return r;
}

int chdir(const char* path) {
	int r = 0;
	asm volatile("syscall"
				 : "=a"(r), "=b"(_errno)
				 : "a"(SYS_CHDIR), "b"(path)
				 : "rcx", "r11", "memory");
	return r;
}
