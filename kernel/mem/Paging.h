#pragma once
#include <kernel/arch/i386/kernel.h>
#include <kernel/assert.h>
#include <kernel/mem/PageFrameAllocator.h>
#include <kernel/mem/malloc.h>
#include <kernel/printk.h>
#include <kernel/stdint.h>
#include <kernel/util/Vec.h>

struct PageDirectory;

enum PageFlags {
	NONE,
	READONLY,
	USER,
};

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

	inline void set_page_base(uint32_t base) { page_base = (base) >> 12; }

	inline uint32_t get_page_base() const { return (page_base) << 12; }
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
		return reinterpret_cast<PageTable *>((page_table_base << 12) +
											 VIRTUAL_ADDRESS);
	}

	inline void set_page_table(PageTable *page_table) {
		page_table_base =
			(reinterpret_cast<uintptr_t>(page_table) - VIRTUAL_ADDRESS) >> 12;
	}
} __attribute__((packed));

struct PageDirectory {
	MUST_BE_PAGE_ALIGNED
	PageDirectory *clone();

	inline uint32_t get_page_directory_index(size_t virtual_address) const {
		return (virtual_address >> 22) & 0x3ff;
	}

	inline uint32_t get_page_table_index(size_t virtual_address) const {
		return (virtual_address >> 12) & 0x3ff;
	}

	bool is_mapped(size_t virtual_address);
	void map_page(size_t virtual_address, size_t physical_address, int flags);
	[[maybe_unused]] Vec<uintptr_t> map_range(size_t virtual_address,
											  size_t length, int flags);

	void unmap_page(size_t virtual_address);

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

	inline static PageDirectory *current_page_directory() {
		auto pd =
			reinterpret_cast<PageDirectory *>(get_cr3() + VIRTUAL_ADDRESS);
		return pd;
	}

	// FIXME We should be consistent with addresses
	inline static uintptr_t current_page_directory_physical() {
		return reinterpret_cast<uintptr_t>(get_physical_address(
			reinterpret_cast<void *>(current_page_directory())));
	}

	inline void map_page(size_t virtual_address, size_t physical_address,
						 int flags) {
		current_page_directory()->map_page(virtual_address, physical_address,
										   flags);
	}

	[[maybe_unused]] inline Vec<uintptr_t> map_range(size_t virtual_address,
													 size_t length, int flags) {
		return current_page_directory()->map_range(virtual_address, length,
												   flags);
	}

	inline static size_t page_align(size_t address) {
		return address & ~(PAGE_SIZE - 1);
	}

	inline void unmap_page(size_t virtual_address) {
		current_page_directory()->unmap_page(virtual_address);
	}

	static Paging *the();

	inline static size_t get_physical_address(void *virtual_address) {
		return reinterpret_cast<size_t>(virtual_address) - VIRTUAL_ADDRESS;
	}

	inline static uint32_t get_cr3() {
		uint32_t cr3 = 0;
		asm volatile("mov %%cr3, %%eax" : "=a"(cr3));
		assert(cr3);
		return cr3;
	}

  private:
	friend struct PageDirectory;
	friend struct PageTable;
	static PageDirectory *s_kernel_page_directory;

	inline static void switch_page_directory(PageDirectory *page_directory) {
		asm volatile(
			"mov %%eax, %%cr3" ::"a"(get_physical_address(page_directory)));
		s_current_page_directory = page_directory;
	}

	PageFrameAllocator *m_allocator;
	static Paging *s_instance;
	static PageDirectory *s_current_page_directory;
};
