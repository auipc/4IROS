#pragma once
#include <kernel/arch/i386/kernel.h>
#include <kernel/assert.h>
#include <stddef.h>
#include <stdint.h>

extern bool g_use_halfway_allocator;
void kmalloc_temp_init();
void kmalloc_init();


void actual_malloc_init(void* addr, size_t size);
void* actual_malloc(size_t size);
void actual_free(void* addr);
void *kmalloc(size_t size);
void *kmalloc_aligned(size_t size, size_t alignment);
void kfree(void *ptr);

#define MALLOC_NEW_MUST_BE_PAGE_ALIGNED                                                   \
	void *operator new(size_t size) {                                          \
		return kmalloc_aligned(size, PAGE_SIZE);                               \
	}                                                                          \
	void *operator new[](size_t size) {                                        \
		return kmalloc_aligned(size, PAGE_SIZE);                               \
	}                                                                          \
	void *operator new(size_t, void* place) {                                          \
		return place;                               \
	}                                                                          \
	void operator delete(void *) { assert(false); }                            \
	void operator delete[](void *) { assert(false); }

inline void *operator new(size_t, void *p) throw() { return p; }
inline void *operator new[](size_t, void *p) throw() { return p; }
inline void operator delete(void *, void *) throw(){};
inline void operator delete[](void *, void *) throw(){};
