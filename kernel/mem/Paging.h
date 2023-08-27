#pragma once
#include <kernel/arch/i386/kernel.h>
#include <kernel/assert.h>
#include <kernel/mem/PageFrameAllocator.h>
#include <kernel/mem/malloc.h>
#include <kernel/printk.h>
#include <kernel/stdint.h>

struct PageDirectory;

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

	inline void set_page_base(uint32_t base) {
		page_base = (base - VIRTUAL_ADDRESS) >> 12;
	}

	inline uint32_t get_page_base() const {
		return (page_base + VIRTUAL_ADDRESS) << 12;
	}
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
		return reinterpret_cast<PageTable*>((page_table_base << 12) + VIRTUAL_ADDRESS);
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
		return (reinterpret_cast<size_t>(virtual_address) >> 22) & 0x3ff;
	}

	inline uint32_t get_page_table_index(size_t virtual_address) const {
		return (reinterpret_cast<size_t>(virtual_address) >> 12) & 0x3ff;
	}

	void map_page(size_t virtual_address, size_t physical_address,
				  bool user_supervisor);
	void map_range(size_t virtual_address, size_t length, bool user_supervisor);

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
		return s_current_page_directory;
	}

	// FIXME We should be consistent with addresses
	inline static uintptr_t current_page_directory_physical() {
		return reinterpret_cast<uintptr_t>(get_physical_address(
			reinterpret_cast<void *>(s_current_page_directory)));
	}

	inline void map_page(size_t virtual_address, size_t physical_address,
						 bool user_supervisor) {
		s_current_page_directory->map_page(virtual_address, physical_address,
										   user_supervisor);
	}

	inline void map_range(size_t virtual_address, size_t length,
						 bool user_supervisor) {
		s_current_page_directory->map_range(virtual_address, length,
										   user_supervisor);
	}

	inline static size_t page_align(size_t address) {
		return address & ~(PAGE_SIZE - 1);
	}

	inline void unmap_page(size_t virtual_address) {
		s_current_page_directory->unmap_page(virtual_address);
	}

	static Paging *the();

	inline static size_t get_physical_address(void *virtual_address) {
		return reinterpret_cast<size_t>(virtual_address) - VIRTUAL_ADDRESS;
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
