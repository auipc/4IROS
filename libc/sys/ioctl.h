#ifndef _LIBC_SYS_IOCTL_H
#define _LIBC_SYS_IOCTL_H
#pragma once
#define FIONREAD 0
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

#define TCGETA 1
#define TCSETA 2

int ioctl(int fd, unsigned long op, char *argp);

#ifdef __cplusplus
};
#endif
#endif
