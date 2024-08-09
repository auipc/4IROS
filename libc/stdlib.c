#include <stdlib.h>
#include <sys/mman.h>

#ifdef __i386__
static size_t addr = 0;

void *malloc(size_t size) {
	if (!addr) {
		addr = (size_t)mmap((void *)0x10000, 0x10000);
	}
	void *p = (void *)addr;
	addr += size;
	return p;
}
#endif
