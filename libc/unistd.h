#ifndef _LIBC_UNISTD_H
#define _LIBC_UNISTD_H
#pragma once
#include <stdint.h>

int fork();
int write(int fd, void *buffer, size_t len);
_Noreturn void exit(int status);

#endif
