#include <kernel/assert.h>
#include <kernel/mem/Paging.h>
#include <kernel/printk.h>

extern "C" PageDirectory boot_page_directory;

PageDirectory *PageDirectory::clone(PageDirectory *src) {
	PageDirectory *dst = new PageDirectory();
	// TODO find a good way to allocate page tables
	for (int i = 0; i < 1024; i++) {
		dst->entries[i].value = src->entries[i].value;
	}
	return dst;
}


void Paging::setup() {
	for (int i = 0; i < 1024; i++) {
		assert(boot_page_directory.entries[i].present <= 1);
		if (boot_page_directory.entries[i].present) {
			printk("Page directory entry %d is present\n", i);
			// page_table_base
			uint32_t page_table_base = boot_page_directory.entries[i].page_table_base;
			printk("Page table base: %x\n", page_table_base & 0xfffff000u);
			// page_table_base is a physical address, so we need to convert it
			// to a virtual address
			auto page_table = boot_page_directory.entries[i].get_page_table();
			printk("page_table: %x\n", page_table);

			for (int j = 0; j < 1024; j++) {
				if (page_table[j].present) {
					printk("Page table entry %d is present\n", j);
				}
			}
		}
	}
}
