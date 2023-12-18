#include <kernel/assert.h>
#include <kernel/mem/Paging.h>
#include <kernel/mem/malloc.h>
#include <kernel/printk.h>
#include <kernel/string.h>

// #define DEBUG_PAGING

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
	PageTable *src = this;
	PageTable *dst = new PageTable();

	// page aligned
	assert(reinterpret_cast<uint32_t>(dst) % PAGE_SIZE == 0);

	for (int i = 0; i < 1024; i++) {
		if (!src->entries[i].present)
			continue;

		// just copy all values, what could go wrong.
		dst->entries[i].value = src->entries[i].value;
		auto free_page = Paging::the()->m_allocator->find_free_page();

#ifdef DEBUG_PAGING
		printk("free page: %x\n", free_page);
		printk("physical address: %x\n", free_page);
#endif

		Paging::the()->map_page(free_page + VIRTUAL_ADDRESS, free_page, 0);
		Paging::the()->map_page(src->entries[i].get_page_base() + VIRTUAL_ADDRESS, src->entries[i].get_page_base(), 0);
#ifdef DEBUG_PAGING
		printk("Copying from %x to %x\n",
			   src->entries[i].get_page_base() + VIRTUAL_ADDRESS,
			   free_page + VIRTUAL_ADDRESS);
#endif
		memcpy(reinterpret_cast<void *>(free_page + VIRTUAL_ADDRESS),
			   reinterpret_cast<void *>(src->entries[i].get_page_base() + VIRTUAL_ADDRESS),
			   PAGE_SIZE);

		// TODO we need to be able to temporarily identity map this address so
		// we can copy it's contents
#ifdef DEBUG_PAGING
		printk("free page: %x\n", Paging::the()->m_allocator->find_free_page());
#endif
	}

	// FIXME memory leak
	return dst;
}

void PageDirectory::map_page(size_t virtual_address, size_t physical_address,
							 int flags) {
	bool user_supervisor = (flags & PageFlags::USER) != 0;
	bool read_only = (flags & PageFlags::READONLY) != 0;
#ifdef DEBUG_PAGING
	printk("user_supervisor %d read_only %d\n", user_supervisor, read_only);
#endif

	auto page_directory_index = get_page_directory_index(virtual_address);
	auto page_table_index = get_page_table_index(virtual_address);

	if (!entries[page_directory_index].present) {
		entries[page_directory_index].set_page_table(new PageTable());
		entries[page_directory_index].present = 1;
		entries[page_directory_index].read_write = 1;
		if (!entries[page_directory_index].user_supervisor)
			entries[page_directory_index].user_supervisor = user_supervisor;
#ifdef DEBUG_PAGING
		printk("Allocating page table: %x\n",
			   entries[page_directory_index].get_page_table());
#endif
	} else {
#ifdef DEBUG_PAGING
		printk("Page table already exists: %x\n", page_directory_index);
		printk("Is writable: %d\n", entries[page_directory_index].read_write);
#endif
	}

	auto &page_table_entry = entries[page_directory_index]
								 .get_page_table()
								 ->entries[page_table_index];

	memset(reinterpret_cast<char *>(&page_table_entry), 0,
		   sizeof(PageTableEntry));

	page_table_entry.set_page_base(physical_address);
	page_table_entry.user_supervisor = user_supervisor;
	page_table_entry.read_write = !read_only;
	page_table_entry.present = 1;

#ifdef DEBUG_PAGING
	printk("Mapped page: %x -> %x\n", virtual_address, physical_address);
#endif

	if (this == Paging::the()->current_page_directory()) {
#ifdef DEBUG_PAGING
		printk("Invalidating page: %x\n", virtual_address);
#endif
		asm volatile("invlpg (%0)" ::"r"(virtual_address) : "memory");
	}
}

bool PageDirectory::is_mapped(size_t virtual_address) {
	auto page_directory_index = get_page_directory_index(virtual_address);
	auto page_table_index = get_page_table_index(virtual_address);

	if (!entries[page_directory_index].present)
		return false;
	if (!entries[page_directory_index]
			 .get_page_table()
			 ->entries[page_table_index]
			 .present)
		return false;

	return true;
}

bool PageDirectory::is_user_page(size_t virtual_address) {
	auto page_directory_index = get_page_directory_index(virtual_address);
	auto page_table_index = get_page_table_index(virtual_address);
	
	if (!entries[page_directory_index].user_supervisor)
		return false;


	if (!entries[page_directory_index].get_page_table()->entries[page_table_index].user_supervisor)
		return false;

	return true;
}

void PageDirectory::map_range(size_t virtual_address, size_t length,
										int flags) {
	if (!length)
		return;
	if (length < PAGE_SIZE) {
		length += PAGE_SIZE - length;
	}

	size_t number_of_pages = length / PAGE_SIZE;
	size_t free_pages =
		Paging::the()->m_allocator->find_free_pages(number_of_pages);

	for (size_t i = 0; i < (length / PAGE_SIZE); i++) {
		if (!is_mapped(virtual_address + (i * PAGE_SIZE))) {
			for (size_t j = 0; j < number_of_pages; j++) {
				auto free_page = free_pages + (j * PAGE_SIZE);
				map_page(virtual_address + (i * PAGE_SIZE), free_page, flags);
			}
		} else {
			// Just modify the flags
			bool user_supervisor = (flags & PageFlags::USER) != 0;
			bool read_only = (flags & PageFlags::READONLY) != 0;
			auto page_directory_index =
				get_page_directory_index(virtual_address);
			auto page_table_index = get_page_table_index(virtual_address);
			auto &page_table_entry = entries[page_directory_index]
										 .get_page_table()
										 ->entries[page_table_index];
			// risky
			entries[page_directory_index].user_supervisor = user_supervisor;

			// FIXME Find a smart way to turn the flags of the page table on and off.
			// A quick way to do this would be storing counters for each flag per page table, which would amount to being 10 bits per flag (up to 1024).
			// If the counter reaches zero, shut it off. A new flag is +1 a removed flag is -1.
			if (!entries[page_directory_index].read_write)
				entries[page_directory_index].read_write = !read_only;

			page_table_entry.user_supervisor = user_supervisor;
			page_table_entry.read_write = !read_only;
			page_table_entry.present = 1;
		}
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
#ifdef DEBUG_PAGING
			printk("Skipping full clone of kernel page table\n");
#endif
			continue;
		}

#ifdef DEBUG_PAGING
		printk("Cloning page table: %d\n", i);
#endif
		dst->entries[i].set_page_table(
			src->entries[i].get_page_table()->clone());
#ifdef DEBUG_PAGING
		printk("src->entries[%d].value: %x\n", i, src->entries[i].value);
#endif
	}

	return dst;
}

extern "C" char _kernel_start;
extern "C" char _kernel_end;

Paging::Paging(size_t total_memory) {
	m_allocator = new PageFrameAllocator(total_memory);

	// Reserve memory for the kernel
	m_allocator->mark_range(0, get_physical_address(&_kernel_end));
}

Paging::~Paging() { delete m_allocator; }

void Paging::setup(size_t total_memory) {
	s_instance = new Paging(total_memory);
	s_kernel_page_directory = &boot_page_directory;
	switch_page_directory(s_kernel_page_directory);
}
