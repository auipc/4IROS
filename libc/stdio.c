#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int printf(const char *format, ...) {
	__builtin_va_list list;
	__builtin_va_start(list, format);
	for (size_t j = 0; j < strlen(format); j++) {
		char c = format[j];
		switch (c) {
		case '%': {
			j++;
			int precision = 6;
			char c2 = format[j];
			if (c2 == '.') {
				precision = format[j + 1] - '0';
				j += 2;
				c2 = format[j];

				// Skip invalid precision
				if (precision > 9)
					break;
			}

			switch (c2) {
			case 'd': {
				int value = __builtin_va_arg(list, int);
				char buffer[32];
				itoa(buffer, value, 10);
				write(1, buffer, strlen(buffer));
				break;
			}
			case 'x': {
				int value = __builtin_va_arg(list, int);
				char buffer[32];
				itoa(buffer, value, 16);
				write(1, buffer, strlen(buffer));
				break;
			}
			case 's': {
				char *value = __builtin_va_arg(list, char *);
				write(1, &value, strlen(value));
				break;
			}
			case 'c': {
				char value = __builtin_va_arg(list, int);
				write(1, &value, 1);
				break;
			}
			default:
				write(1, &c, 1);
				write(1, &c2, 1);
				break;
			}
			break;
		}
		default:
			write(1, &c, 1);
			break;
		}
	}

	/*
	while (*format++) {
		if (*format == '%') {
			const char* format_t = format;
			while(*format_t++) {
				switch (*format_t) {
					case 's':
						char* str = __builtin_va_arg(list, char*);
						write(1, str, strlen(str));
						format++;
						break;
					default:
						write(1, format, 1);
						break;
				}
			}
		} else {
			write(1, format, 1);
		}
	}*/
	__builtin_va_end(list);
}
