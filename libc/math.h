#ifndef _LIBC_MATH_H
#define _LIBC_MATH_H
#pragma once
#include <stdint.h>

#define M_PI 3.141592653589793

#ifdef __cplusplus
extern "C" {
#endif

int abs(int x);
double fabs(double x);
long double fabsl(long double x);
double sqrt(double x);
float sqrtf(float x);
float floorf(float x);
double floor(double x);
double fract(double x);
double round(double x);
double pow(double x, double y);
void sincos(double x, double *sin, double *cos);
double cos(double x);
double sin(double x);
double acos(double x);
double fmod(double x, double y);
float ceilf(float x);
double ceil(double x);

#ifdef __cplusplus
};
#endif

#endif
