#ifndef _LIBC_STDIO_H
#define _LIBC_STDIO_H
#pragma once

// Sidenote: printf can fail? I'd like to see the C code that has error checking for every print call...
int printf(const char* format, ...);

#endif
