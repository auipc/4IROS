#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#define INT_MIN (-((1 << 32) - 1))
#define SHRT_MAX (((1 << 16) - 1))
#define SHRT_MIN (-((1 << 16) - 1))
#define STBI_NO_HDR
#define STBI_NO_GIF
#define STBI_ASSERT
#define STBI_NO_SIMD
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_NO_THREAD_LOCALS
#include <dirent.h>
#include <stb_image.h>
#include <fcntl.h>

uint16_t endswap16(uint16_t n) {
	uint8_t *b = (uint8_t *)(&n);
	uint8_t t = b[1];
	b[1] = b[0];
	b[0] = t;
	return n;
}

uint32_t endswap32(uint32_t n) {
	uint16_t *b = (uint16_t *)(&n);
	uint16_t t = endswap16(b[1]);
	b[1] = endswap16(b[0]);
	b[0] = t;
	return n;
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

uint32_t *downscale(uint32_t *buf, size_t w, size_t h, size_t dw, size_t dh) {
	if ((w < dw) && (h < dh))
		return buf;
	if ((w == dw) && (h == dh))
		return buf;
	uint32_t *dest_buf = (uint32_t *)malloc(sizeof(uint32_t) * dw * dh);
	float ratio_x = w / dw;
	printf("RATIO %d %d %f\n", w, dw, ratio_x);
	for (size_t y = 0; y < dh; y++) {
		for (size_t x = 0; x < dw; x++) {
			dest_buf[y * dw + x] = buf[y * w + (int)(x * ratio_x)];
		}
	}
	free(buf);
	return dest_buf;
}

int main(int argc, const char **argv) {
	int fd = open("/dev/bochs", O_WRONLY);
	if (fd < 0)
		return 1;

	int kbdfd = open("/dev/keyboard", O_RDONLY);
	uint16_t xres = 1280;
	uint16_t yres = 800;
	uint32_t *display_buffer =
		(uint32_t *)malloc(sizeof(uint32_t) * xres * yres);

	char *path = "hi";
	DIR *d = opendir(path);
	if (!d) {
		printf("Error: %d\n", errno);
		return 1;
	}
	struct dirent *dir;
	while ((dir = readdir(d))) {
		printf("Opening image %s\n", dir->d_name);
		char path[100];
		sprintf(path, "hi/%s", dir->d_name);
		// Read farbfeld image
		int image_fd = open(path, O_RDONLY);
		if (image_fd < 0)
			return 1;

		size_t sz = lseek(image_fd, 0, SEEK_END);
		lseek(image_fd, 0, SEEK_SET);

		uint8_t *buf = (uint8_t *)malloc(sz);
		read(image_fd, buf, sz);
		close(image_fd);

		int height, width, channels;
		char *image_buffer =
			stbi_load_from_memory(buf, sz, &width, &height, &channels, 3);

		int render_height = (height > yres) ? yres : height;
		int render_width = (width > xres) ? xres : width;

		// image_buffer = downscale(image_buffer, width, height, render_width,
		// render_height);
		for (size_t py = 0; py < render_height; py++) {
			for (size_t px = 0; px < render_width; px++) {
				uint8_t r = image_buffer[(py * width + px) * 3];
				uint8_t g = image_buffer[(py * width + px) * 3 + 1];
				uint8_t b = image_buffer[(py * width + px) * 3 + 2];

				uint32_t col =
					((b & 0xff)) | ((g & 0xff) << 8) | ((r & 0xff) << 16);
				display_buffer[py * xres + px] = col;
			}
		}
		free(buf);
		free(image_buffer);
		lseek(fd, 0, SEEK_SET);
		write(fd, display_buffer, sizeof(uint32_t) * xres * yres);
#if 0
		char lol[2];
		do {
			read(kbdfd, lol, 2);
		} while(lol[0]);
#endif
	}
#if 0
	uint64_t magic;
	read(image_fd, &magic, 8);
	// "farbfeld" in hex
	if (magic != 0x646C656662726166) {
		printf("Bad farbfeld header\n");
		return 0;
	}
	uint32_t width = 0;
	uint32_t height = 0;
	read(image_fd, &width, 4);
	read(image_fd, &height, 4);
	width = endswap32(width);
	height = endswap32(height);

	uint16_t *image_buffer =
		(uint16_t *)malloc(sizeof(uint16_t) * width * height * 4);
	read(image_fd, image_buffer, width * height * 4 * sizeof(uint16_t));

	for (size_t py = 0; py < height; py++) {
		for (size_t px = 0; px < width; px++) {
			uint16_t r = image_buffer[(py * width + px) * 4];
			r = endswap16(r);
			uint16_t g = image_buffer[(py * width + px) * 4 + 1];
			g = endswap16(g);
			uint16_t b = image_buffer[(py * width + px) * 4 + 2];
			b = endswap16(b);
			uint16_t a = image_buffer[(py * width + px) * 4 + 3];
			a = endswap16(a);

			uint32_t col =
				((b & 0xff)) | ((g & 0xff) << 8) | ((r & 0xff) << 16);
			buffer[py*1280+px] = col;
		}
	}
	lseek(fd, 0, SEEK_SET);
	write(fd, buffer, sizeof(uint32_t)*xres*yres);

	// auto kbd_size = kbd->size();
	// while (kbd->size() == kbd_size);
	free(image_buffer);
#endif
	return 0;
}
#if 0
int main() { 
	int fd = open("/dev/bochs", 0);
	if (fd < 0) return 1;
	for (size_t y = 0; y < 800; y++) {
		for (size_t x = 0; x < 1280; x++) {
			lseek(fd, (y*1280+x)*4, SEEK_SET);
			size_t xx = x-500;
			size_t yy = y-500;
			uint32_t lol = 0;

			float col_f = 1.0f/(sqrt((xx*xx)+(yy*yy))/(100.0f));
			uint8_t col = 255.0f*(col_f);
			col = (sqrt((xx*xx)+(yy*yy))<100) ? col : 0;
			//printf("%d\n", x-xx);

			lol += col | (col<<8) | (col<<16);

			write(fd, &lol, sizeof(lol));
		}
	}
	return 0; 
}
#endif
