#ifndef _LIBC_TIME_H
#define _LIBC_TIME_H
#pragma once
#include <stdint.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif
time_t time(time_t* t);
#ifdef __cplusplus
};
#endif
#endif
