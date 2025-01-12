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
}; //PACKED;

SlowLinkedList *last_node = nullptr;
uintptr_t last_addr = 0;
SlowLinkedList *mem_slow_linked_list = nullptr;
void *alloc_base_addr = nullptr;
size_t alloc_base_size = 0;

void actual_malloc_init(void *addr, size_t size) {
	alloc_base_size = size;
	alloc_base_addr = addr;
	mem_slow_linked_list = new (alloc_base_addr) SlowLinkedList();
	mem_slow_linked_list->size = 0;
	mem_slow_linked_list->free = true;
	s_use_actual_allocator = true;
}

static void group_free(SlowLinkedList * node, uintptr_t addr) {
	SlowLinkedList * cur = node;
	(void)cur;
	(void)addr;
	do {
		if (!node->next) {
			cur->next = nullptr;
			return;
		}
		node = node->next;
	} while (node->free);

	cur->size = ((uintptr_t)node)-addr;
	cur->next = node;
}

static uintptr_t slow_align(uintptr_t addr, uintptr_t align) {
	return (addr % align) ? (addr+(align - (addr % align))) : addr;
}

static void *actual_malloc_find_free(size_t size, size_t alignment = 0) {
	SlowLinkedList *selected_node = nullptr;
	SlowLinkedList *current_node = mem_slow_linked_list;
	uintptr_t next_addr = ((uintptr_t)alloc_base_addr);
	next_addr += sizeof(SlowLinkedList);

	do {

		if (current_node->free && current_node->size >= size &&
			current_node->alignment >= alignment) {
			current_node->alignment = alignment;
			if (current_node->alignment > 1)
				next_addr = slow_align(next_addr, current_node->alignment);

			current_node->free = false;
			if (current_node->next)
				group_free(current_node, next_addr);

			if ((current_node->size-size) > sizeof(SlowLinkedList)) {
				auto split_node =
					new ((void *)(next_addr+size)) SlowLinkedList();
				split_node->size = (current_node->size-size)-sizeof(SlowLinkedList);
				split_node->free = true;
				if (!((((uintptr_t)split_node)+sizeof(SlowLinkedList))%16))
					split_node->alignment = 16;
				else
					split_node->alignment = 0;
				split_node->next = current_node->next;
				current_node->size = size;
				current_node->next = split_node;
			}

			memset((void *)next_addr, 0, current_node->size);
			return (void *)next_addr;
		}

		if (current_node->alignment > 1)
			next_addr =
				slow_align(next_addr, current_node->alignment);

#ifdef DEBUG_CANARY
		if (current_node->dbg_canary != 0xDEAD) {
			printk("current_node %x %x %x\n", current_node, &current_node->dbg_canary, current_node->size);
		}
		assert(current_node->dbg_canary == 0xDEAD);
#endif
		next_addr += current_node->size;
		selected_node = current_node;
		current_node = current_node->next;
		next_addr += sizeof(SlowLinkedList);
	} while (current_node);

	if ((next_addr + size) >= ((size_t)alloc_base_addr + alloc_base_size)) {
		Debug::stack_trace();
		panic("OOM");
	}

	auto next_node =
		new ((void *)(next_addr - sizeof(SlowLinkedList))) SlowLinkedList();
	if (alignment > 1)
		next_addr = slow_align(next_addr, alignment);
	selected_node->next = next_node;
	selected_node->next->free = false;
	selected_node->next->size = size;
	selected_node->next->alignment = alignment;
	memset((void *)next_addr, 0, size);
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
			next_addr =
				slow_align(next_addr, current_node->alignment);

		if(current_node->dbg_canary != 0xDEAD) {
			printk("corrupted header addr %x next_addr %x header %x prev_node %x prev_node->next %x\n", addr, next_addr, current_node, prev_node, prev_node->next);
		}
		assert(current_node->dbg_canary == 0xDEAD);
		if (next_addr == (uintptr_t)addr) {
			memset((void *)addr, 0, current_node->size);
			current_node->free = true;
			if (!current_node->next)
				prev_node->next = nullptr;
			return;
		}

		next_addr += current_node->size;
		prev_node = current_node;
		current_node = current_node->next;
		next_addr = (uintptr_t)current_node;
		next_addr += sizeof(SlowLinkedList);
	} while (current_node);

	printk("Couldn't find address %x %x %x\n", addr, current_node, prev_node);
	Debug::stack_trace();
	panic("FIXME");
}
