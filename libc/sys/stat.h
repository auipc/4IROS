#ifndef _LIBC_SYS_STAT_H
#define _LIBC_SYS_STAT_H
#include <stddef.h>

#define S_IFMT 1
#define S_IFBLK 2
#define S_IFCHR 3
#define S_IFDIR 4
#define S_IFIFO 5
#define S_IFLNK 6
#define S_IFREG 7

struct stat {
	int st_mode;
	size_t st_size;
};

int stat(const char *path, struct stat *s);
int fstat(int fd, struct stat *s);

#endif
