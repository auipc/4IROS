#include <kernel/arch/i386/kernel.h>
#include <kernel/assert.h>
#include <kernel/mem/PageFrameAllocator.h>
#include <kernel/printk.h>
#include <kernel/util/Bitmap.h>

PageFrameAllocator::PageFrameAllocator(size_t memory_size) {
	m_pages = memory_size / PAGE_SIZE;
	m_bitmap = new Bitmap(m_pages);
}

PageFrameAllocator::~PageFrameAllocator() { delete m_bitmap; }

void PageFrameAllocator::mark_range(uint32_t start, uint32_t end) {
	for (uint32_t i = start; i < end; i += PAGE_SIZE) {
		m_bitmap->set(i / PAGE_SIZE);
	}
}

size_t PageFrameAllocator::find_free_page() {
	size_t address = (m_bitmap->scan(1) * PAGE_SIZE);
	assert(address % PAGE_SIZE == 0);
	m_bitmap->set(address / PAGE_SIZE);
	return address;
}

void PageFrameAllocator::release_page(size_t page) {
	if (!m_bitmap->get(page / PAGE_SIZE)) {
		printk("Trying to free page that doesn't exist!\n");
		return;
	}

	m_bitmap->unset(page / PAGE_SIZE);
}
