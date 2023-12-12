#ifndef _LIBC_PRIV_UTIL_H
#define _LIBC_PRIV_UTIL_H

#define SYSCALL(op, r) asm volatile("int $0x80": "=a"(r) : "a"(op));

#endif
