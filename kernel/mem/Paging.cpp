#include <kernel/assert.h>
#include <kernel/mem/PageFrameAllocator.h>
#include <kernel/mem/Paging.h>
#include <kernel/mem/malloc.h>
#include <kernel/printk.h>
#include <kernel/arch/amd64/x86_64.h>
#include <string.h>

// #define DEBUG_PAGING

//extern "C" PageLevel boot_page_directory;

Paging *Paging::s_instance = nullptr;
RootPageLevel *Paging::s_kernel_page_directory = nullptr;

Paging *Paging::the() { return s_instance; }

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

Paging::Paging(size_t total_memory, const KernelBootInfo& kbootinfo) {
	static const size_t kstart = reinterpret_cast<size_t>(kbootinfo.kmap_start);
	static const size_t ksz = reinterpret_cast<size_t>(kbootinfo.kmap_end);

	//PageLevel *pd = s_kernel_page_directory;

	/*
	s_pf_allocator_base = page_align(kend + PAGE_SIZE);
	s_pf_allocator_end = page_align(
		s_pf_allocator_base + PageFrameAllocator::alloc_size(total_memory) +
		PAGE_SIZE + PAGE_SIZE);

	for (size_t ptr = s_pf_allocator_base; ptr < s_pf_allocator_end;
		 ptr += PAGE_SIZE) {
		PageLevel *pd = s_kernel_page_directory;
		pd->map_page(ptr, ptr - VIRTUAL_ADDRESS, 0);
	}*/

	//g_use_halfway_allocator = true;
	// PageFrameAllocator structures
	m_allocator = new PageFrameAllocator(total_memory);
	m_allocator->mark_range(0x0, 0xFFFFF+kstart+ksz+0x00100000+100*KB);
	printk("lol %x\n", 0xFFFFF+kstart+ksz+0x00100000+100*KB);
	//m_allocator->mark_range(kstart, ksz);
	// I forgor the size of the bootloader help
	//m_allocator->mark_range(0x00100000, 100*KB);

	// Safe area to map and edit pages
	m_safe_area = (uint8_t*)kmalloc_aligned(PAGE_SIZE * 2, PAGE_SIZE);
}

Paging::~Paging() { delete m_allocator; }

// FIXME we need a ref counter for allocated pages or smth
void Paging::unmap_page(RootPageLevel& pd, uintptr_t virt) {
	auto current_pgl = (RootPageLevel*)get_cr3();
	switch_page_directory(m_safe_pgl);
	auto& pdpt = pd.entries[(virt>>39)&0x1ff];
	if (!pdpt.is_mapped()) return;
	auto& pdpte = pdpt.level()->entries[(virt>>30)&0x1ff];
	if (!pdpte.is_mapped()) return;
	auto& pdt = pdpte.level()->entries[(virt>>21)&0x1ff];
	if (!pdt.is_mapped()) return;
	auto& pt = pdt.level()->entries[(virt>>12)&0x1ff];
	if (!pt.is_mapped()) return;

	m_allocator->release_page(pt.addr());
	pt.pdata = 0;

	// FIXME INVLPG isn't working and I'm too lazy to figure it out
	asm volatile("mov %0, %%cr3"::"a"(get_cr3()));
	switch_page_directory(current_pgl);
}

#if 0
void Paging::map_page_assume(RootPageLevel& pd, uintptr_t virt, uintptr_t phys, uint64_t flags) {
	auto current_pgl = (RootPageLevel*)get_cr3();
	switch_page_directory(m_safe_pgl);
	auto& pdpt = pd.entries[(virt>>39)&0x1ff];
	if (!pdpt.is_mapped()) {
		Debug::stack_trace();
		panic("pdpt NM");
	}

	auto& pdpte = pdpt.level()->entries[(virt>>30)&0x1ff];
	if (!pdpte.is_mapped()) {
		printk("lol %x\n", pdpt.pdata);
		printk("lol %x\n", pdpte.pdata);
		Debug::stack_trace();
		panic("pdpte NM");
	}
	auto& pdt = pdpte.level()->entries[(virt>>21)&0x1ff];
	if (!pdt.is_mapped()) panic("pdt NM");
	auto& pt = pdt.level()->entries[(virt>>12)&0x1ff];
	if (!pt.is_mapped()) panic("pt NM");

	pdpt.copy_flags(flags);
	pdpte.copy_flags(flags);
	pdt.copy_flags(flags);
	pt.pdata = phys | flags;

	// FIXME INVLPG isn't working and I'm too lazy to figure it out
	asm volatile("mov %0, %%cr3"::"a"(get_cr3()));
	switch_page_directory(current_pgl);
}
#endif

