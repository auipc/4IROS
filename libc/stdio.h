#ifndef _LIBC_STDIO_H
#define _LIBC_STDIO_H
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

// Sidenote: printf can fail? I'd like to see the C code that has error checking
// for every print call...
int printf(const char *format, ...);
#ifdef __cplusplus
};
#endif
#endif
