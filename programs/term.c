#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <math.h>
#include <poll.h>
#include <stb_truetype.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>

static const char scancode_ascii[] = {
	0,	 0,	  '1',	'2',  '3',	'4', '5', '6', '7', '8', '9', '0',
	'-', '=', '\b', '\t', 'q',	'w', 'e', 'r', 't', 'y', 'u', 'i',
	'o', 'p', '[',	']',  '\n', 0,	 'a', 's', 'd', 'f', 'g', 'h',
	'j', 'k', 'l',	';',  '\'', '`', 0,	  '|', 'z', 'x', 'c', 'v',
	'b', 'n', 'm',	',',  '.',	'/', 0,	  0,   0,	' '};

static const char scancode_ascii_upper[] = {
	0,	 0,	  '!',	'@',  '#',	'$', '%', '^', '&', '*', '(', ')',
	'_', '+', '\b', '\t', 'Q',	'W', 'E', 'R', 'T', 'Y', 'U', 'I',
	'O', 'P', '{',	'}',  '\n', 0,	 'A', 'S', 'D', 'F', 'G', 'H',
	'J', 'K', 'L',	':',  '"',	'~', 0,	  '|', 'Z', 'X', 'C', 'V',
	'B', 'N', 'M',	'<',  '>',	'?', 0,	  0,   0,	' '};

static const char scancode_ascii_caps[] = {
	0,	 0,	  '1',	'2',  '3',	'4', '5', '6', '7', '8', '9', '0',
	'-', '=', '\b', '\t', 'Q',	'W', 'E', 'R', 'T', 'Y', 'U', 'I',
	'O', 'P', '[',	']',  '\n', 0,	 'A', 'S', 'D', 'F', 'G', 'H',
	'J', 'K', 'L',	';',  '"',	'`', 0,	  '|', 'Z', 'X', 'C', 'V',
	'B', 'N', 'M',	',',  '.',	'/', 0,	  0,   0,	' '};

#define FRAMEBUFFER_SIZE 1280 * 800 * 4

uint32_t get_pxl(uint32_t *framebuffer, uint32_t x, uint32_t y) {
	if ((x >= 1280) || (y >= 800))
		return 0;
	return framebuffer[1280 * y + x];
}

void draw_pxl(uint32_t *framebuffer, uint32_t x, uint32_t y, uint8_t r,
			  uint8_t g, uint8_t b) {
	// printf("%d %d\n", x,y);
	if ((x >= 1280) || (y >= 800))
		return;
	framebuffer[1280 * y + x] = ((r << 16) | (g << 8) | b);
}

void draw_pxl_immediate(int fd, uint32_t x, uint32_t y, uint8_t r, uint8_t g,
						uint8_t b) {
	// printf("%d %d\n", x,y);
	if ((x >= 1280) || (y >= 800))
		return;
	lseek(fd, (1280 * y + x) * 4, SEEK_SET);
	uint32_t c = ((r << 16) | (g << 8) | b);
	write(fd, &c, 4);
}

double max(double x, double y) { return (x > y) ? x : y; }

int startanim(int fd, uint32_t *framebuffer) {
	if (fd < 0) {
		printf("%d\n", errno);
		return 1;
	}
	memset((void *)framebuffer, 0, FRAMEBUFFER_SIZE);
	write(fd, framebuffer, FRAMEBUFFER_SIZE);

	float rot = ((M_PI * 2) / 360.0) * 120.0;
	for (double t = 0; t < (2.0 * M_PI); t += (1.0 / 800.0)) {
		double rot_sin, rot_cos, t_sin, t_cos;
		sincos(t, &t_sin, &t_cos);
		double x = (t_sin + (2 * sin(2 * t)));
		double y = (t_cos - (2 * cos(2 * t)));
		double z = -((sin(3 * t) + 1) / 2);

		sincos(rot, &rot_sin, &rot_cos);

		rot_sin /= 2.0;
		rot_cos /= 2.0;
		x *= (rot_sin + rot_cos);
		x = (x * 120) + 640;
		y = (y * 120) + 400;
		double color = 255.0 - (-z * 255.0);
		draw_pxl_immediate(fd, x + (z * 120.0), y, color, color, color);
		usleep(198);
	}

	double cbase = 255.0;
	while (cbase > 0) {
		memset((void *)framebuffer, 0, FRAMEBUFFER_SIZE);
		for (double t = 0; t < (2.0 * M_PI); t += (1.0 / 800.0)) {
			double rot_sin, rot_cos, t_sin, t_cos;
			sincos(t, &t_sin, &t_cos);
			double x = (t_sin + (2 * sin(2 * t)));
			double y = (t_cos - (2 * cos(2 * t)));
			double z = -((sin(3 * t) + 1) / 2);

			sincos(rot, &rot_sin, &rot_cos);

			rot_sin /= 2.0;
			rot_cos /= 2.0;
			x *= (rot_sin + rot_cos);

			x = (x * 120) + 640;
			y = (y * 120) + 400;
			double color = cbase - (-z * 255.0);
			color = (color > 0) ? color : 0;
			draw_pxl(framebuffer, x + (z * 120.0), y, color, color, color);
		}
		lseek(fd, 0, SEEK_SET);
		write(fd, framebuffer, FRAMEBUFFER_SIZE);
		rot += 0.01;
		cbase -= 8.0;
		usleep(10000);
	}

	return 0;
}