extern "C" char _heap_start;

void Paging::create_page_level(PageSkellington& lvl) {
	PageLevel* pvl = new PageLevel();
	// FIXME oh no
	lvl.set_addr(((uintptr_t)pvl)-0x280103000+0x20000+0x4000);
}

// change flags of subpages, use when flags are mutually exclusive 
// i.e. user can exist in a kernel root page but kernel pages cannot exist in a user page.
#if 0
void Paging::mark_sub(PageLevel& plv, uint64_t flags) {
	for (int i = 0; i < 512; i++) {
		auto e = plv->entries[i];
		if ((e.flags() & PAEPageFlags::User) == (flags & PAEPageFlags::User)) 
		for (int j = 0; j < 512; j++) {
			for (int k = 0; k < 512; K++) {
			}
		}
	}
}
#endif

// FIXME when permissions are conflicting, the lesser strict permission.
void Paging::map_page(RootPageLevel& pd, uintptr_t virt, uintptr_t phys, uint64_t flags) {
	auto current_pgl = (RootPageLevel*)get_cr3();
	switch_page_directory(m_safe_pgl);
	auto& pdpt = pd.entries[(virt>>39)&0x1ff];
	uint64_t cast_flags = flags & (PAEPageFlags::User | PAEPageFlags::Write | PAEPageFlags::ExecuteDisable);

	if (!pdpt.is_mapped()) {
		printk("vvvv %x\n", (virt>>39)&0x1ff);
		create_page_level(pdpt);
		pdpt.pdata |= pdpt.flags() | flags;
	}

	pdpt.pdata |= pdpt.flags() | cast_flags;

	auto& pdpte = pdpt.level()->entries[(virt>>30)&0x1ff];
	if (!pdpte.is_mapped()) {
		create_page_level(pdpte);
		pdpte.pdata |= pdpt.flags() | flags;
	}

	pdpte.pdata |= pdpte.flags() | cast_flags;

	auto& pdt = pdpte.level()->entries[(virt>>21)&0x1ff];
	if (!pdt.is_mapped()) {
		create_page_level(pdt);
		pdt.pdata |= pdpt.flags() | flags;
	}

	pdt.pdata |= pdt.flags() | cast_flags;

	auto& pt = pdt.level()->entries[(virt>>12)&0x1ff];
	pt.pdata = phys | flags;

	// FIXME INVLPG isn't working and I'm too lazy to figure it out
	//asm volatile("mov %0, %%cr3"::"a"(get_cr3()));
	switch_page_directory(current_pgl);
}

uintptr_t RootPageLevel::get_page_flags(uintptr_t addr) {
	auto& pdpt = entries[addr>>39];
	if (!pdpt.is_mapped()) return 0;
	auto& pdpte = pdpt.level()->entries[addr>>30];
	if (!pdpte.is_mapped()) return 0;
	auto& pdt = pdpte.level()->entries[addr>>21];
	if (!pdt.is_mapped()) return 0;
	auto& pt = pdt.level()->entries[addr>>12];
	return pt.flags();
}

