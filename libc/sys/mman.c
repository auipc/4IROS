#include <priv/common.h>
#include <sys/mman.h>

void *mmap(void *addr, size_t length) {
	void *r = NULL;
#ifdef __i386__
	asm volatile("int $0x80" : "=a"(r) : "a"(SYS_MMAP), "b"(addr), "c"(length));
#else
#warning Unsupported architecture
#endif
	return r;
}
