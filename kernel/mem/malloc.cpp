#include <kernel/mem/malloc.h>

extern "C" uint32_t _kernel_end;

static size_t s_mem_pointer = _kernel_end;
static size_t s_mem_end = _kernel_end + 2097152;

void kmalloc_init() {
	//s_mem_end = _kernel_end + 2097152;
}

void* kmalloc(size_t size) {
	size_t ptr = s_mem_pointer;
	s_mem_pointer += size;
	if (s_mem_pointer < s_mem_end) {
		// OOM, halt
		while(1);
	}

	return reinterpret_cast<void*>(ptr);
}

void *operator new(size_t size)
{
    return kmalloc(size);
}
 
void *operator new[](size_t size)
{
    return kmalloc(size);
}
 
void operator delete(void *p)
{
    //free(p);
}
 
void operator delete[](void *p)
{
    //free(p);
}
