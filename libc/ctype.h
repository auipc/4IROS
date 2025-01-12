#ifndef _LIBC_CTYPE_H
#define _LIBC_CTYPE_H
#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

int isalpha(char c);
int isalnum(char c);
int isdigit(char c);
int isupper(char c);
int toupper(int ch);
int tolower(int ch);
int islower(int ch);
int isprint(int ch);
int iscntrl(int ch);
int isspace(int ch);
int ispunct(int ch);
int isblank(int ch);

#ifdef __cplusplus
};
#endif
#endif
