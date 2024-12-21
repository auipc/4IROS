#ifndef _LIBC_LIMITS_H
#define _LIBC_LIMITS_H

#pragma once

#define CHAR_BIT 8
#define CHAR_MIN (-(1 << 7))
#define CHAR_MAX ((1 << 7) - 1)
#define UCHAR_MAX ((1 << 8) - 1)

#define INT_MIN (-(1 << 31))
#define INT_MAX 0x7FFFFFFF
#define UINT_MAX 0xFFFFFFFF
#define ULONG_MAX ((1 << 64) - 1)
#define ULLONG_MAX ((1 << 64) - 1)
#define SIZE_MAX ULLONG_MAX
#endif
