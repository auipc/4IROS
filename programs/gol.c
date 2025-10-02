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

typedef struct {
	uint16_t x : 11;
	uint16_t y : 10;
	bool set : 1;
} Change;

Change *change_list;
size_t change_sz = 0;

int get(int x, int y) {
	if (x < 0)
		return 0;
	if (y < 0)
		return 0;
	if (x >= xres)
		return 0;
	if (y >= yres)
		return 0;

	return display_buffer[y * xres + x] > 0;
}

void set(int x, int y) {
	if (x < 0)
		return;
	if (y < 0)
		return;
	if (x >= xres)
		return;
	if (y >= yres)
		return;

	change_list[change_sz].x = x;
	change_list[change_sz].y = y;
	change_list[change_sz].set = 1;
	change_sz++;
}

void unset(int x, int y) {
	if (x < 0)
		return;
	if (y < 0)
		return;
	if (x >= xres)
		return;
	if (y >= yres)
		return;

	change_list[change_sz].x = x;
	change_list[change_sz].y = y;
	change_list[change_sz].set = 0;
	change_sz++;
}

void apply() {
	for (size_t i = 0; i < change_sz; i++) {
		Change l = change_list[i];
		display_buffer[l.y * xres + l.x] = l.set ? /*0xffffff*/ 0xaaaaaa : 0;
	}
	change_sz = 0;
}

int neighborsg1(int x, int y) {
	return get(x - 1, y) + get(x + 1, y) + get(x, y - 1) + get(x, y + 1);
}

int neighborsg2(int x, int y) {
	return get(x - 1, y - 1) + get(x + 1, y + 1) + get(x - 1, y + 1) +
		   get(x + 1, y - 1);
}

int neighbors(int x, int y) {
	return get(x - 1, y) + get(x + 1, y) + get(x, y - 1) + get(x, y + 1) +
		   get(x - 1, y - 1) + get(x + 1, y + 1) + get(x - 1, y + 1) +
		   get(x + 1, y - 1);
}

uint64_t rdtsc() {
	union {
		struct {
			uint32_t p1;
			uint32_t p2;
		};
		uint64_t counter;
	} tsc = {};
	asm volatile("rdtsc" : "=a"(tsc.p1), "=d"(tsc.p2) :);
	return tsc.counter;
}

const char RULESTRING[] = "B3/S23";

// #define FAST
int main(int argc, const char **argv) {
	int fd = open("/dev/bochs", O_WRONLY);
	if (fd < 0)
		return 1;
	display_buffer = (uint32_t *)malloc(sizeof(uint32_t) * xres * yres);

	change_list = (Change *)malloc(sizeof(Change) * xres * yres);
	memset(change_list, 0, sizeof(Change) * xres * yres);

	int t = 0;
	for (size_t py = 0; py < yres; py++) {
		for (size_t px = 0; px < xres; px++) {
			uint8_t r = (rand() % 4) ? 0 : 255;

			if (r)
				set(px, py);
		}
	}
	lseek(fd, 0, SEEK_SET);
	read(fd, display_buffer, sizeof(uint32_t) * xres * yres);
#if 0
	for (int i = -2; i < 2; i++) {
		set((xres/2)+i, (yres/2));
	}
#endif

	apply();
	lseek(fd, 0, SEEK_SET);
	write(fd, display_buffer, sizeof(uint32_t) * xres * yres);

	uint8_t birth_list[9] = {};
	uint8_t survival_list[9] = {};
	uint8_t birth_sz = 0;
	uint8_t survive_sz = 0;
	bool birth = false, survive = false;
	for (size_t i = 0; i < sizeof(RULESTRING) / sizeof(RULESTRING[0]); i++) {
		switch (RULESTRING[i]) {
		case 'B': {
			survive = false;
			birth = true;
		} break;
		case 'S': {
			birth = false;
			survive = true;
		} break;
		case '0' ... '9':
			if (birth) {
				birth_list[birth_sz] = RULESTRING[i] - '0';
				birth_sz++;
			} else if (survive) {
				survival_list[survive_sz] = RULESTRING[i] - '0';
				survive_sz++;
			}
			break;
		case '/':
		default:
			break;
		}
	}

	while (1) {
		uint64_t begin = rdtsc();
		for (size_t py = 0; py < yres * xres; py++) {
			size_t y = (py) / xres;
			size_t x = (py) % xres;

			if (get(x, y)) {
#ifdef FAST
				int n1 = neighborsg1(x, y);
				if (n1 > 3) {
					unset(x, y);
					continue;
				}
				int n2 = neighborsg2(x, y);
				int n = n1 + n2;
#else
				int n = neighbors(x, y);
#endif
				// FIXME survive string
				if ((n > 3) || (n < 2)) {
					unset(x, y);
				}
			} else {
				int n = neighbors(x, y);
				for (int i = 0; i < birth_sz; i++) {
					if (n == birth_list[i]) {
						set(x, y);
					}
				}
			}
		}
		apply();
		lseek(fd, 0, SEEK_SET);
		write(fd, display_buffer, sizeof(uint32_t) * xres * yres);
		t++;
	}

	free(display_buffer);
	return 0;
}
