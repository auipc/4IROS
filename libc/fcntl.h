#ifndef _LIBC_FCNTL_H
#define _LIBC_FCNTL_H
#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define O_RDONLY (1 << 0)
#define O_WRONLY (1 << 1)
#define O_RDWR (O_RDONLY | O_WRONLY)
#define O_CREAT (1 << 2)
#define O_TRUNC (1 << 3)

#ifdef __cplusplus
};
#endif
#endif
