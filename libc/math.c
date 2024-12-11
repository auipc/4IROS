#include <math.h>
#include <stdint.h>
#include <stddef.h>

#define E_C 2.718281828459045
#define PI 3.141592653589793

double fabs(double x) { return x > 0.0 ? x : -x; }

double sqrt(double x) {
	double res = 0;
	asm volatile("fldl %1\n"
				 "fsqrt\n"
				 "fstl %0"
				 : "=m"(res)
				 : "m"(x));
	return res;
}

// double round_to_nearest(double value);

double floor(double x) {
	// return round_to_nearest(x);
	return (double)(int64_t)x;
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

	if (x == 2 && fabs(floor(y)) == y && x < ((1ull<<64ull)-1ull)) {
		return 1ull<<(size_t)(floor(y));
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

double cos(double x) { return __builtin_cos(x); }

double sin(double x) { return __builtin_sin(x); }

// Yes, I know this sucks.
double acos(double x) {
	float sum = 0;
	// Integrate until we have an acceptable acos value (lol no)
	// FIXME use the trapezoidal rule for this
	for (float y = x; y < 1.0; y += 0.00001) {
		sum += (1.0 / sqrt(1.0 - (y * y))) * 0.00001;
	}

	return sum;
}

int abs(int x) {
	return x > 0 ? x : -x;
}

double fmod(double x, double y) { return __builtin_fmod(x, y); }

double ceil(double x) {
	double f = fract(x);
	if (!f)
		return x;
	return floor(x) + 1;
}
