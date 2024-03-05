#pragma once

#include <kernel/stdint.h>
#include <kernel/util/Bitmap.h>
#include <kernel/util/Vec.h>

class PageFrameAllocator {
  public:
	PageFrameAllocator(size_t memory_size);
	~PageFrameAllocator();
	void mark_range(uint32_t start, uint32_t end);
	size_t find_free_page();
	size_t find_free_pages(size_t pages);
	void release_page(size_t page);
	static size_t alloc_size(size_t memory_size);

  private:
	size_t largest_container(size_t size);
	uint32_t m_pages;
	Vec<Bitmap *> m_buddies;
};
