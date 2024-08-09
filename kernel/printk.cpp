#include <kernel/arch/x86_common/IO.h>
#include <kernel/printk.h>
#include <kernel/util/Spinlock.h>
#include <string.h>

struct VGAChar {
	char c;
	uint8_t color;
};

// lol
VGAChar* screen_buf = (VGAChar*)0xB8000;
size_t screen_buf_x = 0;
size_t screen_buf_y = 0;
#define PORT 0x3f8
#define SCREEN_SIZE (80*25*sizeof(VGAChar))

void write_scb(char c, uint8_t color) {
	if (c == '\n') {
		screen_buf_x = 0;
		screen_buf_y++;
		return;
	}

	if (screen_buf_x == 80) {
		screen_buf_x = 0;
		screen_buf_y++;
	}

	if (screen_buf_y >= 24) {
		for (int i = 1; i < 25; i++) {
			memcpy((char*)&screen_buf[screen_buf_x+80*(i-1)], (char*)&screen_buf[screen_buf_x+80*(i)], 80*sizeof(VGAChar));
		}
		memset((char*)&screen_buf[screen_buf_x+80*screen_buf_y], 0, 80*sizeof(VGAChar));
		screen_buf_y--;
	}

	screen_buf[screen_buf_x+80*screen_buf_y].c = c;
	screen_buf[screen_buf_x+80*screen_buf_y].color = color;
	screen_buf_x++;
}

void write_scb_str(const char* str, uint8_t color) {
	while (*str != '\0') {
		write_scb(*str, color);
		str++;
	}
}

void screen_init() {
	memset((char*)screen_buf, 0, SCREEN_SIZE);
}

void printk(const char *str, ...) {
	__builtin_va_list list;
	__builtin_va_start(list, str);
	size_t next_str_length = 0;
	while (*str != '\0') {
		char c = *str;
		switch(c) {
			case '%': {
				char c1 = *(++str);
				switch(c1) {
					case 'x': {
						uint64_t n = __builtin_va_arg(list, uint64_t);
						char buf[33] = {};
						itoa(n, buf, 16);
						write_scb_str(buf, 7);
					} break;
					case 'd': {
						uint64_t n = __builtin_va_arg(list, uint64_t);
						char buf[33] = {};
						itoa(n, buf, 10);
						write_scb_str(buf, 7);
					} break;
					case 's': {
						char* str = __builtin_va_arg(list, char*);
						write_scb_str(str, next_str_length ? next_str_length : strlen(str));
						next_str_length = 0;
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
}

[[noreturn]] void panic(const char *str) {
	write_scb_str("PANIC: ", 7|(4<<4)|(8<<4));
	write_scb_str(str, 7|(4<<4)|(8<<4));
	
	outb(0x43, (2<<6) | (3<<4) | (0b011<<1));
	outb(0x42, 0xFF);
	outb(0x42, 0xFF);

	outb(0x61, 1<<1 | 1<<0);
	for (size_t i = 0; i < 10000000; i++) {
		asm volatile("nop");
	}
	outb(0x61, 0);

	while(1);
}
