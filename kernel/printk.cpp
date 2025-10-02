#include <kernel/arch/x86_common/IO.h>
#include <kernel/printk.h>
#include <kernel/util/Spinlock.h>
#include <string.h>

struct VGAChar {
	char c;
	uint8_t color;
};

Spinlock print_spinlock;

// lol
VGAChar *screen_buf = (VGAChar *)0xB8000;
size_t screen_buf_x = 0;
size_t screen_buf_y = 0;
#define PORT 0x3f8
#define SCREEN_SIZE (80 * 25 * sizeof(VGAChar))

// Yoinked from OSDev.org
static int init_serial() {
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

void write_scb(char c, uint8_t color) {
	write_serial(c);
	if (c == '\n') {
		screen_buf_x = 0;
		screen_buf_y++;
		return;
	}

	if (c == '\b') {
		if (screen_buf_x > 0)
			screen_buf_x -= 1;
		screen_buf[screen_buf_x + 80 * screen_buf_y].c = 0;
		return;
	}

	if (screen_buf_x == 80) {
		screen_buf_x = 0;
		screen_buf_y++;
	}

	if (screen_buf_y >= 24) {
		for (int i = 1; i < 25; i++) {
			memcpy((char *)&screen_buf[screen_buf_x + 80 * (i - 1)],
				   (char *)&screen_buf[screen_buf_x + 80 * (i)],
				   80 * sizeof(VGAChar));
		}
		memset((char *)&screen_buf[screen_buf_x + 80 * screen_buf_y], 0,
			   80 * sizeof(VGAChar));
		screen_buf_y--;
	}

	screen_buf[screen_buf_x + 80 * screen_buf_y].c = c;
	screen_buf[screen_buf_x + 80 * screen_buf_y].color = color;
	screen_buf_x++;
}

void write_scb_str(const char *str, uint8_t color) {
	while (*str != '\0') {
		write_scb(*str, color);
		str++;
	}
}

void screen_init() {
	init_serial();
	memset((char *)screen_buf, 0, SCREEN_SIZE);
}

void vprintk(const char *str, __builtin_va_list list) {
	// size_t next_str_length = 0;
	print_spinlock.acquire();
	while (*str != '\0') {
		char c = *str;
		switch (c) {
		case '%': {
			char c1 = *(++str);
			switch (c1) {
			case 'x': {
				uint64_t n = __builtin_va_arg(list, uint64_t);
				char buf[33] = {};
				ulltoa(n, buf, 16);
				write_scb_str(buf, 7);
			} break;
			case 'd': {
				uint64_t n = __builtin_va_arg(list, uint64_t);
				char buf[33] = {};
				itoa(n, buf, 10);
				write_scb_str(buf, 7);
			} break;
			case 'c': {
				unsigned int ch = __builtin_va_arg(list, unsigned int);
				write_scb(ch & 0xff, 7);
			} break;
			case 's': {
				char *str = __builtin_va_arg(list, char *);
				write_scb_str(str, 7);
				// next_str_length = 0;
			} break;
			default: {
				write_scb(c, 7);
				write_scb(c1, 7);
			} break;
			}
		} break;
		default:
			write_scb(c, 7);
			break;
		}
		str++;
	}
	__builtin_va_end(list);
	print_spinlock.release();
}

void printk(const char *str, ...) {
	__builtin_va_list list;
	__builtin_va_start(list, str);
	vprintk(str, list);
	__builtin_va_end(list);
}

void info(const char *str, ...) {
#ifdef DEBUG_VERBOSE
	__builtin_va_list list;
	__builtin_va_start(list, str);
	vprintk(str, list);
	__builtin_va_end(list);
#else
	(void)str;
#endif
}

[[noreturn]] void panic(const char *str) {
	asm volatile("cli");
	write_scb_str("PANIC: ", 7 | (4 << 4) | (8 << 4));
	write_scb_str(str, 7 | (4 << 4) | (8 << 4));

	/*
	outb(0x43, (2<<6) | (3<<4) | (0b011<<1));
	outb(0x42, 0xFF);
	outb(0x42, 0xFF);

	outb(0x61, 1<<1 | 1<<0);
	for (size_t i = 0; i < 10000000; i++) {
		asm volatile("nop");
	}
	outb(0x61, 0);
	*/

	while (1) {
		asm volatile("nop");
	}
}
