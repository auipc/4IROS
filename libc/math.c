#include <math.h>
#include <stddef.h>
#include <stdint.h>

// FIXME this should just flip the sign bit!!
// there's even a handy instruction fchs
double fabs(double x) { return x > 0.0 ? x : -x; }
long double fabsl(long double x) { return x > 0.0 ? x : -x; }

double sqrt(double x) {
	double res = 0;
	asm volatile("fldl %1\n"
				 "fsqrt\n"
				 "fstl %0"
				 : "=m"(res)
				 : "m"(x));
	return res;
}

float sqrtf(float x) {
	float res = 0;
	asm volatile("fldl %1\n"
				 "fsqrt\n"
				 "fstl %0"
				 : "=m"(res)
				 : "m"(x));
	return res;
}

// double round_to_nearest(double value);
//
float floorf(float x) {
	return floor((double)x);
}

double floor(double x) {
	volatile int64_t y = x;
	return y;
}

double fract(double x) { return x - floor(x); }

double round(double x) {
	if (fract(x) != 0)
		return x + 1.0;
	return x;
}

#define STEPS 9
double pow(double x, double y) {
	double result = 1;

	if (x == 2 && fabs(floor(y)) == y && x < ((1ull << 64ull) - 1ull)) {
		return 1ull << (size_t)(floor(y));
	}

	for (int i = 0; i < round(y); i++) {
		result *= x;
	}

	// FIXME this is wrong but idc
	// Last bit
	if (fract(y)) {
		// double guess = ((sqrt(x)/(x)));
		double val = x;
		double nth = 1.0 / fract(y);
		for (int i = 1; i <= STEPS; i++) {
			val = (i + (val / i)) / nth;
		}

		result *= val;
	}

	return result;
}

double cos(double x) {
	double res = 0;
	asm volatile("fcos":"=t"(res):"0"(x));
	return res;
}

double sin(double x) {
	double res = 0;
	asm volatile("fsin":"=t"(res):"0"(x));
	return res;
}

void sincos(double x, double *s, double *c) {
	asm volatile("fsincos":"=t"(*c), "=u"(*s):"0"(x));
}

// Yes, I know this sucks.
// There's a much faster way to do this, however I like calculus (no)
double acos(double x) {
	float sum = 0;
	for (float y = x; y < 1.0; y += 0.00001) {
		sum += (1.0 / sqrt(1.0 - (y * y))) * 0.00001;
	}

	return sum;
}

int abs(int x) { return x > 0 ? x : -x; }

double fmod(double x, double y) { return __builtin_fmod(x, y); }

float ceilf(float x) {
	return ceil((double)x);
}

double ceil(double x) {
	int64_t y = x;
	if (y == x)
		return x;
	return y + 1;
}
