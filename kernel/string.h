#pragma once
#include <kernel/stdint.h>

void itoa(char *buf, unsigned long int n, int base);
void ftoa(char *buf, double f);
void *memcpy(void *dest, const void *src, size_t size);
void memset(char *buffer, char value, size_t size);
size_t strlen(const char *str);
