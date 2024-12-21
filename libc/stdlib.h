#ifndef _LIBC_STDLIB_H
#define _LIBC_STDLIB_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

#define RAND_MAX ((1 << 32) - 1)

int rand();
void srand(unsigned int seed);
void qsort(void *base, size_t nel, size_t width,
		   int (*compar)(const void *, const void *));
void *malloc(size_t);
void *realloc(void *ptr, size_t size);
void free(void *addr);

#ifdef __cplusplus
};
#endif
#endif
