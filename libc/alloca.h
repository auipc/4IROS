#ifndef _LIBC_ALLOCA_H
#define _LIBC_ALLOCA_H
#include <stddef.h>
inline __attribute__((always_inline)) void *alloca(size_t size) {
	return __builtin_alloca(size);
}
#endif
