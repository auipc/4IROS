#pragma once
#include <kernel/stdint.h>

// TODO Specify the amount of memory available to us
void kmalloc_init();

void *kmalloc(size_t size);

inline void *operator new(size_t, void *p) throw() { return p; }
inline void *operator new[](size_t, void *p) throw() { return p; }
inline void operator delete(void *, void *) throw(){};
inline void operator delete[](void *, void *) throw(){};
