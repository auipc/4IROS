#ifndef _LIBC_STRING_H
#define _LIBC_STRING_H
#pragma once

void itoa(char *buf, unsigned long int n, int base);
void ftoa(char *buf, double f, int precision);

int strlen(const char* str);
#endif
