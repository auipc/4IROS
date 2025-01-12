#ifndef _LIBC_ERRNO_H
#define _LIBC_ERRNO_H
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#define ENOENT 1
#define EBADF 2
#define EINVAL 3
#define EFAULT 4
#define ESRCH 5
#define ENODEV 6
#define ENOMEM 7

#ifndef LIBK
extern int _errno;
#define errno _errno
#endif

#ifdef __cplusplus
};
#endif
#endif
