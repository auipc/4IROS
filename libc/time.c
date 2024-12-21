#include <time.h>
#include <stddef.h>

time_t time(time_t* t) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec;
}
