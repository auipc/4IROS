#include <kernel/driver/BochsFramebuffer.h>
#include <kernel/mem/Paging.h>

BochsFramebuffer::BochsFramebuffer() : VFSNode("bochs") {}

void BochsFramebuffer::init() {
	Paging::the()->map_page(*(RootPageLevel *)get_cr3(),
							FRAMEBUFFER_CONFIG_ADDR, FRAMEBUFFER_CONFIG_ADDR);
	*(uint16_t *)(FRAMEBUFFER_CONFIG_ADDR + 8) = 0;
	uint16_t xres = *(uint16_t *)(FRAMEBUFFER_CONFIG_ADDR + 2);
	uint16_t yres = *(uint16_t *)(FRAMEBUFFER_CONFIG_ADDR + 4);
	printk("%dx%d\n", xres, yres);
	for (int i = 0; i < (xres * yres * 4); i += 4096) {
		Paging::the()->map_page(*(RootPageLevel *)get_cr3(),
								FRAMEBUFFER_ADDR + i, FRAMEBUFFER_ADDR + i);
	}

	*(uint16_t *)(FRAMEBUFFER_CONFIG_ADDR + 8) = 0x40 | 1;
	// memset((char *)FRAMEBUFFER_ADDR, 0, xres * yres * 4);
}

int BochsFramebuffer::write(void *buffer, size_t size) {
	memcpy((void *)(FRAMEBUFFER_ADDR + m_position), buffer, size);
	m_position += size;
	return size;
}
