#ifndef _LIBC_STRING_H
#define _LIBC_STRING_H
#pragma once
#ifndef STUPID_BAD_UGLY_BAD_DESIGN_TURN_OFF_STRING_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

const char *strrchr(const char *str, int character);
char *strdup(const char *s);
char *strerror(int errnum);
const char *strchr(const char *str, int character);
char *strndup(const char *s, size_t n);
char *strncat(char *destination, const char *source, size_t num);
char *strcat(char *destination, const char *source);
char *strcpy(char *destination, const char *source);
char *strncpy(char *destination, const char *source, size_t n);
int strncmp(const char *s1, const char *s2, size_t n);
int strncasecmp(const char *s1, const char *s2, size_t n);
int strcmp(const char *s1, const char *s2);
void *memcpy(void *dest, const void *src, size_t size);
void memset(char *buffer, char value, size_t size);
void itoa(uint64_t n, char *buf, int base);
void ftoa(double f, char *buf, int precision);
char *strtok(char *s1, const char *s2);

size_t strlen(const char *str);

#ifdef __cplusplus
};
#endif

#ifdef __cplusplus
template <typename T> void *memcpy(T *dest, const void *src, size_t size) {
	return memcpy((void *)dest, src, size);
}

template <typename T> void memset(T *buffer, char value, size_t size) {
	return memset((char *)buffer, value, size);
}
#endif
#endif
#endif
