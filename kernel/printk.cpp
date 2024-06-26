#include <kernel/arch/i386/IO.h>
#include <kernel/printk.h>
#include <kernel/util/Spinlock.h>
#include <string.h>

#define PORT 0x3f8

int init_serial() {
	outb(PORT + 1, 0x00); // Disable all interrupts
	outb(PORT + 3, 0x80); // Enable DLAB (set baud rate divisor)
	outb(PORT + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
	outb(PORT + 1, 0x00); //                  (hi byte)
	outb(PORT + 3, 0x03); // 8 bits, no parity, one stop bit
	outb(PORT + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
	outb(PORT + 4, 0x0B); // IRQs enabled, RTS/DSR set
	outb(PORT + 4, 0x1E); // Set in loopback mode, test the serial chip
	outb(PORT + 0, 0xAE); // Test serial chip (send byte 0xAE and check if
						  // serial returns same byte)

	// Check if serial is faulty (i.e: not same byte as sent)
	if (inb(PORT + 0) != 0xAE) {
		return 1;
	}

	// If serial is not faulty set it in normal operation mode
	// (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
	outb(PORT + 4, 0x0F);
	return 0;
}

int is_transmit_empty() { return inb(PORT + 5) & 0x20; }

void write_serial(char a) {
	while (is_transmit_empty() == 0)
		;

	outb(PORT, a);
}

static Spinlock s_print_spinlock;
static PrintInterface *s_interface = nullptr;

VGAInterface::VGAInterface()
	: m_x(0), m_y(0), m_screen(reinterpret_cast<uint16_t *>(0xC03FF000)) {
	init_serial();
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
		memcpy((char *)m_screen, (char *)(m_screen) + 160, 160 * 24);
		for (int i = 0; i < 80; i++) {
			m_screen[24 * 80 + i] = ' ' | (0x7 << 8);
		}
		m_y--;
	}

	write_serial(c);
}

PrintInterface *printk_get_interface() { return s_interface; }

void printk_use_interface(PrintInterface *interface) {
	s_interface = interface;
}

extern "C" void itoa(unsigned int n, char *buf, int base);
extern "C" void ftoa(double f, char *buf, int precision);

void printk(const char *str, ...) {
	if (!s_interface)
		return;
	s_print_spinlock.acquire();

	__builtin_va_list ap;
	__builtin_va_start(ap, str);
	for (size_t j = 0; j < strlen(str); j++) {
		char c = str[j];
		switch (c) {
		case '%': {
			j++;
			int precision = 6;
			char c2 = str[j];
			if (c2 == '.') {
				precision = str[j + 1] - '0';
				j += 2;
				c2 = str[j];

				// Skip invalid precision
				if (precision > 9)
					break;
			}

			switch (c2) {
			case 'd': {
				unsigned int value = __builtin_va_arg(ap, unsigned int);
				char buffer[32];
				itoa(value, buffer, 10);
				for (size_t i = 0; i < strlen(buffer); i++) {
					s_interface->write_character(buffer[i]);
				}
				break;
			}
			case 'x': {
				int value = __builtin_va_arg(ap, unsigned long long);
				char buffer[32];
				itoa(value, buffer, 16);
				for (size_t i = 0; i < strlen(buffer); i++) {
					s_interface->write_character(buffer[i]);
				}
				break;
			}
			case 's': {
				char *value = __builtin_va_arg(ap, char *);
				for (size_t i = 0; i < strlen(value); i++) {
					s_interface->write_character(value[i]);
				}
				break;
			}
			case 'c': {
				char value = __builtin_va_arg(ap, int);
				s_interface->write_character(value);
				break;
			}
			case 'f': {
				float value = __builtin_va_arg(ap, double);
				char buffer[32];
				ftoa(value, buffer, precision);
				for (size_t i = 0; i < strlen(buffer); i++) {
					s_interface->write_character(buffer[i]);
				}
				break;
			}
			default:
				s_interface->write_character(c);
				s_interface->write_character(c2);
				break;
			}
			break;
		}
		default:
			if (s_interface)
				s_interface->write_character(c);
			break;
		}
	}
	__builtin_va_end(ap);
	s_print_spinlock.release();
}
