#include <kernel/assert.h>
#include <kernel/mem/Paging.h>
#include <kernel/mem/malloc.h>
#include <kernel/printk.h>
#include <kernel/string.h>

// FIXME What should we do with boot_page_directory? Since it's wasted memory
// after we switch to our own page directory.
// Maybe move it to a different section that we can discard for extra memory?
extern "C" PageDirectory boot_page_directory;
Paging *Paging::s_instance = nullptr;
PageDirectory *Paging::s_kernel_page_directory = nullptr;
PageDirectory *Paging::s_current_page_directory = &boot_page_directory;

Paging *Paging::the() { return s_instance; }

// Instead of new_directory, maybe we could just have a page directory for
// operating on pages...
PageTable *PageTable::clone() {
	PageTable *src = new PageTable();
	memcpy(src, this, sizeof(PageTable));
	PageTable *dst = new PageTable();

	// page aligned
	assert(reinterpret_cast<uint32_t>(dst) % PAGE_SIZE == 0);

	for (int i = 0; i < 1024; i++) {
		if (!src->entries[i].present)
			continue;

		// just copy all values, what could go wrong.
		dst->entries[i].value = src->entries[i].value;
		auto free_page = Paging::the()->m_allocator->find_free_page();

		printk("free page: %x\n", free_page);
		printk("physical address: %x\n", free_page);

		Paging::the()->map_page(free_page + VIRTUAL_ADDRESS, free_page, false);
		printk("Copying from %x to %x\n",
			   src->entries[i].get_page_base() + VIRTUAL_ADDRESS,
			   free_page + VIRTUAL_ADDRESS);
		memcpy(reinterpret_cast<void *>(free_page + VIRTUAL_ADDRESS),
			   reinterpret_cast<void *>(src->entries[i].get_page_base() +
										VIRTUAL_ADDRESS),
			   PAGE_SIZE);

		// TODO we need to be able to temporarily identity map this address so
		// we can copy it's contents
		printk("free page: %x\n", Paging::the()->m_allocator->find_free_page());
	}

	// FIXME memory leak
	return dst;
}

void PageDirectory::map_page(size_t virtual_address, size_t physical_address,
							 bool user_supervisor) {
	auto page_directory_index = get_page_directory_index(virtual_address);
	auto page_table_index = get_page_table_index(virtual_address);

	if (!entries[page_directory_index].present) {
		entries[page_directory_index].set_page_table(new PageTable());
		entries[page_directory_index].present = 1;
		entries[page_directory_index].read_write = 1;
		entries[page_directory_index].user_supervisor = user_supervisor;
		printk("Allocating page table: %x\n",
			   entries[page_directory_index].get_page_table());
	} else {
		printk("Page table already exists: %x\n", page_directory_index);
		printk("Is writable: %d\n", entries[page_directory_index].read_write);
	}

	auto &page_table_entry = entries[page_directory_index]
								 .get_page_table()
								 ->entries[page_table_index];

	page_table_entry.set_page_base(physical_address);
	page_table_entry.user_supervisor = user_supervisor;
	page_table_entry.read_write = 1;
	page_table_entry.present = 1;

	printk("Mapped page: %x -> %x\n", virtual_address, physical_address);

	if (this == Paging::the()->current_page_directory()) {
		printk("Invalidating page: %x\n", virtual_address);
		asm volatile("invlpg (%0)" ::"r"(virtual_address) : "memory");
	}
}

void PageDirectory::map_range(size_t virtual_address, size_t length,
							  bool user_supervisor) {
	if (!length)
		return;
	if (length < PAGE_SIZE) {
		length += PAGE_SIZE - length;
	}

	for (size_t i = 0; i < (length / PAGE_SIZE); i++) {
		map_page(virtual_address + (i * PAGE_SIZE),
				 Paging::the()->m_allocator->find_free_page(), user_supervisor);
	}
}

void PageDirectory::unmap_page(size_t virtual_address) {
	(void)virtual_address;
	assert(!"Implement me!");
}

PageDirectory *PageDirectory::clone() {
	PageDirectory *src = this;
	PageDirectory *dst = new PageDirectory();

	// page aligned
	assert(reinterpret_cast<uint32_t>(dst) % PAGE_SIZE == 0);

	// TODO find a good way to allocate page tables
	// TODO every page should share the kernels page tables, mostly for
	// interrupts and kernel tasks.
	for (int i = 0; i < 1024; i++) {
		if (!src->entries[i].present)
			continue;
		// just copy all values, what could go wrong.
		dst->entries[i].value = src->entries[i].value;

		if (Paging::the()->s_kernel_page_directory->entries[i].present) {
			printk("Skipping full clone of kernel page table\n");
			continue;
		}

		printk("Cloning page table: %d\n", i);
		dst->entries[i].set_page_table(
			src->entries[i].get_page_table()->clone());
		printk("src->entries[%d].value: %x\n", i, src->entries[i].value);
	}

	return dst;
}

extern "C" char _kernel_start;
extern "C" char _kernel_end;

Paging::Paging() {
	m_allocator = new PageFrameAllocator(TOTAL_MEMORY);

	// Reserve memory for the kernel
	m_allocator->mark_range(0, get_physical_address(&_kernel_end));
}

Paging::~Paging() { delete m_allocator; }

void Paging::setup() {
	s_instance = new Paging();
	s_kernel_page_directory = &boot_page_directory;
	switch_page_directory(s_kernel_page_directory);
}
