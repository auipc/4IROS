#include <kernel/assert.h>
#include <kernel/mem/malloc.h>
#include <kernel/printk.h>
#include <kernel/string.h>

extern "C" char _heap_start;
static size_t s_mem_pointer = 0;
static size_t s_mem_end = 0;

static const size_t k_allocation_block_size = 4096;

// sort of a bitmap
// include more note keeping information in the header.
struct AllocationBlockHeader {
	bool used : 1;
	size_t span_in_blocks;
};

static AllocationBlockHeader *block_headers = nullptr;
static size_t block_headers_length = 0;
static const size_t BOOTSTRAP_MEMORY = 0x100000;

void kmalloc_init() {
	// Reserve space for the block headers.
	size_t block_header_size = sizeof(AllocationBlockHeader) *
							   (BOOTSTRAP_MEMORY / k_allocation_block_size);
	block_headers = reinterpret_cast<AllocationBlockHeader *>(&_heap_start);
	memset(reinterpret_cast<char *>(block_headers), 0, block_header_size);

	block_headers_length = block_header_size / sizeof(AllocationBlockHeader);

	s_mem_pointer = (size_t)&_heap_start + block_header_size;
	// FIXME: When paging is enabled, this should be able to grow.
	s_mem_end = reinterpret_cast<size_t>(&_heap_start) +
				(BOOTSTRAP_MEMORY - block_header_size);
	printk("s_mem_pointer %x, s_mem_end %x\n", s_mem_pointer, s_mem_end);
	assert(s_mem_pointer != 0);
	assert(s_mem_end != 0);
	assert(block_headers_length != 0);
}

void *allocate_block(size_t blocks_needed) {
	assert(block_headers_length != 0);
	assert(block_headers != nullptr);
	for (size_t i = 0; i < block_headers_length; i++) {
		if (!block_headers[i].used) {
			block_headers[i].used = true;
			block_headers[i].span_in_blocks = blocks_needed;
			printk("Allocating block at %x\n",
				   s_mem_pointer + (i * k_allocation_block_size));
			return reinterpret_cast<void *>(s_mem_pointer +
											(i * k_allocation_block_size));
		} else {
			// skip ahead
			i += block_headers[i].span_in_blocks;
		}
	}

	return nullptr;
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

	return block;
}

void *kmalloc_aligned(size_t size, size_t alignment) {
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

	// TODO hack, discards too much memory, also might fail on crazy
	// misalignment's.
	size_t misalignment = reinterpret_cast<size_t>(block) % alignment;
	if (misalignment != 0) {
		if ((k_allocation_block_size - misalignment) > size) {
			printk("Aligned allocation failed, halting...\n");
			while (1)
				;
		}
		block += alignment - misalignment;
	}

	return block;
}

void kfree(void *ptr) { free_blocks(ptr); }

void *operator new(size_t size) { return kmalloc(size); }

void *operator new[](size_t size) { return kmalloc(size); }

void operator delete(void *p) { kfree(p); }

void operator delete[](void *p) { kfree(p); }
