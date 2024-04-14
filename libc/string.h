#ifndef _LIBC_STRING_H
#define _LIBC_STRING_H
#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int strncmp(const char *s1, const char *s2, size_t n);
int strcmp(const char *s1, const char *s2);
void *memcpy(void *dest, const void *src, size_t size);
void memset(char *buffer, char value, size_t size);
void itoa(unsigned int n, char *buf, int base);
void ftoa(double f, char *buf, int precision);

size_t strlen(const char *str);
#ifdef __cplusplus
};
#endif
#endif
