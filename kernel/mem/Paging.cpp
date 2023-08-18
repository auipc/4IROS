#include <kernel/mem/Paging.h>
#include <kernel/printk.h>
#include <kernel/assert.h>

extern "C" PageDirectoryEntry boot_page_directory[4096];

void Paging::setup() {
	int num_present = 0;
	for (int i = 0; i < 4096; i++) {
		assert(boot_page_directory[i].present <= 1);
		if (boot_page_directory[i].present) {
			printk("Page directory entry %d is present\n", i);
			// page_table_base
			uint32_t page_table_base = boot_page_directory[i].page_table_base;
			printk("Page table base: %x\n", page_table_base & 0xfffff000u);
			// page_table_base is a physical address, so we need to convert it
			// to a virtual address
			uint32_t *page_table = (uint32_t *)(page_table_base + 0xC0000000);
			printk("page_table: %x\n", page_table);
			/*for (int j = 0; j < 1024; j++) {
				if (page_table[j] & 1) {
					printk("Page table entry %d is present\n", j);
				}
			}*/
			num_present++;
		}
	}

	printk("Page directory has %d present entries\n", num_present);
}