#pragma once
#include <kernel/printk.h>

namespace Debug {
void stack_trace();

void print_type(char);
void print_type(const char *);
void print_type(int);
void print_type(long long unsigned int);
void print_type(unsigned int);
void print_type_hex(int);
template <class... Args>
void dbg_print(int level, const char *&format, Args... args) {
	(void)level;

	bool format_fill = false;
	bool hex = false;
	while (*format) {
		switch (*format) {
		case '{': {
			format_fill = true;
		} break;
		case '}': {
			if (!format_fill) {
				printk("Bad format string\n");
				return;
			}

			if (hex)
				print_type_hex(args...);
			else
				print_type(args...);

			format_fill = false;
			hex = false;
		} break;
		case 'x': {
			if (format_fill)
				hex = true;
			else
				printk("%c", *format);
		} break;
		default: {
			printk("%c", *format);
		} break;
		}
		format++;
	}

	if (format_fill) {
		printk("Bad format string\n");
		return;
	}
}

template <class... Args>
[[maybe_unused]] void dbg(const char *format, Args... args) {
	dbg_print(0, format, args...);
}

template <class... Args>
[[maybe_unused]] void msg(const char *format, Args... args) {
	dbg_print(1, format, args...);
}

template <class... Args>
[[maybe_unused]] void error(const char *format, Args... args) {
	dbg_print(2, format, args...);
}
}; // namespace Debug
