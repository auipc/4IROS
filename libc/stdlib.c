#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <math.h>
#include <limits.h>

static const uint64_t SALT = 0x3555555555333333;
static uint64_t _seed = 9438029384980948;
void srand(unsigned int seed) {
	_seed = seed;
}
int rand() {
	asm volatile("rorq $1, %0":"=m"(_seed):"m"(_seed));
	_seed ^= SALT;
	return _seed%INT_MAX;
}
// I suck
void qsort(void *base, size_t nel, size_t width, int (*compar)(const void *, const void *)) {
	if (nel <= 1) return;
	uint8_t* a = (uint8_t*)base;

        for (size_t i = 0; i < nel; i++) {
                for (size_t j = i; j < nel; j++) {
                        if (compar(&a[(j*width)], &a[(i*width)]) == -1)
			{
				uint8_t* m = (uint8_t*)malloc(width);
				memcpy(m, &a[i*width], width);
				memcpy(&a[i*width], &a[j*width], width);
				memcpy(&a[j*width], m, width);
				free(m);
			}
                }
        }
}
