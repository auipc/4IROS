#ifndef _LIBC_STDLIB_H
#define _LIBC_STDLIB_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

void *malloc(size_t);
void free(void* addr);

#ifdef __cplusplus
};
#endif
#endif