struct TextRendererInfo {
	uint32_t *framebuffer;
	uint32_t *xtra_framebuffer;
	float scale;
	int ascent, descent, lineGap;
	struct stbtt_fontinfo fontinfo;
};

int x_off = 0;
char prev_char = 0;

void draw_line(struct TextRendererInfo tr, int y_adv, const char *str,
			   size_t len) {
	int baseline = tr.ascent * tr.scale;
	while (len--) {
		int width, height;
		int x0, y0, x1, y1;
		stbtt_GetCodepointBox(&tr.fontinfo, *str, &x0, &y0, &x1, &y1);
		int advanceWidth, lsBearing;
		stbtt_GetCodepointHMetrics(&tr.fontinfo, *str, &advanceWidth,
								   &lsBearing);
		unsigned char *bitmap =
			stbtt_GetCodepointBitmap(&tr.fontinfo, tr.scale, tr.scale, *str,
									 &width, &height, NULL, NULL);

		int y_off = baseline + ((int)(-y1 * tr.scale));
		y_off += y_adv;
		// printf("%c y0=%d y1=%d\n", *str, (int)(y0*tr.scale),
		// (int)(y1*tr.scale));
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				uint8_t color = bitmap[width * y + x];
				if (color > 0)
					draw_pxl(tr.framebuffer, x + x_off, y + y_off, color, color,
							 color);
			}
		}

		int advanceXtra = 0;
		if (prev_char) {
			advanceXtra = stbtt_GetCodepointKernAdvance(&tr.fontinfo, prev_char, *str);
		}

		x_off += (advanceWidth+advanceXtra) * tr.scale;
		stbtt_FreeBitmap(bitmap, tr.fontinfo.userdata);
		prev_char = *str;
		str++;
	}
}

int linebreak_index[31];
int linebreak_index_size = 0;

struct Line {
	char *str;
	size_t str_sz;
};

struct Line lines[512] = {0};
size_t last_line_index = 0;
size_t current_line = 0;

int y_advance = 0;
// FIXME make the spacing between lines completely uniform so we can use memcpy
// to scroll back
void push_char(struct TextRendererInfo renderinfo, int tty_fd, char c) {
	int old_x_off = x_off;
	if (c == '\n') {
		int line_stride =
			(renderinfo.ascent - renderinfo.descent) + renderinfo.lineGap;
		y_advance += line_stride;
		x_off = 0;
		prev_char = 0;
		current_line++;
		// fprintf(stderr, "line stride %d\n",
		// (int)(line_stride*renderinfo.scale));
		//  Scroll the screen
		size_t stride = line_stride * renderinfo.scale;
		if ((y_advance * renderinfo.scale) >
			(800 - (line_stride * renderinfo.scale))) {
			memcpy(renderinfo.xtra_framebuffer,
				   renderinfo.framebuffer + (stride * 1280),
				   31 * stride * 1280 * 4);
			memcpy(renderinfo.framebuffer, renderinfo.xtra_framebuffer,
				   FRAMEBUFFER_SIZE);
		}
		while ((y_advance * renderinfo.scale) >
			   (800 - (line_stride * renderinfo.scale))) {
			y_advance -= stride;
		}
		return;
	}

	if ((1280-25) < x_off) {
		linebreak_index[linebreak_index_size++] = old_x_off;
		prev_char = 0;
		int line_stride =
			(renderinfo.ascent - renderinfo.descent) + renderinfo.lineGap;
		y_advance += line_stride;
		x_off = 0;
	}

	// FIXME use size
	draw_line(renderinfo, y_advance * renderinfo.scale, &c, 1);
}

void feed_lines(struct TextRendererInfo renderinfo, int tty_fd) {
	int sz_to_read = 0;
	ioctl(tty_fd, FIONREAD, (char *)&sz_to_read);
	if (!sz_to_read)
		return;
	char *linebuf = (char *)malloc(sz_to_read);
	read(tty_fd, linebuf, sz_to_read);
	for (int i = 0; i < sz_to_read; i++) {
		push_char(renderinfo, tty_fd, linebuf[i]);
	}
	free(linebuf);
}

