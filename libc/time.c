#include <stddef.h>
#include <time.h>

time_t time(time_t *t) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec;
}
