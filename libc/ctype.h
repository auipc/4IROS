#ifndef _LIBC_CTYPE_H
#define _LIBC_CTYPE_H
#pragma once
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

int toupper(int ch);
int tolower(int ch);
int islower(int ch);

#ifdef __cplusplus
};
#endif
#endif
