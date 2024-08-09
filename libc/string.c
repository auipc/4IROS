#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
int strncmp(const char *s1, const char *s2, size_t n) {

	if (n == 0)
		return (0);
	do {
		if (*s1 != *s2++)
			return (*(unsigned char *)s1 - *(unsigned char *)--s2);
		if (*s1++ == 0)
			break;
	} while (--n != 0);
	return (0);
}

int strcmp(const char *s1, const char *s2) {
	while (*s1 == *s2++)
		if (*s1++ == 0)
			return (0);
	return (*(unsigned char *)s1 - *(unsigned char *)--s2);
}

void memset(char *buffer, char value, size_t size) {
	for (size_t i = 0; i < size; i++) {
		buffer[i] = value;
	}
}

void *memcpy(void *s1, const void *s2, size_t n) {
	const char *f = (const char *)s2;
	char *t = (char *)s1;

	while (n-- > 0)
		*t++ = *f++;
	return s1;
}

void itoa(uint64_t n, char *buf, int base) {
	int i = 0;

	buf[i++] = '\0';
	do {
		if ((n%base) < 10)
			buf[i++] = '0'+(n%base);
		else
			buf[i++] = 'A'+((n-10)%base);
		n /= base;
	} while (n);

	i--;
	for (int j = 0; j < i; j++, i--) {
		uint64_t tmp = buf[i];
		buf[i] = buf[j];
		buf[j] = tmp;
	}
}

size_t strlen(const char *str) {
	const char *start = str;

	while (*str)
		str++;
	return str - start;
}
#ifdef __cplusplus
};
#endif
