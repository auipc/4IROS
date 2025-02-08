#ifndef _LIBC_STDDEF_H
#define _LIBC_STDDEF_H

#define NULL 0
typedef __SIZE_TYPE__ size_t;
typedef long long ssize_t;
#define offsetof(t, m) ((size_t)(&((t *)0)->m))

#endif
