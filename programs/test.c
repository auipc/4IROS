#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#define INT_MIN (-((1 << 32) - 1))
#define SHRT_MAX (((1 << 16) - 1))
#define SHRT_MIN (-((1 << 16) - 1))

static uint16_t xres = 1280;
static uint16_t yres = 800;
uint32_t *display_buffer;

// #define FAST
int main(int argc, const char **argv) {
	int fd = open("/dev/bochs", O_WRONLY);
	if (fd < 0)
		return 1;
	display_buffer = (uint32_t *)malloc(sizeof(uint32_t) * xres * yres);

	while (1) {
		for (int j = 0; j < 800; j++) {
			for (int i = 0; i < 1280; i++) {
				double x = (i/(1280.0/4.0))-3.0;
				double y = (j/(800.0/4.0))-2.0;
				long double zr = 0,zc = 0;
				int k = 0;
#define ITERS 32
#define PALETTE_COLORS 1
				for (k = 0; k < ITERS; k++) {
					if (fabsl(zr) > 20000000.0) break;
					//if (fabs(zc-old_zc) > pow(10,16)) break;
					double ha = zr;
					double haa = zc;
					zr = ((zr*zr)-(haa*haa)) + x;
					zc = ((ha*zc)+(zc*ha)) + y;
				}

				display_buffer[j*1280+i] = k*(((1<<24)-1)/128);
			}
		}
		lseek(fd, 0, SEEK_SET);
		write(fd, display_buffer, sizeof(uint32_t) * xres * yres);
	}

	free(display_buffer);
	return 0;
}
