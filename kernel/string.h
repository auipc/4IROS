#pragma once
#include <kernel/stdint.h>

int strncmp(const char* s1, const char* s2, size_t n);
int strcmp(const char* s1, const char* s2);
void itoa(char *buf, unsigned long int n, int base);
void ftoa(char *buf, double f, int precision);
void *memcpy(void *dest, const void *src, size_t size);
void memset(char *buffer, char value, size_t size);
size_t strlen(const char *str);
