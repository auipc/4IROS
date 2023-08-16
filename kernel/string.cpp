#include <kernel/string.h>

size_t strlen(const char* str) {
	size_t length = 0;
	while (*str != '\0') {
		length++;
		str++;
	}

	return length;
}
