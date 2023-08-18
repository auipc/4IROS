#pragma once
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

	inline PageTableEntry *get_page_table() {
		return (PageTableEntry *)((page_table_base << 12) + 0xC0000000);
	}
} __attribute__((packed));

struct PageDirectory {
	static PageDirectory* clone(PageDirectory* src);

	PageDirectoryEntry entries[1024];
} __attribute__((packed));

class Paging {
  public:
	Paging();
	~Paging();
	static void setup();
};