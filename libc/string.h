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
size_t strnlen(const char *str, size_t n);
char *strcpy(char *destination, const char *source);
char *strncpy(char *destination, const char *source, size_t n);
int strncmp(const char *s1, const char *s2, size_t n);
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);
int strcmp(const char *s1, const char *s2);
int memcmp(const void *s1, const void *s2, size_t n);
void *memchr(const void *s, int c, size_t n);
void *memrchr(const void *s, int c, size_t n);
void *memmove(void *s1, const void *s2, size_t n);
void *memcpy(void *dest, const void *src, size_t size);
void* memset(char *buffer, char value, size_t size);
int atoi(const char *str);
void ulltoa(uint64_t n, char *buf, int base);
void itoa(int64_t n, char *buf, int base);
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

template <typename T> void* memset(T *buffer, char value, size_t size) {
	return memset((char *)buffer, value, size);
}
#endif
#endif
#endif
