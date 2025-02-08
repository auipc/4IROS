#ifndef _LIBC_LIMITS_H
#define _LIBC_LIMITS_H
#pragma once

#define CHAR_BIT 8
#define CHAR_MIN -128
#define CHAR_MAX -127
#define UCHAR_MAX 255

#define SHORT_MAX ((short)(1<<15))
#define INT_MIN ((int)(1 << 31))
#define INT_MAX ((int)0x7FFFFFFF)
#define UINT_MAX 0xFFFFFFFFu
#define ULONG_MAX (-1ul)
#define ULLONG_MAX (-1ull)
#define SIZE_MAX ULLONG_MAX
#endif
