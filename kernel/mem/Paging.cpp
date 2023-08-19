#include <kernel/assert.h>
#include <kernel/mem/Paging.h>
#include <kernel/printk.h>
#include <kernel/mem/malloc.h>

// FIXME What should we do with boot_page_directory? Since it's wasted memory
// after we switch to our own page directory.
// Maybe move it to a different section that we can discard for extra memory?
extern "C" PageDirectory boot_page_directory;
Paging *Paging::s_instance = nullptr;
PageDirectory *Paging::s_kernel_page_directory = nullptr;

Paging *Paging::the() { return s_instance; }

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

		// TODO we need to be able to temporarily identity map this address so we can copy it's contents
		printk("free page: %x\n", Paging::the()->m_allocator->find_free_page());
	}

	return dst;
}

PageDirectory *PageDirectory::clone() {
	PageDirectory *src = this;
	PageDirectory *dst = new PageDirectory();

	// page aligned
	assert(reinterpret_cast<uint32_t>(dst) % PAGE_SIZE == 0);

	// TODO find a good way to allocate page tables
	// TODO every page should share the kernels page tables, mostly for interrupts and kernel tasks.
	for (int i = 0; i < 1024; i++) {
		if (!src->entries[i].present)
			continue;
		// just copy all values, what could go wrong.
		dst->entries[i].value = src->entries[i].value;
		dst->entries[i].set_page_table(src->entries[i].get_page_table()->clone());
		printk("src->entries[%d].value: %x\n", i, src->entries[i].value);
	}

	return dst;
}

extern "C" char _kernel_start;
extern "C" char _kernel_end;

Paging::Paging() {
	m_allocator = new PageFrameAllocator(TOTAL_MEMORY);

	// Reserve memory for the kernel
	m_allocator->mark_range(0,
							get_physical_address(&_kernel_end));
	printk("free page: %x\n", m_allocator->find_free_page());
}

//Paging::~Paging() { delete m_allocator; }

void Paging::setup() {
	s_instance = new Paging();

	s_kernel_page_directory = boot_page_directory.clone();
	printk("boot_page_directory: %x\n", &boot_page_directory);
	printk("boot_page_directory physical: %x\n",
		   get_physical_address(&boot_page_directory));
	printk("s_kernel_page_directory: %x\n", s_kernel_page_directory);
	printk("s_kernel_page_directory physical: %x\n",
		   get_physical_address(s_kernel_page_directory));
	switch_page_directory(s_kernel_page_directory);
}
