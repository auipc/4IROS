#include <kernel/arch/i386/kernel.h>
#include <kernel/assert.h>
#include <kernel/mem/Paging.h>
#include <kernel/mem/malloc.h>
#include <kernel/printk.h>
#include <kernel/util/NBitmap.h>
#include <string.h>

extern "C" char _heap_start;
extern "C" char _kernel_end;
static size_t s_mem_offset = 0;
static size_t s_mem_end = 0;
static bool s_use_real_allocator = false;
bool g_use_halfway_allocator = false;
static NBitmap *s_mem_nbm;

static size_t s_alloc_base = 0;
static const size_t k_allocation_block_size = 4096;
static const size_t BOOTSTRAP_MEMORY = 0x100000;

void kmalloc_temp_init() {
	s_mem_offset = reinterpret_cast<size_t>(&_heap_start);
	s_mem_offset += 4096 - (s_mem_offset % 4096);
	s_mem_end = s_mem_offset + BOOTSTRAP_MEMORY;
	assert(s_mem_end != 0);
}

void kmalloc_init() {
	s_alloc_base = Paging::page_align(reinterpret_cast<size_t>(&_kernel_end) +
									  (2 * PAGE_SIZE));
	s_mem_nbm = new NBitmap((5000 * KB) / k_allocation_block_size);

	for (size_t i = 0; i < 5000 * KB; i += k_allocation_block_size) {
		if (Paging::the()->is_mapped(Paging::s_pf_allocator_end + i)) {
			s_mem_nbm->set(i, 1);
		}
	}

	for (size_t i = Paging::page_align(0xC03FF000 - Paging::s_pf_allocator_end);
		 i < Paging::page_align(0xC03FF000 - Paging::s_pf_allocator_end) +
				 (PAGE_SIZE * 10);
		 i += PAGE_SIZE) {
		s_mem_nbm->set(i / k_allocation_block_size, 1);
	}

	Paging::s_kernel_page_directory->map_range(Paging::s_pf_allocator_end,
											   (5000 * KB), 0);

	s_use_real_allocator = true;
}

static void *allocate_block_temp(size_t blocks_needed) {
	assert(blocks_needed > 0);
	s_mem_offset += blocks_needed * k_allocation_block_size;
	assert(s_mem_offset < s_mem_end);
	return reinterpret_cast<void *>(s_mem_offset);
}

static void *allocate_block(size_t blocks_needed) {
	assert(blocks_needed > 0);
	if (!s_use_real_allocator)
		return allocate_block_temp(blocks_needed);

	// FIXME: We really need to stop playing fast and loose with types
	size_t free_blocks =
		Paging::s_pf_allocator_end +
		(s_mem_nbm->scan(blocks_needed) * k_allocation_block_size);

	if (!Paging::the()->is_mapped(free_blocks)) {
		assert(false);
	}

	return reinterpret_cast<void *>(free_blocks);
}

void *kmalloc(size_t size) {
	if (g_use_halfway_allocator) {
		return reinterpret_cast<void *>(Paging::pf_allocator(size));
	}

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

	memset((char *)block, 0, blocks_needed * k_allocation_block_size);
	return block;
}

void *kmalloc_aligned(size_t size, size_t alignment) {
	(void)alignment;
	// fallback because lazy
	if (!s_use_real_allocator)
		return kmalloc(size);
	return allocate_block_temp(size / k_allocation_block_size);
}

void kfree(void *ptr) {
	if (s_mem_nbm)
		s_mem_nbm->group_free(((size_t)ptr - Paging::s_pf_allocator_end) /
							  k_allocation_block_size);
}

void *operator new(size_t size) { return kmalloc(size); }

void *operator new[](size_t size) { return kmalloc(size); }

void operator delete(void *p) { kfree(p); }
void operator delete(void *p, unsigned long) { kfree(p); }
void operator delete(void *p, unsigned int) { kfree(p); }
void operator delete[](void *p) { kfree(p); }
void operator delete[](void *p, unsigned long) { kfree(p); }
void operator delete[](void *p, unsigned int) { kfree(p); }
