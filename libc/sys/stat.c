#include <errno.h>
#include <priv/common.h>
#include <sys/stat.h>

int stat(const char *path, struct stat *s) {
	int r = 0;
	// FIXME LEAVING THIS HERE WE NEED TO STOP OVERWRITING ERRNO WHEN THERE IS
	// NO ERROR!!!
	asm volatile("syscall"
				 : "=a"(r), "=b"(_errno)
				 : "a"(SYS_STAT), "b"(path), "d"(s)
				 : "rcx", "r11", "memory");
	return r;
}

int fstat(int fd, struct stat *s) {
	int r = 0;
	// FIXME LEAVING THIS HERE WE NEED TO STOP OVERWRITING ERRNO WHEN THERE IS
	// NO ERROR!!!
	asm volatile("syscall"
				 : "=a"(r), "=b"(_errno)
				 : "a"(SYS_FSTAT), "b"(fd), "d"(s)
				 : "rcx", "r11", "memory");
	return r;
}
