#include <string.h>

void itoa(unsigned long int n, char *buf, int base) {
	unsigned long int tmp;
	int i, j;

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

void ftoa(double f, char *buf, int precision) {
	int pos = 0;
	if (f < 0) {
		buf[pos++] = '-';
		f = -f;
	}

	int intPart = (int)f;
	double remainder = f - (double)intPart;

	itoa(intPart, buf + pos, 10);
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

size_t strlen(const char *str) {
	size_t length = 0;
	while (*str != '\0') {
		length++;
		str++;
	}

	return length;
}
