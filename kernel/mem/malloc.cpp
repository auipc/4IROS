#include <kernel/arch/i386/kernel.h>
#include <kernel/assert.h>
#include <kernel/limits.h>
#include <kernel/mem/Paging.h>
#include <kernel/mem/malloc.h>
#include <kernel/printk.h>
#include <kernel/string.h>

extern "C" char _heap_start;
extern "C" char _kernel_end;
static size_t s_mem_offset = 0;
static size_t s_mem_end = 0;
static bool s_use_real_allocator = false;
bool g_use_halfway_allocator = false;
static Bitmap *s_alloc_bitmap;

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
	size_t mem = Paging::page_align(SIZE_MAX - s_alloc_base);
	s_alloc_bitmap = new Bitmap(mem / k_allocation_block_size);

	Paging::the()->map_range(s_alloc_base, 150 * k_allocation_block_size, 0);

	// Find pages used for the PageFrameAllocator
	for (size_t i = s_alloc_base; i < Paging::s_pf_allocator_end; i++) {
		s_alloc_bitmap->set((i - s_alloc_base) / k_allocation_block_size);
	}

	for (size_t i = s_alloc_base; i < s_alloc_base + 4096 * PAGE_SIZE;
		 i += k_allocation_block_size) {
		if (Paging::the()->is_mapped(i)) {
			s_alloc_bitmap->set((i - s_alloc_base) / k_allocation_block_size);
		}
	}

	// FIXME: s_use_real_allocator = true;
}

static void *allocate_block_temp(size_t blocks_needed) {
	assert(blocks_needed > 0);
	s_mem_offset += blocks_needed * k_allocation_block_size;
	printk("mem offset: 0x%x/0x%x\n", s_mem_offset / k_allocation_block_size,
		   s_mem_end / k_allocation_block_size);
	return reinterpret_cast<void *>(s_mem_offset);
}

static void *allocate_block(size_t blocks_needed) {
	assert(blocks_needed > 0);
	if (!s_use_real_allocator)
		return allocate_block_temp(blocks_needed);

	// printk("malloc\n");
	printk("scan %x %d\n", s_alloc_bitmap->scan_no_set(blocks_needed),
		   blocks_needed);
	size_t free_blocks =
		((s_alloc_bitmap->scan(blocks_needed) * k_allocation_block_size) +
		 s_alloc_base);

	printk("k %x\n", s_alloc_base);
	printk("j %x\n", free_blocks);
	if (Paging::the()->is_mapped(free_blocks)) {
		s_alloc_bitmap->set((free_blocks / k_allocation_block_size) -
							s_alloc_base);
		return allocate_block(free_blocks);
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
	// if (Paging::the())
	// assert(false);

	return kmalloc(size);
}

void kfree(void *ptr) { (void)ptr; /*free_blocks(ptr);*/ }

void *operator new(size_t size) { return kmalloc(size); }

void *operator new[](size_t size) { return kmalloc(size); }

void operator delete(void *p) { kfree(p); }
void operator delete(void *p, unsigned long) { kfree(p); }
void operator delete(void *p, unsigned int) { kfree(p); }
void operator delete[](void *p) { kfree(p); }
void operator delete[](void *p, unsigned long) { kfree(p); }
void operator delete[](void *p, unsigned int) { kfree(p); }
