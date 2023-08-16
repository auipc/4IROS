#include <kernel/printk.h>
#include <kernel/string.h>

static PrintInterface* s_interface = nullptr;

VGAInterface::VGAInterface() 
	: m_x(0)
	, m_y(0)
	, m_screen(reinterpret_cast<uint16_t*>(0xC03FF000))
{
	clear_output();
}

void VGAInterface::clear_output() {
	for (int i = 0; i < 80 * 25; i++) {
		m_screen[i] = 0;
	}
}

void VGAInterface::write_character(char c) {
	if (m_x > 80) {
		m_x = 0;
		m_y++;
	}

	if (c == '\n') {
		m_x = 0;
		m_y++;
		return;
	}

	m_screen[m_y * 80 + m_x] = c | (0xF << 8);
	m_x++;
}

void printk_use_interface(PrintInterface* interface) {
	s_interface = interface;
}

void printk(const char* str, ...) {
	if (!s_interface) return;
	for (int i = 0; i < strlen(str); i++) {
		s_interface->write_character(str[i]);
	}
}
