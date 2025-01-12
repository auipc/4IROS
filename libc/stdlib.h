#ifndef _LIBC_STDLIB_H
#define _LIBC_STDLIB_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

#define RAND_MAX ((1 << 32) - 1)

long strtol(const char* nptr, char** endptr, int base);
char* getenv(const char* name);
int abs(int x);
int rand();
__attribute__ ((malloc))
void *calloc(size_t, size_t);
__attribute__ ((malloc))
void *malloc(size_t);
__attribute__ ((malloc))
void *realloc(void *ptr, size_t size);
void free(void *addr);
void srand(unsigned int seed);
void qsort(void *base, size_t nel, size_t width,
		   int (*compar)(const void *, const void *));

#ifdef __cplusplus
};
#endif
#endif
