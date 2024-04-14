#include <kernel/arch/i386/kernel.h>
#include <kernel/assert.h>
#include <kernel/mem/PageFrameAllocator.h>
#include <kernel/mem/Paging.h>
#include <kernel/mem/malloc.h>
#include <kernel/printk.h>
#include <string.h>

// #define DEBUG_PAGING

extern "C" PageDirectory boot_page_directory;

Paging *Paging::s_instance = nullptr;
PageDirectory *Paging::s_kernel_page_directory = nullptr;
PageDirectory *Paging::s_current_page_directory = &boot_page_directory;

Paging *Paging::the() { return s_instance; }

// Instead of new_directory, maybe we could just have a page directory for
// operating on pages...
#if 0
PageTable *PageTable::clone(size_t idx) {
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
		Paging::the()->map_page(src->entries[i].get_page_base() +
									VIRTUAL_ADDRESS,
								src->entries[i].get_page_base(), 0);
#ifdef DEBUG_PAGING
		printk("Copying from %x to %x\n",
			   src->entries[i].get_page_base() + VIRTUAL_ADDRESS,
			   free_page + VIRTUAL_ADDRESS);
#endif

		memcpy(reinterpret_cast<void *>(free_page + VIRTUAL_ADDRESS),
			   reinterpret_cast<void *>(src->entries[i].get_page_base() +
										VIRTUAL_ADDRESS),
			   PAGE_SIZE);

		Paging::the()->unmap_page(free_page + VIRTUAL_ADDRESS);
		Paging::the()->unmap_page(src->entries[i].get_page_base() +
								  VIRTUAL_ADDRESS);

		printk("ent %x\n", &dst->entries[i]);
		printk("fp %x\n", free_page);
		dst->entries[i].set_page_base(free_page);

		// sanity check
		assert(!Paging::the()->is_mapped(free_page + VIRTUAL_ADDRESS));
		assert(!Paging::the()->is_mapped(src->entries[i].get_page_base() +
										 VIRTUAL_ADDRESS));
		// TODO we need to be able to temporarily identity map this address so
		// we can copy it's contents
#ifdef DEBUG_PAGING
		printk("free page: %x\n", Paging::the()->m_allocator->find_free_page());
#endif
	}

	// FIXME memory leak
	return dst;
}
#endif

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

	if (!entries[page_directory_index]
			 .get_page_table()
			 ->entries[page_table_index]
			 .user_supervisor)
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

	size_t number_of_pages = (length + PAGE_SIZE - 1) / PAGE_SIZE;

	/*
	size_t free_pages =
		Paging::the()->m_allocator->find_free_pages(number_of_pages);*/
	for (size_t i = 0; i < number_of_pages; i++) {
		size_t free_page = Paging::the()->m_allocator->find_free_page();
		auto address = virtual_address + (i * PAGE_SIZE);
		if (!is_mapped(address)) {
			map_page(address, free_page, flags);
		} else {
			bool user_supervisor = (flags & PageFlags::USER) != 0;
			bool read_only = (flags & PageFlags::READONLY) != 0;

			auto page_directory_index = get_page_directory_index(address);
			auto page_table_index = get_page_table_index(address);

			if (!entries[page_directory_index].present) {
				return;
			}

			entries[page_directory_index].user_supervisor = 1;
			entries[page_directory_index].read_write = 1;

			auto &page_table_entry = entries[page_directory_index]
										 .get_page_table()
										 ->entries[page_table_index];

			page_table_entry.user_supervisor = user_supervisor;
			page_table_entry.read_write = !read_only;
			page_table_entry.present = 1;

			if (this == Paging::the()->current_page_directory()) {
				asm volatile("invlpg (%0)" ::"r"(virtual_address) : "memory");
			}
		}
	}
}

void PageDirectory::unmap_page(size_t virtual_address) {
	auto pt_e = entries[get_page_directory_index(virtual_address)];
	if (!pt_e.present) {
		assert(false);
		return;
	}
	auto pt = pt_e.get_page_table();

	auto page = &pt->entries[get_page_table_index(virtual_address)];
	Paging::the()->allocator()->release_page(page->get_page_base());
	page->present = 0;

	if (this == Paging::the()->current_page_directory()) {
		asm volatile("invlpg (%0)" ::"r"(virtual_address) : "memory");
	}
}

void PageDirectory::unmap_range(size_t virtual_address, size_t length) {
	if (!length)
		return;
	if (length < PAGE_SIZE) {
		length += PAGE_SIZE - length;
	}

	size_t number_of_pages = (length + PAGE_SIZE - 1) / PAGE_SIZE;

	for (size_t i = 0; i < number_of_pages; i++) {
		unmap_page(virtual_address + (number_of_pages * PAGE_SIZE));
	}
}

