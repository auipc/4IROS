#ifndef _LIBC_UNISTD_H
#define _LIBC_UNISTD_H
#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

int fork();
int write(int fd, void *buffer, size_t len);
_Noreturn void exit(int status);

#ifdef __cplusplus
};
#endif
#endif
