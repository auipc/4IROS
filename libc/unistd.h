#ifndef _LIBC_UNISTD_H
#define _LIBC_UNISTD_H
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#ifdef __cplusplus
extern "C" {
#endif

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2


int sleep(unsigned int s);
int isatty();
int fork();
int open(const char *path, int flags, ...);
int read(int fd, void *buf, size_t count);
int write(int fd, void *buffer, size_t len);
off_t lseek(int fd, off_t offset, int whence);
_Noreturn void exit(int status);
int execvp(const char *path, const char **argv);
int waitpid(pid_t pid, int *wstatus, int options);
void *mmap(void *address, size_t length);
int msleep(uint64_t ms);
int usleep(uint64_t us);

#ifdef __cplusplus
};
#endif
#endif
