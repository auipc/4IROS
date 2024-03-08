#ifndef _LIBC_SYS_MMAN_H
#define _LIBC_SYS_MMAN_H
#pragma once
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

void *mmap(void *addr, size_t length);

#ifdef __cplusplus
};
#endif
#endif
