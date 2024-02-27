#include <kernel/arch/i386/kernel.h>
#include <kernel/assert.h>
#include <kernel/mem/Paging.h>
#include <kernel/mem/malloc.h>
#include <kernel/printk.h>
#include <kernel/string.h>

extern "C" char _heap_start;
static size_t s_mem_pointer = 0;
static size_t s_mem_end = 0;

static const size_t k_allocation_block_size = 512;

// sort of a bitmap
// include more note keeping information in the header.
struct AllocationBlockHeader {
	bool used : 1;
	size_t span_in_blocks;
};

static AllocationBlockHeader *block_headers = nullptr;
static size_t block_headers_length = 0;
static const size_t BOOTSTRAP_MEMORY = 0x100000;

// FIXME this is a lie
#define TOTAL_MEMORY 100 * MB

void kmalloc_init() {
	// Reserve space for the block headers.
	size_t block_header_size = sizeof(AllocationBlockHeader) *
							   (BOOTSTRAP_MEMORY / k_allocation_block_size);
	block_headers = reinterpret_cast<AllocationBlockHeader *>(&_heap_start);
	memset(reinterpret_cast<char *>(block_headers), 0, block_header_size);

	block_headers_length = block_header_size / sizeof(AllocationBlockHeader);

	s_mem_pointer = (size_t)&_heap_start + block_header_size;

	// FIXME: When paging is enabled, this should be able to grow.
	// 400 padding to grow not really growth lol
	s_mem_end = reinterpret_cast<size_t>(&_heap_start) +
				(BOOTSTRAP_MEMORY - block_header_size -
				 ((TOTAL_MEMORY / k_allocation_block_size) *
				  sizeof(AllocationBlockHeader)));
	assert(s_mem_pointer != 0);
	assert(s_mem_end != 0);
	assert(block_headers_length != 0);
}

int count_used_memory() {
	int available = 0;
	for (size_t i = 0; i < block_headers_length; i++) {
		if (block_headers[i].used)
			available += k_allocation_block_size;
	}

	return available;
}

void *allocate_block(size_t blocks_needed) {
	assert(block_headers_length != 0);
	assert(block_headers != nullptr);
	for (size_t i = 0; i < block_headers_length; i++) {
		if (!block_headers[i].used) {
			block_headers[i].used = true;
			block_headers[i].span_in_blocks = blocks_needed;
			printk("Memory usage %.2f/%d KB\n",
				   ((float)count_used_memory()) / ((float)KB),
				   (block_headers_length * k_allocation_block_size) / KB);
			return reinterpret_cast<void *>(s_mem_pointer +
											(i * k_allocation_block_size));
		} else {
			// skip ahead
			i += block_headers[i].span_in_blocks;
		}
	}

	// If we're out of memory and paging isn't setup, return nullptr.
	if (!Paging::the())
		return nullptr;

	printk("Out of memory, allocating more...\n");
	// Allocate more memory
	Paging::the()->map_range(
		Paging::page_align(
			reinterpret_cast<size_t>(block_headers) +
			(block_headers_length * sizeof(AllocationBlockHeader))),
		k_allocation_block_size * blocks_needed, PageFlags::NONE);
	block_headers_length += blocks_needed;

	return reinterpret_cast<void *>(
		(reinterpret_cast<size_t>(block_headers_length - blocks_needed) *
		 reinterpret_cast<size_t>(k_allocation_block_size)));
	// Paging::the()->map_page(size_t virtual_address, size_t physical_address,
	// bool user_supervisor)
}

void free_blocks(void *block) {
	assert(block_headers_length != 0);
	assert(block_headers != nullptr);

	// dumb slow linear search
	for (size_t i = 0; i < block_headers_length; i++) {
		if (reinterpret_cast<size_t>(block) ==
			s_mem_pointer + (i * k_allocation_block_size)) {
			block_headers[i].used = false;
			block_headers[i].span_in_blocks = 0;
		}
	}
}

void *kmalloc(size_t size) {
	size_t blocks_needed = size / k_allocation_block_size;
	if (size % k_allocation_block_size != 0) {
		blocks_needed++;
	}

	void *block = allocate_block(blocks_needed);
	if (block == nullptr) {
		printk("Out of memory, halting...\n");
		while (1)
			;
	}

	memset((char*)block, 0, blocks_needed*k_allocation_block_size);
	return block;
}

void *kmalloc_aligned(size_t size, size_t alignment) {
	size_t blocks_needed = size / k_allocation_block_size;
	if (size % k_allocation_block_size != 0) {
		blocks_needed++;
	}

	size_t block = reinterpret_cast<size_t>(allocate_block(blocks_needed));
	if (!block) {
		printk("Out of memory, halting...\n");
		while (1)
			;
	}

	// TODO hack, discards too much memory, also might fail on crazy
	// misalignment's.
	size_t misalignment = block % alignment;
	if (misalignment != 0) {
		if ((k_allocation_block_size - misalignment) > size) {
			printk("Aligned allocation failed, halting...\n");
			while (1)
				;
		}
		block += alignment - misalignment;
	}

	memset((char*)block, 0, blocks_needed*k_allocation_block_size);
	return reinterpret_cast<void *>(block);
}

void kfree(void *ptr) { free_blocks(ptr); }

void *operator new(size_t size) { return kmalloc(size); }

void *operator new[](size_t size) { return kmalloc(size); }

void operator delete(void *p) { kfree(p); }
void operator delete(void *p, unsigned long) { kfree(p); }
void operator delete(void *p, unsigned int) { kfree(p); }
void operator delete[](void *p) { kfree(p); }
void operator delete[](void *p, unsigned long) { kfree(p); }
void operator delete[](void *p, unsigned int) { kfree(p); }
