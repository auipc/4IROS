#pragma once
#include <kernel/arch/i386/kernel.h>
#include <kernel/assert.h>
#include <kernel/stdint.h>

// TODO Specify the amount of memory available to us
void kmalloc_init();

void *kmalloc(size_t size);
void *kmalloc_aligned(size_t size, size_t alignment);
void kfree(void *ptr);

#define MUST_BE_PAGE_ALIGNED                                                   \
	void *operator new(size_t size) {                                          \
		return kmalloc_aligned(size, PAGE_SIZE);                               \
	}                                                                          \
	void *operator new[](size_t size) {                                        \
		return kmalloc_aligned(size, PAGE_SIZE);                               \
	}                                                                          \
	void operator delete(void *) { assert(false); }                            \
	void operator delete[](void *) { assert(false); }

inline void *operator new(size_t, void *p) throw() { return p; }
inline void *operator new[](size_t, void *p) throw() { return p; }
inline void operator delete(void *, void *) throw(){};
inline void operator delete[](void *, void *) throw(){};
