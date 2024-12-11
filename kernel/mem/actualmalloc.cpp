#include <kernel/Debug.h>
#include <kernel/mem/malloc.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define DEBUG_CANARY

struct SlowLinkedList {
#ifdef DEBUG_CANARY
	uint16_t dbg_canary = 0xDEAD;
#endif
	bool free : 1;
	// x86_64 (4-level) page addresses are only 48 bits wide
	size_t size : 48;
	size_t alignment = 0;
	SlowLinkedList *next = nullptr;
} PACKED;

SlowLinkedList *last_node = nullptr;
uintptr_t last_addr = 0;
SlowLinkedList *mem_slow_linked_list = nullptr;
void *alloc_base_addr = nullptr;
size_t alloc_base_size = 0;
extern bool s_use_actual_allocator;

void actual_malloc_init(void *addr, size_t size) {
	alloc_base_size = size;
	alloc_base_addr = addr;
	mem_slow_linked_list = new (alloc_base_addr) SlowLinkedList();
	mem_slow_linked_list->size = 0;
	mem_slow_linked_list->free = true;
	s_use_actual_allocator = true;
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
		panic("OOM");
	}

	auto next_node = new ((void *)(next_addr-sizeof(SlowLinkedList))) SlowLinkedList();
	if (alignment > 1)
		next_addr += alignment-(next_addr%alignment);
	selected_node->next = next_node;
	selected_node->next->free = false;
	selected_node->next->size = size;
	selected_node->next->alignment = alignment;
	memset((void*)next_addr, 0, size);
	return (void *)next_addr;

}

void *actual_malloc(size_t size) {
	assert(size);
	assert(alloc_base_addr);

	void *free = actual_malloc_find_free(size);
	return free;
}

void *kmalloc_really_aligned(size_t size, size_t aligned) {
	assert(size);
	assert(alloc_base_addr);

	void *free = actual_malloc_find_free(size, aligned);
	return free;
}

void actual_free(void *addr) {
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

	Debug::stack_trace();
	panic("FIXME");
}