int main() {
	uint8_t ctrl_held = 0;
	int kbd_fd = open("/dev/keyboard", O_RDONLY);
	uint8_t use_upper = 0;

	int tty_fd = open("/dev/tty", O_RDONLY);
	int shell_pid = 0;
	spawn(&shell_pid, "shell", NULL);

	int fd = open("/dev/bochs", O_WRONLY);
	size_t input_linesz = 1632;
	size_t input_idx = 0;
	char* input_linebuf = (char*)malloc(input_linesz);

	uint32_t *framebuffer = malloc(FRAMEBUFFER_SIZE);
	uint32_t *xtra_framebuffer = malloc(FRAMEBUFFER_SIZE);
	if (fd < 0) {
		printf("%d\n", errno);
		return 1;
	}
	int ret;
	if((ret = startanim(fd, framebuffer))) {
		free(framebuffer);
		close(fd);
		return ret;
	}

	struct stat statbuf;
	struct TextRendererInfo renderinfo;

	int font_fd = open("ComicMono.ttf", O_RDONLY);
	fstat(font_fd, &statbuf);
	uint8_t *font_buf = malloc(statbuf.st_size);
	read(font_fd, font_buf, statbuf.st_size);
	stbtt_InitFont(&renderinfo.fontinfo, font_buf, 0);
	close(font_fd);

	stbtt_GetFontVMetrics(&renderinfo.fontinfo, &renderinfo.ascent,
						  &renderinfo.descent, &renderinfo.lineGap);

	float scale = stbtt_ScaleForPixelHeight(&renderinfo.fontinfo, 25);

	renderinfo.framebuffer = framebuffer;
	renderinfo.xtra_framebuffer = xtra_framebuffer;
	renderinfo.scale = scale;

	struct pollfd poll_fds[2] = {};
	poll_fds[0].fd = kbd_fd;
	poll_fds[0].events = POLLIN;
	poll_fds[1].fd = tty_fd;
	poll_fds[1].events = POLLIN;

	while (1) {
		if (poll(poll_fds, 2, 0) > 0) {
			if (poll_fds[0].revents & POLLIN) {
				int sz_to_read = 0;
				ioctl(kbd_fd, FIONREAD, (char *)&sz_to_read);
				// BAD
				char buf[sz_to_read];
				size_t size_read = read(kbd_fd, buf, sz_to_read);
				if (((int)size_read) == -1) {
					printf("errno %d\n", errno);
					break;
				}
				for (int i = 0; i < size_read; i += 2) {
					if (buf[i + 1] == 42)
						use_upper = !buf[i];

					if (buf[i + 1] == 29)
						ctrl_held = !buf[i];

					if (buf[i])
						continue;

					if (!scancode_ascii[buf[i + 1]])
						continue;

					if (ctrl_held) {
						switch (scancode_ascii[buf[i + 1]]) {
							case 'c':
								kill(shell_pid, SIGINT);
								input_idx = 0;
								break;
							default:
								break;
						}
						continue;
					}

					const char *scancode_set =
						use_upper ? scancode_ascii_upper : scancode_ascii;

					if (buf[i + 1] >= sizeof(scancode_ascii))
						continue;
					input_linebuf[input_idx++] = scancode_set[buf[i + 1]];
					if (input_linebuf[input_idx-1] == '\b') {
						if (input_idx < 2) {
							input_idx -= 1;
							continue;
						}
						input_idx -= 2;

						int advanceWidth, lsBearing;
						stbtt_GetCodepointHMetrics(&renderinfo.fontinfo, input_linebuf[input_idx], &advanceWidth,
												   &lsBearing);
						int advanceXtra = 0;
						int line_stride =
							(renderinfo.ascent - renderinfo.descent) + renderinfo.lineGap;
						int advance = ((advanceWidth+advanceXtra)*scale);
						if ((x_off-advance) < 0) {
							x_off = linebreak_index[linebreak_index_size-1];
							y_advance -= line_stride;
							linebreak_index_size--;
						}
						x_off -= advance;
						for (int line = 0; line < (int)(line_stride)*scale; line++) {
							memset(&renderinfo.framebuffer[((int)(y_advance*scale)+line)*1280+x_off], 0, advance*sizeof(uint32_t));
						}
						continue;
					}

					if (input_linebuf[input_idx-1] == '\n') {
						write(0, input_linebuf, input_idx);
						input_idx = 0;
					}
					push_char(renderinfo, tty_fd, scancode_set[buf[i + 1]]);
				}
			}

			if (poll_fds[1].revents & POLLIN) {
				feed_lines(renderinfo, tty_fd);
			}
			lseek(fd, 0, SEEK_SET);
			write(fd, framebuffer, FRAMEBUFFER_SIZE);
		}
	}

	free(input_linebuf);
	free(framebuffer);
	free(xtra_framebuffer);
	free(font_buf);
	close(kbd_fd);
	close(tty_fd);
	close(fd);
	return 0;
}
