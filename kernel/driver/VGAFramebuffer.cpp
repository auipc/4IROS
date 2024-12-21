#include <kernel/arch/x86_common/IO.h>
#include <kernel/driver/VGAFramebuffer.h>

void VGAFramebuffer::write_misc_port(uint8_t byte) { outb(0x3C2, byte); }

void VGAFramebuffer::write_fixed_port(uint8_t index, uint8_t byte) {
	outb(0x3C0, index);
	outb(0x3C0, byte);
}

uint8_t VGAFramebuffer::read_fixed_port(uint8_t index) {
	outb(0x3C0, index);
	uint8_t val = inb(0x3C1);
	// Reading from port 0x3DA will set port 0x3C0 to it's index state.
	inb(0x3DA);
	return val;
}

void VGAFramebuffer::write_index_port(uint16_t port, uint8_t index,
									  uint8_t byte) {
	outb(port, index);
	outb(port + 1, byte);
}

void VGAFramebuffer::init() {
	// Reading from port 0x3DA will set port 0x3C0 to it's index state.
	inb(0x3DA);

	// Switch to mode X (I'm too lazy for SVGA!)
	// https://wiki.osdev.org/VGA_Hardware#List_of_register_settings
	write_fixed_port(0x10, 0x41);				  // 0x41
	write_fixed_port(0x11, 0x00);				  // 0x00
	write_fixed_port(0x12, 0x0F);				  // 0x0F
	write_fixed_port(0x13, 0x00);				  // 0x00
	write_fixed_port(0x14, 0x00);				  // 0x00
	write_misc_port(0xE3);						  // 0xE3
	write_index_port(VGA_REGISTER_1, 0x01, 0x01); // 0x01
	write_index_port(VGA_REGISTER_1, 0x03, 0x00); // 0x00
	write_index_port(VGA_REGISTER_1, 0x04, 0x06); // 0x06
	write_index_port(VGA_REGISTER_2, 0x05, 0x40); // 0x40
	write_index_port(VGA_REGISTER_2, 0x06, 0x05); // 0x05
	write_index_port(VGA_REGISTER_3, 0x00, 0x5F); // 0x5F
	write_index_port(VGA_REGISTER_3, 0x01, 0x4F); // 0x4F
	write_index_port(VGA_REGISTER_3, 0x02, 0x50); // 0x50
	write_index_port(VGA_REGISTER_3, 0x03, 0x82); // 0x82
	write_index_port(VGA_REGISTER_3, 0x04, 0x54); // 0x54
	write_index_port(VGA_REGISTER_3, 0x05, 0x80); // 0x80
	write_index_port(VGA_REGISTER_3, 0x06, 0x0D); // 0x0D
	write_index_port(VGA_REGISTER_3, 0x07, 0x3E); // 0x3E
	write_index_port(VGA_REGISTER_3, 0x08, 0x00); // 0x00
	write_index_port(VGA_REGISTER_3, 0x09, 0x41); // 0x41
	write_index_port(VGA_REGISTER_3, 0x10, 0xEA); // 0xEA
	write_index_port(VGA_REGISTER_3, 0x11, 0xAC); // 0xAC
	write_index_port(VGA_REGISTER_3, 0x12, 0xDF); // 0xDF
	write_index_port(VGA_REGISTER_3, 0x13, 0x28); // 0x28
	write_index_port(VGA_REGISTER_3, 0x14, 0x00); // 0x00
	write_index_port(VGA_REGISTER_3, 0x15, 0xE7); // 0xE7
	write_index_port(VGA_REGISTER_3, 0x16, 0x06); // 0x06
	write_index_port(VGA_REGISTER_3, 0x17, 0xE3); // 0xE3
	// Quirk
	outb(0x3C0, 0x20);

	// Clear the screen
	for (size_t _ = 0; _ < 320 * 240; _++) {
		uint8_t zed = 0;
		write(&zed, 1);
	}
	seek(0);

	VFSNode::init();
}

int VGAFramebuffer::write(void *buffer, size_t size) {
	for (size_t i = 0; i < size; i++) {
		auto addr = m_position + i;
		write_index_port(VGA_REGISTER_1, 0x02, 0b1 << (addr & 3));
		*(char *)(FRAMEBUFFER_ADDR + (addr / 4)) =
			reinterpret_cast<uint8_t *>(buffer)[i];
	}
	m_position += size;
	m_position %= 320 * 240;

	return size;
}