PageDirectory *PageDirectory::clone() {
	PageDirectory *src = this;
	PageDirectory *dst = new PageDirectory();

	PageDirectory *old = Paging::current_page_directory();
	Paging::switch_page_directory(Paging::s_kernel_page_directory);

	// page aligned
	assert(reinterpret_cast<uint32_t>(dst) % PAGE_SIZE == 0);

	// sux.
	for (int i = 0; i < 1024; i++) {
		// if (!Paging::s_kernel_page_directory->entries[i].present)
		// continue;

		// if (src->entries[i].present)
		//	continue;

		dst->entries[i].value = src->entries[i].value;

		if (Paging::s_kernel_page_directory->entries[i].present)
			continue;

		if (!src->entries[i].present)
			continue;

		auto dst_pt = new PageTable();
		dst->entries[i].set_page_table(dst_pt);
		for (int j = 0; j < 1024; j++) {
			auto src_pte = src->entries[i].get_page_table()->entries[j];
			if (!src_pte.present)
				continue;
			auto free_page = Paging::the()->m_allocator->find_free_page();
			dst_pt->entries[j].value = src_pte.value;
			dst_pt->entries[j].present = 1;
			dst_pt->entries[j].set_page_base(free_page);

			Paging::current_page_directory()->map_page(free_page + 0xF0000000,
													   free_page, 0);
			Paging::current_page_directory()->map_page(
				src_pte.get_page_base() + 0xF0000000, src_pte.get_page_base(),
				0);

			// gawdy
			memcpy((void *)(free_page + 0xF0000000),
				   (void *)(src_pte.get_page_base() + 0xF0000000), PAGE_SIZE);
		}
	}

	Paging::switch_page_directory(old);
	return dst;
}

size_t Paging::s_pf_allocator_base = 0;
size_t Paging::s_pf_allocator_end = 0;

// A tiny allocator for allocating the space we need to keep track of all
// memory. This is static and will never change.
void *Paging::pf_allocator(size_t size) {
	void *addr = reinterpret_cast<void *>(s_pf_allocator_base);
	s_pf_allocator_base += size;
	assert(s_pf_allocator_end >= s_pf_allocator_base);
	return addr;
}

extern "C" char _kernel_start;
extern "C" char _kernel_end;
Paging::Paging(size_t total_memory) {
	static const size_t kstart = reinterpret_cast<size_t>(&_kernel_start);
	static const size_t kend = reinterpret_cast<size_t>(&_kernel_end);
	printk("kend: %x\n", kend);

	PageDirectory *pd = s_kernel_page_directory;

	s_pf_allocator_base = page_align(kend + PAGE_SIZE);
	s_pf_allocator_end = page_align(
		s_pf_allocator_base + PageFrameAllocator::alloc_size(total_memory) +
		PAGE_SIZE + PAGE_SIZE);

	for (size_t ptr = s_pf_allocator_base; ptr < s_pf_allocator_end;
		 ptr += PAGE_SIZE) {
		PageDirectory *pd = s_kernel_page_directory;
		pd->map_page(ptr, ptr - VIRTUAL_ADDRESS, 0);
	}

	g_use_halfway_allocator = true;
	// PageFrameAllocator structures
	m_allocator = new PageFrameAllocator(total_memory);
	g_use_halfway_allocator = false;

	for (int i = 0; i < 1024; i++) {
		if (!pd->entries[i].present)
			continue;

		auto pt = pd->entries[i].get_page_table();
		for (int j = 0; j < 1024; j++) {
			if (!pt->entries[j].present)
				continue;

			auto base = pt->entries[j].get_page_base();
			m_allocator->mark_range(base, base + PAGE_SIZE);
		}
	}

	m_allocator->mark_range(s_pf_allocator_base - VIRTUAL_ADDRESS,
							s_pf_allocator_end - s_pf_allocator_base);

	// https://wiki.osdev.org/Memory_Map_(x86)#Real_mode_address_space_.28.3C_1_MiB.29
	// The lower half of memory really shouldn't be touched. Way too much BIOS
	// related functionality depends on it, even in protected mode! The benefits
	// of reclaiming this memory are insignificant.
	m_allocator->mark_range(0x0, 0xFFFFF);

	m_allocator->mark_range(kstart, kend - kstart);
	m_safe_area = kmalloc_aligned(PAGE_SIZE * 2, PAGE_SIZE);
}

Paging::~Paging() { delete m_allocator; }

void Paging::setup(size_t total_memory) {
	s_kernel_page_directory = &boot_page_directory;
	s_instance = new Paging(total_memory);
	s_kernel_page_directory = s_kernel_page_directory->clone();
	switch_page_directory(s_kernel_page_directory);
}
