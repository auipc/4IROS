#ifndef _LIBC_STRING_H
#define _LIBC_STRING_H
#pragma once
#include <stdint.h>

void itoa(unsigned long int n, char *buf, int base);
void ftoa(double f, char *buf, int precision);

int strlen(const char* str);
#endif
