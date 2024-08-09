#pragma once
#include <stdint.h>

enum PageInterfaceFlags {
	Present = (1<<0);
	Readable = (1<<1);
	Writable = (1<<2);
	Accessed = (1<<3);
	Executable = (1<<4);
};

// High level interface for individual CPU's paging.
class PagingInterface {
public:
	virtual void map_page(uintptr_t virt, uintptr_t phys);
	virtual void unmap_page(uintptr_t virt);
	virtual bool is_mapped(uintptr_t virt);
	virtual  mapped_page_flags(uintptr_t virt);
};
