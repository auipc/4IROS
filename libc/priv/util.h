#ifndef _LIBC_PRIV_UTIL_H
#define _LIBC_PRIV_UTIL_H
#pragma once

#ifdef __i386__
#define SYSCALL(op, r) asm volatile("int $0x80" : "=a"(r) : "a"(op));
#else
#warning Unsupported architecture
#define SYSCALL(op, r)                                                         \
	while (1)                                                                  \
		;
#endif

#endif
