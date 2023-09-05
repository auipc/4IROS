#include <kernel/arch/i386/kernel.h>
#include <kernel/assert.h>
#include <kernel/mem/PageFrameAllocator.h>
#include <kernel/util/Bitmap.h>

constexpr int pow(int a, int b) {
	int result = 1;

	for (int i = 0; i < b; i++) {
		result *= a;
	}

	return result;
}

static_assert(pow(3,4) == 81);

PageFrameAllocator::PageFrameAllocator(size_t memory_size) {
	m_pages = memory_size / PAGE_SIZE;
	for (int i = 0; i < 4; i++) {
		m_buddies.push(new Bitmap(memory_size / (pow(2, i) * PAGE_SIZE)));
	}
}

PageFrameAllocator::~PageFrameAllocator() {
	for (size_t buddy = 1; buddy < m_buddies.size(); buddy++) {
		delete m_buddies[buddy];
	}
}

void PageFrameAllocator::mark_range(uint32_t start, uint32_t end) {
	for (uint32_t i = start; i < end; i += PAGE_SIZE) {
		m_buddies[0]->set(i / PAGE_SIZE);
		for (size_t buddy = 1; buddy < m_buddies.size(); buddy++) {
			if ((i % (pow(2, buddy) * PAGE_SIZE)) == 0) {
				m_buddies[buddy]->set(i / (pow(2, buddy) * PAGE_SIZE));
			}
		}
	}
}

// lowest level with buddy allocation
size_t PageFrameAllocator::find_free_page() {
	size_t address = (m_buddies[0]->scan(1) * PAGE_SIZE);
	assert(address % PAGE_SIZE == 0);
	m_buddies[0]->set(address / PAGE_SIZE);

	for (size_t buddy = 1; buddy < m_buddies.size(); buddy++) {
		if ((address % (pow(2, buddy) * PAGE_SIZE)) == 0) {
			m_buddies[buddy]->set(address / (pow(2, buddy) * PAGE_SIZE));
		}
	}

	return address;
}

// FIXME have a way to requestt non contigous memory
size_t PageFrameAllocator::find_free_pages(size_t pages) {
	assert(pages > 0);
	size_t container = largest_container(pages * PAGE_SIZE);

	size_t start = m_buddies[container]->scan_no_set(1);

	for (size_t i = start; i < pages; i++) {
		m_buddies[0]->set(i);
		for (size_t buddy = 1; buddy < m_buddies.size(); buddy++) {
			if (((i * PAGE_SIZE) % (pow(2, buddy) * PAGE_SIZE)) == 0) {
				m_buddies[buddy]->set((i * PAGE_SIZE) /
									  (pow(2, buddy) * PAGE_SIZE));
			}
		}
	}

	return (start * PAGE_SIZE);
}

int abs(int a) {
	return a > 0 ? a : -a;
}

size_t PageFrameAllocator::largest_container(size_t size) {
	int container = 0;
	int min = abs(size - (pow(2, container) * PAGE_SIZE));
	for (size_t buddy = 1; buddy < m_buddies.size(); buddy++) {
		size_t buddy_size = pow(2, buddy) * PAGE_SIZE;
		int diff = abs(size - buddy_size);
		if (diff < min) {
			min = diff;
			container = buddy;
		}
	}

	return container;
}

void PageFrameAllocator::release_page(size_t page) {
	(void)page;
	/*if (!m_bitmap->get(page / PAGE_SIZE)) {
		return;
	}

	m_bitmap->unset(page / PAGE_SIZE);*/
}
