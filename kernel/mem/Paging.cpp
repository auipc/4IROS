/* very x86-64 specific paging
 * Abandon all hope all ye who enter here
 */
#include <kernel/arch/amd64/x86_64.h>
#include <kernel/assert.h>
#include <kernel/mem/PageFrameAllocator.h>
#include <kernel/mem/Paging.h>
#include <kernel/mem/malloc.h>
#include <kernel/printk.h>
#include <kernel/shedule/proc.h>
#include <kernel/util/Pool.h>
#include <string.h>

// FIXME HUGE BUG NOT REALLY BUT WE SHOULD SEPARATE THE PAGE LEVEL ALLOCATIONS
// AGAINST THE ACTUAL PAGE MEMORY ALLOCATIONS

// #define TEMP_ZONE_1 (((1ull<<42ull)-1ull)-(PAGE_SIZE*8ull))
#define TEMP_ZONE_1_INDEX 58
#define TEMP_ZONE_1 ((58ull << 39ull))

PageSkellington tz1pdpte[512] __attribute__((aligned(PAGE_SIZE)));
PageSkellington tz1pd[512] __attribute__((aligned(PAGE_SIZE)));
PageSkellington tz1pt[512] __attribute__((aligned(PAGE_SIZE)));

/* The paging interface uses a 'mutate and commit' pattern.
 * When a page level is read, it's fetched from physical memory through the
 * virtual address TEMP_ZONE_X, then the required modifications are made and
 * TEMP_ZONE_X is remapped with the physical address pointing to the
 * physical location of the page level and committed.
 */

// #define DEBUG_PAGING

// extern "C" PageLevel boot_page_directory;

Paging *Paging::s_instance = nullptr;
RootPageLevel *Paging::s_kernel_page_directory = nullptr;

Paging *Paging::the() { return s_instance; }

size_t Paging::s_pf_allocator_base = 0;
size_t Paging::s_pf_allocator_end = 0;
Pool<LovelyPageLevel> *m_lovely_pool = nullptr;

Paging::Paging(size_t total_memory, const KernelBootInfo &kbootinfo) {
	static const size_t kend = reinterpret_cast<size_t>(kbootinfo.kmap_end);

	printk("level allocator\n");
	m_level_allocator = new PageFrameAllocator(0, 8 * MB);
	m_level_allocator->mark_range(0x0, 0xFFFFF + kend + 0x00100000 + 100 * KB);
	printk("m_allocator\n");
	m_allocator = new PageFrameAllocator(8 * MB, total_memory - (8 * MB));
	printk("m_allocator done\n");
}

Paging::~Paging() { delete m_allocator; }

