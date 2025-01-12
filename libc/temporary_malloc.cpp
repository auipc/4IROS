#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define DEBUG_CANARY
#define assert(x) while(!(x));

inline void *operator new(size_t, void *p) throw() { return p; }
inline void *operator new[](size_t, void *p) throw() { return p; }
inline void operator delete(void *, void *) throw(){};
inline void operator delete[](void *, void *) throw(){};

struct SlowLinkedList {
#ifdef DEBUG_CANARY
	uint32_t dbg_canary = 0xDEAD;
#endif
	bool free : 1;
	// x86_64 (4-level) page addresses are only 48 bits wide
	size_t size : 48;
	size_t alignment = 0;
	SlowLinkedList *next = nullptr;
};

#define BLOCK_SIZE 512
SlowLinkedList *last_node = nullptr;
uintptr_t last_addr = 0;
SlowLinkedList *mem_slow_linked_list = nullptr;
void *alloc_base_addr = nullptr;
size_t alloc_base_size = 0;

void actual_malloc_init() {
	alloc_base_size = 0x1000;
	alloc_base_addr = mmap((void*)0x90000000, alloc_base_size);
	mem_slow_linked_list = new (alloc_base_addr) SlowLinkedList();
	mem_slow_linked_list->size = 0;
	mem_slow_linked_list->free = true;
}

static void *actual_malloc_find_free(size_t size, size_t alignment=0) {
	SlowLinkedList *selected_node = nullptr;
	SlowLinkedList *current_node = mem_slow_linked_list;
	uintptr_t next_addr = ((uintptr_t)alloc_base_addr);
	next_addr += sizeof(SlowLinkedList);

	do {
		if (current_node->alignment > 1)
			next_addr += current_node->alignment-(next_addr%current_node->alignment);

		if (current_node->free && current_node->size >= size && current_node->alignment == alignment) {
			current_node->free = false;
			memset((void*)next_addr, 0, current_node->size);
			return (void *)next_addr;
		}

#ifdef DEBUG_CANARY
		assert(current_node->dbg_canary == 0xDEAD);
#endif
		next_addr += current_node->size;
		selected_node = current_node;
		current_node = current_node->next;
		next_addr += sizeof(SlowLinkedList);
	} while (current_node);

	if ((next_addr + size) >=
		((size_t)alloc_base_addr + alloc_base_size)) {
		(void)mmap((void*)((uintptr_t)alloc_base_addr+alloc_base_size), size+0xfff);
		alloc_base_size += size+0xfff;
	}

	auto next_node = new ((void *)(next_addr-sizeof(SlowLinkedList))) SlowLinkedList();
	if (alignment > 1) {
		next_addr += alignment-(next_addr%alignment);
		(void)mmap((void*)((uintptr_t)alloc_base_addr+alloc_base_size), alignment-(next_addr%alignment)+0xfff);
		alloc_base_size += alignment-(next_addr%alignment)+0xfff;
	}

	selected_node->next = next_node;
	selected_node->next->free = false;
	selected_node->next->size = size;
	selected_node->next->alignment = alignment;
	memset((void*)next_addr, 0, size);
	return (void *)next_addr;

}

extern "C"
void *malloc(size_t size) {
	if (!size) return 0;
	if (!mem_slow_linked_list) {
		actual_malloc_init();
	}
	assert(alloc_base_addr);

	void *free = actual_malloc_find_free(size, 16);
	return free;
}
extern "C" void free(void *addr);

extern "C"
void *realloc(void* ptr, size_t size) {
	if (!size) return 0;
	if (!mem_slow_linked_list) {
		actual_malloc_init();
	}
	assert(size);
	assert(alloc_base_addr);

	if (!ptr) {
		return malloc(size);
	}

	SlowLinkedList *current_node = mem_slow_linked_list;
	uintptr_t next_addr = ((uintptr_t)alloc_base_addr);
	next_addr += sizeof(SlowLinkedList);
	do {
		if (current_node->alignment > 1)
			next_addr += current_node->alignment-(next_addr%current_node->alignment);

		if (next_addr == (uintptr_t)ptr) {
			break;
		}

		next_addr += current_node->size;
		current_node = current_node->next;
		next_addr += sizeof(SlowLinkedList);
	} while (current_node);

	if (!current_node) return nullptr;

	void* p = malloc(size);
	if (current_node) {
		memcpy(p, ptr, (current_node->size > size) ? size : current_node->size);
		free(ptr);
	}
	return p;
}

extern "C"
void *calloc(size_t n, size_t sz) {
	void* p = malloc(n*sz);
	if (p)
		memset(p, 0, n*sz);
	return p;
}

extern "C"
void free(void *addr) {
	if (!addr) return;
	SlowLinkedList *prev_node = nullptr;
	SlowLinkedList *current_node = mem_slow_linked_list;
	uintptr_t next_addr = ((uintptr_t)alloc_base_addr);
	next_addr += sizeof(SlowLinkedList);

	do {
		if (current_node->alignment > 1)
			next_addr += current_node->alignment-(next_addr%current_node->alignment);

		if (next_addr == (uintptr_t)addr) {
			current_node->free = true;
			memset((void*)addr, 0, current_node->size);
			if (!current_node->next) prev_node->next = nullptr;
			return;
		}

		next_addr += current_node->size;
		prev_node = current_node;
		current_node = current_node->next;
		next_addr += sizeof(SlowLinkedList);
	} while (current_node);

	printf("FIXME\n");
	assert(!"FIXME");
}

