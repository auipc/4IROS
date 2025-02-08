#pragma once

#include <kernel/util/Bitmap.h>
#include <kernel/util/Vec.h>
#include <stdint.h>

class PageFrameAllocator {
  public:
	PageFrameAllocator(size_t base, size_t memory_size);
	~PageFrameAllocator();
	size_t mem_in_use();
	void mark_range(uintptr_t start, size_t end);
	size_t find_free_page();
	size_t find_free_pages(size_t pages);
	bool pertains(size_t page);
	void release_page(size_t page);
  private:
	size_t largest_container(size_t size);
	uint32_t m_pages;
	Vec<Bitmap *> m_buddies;
	size_t m_addr_base = 0;
};