char* temp_area[6] = {};
RootPageLevel* Paging::clone(const RootPageLevel& pml4) {
	auto root_page = m_allocator->find_free_page();
	//auto current_pgl = (RootPageLevel*)get_cr3();
	map_page(*(RootPageLevel*)get_cr3(), root_page, root_page);
	//switch_page_directory(m_safe_pgl);
	//map_page(*(RootPageLevel*)get_cr3(), root_page, root_page);
	auto root_pd = new ((void*)root_page) RootPageLevel();

	for (int i = 0; i < 512; i++) {
		auto& root_pdpt = root_pd->entries[i];
		auto pdpt = pml4.entries[i];
		if (!pdpt.is_mapped()) continue;
		if (!(pdpt.flags() & PAEPageFlags::User)) {
			root_pdpt.pdata = pdpt.pdata;
			continue;
		}
		// TODO copy
		if (!root_pdpt.is_mapped()) {
			create_page_level(root_pdpt);
		}

		for (int j = 0; j < 512; j++) {
			auto& root_pdpte = root_pdpt.level()->entries[j];
			auto pdpte = pdpt.level()->entries[j];
			root_pdpte.pdata = pdpte.pdata;
		}
		panic("wow");
	}

	//switch_page_directory(current_pgl);
	return root_pd;
}

RootPageLevel* Paging::clone_for_fork(const RootPageLevel& pml4, bool just_copy) {
	auto root_page = m_allocator->find_free_page();
	//auto current_pgl = (RootPageLevel*)get_cr3();
	map_page(*(RootPageLevel*)get_cr3(), root_page, root_page);
	//switch_page_directory(m_safe_pgl);
	//map_page(*(RootPageLevel*)get_cr3(), root_page, root_page);
	auto root_pd = new ((void*)root_page) RootPageLevel();

	for (int i = 0; i < 512; i++) {
		auto& root_pdpt = root_pd->entries[i];
		auto pdpt = pml4.entries[i];
		if (!pdpt.is_mapped()) continue;
		if (!pdpt.is_user() && !just_copy) {
			root_pdpt.pdata = pdpt.pdata;
			continue;
		}
		// TODO copy
		if (!root_pdpt.is_mapped()) {
			root_pdpt.copy_flags(pdpt.pdata);
			create_page_level(root_pdpt);
		}

		for (int j = 0; j < 512; j++) {
			auto& root_pdpte = root_pdpt.level()->entries[j];
			auto pdpte = pdpt.level()->entries[j];
			if (!pdpte.is_mapped()) continue;

			if (!pdpte.is_user() && !just_copy) {
				root_pdpte.pdata = pdpte.pdata;
				continue;
			}

			if (!root_pdpte.is_mapped()) {
				root_pdpte.copy_flags(pdpte.pdata);
				create_page_level(root_pdpte);
			}

			for (int k = 0; k < 512; k++) {
				auto& root_pdt = root_pdpte.level()->entries[k];
				auto pdt = pdpte.level()->entries[k];
				if (!pdt.is_mapped()) continue;

				if (!pdt.is_user()) {
					root_pdt.pdata = pdt.pdata;
					continue;
				}

				if (!root_pdt.is_mapped()) {
					root_pdt.copy_flags(pdt.pdata);
					create_page_level(root_pdt);
				}

				for (int l = 0; l < 512; l++) {
					auto& root_pt = root_pdt.level()->entries[l];
					auto pt = pdt.level()->entries[l];
					if (!pt.is_mapped()) continue;

					if (!pt.is_user()) {
						root_pt.pdata = pt.pdata;
						continue;
					}

					auto free_page = m_allocator->find_free_page();

					map_page(*(RootPageLevel*)get_cr3(), (uintptr_t)temp_area[0], pt.addr());
					map_page(*(RootPageLevel*)get_cr3(), (uintptr_t)temp_area[1], free_page);
					memcpy((void*)temp_area[1], (void*)temp_area[0], sizeof(char)*PAGE_SIZE);

					root_pt.copy_flags(pt.pdata);
					root_pt.set_addr(free_page);
				}
			}
		}
		//panic("wow");
	}

	return root_pd;
}

void Paging::setup(size_t total_memory, const KernelBootInfo& kbootinfo) {
	for (int i = 0; i < 2; i++) {
		temp_area[i] = (char*)kmalloc_aligned(PAGE_SIZE, PAGE_SIZE);
	}
	s_instance = new Paging(total_memory, kbootinfo);
	Paging::the()->m_safe_pgl = (RootPageLevel*)get_cr3();
	s_kernel_page_directory = Paging::the()->clone(*(RootPageLevel*)get_cr3());
	switch_page_directory(s_kernel_page_directory);
	printk("Clone done\n");
}
