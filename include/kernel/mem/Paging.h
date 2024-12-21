#pragma once
#include <kernel/Debug.h>
#include <kernel/arch/amd64/kernel.h>
#include <kernel/arch/amd64/x86_64.h>
#include <kernel/assert.h>
#include <kernel/mem/PageFrameAllocator.h>
#include <kernel/mem/malloc.h>
#include <kernel/util/HashTable.h>
#include <kernel/util/Vec.h>
#include <stdint.h>

struct CoWInfo;
struct PageLevel;
struct LovelyPageLevel;
struct RootPageLevel;
struct PageSkellington;

enum PageFlags {
	NONE,
	READONLY,
	USER,
};

inline uint64_t get_cr3() {
	uint64_t cr3 = 0;
	asm volatile("movq %%cr3, %0" : "=a"(cr3));
	assert(cr3);
	return cr3;
}

#define PAGE_ADDRESS_MASK 0x000ffffffffff000

class Paging {
  public:
	~Paging();
	static void setup(size_t total_memory, const KernelBootInfo &kbootinfo);

	RootPageLevel *clone(const RootPageLevel &);
	RootPageLevel *clone_for_fork(const RootPageLevel &,
								  bool just_copy = false);

	RootPageLevel *clone_for_fork_shallow_cow(RootPageLevel &pml4,
											  HashTable<CoWInfo>* owner_table, HashTable<CoWInfo> *table,
											  bool just_copy = false);

	inline static RootPageLevel *kernel_root_directory() {
		// This shouldn't be null
		assert(s_kernel_page_directory != nullptr);
		return s_kernel_page_directory;
	}

	inline static PageLevel *current_root_directory() {
		auto pd = reinterpret_cast<PageLevel *>(get_cr3() + VIRTUAL_ADDRESS);
		return pd;
	}

	inline static uintptr_t current_root_directory_physical() {
		return reinterpret_cast<uintptr_t>(get_physical_address(
			reinterpret_cast<void *>(current_root_directory())));
	}

	inline static size_t page_align(size_t address) {
		return address & ~(PAGE_SIZE - 1);
	}

	void test(RootPageLevel &owner, RootPageLevel &dest_pd,
			  uintptr_t fault_addr);
	bool resolve_fault(PageLevel *pd, size_t fault_addr);

	void release(RootPageLevel &page_level);
	static Paging *the();
	static void *pf_allocator(size_t size);

	void copy_range(PageLevel *src, PageLevel *dst, size_t virtual_address,
					size_t range);

	inline PageFrameAllocator *allocator() const { return m_allocator; }

	inline static size_t get_physical_address(void *virtual_address) {
		return reinterpret_cast<size_t>(virtual_address) - VIRTUAL_ADDRESS;
	}

	inline static void switch_page_directory(PageLevel *page_directory) {
		asm volatile("movq %0, %%cr3" ::"a"(page_directory));
	}

	void unmap_page(RootPageLevel &pd, uintptr_t virt);
	void map_page_assume(RootPageLevel &pd, uintptr_t virt, uintptr_t phys,
						 uint64_t flags = PAEPageFlags::Present |
										  PAEPageFlags::Write);
	void map_page_temp(RootPageLevel &pd, uintptr_t virt, uintptr_t phys,
					   uint64_t flags = PAEPageFlags::Present |
										PAEPageFlags::Write);
	void map_page(RootPageLevel &pd, uintptr_t virt, uintptr_t phys,
				  uint64_t flags = PAEPageFlags::Present | PAEPageFlags::Write);
	void create_page_level(PageSkellington &lvl);
	void resolve_cow_fault(RootPageLevel &owner_pd, RootPageLevel &dest_pd,
						   uintptr_t fault_addr, bool owner_initiated=false);
	// Allows for the mapping a page without a proper way to address it later.
	// For example, if we want to map one of our page levels so it can be
	// modified with a lower level or new permissions. Then we'd have to map
	// with impunity. Sooo the solution is to map the current level with
	// impunity and map the level beyond that with no impunity (I know this is
	// horrible). The only other solution I can come up with is having a
	// temporary address outside the virtual address of the highest page level
	// and temporarily mapping the page levels we want to modify to that
	// address.
	// void map_page_with_no_impunity(RootPageLevel& pd, uintptr_t virt,
	// uintptr_t phys);

	static RootPageLevel *s_kernel_page_directory;

	static size_t s_pf_allocator_end;
	static size_t s_pf_allocator_base;

  private:
	friend struct PageLevel;
	friend struct LovelyPageLevel;

	RootPageLevel *m_safe_pgl;
	Paging(size_t total_memory, const KernelBootInfo &kbootinfo);
	// A safe area of memory reserved for copying pages
	uint8_t *m_safe_area;
	PageFrameAllocator *m_allocator;
	static Paging *s_instance;
};

// Helper, this sucks
struct PageSkellington {
	PageSkellington() = default;
	// PageSkellington(const PageSkellington&) = delete;

	inline bool is_mapped() const { return (pdata & PAEPageFlags::Present); }

	inline bool is_user() const { return (pdata & PAEPageFlags::User); }

	PageLevel *level() const {
		return reinterpret_cast<PageLevel *>(pdata & PAGE_ADDRESS_MASK);
	}

	LovelyPageLevel *fetch();
	LovelyPageLevel *fetch_pool();
	void commit(LovelyPageLevel &level);

	inline uintptr_t addr() const {
		return static_cast<uintptr_t>(pdata & PAGE_ADDRESS_MASK);
	}

	inline uintptr_t flags() const {
		return static_cast<uintptr_t>(pdata & ~PAGE_ADDRESS_MASK);
	}

	inline void set_addr(uintptr_t addr) {
		pdata &= ~PAGE_ADDRESS_MASK;
		pdata |= (addr & PAGE_ADDRESS_MASK);
	}

	inline void copy_flags(uint64_t data) {
		pdata &= PAGE_ADDRESS_MASK;
		pdata |= (data & ~PAGE_ADDRESS_MASK);
	}

	uint64_t pdata;
} __attribute__((packed));

static_assert(sizeof(PageSkellington) == 8);

struct PageLevel {
	MALLOC_NEW_MUST_BE_PAGE_ALIGNED;

	inline PageLevel *map() const {
		auto map_page = Paging::the()->m_safe_area + PAGE_SIZE;
		Paging::the()->map_page_assume(*(RootPageLevel *)get_cr3(),
									   (uintptr_t)map_page, (uintptr_t)this);
		return (PageLevel *)map_page;
	}

	PageSkellington entries[512];
} __attribute__((packed)) __attribute__((aligned(PAGE_SIZE)));

// Without the malloc constraint
struct LovelyPageLevel {
	inline PageLevel *map() const {
		auto map_page = Paging::the()->m_safe_area + PAGE_SIZE;
		Paging::the()->map_page_assume(*(RootPageLevel *)get_cr3(),
									   (uintptr_t)map_page, (uintptr_t)this);
		return (PageLevel *)map_page;
	}

	PageSkellington entries[512];
} __attribute__((packed));

struct RootPageLevel : PageLevel {
	uintptr_t get_page_flags(uintptr_t addr);
};
