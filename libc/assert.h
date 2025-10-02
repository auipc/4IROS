#ifndef _LIBC_ASSERT_H
#define _LIBC_ASSERT_H
#pragma once
#include <stdio.h>
#include <unistd.h>

#define assert(x)                                                              \
	{                                                                          \
		if (!(x)) {                                                            \
			printf("Assert failed");                                           \
			exit(1);                                                           \
		}                                                                      \
	}

#endif
