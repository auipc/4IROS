#pragma once
#include <kernel/stdint.h>

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
} __attribute__((packed));

class Paging {
  public:
	Paging();
	~Paging();
	static void setup();
};