#include <kernel/arch/i386/kernel.h>
#include <kernel/assert.h>
#include <kernel/mem/PageFrameAllocator.h>
#include <kernel/printk.h>
#include <kernel/util/Bitmap.h>

int pow(int a, int b) {
	int result = a;

    // Wow, such problem solving
    // Pure genius
    if (b == 0) return 1;
    if (b == 1) return a;

	for (int i = 0; i < b-1; i++) {
		result *= a;
	}
    
	return result;
}

PageFrameAllocator::PageFrameAllocator(size_t memory_size) {
	m_pages = memory_size / PAGE_SIZE;
	for (int i = 0; i < 4; i++) {
		m_buddies.push(new Bitmap(memory_size / (pow(2,i)*PAGE_SIZE)));
		printk("Size of buddy %d: %d, size block %d\n", i, memory_size / (pow(2,i)*PAGE_SIZE), pow(2,i)*PAGE_SIZE);
	}
	//m_bitmap = new Bitmap(m_pages);
}

PageFrameAllocator::~PageFrameAllocator() { /*delete m_bitmap;*/ }

void PageFrameAllocator::mark_range(uint32_t start, uint32_t end) {
	for (uint32_t i = start; i < end; i += PAGE_SIZE) {
		m_buddies[0]->set(i / PAGE_SIZE);
		for (size_t buddy = 1; buddy < m_buddies.size(); buddy++) {
			if ((i % (pow(2, buddy)*PAGE_SIZE)) == 0) {
				m_buddies[buddy]->set(i / (pow(2, buddy)*PAGE_SIZE));
				printk("lol %d, %d, %d\n", i % (pow(2, buddy)*PAGE_SIZE), (pow(2, buddy)*PAGE_SIZE), buddy);
			}
			printk("lol %d\n", i % (pow(2, buddy)*PAGE_SIZE));
		}
	}
}

// lowest level with buddy allocation
size_t PageFrameAllocator::find_free_page() {
	size_t address = (m_buddies[0]->scan(1) * PAGE_SIZE);
	assert(address % PAGE_SIZE == 0);
	m_buddies[0]->set(address / PAGE_SIZE);
	return address;
}

void PageFrameAllocator::release_page(size_t page) {
	(void)page;
	/*if (!m_bitmap->get(page / PAGE_SIZE)) {
		printk("Trying to free page that doesn't exist!\n");
		return;
	}

	m_bitmap->unset(page / PAGE_SIZE);*/
}
