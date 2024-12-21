#ifndef _LIBC_SYS_TIME_H
#define _LIBC_SYS_TIME_H
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t time_t;
typedef uint64_t suseconds_t;

struct timeval {
	time_t tv_sec;
	suseconds_t tv_usec;
};

int gettimeofday(struct timeval *tv, void *tz);

#ifdef __cplusplus
};
#endif

#endif
