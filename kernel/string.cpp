#include <kernel/string.h>

void memset(char* buffer, char value, size_t size) {
	for (size_t i = 0; i < size; i++) {
		buffer[i] = value;
	}
}

void* memcpy(void* dest, const void* src, size_t size) {
	for (size_t i = 0; i < size; i++) {
		((char*)dest)[i] = ((char*)src)[i];
	}

	return dest;
}

size_t strlen(const char* str) {
	size_t length = 0;
	while (*str != '\0') {
		length++;
		str++;
	}

	return length;
}
