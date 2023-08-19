#pragma once
#include <kernel/arch/i386/kernel.h>
#include <kernel/assert.h>
#include <kernel/mem/PageFrameAllocator.h>
#include <kernel/mem/malloc.h>
#include <kernel/printk.h>
#include <kernel/stdint.h>

union PageTableEntry {
	struct {
		uint32_t present : 1;
		uint32_t read_write : 1;
		uint32_t user_supervisor : 1;
		uint32_t write_through : 1;
		uint32_t cache_disabled : 1;
		uint32_t accessed : 1;
		uint32_t dirty : 1;
		uint32_t page_table_attribute_index : 1;
		uint32_t global : 1;
		uint32_t ignored : 3;
		uint32_t page_base : 20;
	};

	uint32_t value;
} __attribute__((packed));

struct PageTable {
	MUST_BE_PAGE_ALIGNED
	PageTable *clone();
	PageTableEntry entries[1024];
} __attribute__((packed)) __attribute__((aligned(PAGE_SIZE)));

union PageDirectoryEntry {
	struct {
		uint32_t present : 1;
		uint32_t read_write : 1;
		uint32_t user_supervisor : 1;
		uint32_t write_through : 1;
		uint32_t cache_disabled : 1;
		uint32_t accessed : 1;
		uint32_t ignored : 1;
		uint32_t page_size : 1;
		uint32_t ignored2 : 4;
		uint32_t page_table_base : 20;
	};

	uint32_t value;

	inline PageTable *get_page_table() {
		return (PageTable *)((page_table_base << 12) + VIRTUAL_ADDRESS);
	}

	inline void set_page_table(PageTable *page_table) {
		page_table_base =
			(reinterpret_cast<uint32_t>(page_table) - VIRTUAL_ADDRESS) >> 12;
	}
} __attribute__((packed));

struct PageDirectory {
	MUST_BE_PAGE_ALIGNED
	PageDirectory *clone();

	PageDirectoryEntry entries[1024];
} __attribute__((packed)) __attribute__((aligned(PAGE_SIZE)));

class Paging {
  public:
	Paging();
	~Paging();
	static void setup();

	inline static PageDirectory *kernel_page_directory() {
		// This shouldn't be null
		assert(s_kernel_page_directory != nullptr);
		return s_kernel_page_directory;
	}

	static Paging *the();

  private:
	friend struct PageTable;
	static PageDirectory *s_kernel_page_directory;

	inline static void switch_page_directory(PageDirectory *page_directory) {
		asm volatile(
			"mov %%eax, %%cr3" ::"a"(get_physical_address(page_directory)));
	}

	inline static size_t get_physical_address(void *virtual_address) {
		return reinterpret_cast<size_t>(virtual_address) - VIRTUAL_ADDRESS;
	}

	PageFrameAllocator *m_allocator;
	static Paging *s_instance;
};