#ifndef _LIBC_COMMON_H
#define _LIBC_COMMON_H
#pragma once
enum {
	SYS_EXIT = 1,
	SYS_EXEC = 2,
	SYS_FORK = 3,
	SYS_WRITE = 4,
	SYS_READ = 5,
	SYS_OPEN = 6,
	SYS_MMAP = 8,
	SYS_WAITPID = 9,
};
#endif
