#pragma once

#include <kernel/stdint.h>
#include <kernel/util/Bitmap.h>

class PageFrameAllocator {
  public:
	PageFrameAllocator(size_t memory_size);
	~PageFrameAllocator();
	void mark_range(uint32_t start, uint32_t end);
	size_t find_free_page();
	void release_page(size_t page);

  private:
	uint32_t m_pages;
	Bitmap *m_bitmap;
};
