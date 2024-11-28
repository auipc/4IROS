#include <stdint.h>
#include <stddef.h>
#include <kernel/mem/malloc.h>
#include <kernel/Debug.h>
#include <string.h>

#define DEBUG_CANARY

struct SlowLinkedList {
#ifdef DEBUG_CANARY
	uint32_t dbg_canary = 0xDEAD;
#endif
	bool free : 1;
	// x86_64 (4-level) page addresses are only 48 bits wide
	size_t size : 48;
	SlowLinkedList* next = nullptr;
};

#define BLOCK_SIZE 512
SlowLinkedList* last_node = nullptr;
uintptr_t last_addr = 0;
SlowLinkedList* mem_slow_linked_list = nullptr;
void* alloc_base_addr = nullptr;
size_t alloc_base_size = 0;
extern bool s_use_actual_allocator;

void actual_malloc_init(void* addr, size_t size) {
	alloc_base_size = size;
	alloc_base_addr = addr;
	mem_slow_linked_list = new (alloc_base_addr) SlowLinkedList();
	mem_slow_linked_list->size = 0;
	mem_slow_linked_list->free = true;
	s_use_actual_allocator = true;
}


static void* actual_malloc_find_free(size_t size) {
	SlowLinkedList* current_node = mem_slow_linked_list;
	uintptr_t next_addr = ((uintptr_t)alloc_base_addr);
	next_addr += sizeof(SlowLinkedList);

	if (last_node) {
		current_node = last_node;
		next_addr = last_addr;
	}

	while((!current_node->free && current_node->size < size) || current_node->size != 0) {
		next_addr += current_node->size;
		if (!current_node->next) {
			if ((next_addr+size) >= ((size_t)alloc_base_addr+alloc_base_size)) {
				panic("OOM");
			}
			//panic("Hgelp\n");
			auto next_node = new ((void*)next_addr) SlowLinkedList();
			next_node->free = true;
			current_node->next = next_node;
		}
#ifdef DEBUG_CANARY
		if (current_node->dbg_canary != 0xDEAD) {
			printk("Debug canary corrupted: %x canary %x size %x\n", current_node, current_node->dbg_canary, current_node->size);
		}
		assert(current_node->dbg_canary == 0xDEAD);
#endif
		assert(current_node->size);
		current_node = current_node->next;
		next_addr += sizeof(SlowLinkedList);
	}

	current_node->free = false;
	current_node->size = size;
	last_node = current_node;
	last_addr = next_addr;
	return (void*)next_addr;
}

void* actual_malloc(size_t size) {
	assert(size);
        assert(alloc_base_addr);
        //size_t p = bitmap->scan((size+BLOCK_SIZE-1)/BLOCK_SIZE);
	void* free = actual_malloc_find_free(size);
        return free;
}

void actual_free(void* addr) {
	SlowLinkedList* prev_node = mem_slow_linked_list;
	SlowLinkedList* current_node = mem_slow_linked_list;
	uintptr_t next_addr = ((uintptr_t)alloc_base_addr);
	while(current_node) {
		next_addr += sizeof(SlowLinkedList);
		if (next_addr == (uintptr_t)addr) break;
		next_addr += current_node->size;
		prev_node = current_node;
		current_node = current_node->next;
	}

	// Better to err on the side of caution
	last_node = nullptr;
	if (!current_node) return;
	if (!current_node->next) {
		prev_node->next = nullptr;
		return;
	}

	memset((char*)addr, 0, current_node->size);

	current_node->free = true;
}
