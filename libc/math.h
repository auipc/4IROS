#ifndef _LIBC_MATH_H
#define _LIBC_MATH_H
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int abs(int x);
double fabs(double x);
double sqrt(double x);
double floor(double x);
double fract(double x);
double round(double x);
double pow(double x, double y);
double cos(double x);
double sin(double x);
double acos(double x);
double fmod(double x, double y);
double ceil(double x);

#ifdef __cplusplus
};
#endif

#endif
