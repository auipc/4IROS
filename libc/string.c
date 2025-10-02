#include <alloca.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#ifdef LIBK
extern void *kmalloc(size_t size);
#endif

#ifdef __cplusplus
extern "C" {
#endif

const char *strrchr(const char *str, int character) {
	char *last = NULL;
	do {
		if (*str == character)
			last = str;
	} while (*(str++));
	return last;
}

char *strdup(const char *s) {
	size_t s_sz = strlen(s);
#ifndef LIBK
	char *m = (char *)malloc(s_sz + 1);
#else
	char *m = (char *)kmalloc(s_sz + 1);
#endif
	memcpy(m, s, s_sz);
	*(m + s_sz) = 0;
	return m;
}

#ifndef LIBK
char *strndup(const char *s, size_t n) {
	char *m = (char *)malloc(n + 1);
	memcpy(m, s, n);
	m[n] = '\0';
	return m;
}
#endif

char *strerror(int errnum) { return NULL; }

const char *strchr(const char *str, int character) {
	for (size_t i = 0; i < strlen(str); i++) {
		if (str[i] == character)
			return &str[i];
	}

	return NULL;
}

char *strncat(char *destination, const char *source, size_t num) {
	size_t dst_sz = strlen(destination);
	size_t src_sz = strlen(source);
	size_t sz = ((src_sz > num) ? num : src_sz);
	memcpy(&destination[dst_sz], source, sz);
	destination[dst_sz + sz] = '\0';
	return destination;
}

char *strcat(char *destination, const char *source) {
	memcpy(&destination[strlen(destination)], source, strlen(source) + 1);
	return destination;
}

char *strcpy(char *destination, const char *source) {
	while ((*(destination++) = *(source++)))
		;
	return destination;
}

// FIXME
char *strncpy(char *destination, const char *source, size_t n) {
	if (n == 0)
		return (0);

	memset(destination, 0, n);
	for (size_t i = 0; i < n; i++) {
		if (!source[i])
			break;
		destination[i] = source[i];
	}
	return destination;
}

// stolen from OpenBSD
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

int memcmp(const void *s1, const void *s2, size_t n) {
	if (!n)
		return 0;

	while (n--) {
		if (*(unsigned char *)s1 != *(unsigned char *)s2)
			return (*(unsigned char *)s1 > *(unsigned char *)s2) ? 1 : -1;
		s1++;
		s2++;
	}

	return 0;
}

int strncasecmp(const char *s1, const char *s2, size_t n) {
	if (n == 0)
		return 0;

	for (int i = 0; i < n; i++) {
		if (!s1[i] && !s2[i])
			break;
		if (toupper((unsigned char)s1[i]) != toupper((unsigned char)s2[i])) {
			return -1;
		}
	}

	return 0;
}

void *memchr(const void *s, int c, size_t n) {
	while (n-- && *((const unsigned char *)s++) != c)
		;
	return (n || *((const unsigned char *)s) != c) ? (void *)((char *)s + n)
												   : 0;
}

void *memrchr(const void *s, int c, size_t n) {
	while (n && *(((const unsigned char *)s) + (n--)) != c)
		;
	return (n || *(((const unsigned char *)s) + n) == c) ? (void *)(s + n)
														 : NULL;
}

// stolen from OpenBSD
int strcmp(const char *s1, const char *s2) {
	while (*s1 == *s2++)
		if (*s1++ == 0)
			return (0);
	return (*(unsigned char *)s1 - *(unsigned char *)--s2);
}

// stolen from OpenBSD
int strcasecmp(const char *s1, const char *s2) {
	while ((*s1 == *s2++) || abs(*s1 - (*(s2 - 1))) == 32)
		if (*s1++ == 0)
			return (0);
	return (*(unsigned char *)s1 - *(unsigned char *)--s2);
}

void *memset(char *buffer, char value, size_t size) {
	for (size_t i = 0; i < size; i++) {
		buffer[i] = value;
	}
	return buffer;
}

#ifndef LIBK
void *memmove(void *s1, const void *s2, size_t n) {
	if (s1 == s2)
		return s1;

	// FIXME
	char *m = (char *)malloc(n);
	memcpy(m, s2, n);
	memcpy(s1, m, n);
	return s1;
}
#endif

void *memcpy(void *s1, const void *s2, size_t n) {
	const char *f = (const char *)s2;
	char *t = (char *)s1;

	while (n-- > 0)
		*t++ = *f++;
	return s1;
}

void ulltoa(uint64_t n, char *buf, int base) {
	int i = 0;

	buf[i++] = '\0';
	do {
		if ((n % base) < 10)
			buf[i++] = '0' + (n % base);
		else
			buf[i++] = 'a' + ((n - 10) % base);
		n /= (uint64_t)base;
	} while (n);

	i--;
	for (int j = 0; j < i; j++, i--) {
		uint64_t tmp = buf[i];
		buf[i] = buf[j];
		buf[j] = tmp;
	}
}

void itoa(int64_t n, char *buf, int base) {
	int neg = n < 0;
	int i = 0;

	if (neg)
		n = -n;

	buf[i++] = '\0';
	do {
		if ((n % base) < 10)
			buf[i++] = '0' + (n % base);
		else
			buf[i++] = 'a' + ((n - 10) % base);
		n /= (int64_t)base;
	} while (n);

	if (neg)
		buf[i++] = '-';

	i--;
	for (int j = 0; j < i; j++, i--) {
		uint64_t tmp = buf[i];
		buf[i] = buf[j];
		buf[j] = tmp;
	}
}

int atoi(const char *str) {
	size_t n = 0;
	while (*str) {
		if (*str > '9' || *str < '0') {
			str++;
			continue;
		}
		n *= 10;
		n += *str - '0';
		str++;
	}
	return n;
}

size_t strlen(const char *str) {
	const char *start = str;

	while (*str)
		str++;
	return str - start;
}

size_t strnlen(const char *str, size_t n) {
	const char *start = str;
	while (*str || (n--))
		str++;
	return str - start;
}

char *next = NULL;

char *strtok(char *s1, const char *s2) {
	char *p = s1 ? s1 : next;
	for (int i = 0; i < strlen(p); i++) {
		for (int j = 0; j < strlen(s2); j++) {
			if (p[i] == s2[j]) {
				next = &p[i];
				p[i] = '\0';
				return p;
			}
		}
	}
	if (s1)
		return s1;
	return NULL;
}

#ifdef __cplusplus
};
#endif
