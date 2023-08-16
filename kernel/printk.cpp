#include <kernel/printk.h>
#include <kernel/string.h>

static PrintInterface *s_interface = nullptr;

VGAInterface::VGAInterface()
	: m_x(0), m_y(0), m_screen(reinterpret_cast<uint16_t *>(0xC03FF000)) {
	clear_output();
}

void VGAInterface::clear_output() {
	for (int i = 0; i < 80 * 25; i++) {
		m_screen[i] = ' ' | (0x7 << 8);
	}
}

void VGAInterface::write_character(char c) {
	if (c == '\n') {
		m_x = 0;
		m_y++;
	} else {
		m_screen[m_y * 80 + m_x] = c | (0x7 << 8);
		m_x++;
	}

	if (m_x >= 80) {
		m_x = 0;
		m_y++;
	}

	// scroll the screen
	if (m_y >= 25) {
		memcpy((char *)m_screen, (char *)(m_screen)+160, 160 * 24);
		for (int i = 0; i < 80; i++) {
			m_screen[24 * 80 + i] = ' ' | (0x7 << 8);
		}
		m_y--;
	}
}

void printk_use_interface(PrintInterface *interface) {
	s_interface = interface;
}

void printk(const char *str, ...) {
	if (!s_interface)
		return;
	for (int i = 0; i < strlen(str); i++) {
		s_interface->write_character(str[i]);
	}
}
