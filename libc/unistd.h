#ifndef _LIBC_UNISTD_H
#define _LIBC_UNISTD_H
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#ifdef __cplusplus
extern "C" {
#endif

int fork();
int open(const char *path, int flags);
int read(int fd, void *buf, size_t count);
int write(int fd, void *buffer, size_t len);
_Noreturn void exit(int status);
int exec(const char *path);
int waitpid(pid_t pid, int *wstatus, int options);

#ifdef __cplusplus
};
#endif
#endif