// FIXME we need a ref counter for allocated pages or smth
void Paging::unmap_page(RootPageLevel &pd, uintptr_t virt) {
	auto current_pgl = (RootPageLevel *)get_cr3();
	switch_page_directory(m_safe_pgl);
	auto &pdpt = pd.entries[(virt >> 39) & 0x1ff];
	if (!pdpt.is_mapped())
		return;
	auto &pdpte = pdpt.level()->entries[(virt >> 30) & 0x1ff];
	if (!pdpte.is_mapped())
		return;
	auto &pdt = pdpte.level()->entries[(virt >> 21) & 0x1ff];
	if (!pdt.is_mapped())
		return;
	auto &pt = pdt.level()->entries[(virt >> 12) & 0x1ff];
	if (!pt.is_mapped())
		return;

	m_allocator->release_page(pt.addr());
	pt.pdata = 0;

	// FIXME INVLPG isn't working and I'm too lazy to figure it out
	asm volatile("mov %0, %%cr3" ::"a"(get_cr3()));
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

void Paging::create_page_level(PageSkellington &lvl) {
	printk("create_page_level\n");
	Debug::stack_trace_depth(5);
	PageLevel *pvl = new PageLevel();
	// FIXME oh no
	lvl.set_addr(((uintptr_t)pvl) - 0xffffff8000103000 + 0x20000 + 0x4000);
}

// change flags of subpages, use when flags are mutually exclusive
// i.e. user can exist in a kernel root page but kernel pages cannot exist in a
// user page.
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
void Paging::map_page_temp(RootPageLevel &pd, uintptr_t virt, uintptr_t phys,
						   uint64_t flags) {
	auto current_pgl = (RootPageLevel *)get_cr3();
	if (!Paging::the()->s_kernel_page_directory)
		switch_page_directory(m_safe_pgl);
	else
		switch_page_directory(Paging::the()->s_kernel_page_directory);
	auto &pdpt = pd.entries[(virt >> 39) & 0x1ff];
	uint64_t cast_flags = flags & (PAEPageFlags::User | PAEPageFlags::Write |
								   PAEPageFlags::ExecuteDisable);

	if (!pdpt.is_mapped()) {
		printk("1 calling create_page_level %x %x\n", virt, phys);
		create_page_level(pdpt);
		pdpt.pdata |= pdpt.flags() | flags;
	}

	pdpt.pdata |= pdpt.flags() | cast_flags;

	auto &pdpte = pdpt.level()->entries[(virt >> 30) & 0x1ff];
	if (!pdpte.is_mapped()) {
		printk("2 calling create_page_level %x %x\n", virt, phys);
		create_page_level(pdpte);
		pdpte.pdata |= pdpt.flags() | flags;
	}

	pdpte.pdata |= pdpte.flags() | cast_flags;

	auto &pdt = pdpte.level()->entries[(virt >> 21) & 0x1ff];
	if (!pdt.is_mapped()) {
		printk("3 calling create_page_level %x %x\n", virt, phys);
		create_page_level(pdt);
		pdt.pdata |= pdpt.flags() | flags;
	}

	pdt.pdata |= pdt.flags() | cast_flags;

	auto &pt = pdt.level()->entries[(virt >> 12) & 0x1ff];
	pt.pdata = phys | flags;
	switch_page_directory(current_pgl);
}

extern bool s_use_actual_allocator;
// FIXME when permissions are conflicting, the lesser strict permission.
void Paging::map_page(RootPageLevel &pd, uintptr_t virt, uintptr_t phys,
					  uint64_t flags) {
	if (s_use_actual_allocator) {
		auto &pdpt = pd.entries[(virt >> 39) & 0x1ff];
		uint64_t cast_flags =
			flags & (PAEPageFlags::User | PAEPageFlags::Write |
					 PAEPageFlags::ExecuteDisable);

		if (!pdpt.is_mapped()) {
			create_page_level(pdpt);
			pdpt.pdata |= pdpt.flags() | flags;
		}

		pdpt.pdata |= pdpt.flags() | cast_flags;

		auto pdpte_lvl = pdpt.fetch();
		auto &pdpte = pdpte_lvl->entries[(virt >> 30) & 0x1ff];
		if (!pdpte.is_mapped()) {
			pdpte.set_addr(m_level_allocator->find_free_page());
			pdpte.pdata |= pdpt.flags() | flags;
		}

		pdpte.pdata |= pdpte.flags() | cast_flags;

		auto pdt_lvl = pdpte.fetch();
		auto &pdt = pdt_lvl->entries[(virt >> 21) & 0x1ff];
		if (!pdt.is_mapped()) {
			pdt.set_addr(m_level_allocator->find_free_page());
			pdt.pdata |= pdpt.flags() | flags;
		}

		pdt.pdata |= pdt.flags() | cast_flags;

		auto pt_lvl = pdt.fetch();
		auto &pt = pt_lvl->entries[(virt >> 12) & 0x1ff];
		pt.pdata = phys | flags;
		pdt.commit(*pt_lvl.ptr());

		pdpte.commit(*pdt_lvl.ptr());
		pdpt.commit(*pdpte_lvl.ptr());
		// FIXME INVLPG isn't working and I'm too lazy to figure it out
		// asm volatile("mov %0, %%cr3"::"a"(get_cr3()));
	} else {
		auto &pdpt = pd.entries[(virt >> 39) & 0x1ff];
		uint64_t cast_flags =
			flags & (PAEPageFlags::User | PAEPageFlags::Write |
					 PAEPageFlags::ExecuteDisable);

		if (!pdpt.is_mapped()) {
			create_page_level(pdpt);
			pdpt.pdata |= pdpt.flags() | flags;
		}

		pdpt.pdata |= pdpt.flags() | cast_flags;

		auto &pdpte = pdpt.level()->entries[(virt >> 30) & 0x1ff];
		if (!pdpte.is_mapped()) {
			create_page_level(pdpte);
			pdpte.pdata |= pdpt.flags() | flags;
		}

		pdpte.pdata |= pdpte.flags() | cast_flags;

		auto &pdt = pdpte.level()->entries[(virt >> 21) & 0x1ff];
		if (!pdt.is_mapped()) {
			create_page_level(pdt);
			pdt.pdata |= pdpt.flags() | flags;
		}

		pdt.pdata |= pdt.flags() | cast_flags;

		auto &pt = pdt.level()->entries[(virt >> 12) & 0x1ff];
		pt.pdata = phys | flags;

		// FIXME INVLPG isn't working and I'm too lazy to figure it out
		// asm volatile("mov %0, %%cr3"::"a"(get_cr3()));
	}
	asm volatile("invlpg %0" ::"m"(virt));
}

void Paging::map_page_user(RootPageLevel &pd, uintptr_t virt, uintptr_t phys,
						   uint64_t flags) {
	auto &pdpt = pd.entries[(virt >> 39) & 0x1ff];
	uint64_t cast_flags = flags & (PAEPageFlags::User | PAEPageFlags::Write |
								   PAEPageFlags::ExecuteDisable);

	if (!pdpt.is_mapped()) {
		pdpt.set_addr(m_level_allocator->find_free_page());
		pdpt.pdata |= pdpt.flags() | flags;
	}

	pdpt.pdata |= pdpt.flags() | cast_flags;

	auto pdpte_lvl = pdpt.fetch();
	auto &pdpte = pdpte_lvl->entries[(virt >> 30) & 0x1ff];
	if (!pdpte.is_mapped()) {
		pdpte.set_addr(m_level_allocator->find_free_page());
		pdpte.pdata |= pdpte.flags() | flags;
	}

	pdpte.pdata |= pdpte.flags() | cast_flags;

	auto pdt_lvl = pdpte.fetch();
	auto &pdt = pdt_lvl->entries[(virt >> 21) & 0x1ff];
	if (!pdt.is_mapped()) {
		pdt.set_addr(m_level_allocator->find_free_page());
		pdt.pdata |= pdt.flags() | flags;
	}

	pdt.pdata |= pdt.flags() | cast_flags;

	auto pt_lvl = pdt.fetch();
	auto &pt = pt_lvl->entries[(virt >> 12) & 0x1ff];
	pt.pdata = phys | flags;
	pdt.commit(*pt_lvl.ptr());

	pdpte.commit(*pdt_lvl.ptr());
	pdpt.commit(*pdpte_lvl.ptr());
	asm volatile("invlpg %0" ::"m"(virt));
}

uintptr_t RootPageLevel::get_page_flags(uintptr_t addr) {
	auto &pdpt = entries[addr >> 39];
	if (!pdpt.is_mapped())
		return 0;
	auto pdpte_lvl = pdpt.fetch();
	auto &pdpte = pdpte_lvl->entries[addr >> 30];
	if (!pdpte.is_mapped())
		return 0;
	auto pdt_lvl = pdpte.fetch();
	auto &pdt = pdt_lvl->entries[addr >> 21];
	if (!pdt.is_mapped())
		return 0;
	auto pt_lvl = pdt.fetch();
	auto &pt = pt_lvl->entries[addr >> 12];
	auto flags = pt.flags();
	return flags;
}

// FIXME This pattern is really flawed. Worst case scenario fetching requires an
// exponentional amount of copies.
DumbSmart<LovelyPageLevel> PageSkellington::fetch() {
	LovelyPageLevel *level = new LovelyPageLevel();
	memset(level, 0, sizeof(LovelyPageLevel));
	Paging::the()->map_page_temp(*(RootPageLevel *)get_cr3(), TEMP_ZONE_1,
								 addr());
	memcpy(level, (void *)TEMP_ZONE_1, sizeof(LovelyPageLevel));
	return DumbSmart<LovelyPageLevel>(level);
}

void PageSkellington::commit(LovelyPageLevel &level) {
	Paging::the()->map_page_temp(*(RootPageLevel *)get_cr3(), TEMP_ZONE_1,
								 addr());
	memcpy((void *)TEMP_ZONE_1, &level, sizeof(PageLevel));
}

uintptr_t page_map(uint64_t virt) {
	RootPageLevel *pml4 = (RootPageLevel *)get_cr3();
	auto p1_lvl = pml4->entries[(virt >> 39) & 0x1ff].fetch();
	auto p2_lvl = p1_lvl->entries[(virt >> 30) & 0x1ff].fetch();
	auto p3_lvl = p2_lvl->entries[(virt >> 21) & 0x1ff].fetch();
	auto p4 = p3_lvl->entries[(virt >> 12) & 0x1ff].addr();
	return p4;
}

char *temp_area[6] = {};
// FIXME We should unmap pml4[0] or find some way to make the process page level
// not have it mapped so we can use that virtual space
Pair<RootPageLevel *, void *> Paging::clone(const RootPageLevel &pml4) {
	void *p = s_use_actual_allocator
				  ? kmalloc_really_aligned(PAGE_SIZE, PAGE_SIZE)
				  : kmalloc_aligned(PAGE_SIZE, PAGE_SIZE);
	auto root_page = page_map((uint64_t)p);
	// auto current_pgl = (RootPageLevel*)get_cr3();
	map_page(*(RootPageLevel *)get_cr3(), root_page, root_page);
	// switch_page_directory(m_safe_pgl);
	// map_page(*(RootPageLevel*)get_cr3(), root_page, root_page);
	auto root_pd = new ((void *)root_page) RootPageLevel();

	for (int i = 0; i < 512; i++) {
		auto &root_pdpt = root_pd->entries[i];
		auto pdpt = pml4.entries[i];
		if (!(pdpt.pdata & PAEPageFlags::User) &&
			(pdpt.pdata & PAEPageFlags::Present)) {
			root_pdpt.pdata = pdpt.pdata;
		} else
			root_pdpt.pdata = 0;
	}

	// switch_page_directory(current_pgl);
	return Pair(root_pd, p);
}

Pair<RootPageLevel *, void *>
Paging::clone_for_fork_test(const RootPageLevel &pml4, bool just_copy) {
	void *virt = (void *)kmalloc_really_aligned(4096, 4096);
	auto root_page = page_map((uint64_t)virt);
	map_page(*(RootPageLevel *)get_cr3(), root_page, root_page);
	auto root_pd = new (virt) RootPageLevel();

	for (int i = 0; i < 512; i++) {
		auto &root_pdpt = root_pd->entries[i];
		auto pdpt = pml4.entries[i];
		if (!pdpt.is_mapped()) {
			root_pdpt.pdata = 0;
			continue;
		}
		if ((!pdpt.is_user() && !just_copy) || i == TEMP_ZONE_1_INDEX) {
			root_pdpt.pdata = pdpt.pdata;
			continue;
		}
		// TODO copy
		if (!root_pdpt.is_mapped()) {
			root_pdpt.copy_flags(pdpt.pdata);
			root_pdpt.set_addr(m_level_allocator->find_free_page());
		}

		for (int j = 0; j < 512; j++) {
			auto root_pdpte_lvl = root_pdpt.fetch();
			auto &root_pdpte = root_pdpte_lvl->entries[j];
			auto pdpte = pdpt.fetch()->entries[j];
			if (!pdpte.is_mapped()) {
				root_pdpte.pdata = 0;
				continue;
			}

			if (!pdpte.is_user() && !just_copy) {
				root_pdpte.pdata = pdpte.pdata;
				root_pdpt.commit(*root_pdpte_lvl.ptr());
				continue;
			}

			if (!root_pdpte.is_mapped()) {
				root_pdpte.copy_flags(pdpte.pdata);
				root_pdpte.set_addr(m_level_allocator->find_free_page());
			}

			for (int k = 0; k < 512; k++) {
				auto root_pdt_lvl = root_pdpte.fetch();
				auto &root_pdt = root_pdt_lvl->entries[k];
				auto pdt_lvl = pdpte.fetch();
				auto pdt = pdt_lvl->entries[k];
				if (!pdt.is_mapped()) {
					root_pdt.pdata = 0;
					continue;
				}

				if (!pdt.is_user()) {
					root_pdt.pdata = pdt.pdata;
					root_pdpte.commit(*root_pdt_lvl.ptr());
					continue;
				}

				if (!root_pdt.is_mapped()) {
					root_pdt.copy_flags(pdt.pdata);
					root_pdt.set_addr(m_level_allocator->find_free_page());
				}

				for (int l = 0; l < 512; l++) {
					auto root_pt_lvl = root_pdt.fetch();
					auto &root_pt = root_pt_lvl->entries[l];
					auto pt_lvl = pdt.fetch();
					auto pt = pt_lvl->entries[l];
					if (!pt.is_mapped()) {
						root_pt.pdata = 0;
						continue;
					}

					if (!pt.is_user()) {
						root_pt.pdata = pt.pdata;
						root_pdt.commit(*root_pt_lvl.ptr());
						continue;
					}

					auto free_page = m_level_allocator->find_free_page();

					map_page(*(RootPageLevel *)get_cr3(),
							 (uintptr_t)temp_area[0], pt.addr());
					map_page(*(RootPageLevel *)get_cr3(),
							 (uintptr_t)temp_area[1], free_page);
					memcpy((void *)temp_area[1], (void *)temp_area[0],
						   sizeof(char) * PAGE_SIZE);

					root_pt.copy_flags(pt.pdata);
					root_pt.set_addr(free_page);

					root_pdt.commit(*root_pt_lvl.ptr());
				}
				root_pdpte.commit(*root_pdt_lvl.ptr());
			}
			root_pdpt.commit(*root_pdpte_lvl.ptr());
		}
		// panic("wow");
	}

	return Pair(root_pd, virt);
}

Pair<RootPageLevel *, void *> Paging::cow_clone(HashTable<CoWPage *>* owner,
                                                HashTable<CoWPage *>* child,
                                                RootPageLevel &pml4) {
	void *virt = (void *)kmalloc_really_aligned(4096, 4096);
	auto root_page = page_map((uint64_t)virt);
	map_page(*(RootPageLevel *)get_cr3(), root_page, root_page);
	auto root_pd = new (virt) RootPageLevel();

	for (int i = 0; i < 512; i++) {
		auto &root_pdpt = root_pd->entries[i];
		auto pdpt = pml4.entries[i];
		if (!pdpt.is_mapped()) {
			root_pdpt.pdata = 0;
			continue;
		}
		if ((!pdpt.is_user()) || i == TEMP_ZONE_1_INDEX) {
			root_pdpt.pdata = pdpt.pdata;
			continue;
		}
		// TODO copy
		if (!root_pdpt.is_mapped()) {
			root_pdpt.copy_flags(pdpt.pdata);
			root_pdpt.set_addr(m_level_allocator->find_free_page());
		}

		for (int j = 0; j < 512; j++) {
			auto root_pdpte_lvl = root_pdpt.fetch();
			auto &root_pdpte = root_pdpte_lvl->entries[j];
			auto pdpte = pdpt.fetch()->entries[j];
			if (!pdpte.is_mapped()) {
				root_pdpte.pdata = 0;
				continue;
			}

			if (!pdpte.is_user()) {
				root_pdpte.pdata = pdpte.pdata;
				root_pdpt.commit(*root_pdpte_lvl.ptr());
				continue;
			}

			if (!root_pdpte.is_mapped()) {
				root_pdpte.copy_flags(pdpte.pdata);
				root_pdpte.set_addr(m_level_allocator->find_free_page());
			}

			for (int k = 0; k < 512; k++) {
				auto root_pdt_lvl = root_pdpte.fetch();
				auto &root_pdt = root_pdt_lvl->entries[k];
				auto pdt_lvl = pdpte.fetch();
				auto &pdt = pdt_lvl->entries[k];
				if (!pdt.is_mapped()) {
					root_pdt.pdata = 0;
					continue;
				}

				if (!pdt.is_user()) {
					root_pdt.pdata = pdt.pdata;
					root_pdpte.commit(*root_pdt_lvl.ptr());
					continue;
				}

				if (!root_pdt.is_mapped()) {
					root_pdt.copy_flags(pdt.pdata);
					root_pdt.set_addr(m_level_allocator->find_free_page());
				}

				for (int l = 0; l < 512; l++) {
					auto root_pt_lvl = root_pdt.fetch();
					auto &root_pt = root_pt_lvl->entries[l];
					auto pt_lvl = pdt.fetch();
					auto &pt = pt_lvl->entries[l];
					if (!pt.is_mapped()) {
						root_pt.pdata = 0;
						continue;
					}

					if (!pt.is_user()) {
						root_pt.pdata = pt.pdata;
						root_pdt.commit(*root_pt_lvl.ptr());
						continue;
					}

					uintptr_t addr = ((uint64_t)i<<39ull)|((uint64_t)j<<30ull)|((uint64_t)k<<21ull)|((uint64_t)l<<12ull);
					CoWPage* cow_page = nullptr;

					if (owner->get(addr)) {
						cow_page = *owner->get(addr);
					} else {
						cow_page = new CoWPage();
						memset(cow_page, 0, sizeof(CoWPage));
						cow_page->refs++;
						cow_page->skel = pt;
						owner->push(addr, cow_page);
					}

					cow_page->refs++;
					child->push(addr, cow_page);

					pt.pdata &= ~PAEPageFlags::Write;
					root_pt.pdata = pt.pdata;
					pdt.commit(*pt_lvl.ptr());
					root_pdt.commit(*root_pt_lvl.ptr());
				}
				root_pdpte.commit(*root_pdt_lvl.ptr());
			}
			root_pdpt.commit(*root_pdpte_lvl.ptr());
		}
		// panic("wow");
	}

	return Pair(root_pd, virt);
#if 0
	void *virt = (void *)kmalloc_really_aligned(4096, 4096);
	auto root_page = page_map((uint64_t)virt);
	map_page(*(RootPageLevel *)get_cr3(), root_page, root_page);
	auto root_pd = new (virt) RootPageLevel();

	for (int i = 0; i < 512; i++) {
		auto &root_pdpt = root_pd->entries[i];
		auto &pdpt = pml4.entries[i];
		if (!pdpt.is_mapped()) {
			root_pdpt.pdata = 0;
			continue;
		}
		if ((!pdpt.is_user()) || i == TEMP_ZONE_1_INDEX) {
			root_pdpt.pdata = pdpt.pdata;
			continue;
		}

		if (!root_pdpt.is_mapped()) {
			root_pdpt.copy_flags(pdpt.pdata);
			root_pdpt.set_addr(m_level_allocator->find_free_page());
		}

		for (int j = 0; j < 512; j++) {
			auto root_pdpte_lvl = root_pdpt.fetch();
			auto &root_pdpte = root_pdpte_lvl->entries[j];
			auto &pdpte = pdpt.fetch()->entries[j];
			if (!pdpte.is_mapped()) {
				root_pdpte.pdata = 0;
				continue;
			}

			if (!pdpte.is_user()) {
				root_pdpte.pdata = pdpte.pdata;
				root_pdpt.commit(*root_pdpte_lvl.ptr());
				continue;
			}

			if (!root_pdpte.is_mapped()) {
				root_pdpte.copy_flags(pdpte.pdata);
				root_pdpte.set_addr(m_level_allocator->find_free_page());
			}

			for (int k = 0; k < 512; k++) {
				auto root_pdt_lvl = root_pdpte.fetch();
				auto &root_pdt = root_pdt_lvl->entries[k];
				auto pdt_lvl = pdpte.fetch();
				auto &pdt = pdt_lvl->entries[k];
				if (!pdt.is_mapped()) {
					root_pdt.pdata = 0;
					continue;
				}

				if (!pdt.is_user()) {
					root_pdt.pdata = pdt.pdata;
					root_pdpte.commit(*root_pdt_lvl.ptr());
					continue;
				}

				if (!root_pdt.is_mapped()) {
					root_pdt.copy_flags(pdt.pdata);
					root_pdt.set_addr(m_level_allocator->find_free_page());
				}

				for (int l = 0; l < 512; l++) {
					auto root_pt_lvl = root_pdt.fetch();
					auto &root_pt = root_pt_lvl->entries[l];
					auto pt_lvl = pdt.fetch();
					auto &pt = pt_lvl->entries[l];
					if (!pt.is_mapped()) {
						root_pt.pdata = 0;
						root_pdt.commit(*root_pt_lvl.ptr());
						continue;
					}

					if (!pt.is_user()) {
						root_pt.pdata = pt.pdata;
						root_pdt.commit(*root_pt_lvl.ptr());
						continue;
					}


					uintptr_t addr = ((uint64_t)i<<39ull)|((uint64_t)j<<30ull)|((uint64_t)k<<21ull)|((uint64_t)l<<12ull);
					CoWPage* cow_page = nullptr;

#if 0
					if (owner->get(addr)) {
						cow_page = *owner->get(addr);
					} else {
#endif
						cow_page = new CoWPage();
						memset(cow_page, 0, sizeof(CoWPage));
						cow_page->refs++;
						cow_page->skel = pt;
						owner->push(addr, cow_page);
#if 0
					}
#endif

					cow_page->refs++;
					printk("Pushing %x %x\n", addr, cow_page);
					child->push(addr, cow_page);

					pt.pdata = 0;
					root_pt.pdata = 0;
					pdt.commit(*pt_lvl.ptr());
					root_pdt.commit(*root_pt_lvl.ptr());
				}
				pdpte.commit(*pdt_lvl.ptr());
				root_pdpte.commit(*root_pdt_lvl.ptr());
			}
			root_pdpt.commit(*root_pdpte_lvl.ptr());
		}
		// panic("wow");
	}
	return Pair(root_pd, virt);
#endif
}

// FIXME free level structs
int BasePageLevel::recursive_release(HashTable<CoWPage*>* cow_table, uintptr_t addraccum, int depth) {
	if (depth > 3)
		return 0;

	// if (depth == 0)
	// printk("%x\n", this);

	for (int i = 0; i < 512; i++) {
		if (i != 1 && depth == 0)
			continue;
		uintptr_t vaddr = addraccum|((uintptr_t)i<<(39ull-(depth*9)));
		auto &entry = entries[i];
		auto addr = entry.addr();
		// FIXME
		if ((entry.flags() & (PAEPageFlags::User)) &&
			entry.flags() & (PAEPageFlags::Present)) {
			if (cow_table->get(vaddr)) {
				CoWPage** cow_page = cow_table->get(vaddr);
				(*cow_page)->refs--;
				if ((*cow_page)->refs > 0) {
					continue;
				}
				delete *cow_page;
			}
			if (depth != 3) {
				//printk("Trying to fetch %x at depth %d %d\n", addr, depth, i);
				auto fe = entry.fetch();
				fe->recursive_release(cow_table, vaddr, depth + 1);
				entry.commit(*fe.ptr());
			}
			if (depth == 3) {
				Paging::the()->m_allocator->release_page(addr);
			} else {
				Paging::the()->m_level_allocator->release_page(addr);
			}
			entry.pdata = 0;
		}
	}

	return 0;
}

void Paging::release(RootPageLevel &page_level, HashTable<CoWPage*>* cow_table) {
	page_level.recursive_release(cow_table);
}

uintptr_t page_map_level(uint64_t virt) {
	RootPageLevel *pml4 = (RootPageLevel *)get_cr3();
	auto p1_lvl = pml4->entries[(virt >> 39) & 0x1ff].level();
	auto p2_lvl = p1_lvl->entries[(virt >> 30) & 0x1ff].level();
	auto p3_lvl = p2_lvl->entries[(virt >> 21) & 0x1ff].level();
	auto p4 = p3_lvl->entries[(virt >> 12) & 0x1ff].addr();
	return p4;
}

void Paging::setup(size_t total_memory, const KernelBootInfo &kbootinfo) {
	for (int i = 0; i < 2; i++) {
		temp_area[i] = (char *)kmalloc_aligned(PAGE_SIZE, PAGE_SIZE);
	}
	s_instance = new Paging(total_memory, kbootinfo);
	Paging::the()->m_safe_pgl = (RootPageLevel *)get_cr3();
	Paging::the()->m_safe_pgl->entries[TEMP_ZONE_1_INDEX].set_addr(
		page_map_level((uintptr_t)tz1pdpte));
	Paging::the()->m_safe_pgl->entries[TEMP_ZONE_1_INDEX].pdata |=
		PAEPageFlags::Write | PAEPageFlags::Present;
	tz1pdpte[0].pdata = (page_map_level((uintptr_t)tz1pd)) |
						PAEPageFlags::Write | PAEPageFlags::Present;
	tz1pd[0].pdata = (page_map_level((uintptr_t)tz1pt)) | PAEPageFlags::Write |
					 PAEPageFlags::Present;
	tz1pt[0].pdata = 0 | PAEPageFlags::Write | PAEPageFlags::Present;

	void *p = (void *)TEMP_ZONE_1;
	asm volatile("invlpg %0" ::"m"(p));
	s_kernel_page_directory =
		Paging::the()->clone(*(RootPageLevel *)get_cr3()).left;
	switch_page_directory(s_kernel_page_directory);
}
