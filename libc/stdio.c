#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int printf(const char *str, ...) {
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
				int value = __builtin_va_arg(ap, int);
				char buffer[32];
				itoa(buffer, value, 10);
				write(1, buffer, strlen(buffer));
				break;
			}
			case 'x': {
				int value = __builtin_va_arg(ap, int);
				char buffer[32];
				itoa(buffer, value, 16);
				write(1, buffer, strlen(buffer));
				break;
			}
			case 's': {
				char *value = __builtin_va_arg(ap, char *);
				write(1, value, strlen(value));
				break;
			}
			case 'c': {
				char value = __builtin_va_arg(ap, int);
				write(1, value, 1);
				break;
			}
			default:
				write(1, c, 1);
				write(1, c2, 1);
				break;
			}
			break;
		}
		default:
			write(1, c, 1);
			break;
		}
	}
	__builtin_va_end(ap);
	return 0;
}
