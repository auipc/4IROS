#include <kernel/string.h>

void itoa(char *buf, unsigned long int n, int base) {
	unsigned long int tmp;
	int i, j;

	tmp = n;
	i = 0;

	do {
		tmp = n % base;
		buf[i++] = (tmp < 10) ? (tmp + '0') : (tmp + 'a' - 10);
	} while (n /= base);
	buf[i--] = 0;

	for (j = 0; j < i; j++, i--) {
		tmp = buf[j];
		buf[j] = buf[i];
		buf[i] = tmp;
	}
}

void ftoa(char *buf, double f, int precision) {
	int pos = 0;
	if (f < 0) {
		buf[pos++] = '-';
		f = -f;
	}

	int intPart = (int)f;
	double remainder = f - (double)intPart;

	itoa(buf + pos, intPart, 10);
	pos += strlen(buf + pos);

	buf[pos++] = '.';

	for (int i = 0; i < precision; i++) {
		remainder *= 10;
		int digit = (int)remainder;
		buf[pos++] = digit + '0';
		remainder -= digit;
	}

	buf[pos] = '\0';
}

void memset(char *buffer, char value, size_t size) {
	for (size_t i = 0; i < size; i++) {
		buffer[i] = value;
	}
}

void *memcpy(void *dest, const void *src, size_t size) {
	for (size_t i = 0; i < size; i++) {
		((char *)dest)[i] = ((char *)src)[i];
	}

	return dest;
}

size_t strlen(const char *str) {
	size_t length = 0;
	while (*str != '\0') {
		length++;
		str++;
	}

	return length;
}
