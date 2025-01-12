#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#include <stdbool.h>

static const uint16_t xres = 1280;
static const uint16_t yres = 800;
static const int SPACING = 10;
int text_begin = 0;

typedef struct _Line {
	int bpsp_line_y;
	int rend_begin_y;
	int rend_extent_y;
	size_t text_buf_begin;
	size_t text_buf_end;
} Line;

struct {
	unsigned char *ptr;
	int width;
	int height;
	int xoff;
	int yoff;
} cache[256] = {};
int yline = 0;
float xpos = 2; // leave a little padding in case the character extends left

int min(int x, int y) { return (x < y) ? x : y; }

#if 0
int read_uncode_char(const char* buffer, size_t at, size_t sz) {
	if (at >= sz) return 0;

	if (!(buffer[0] & (1<<7))) return buffer[0]&CHAR_MAX;
	if ((at+1) >= sz) return 0;
	return buffer[0] | (buffer[1]<<)
}
#endif
bool is_multibyte(char c) {
	if (!(c & (1 << 7)))
		return false;
	return true;
}

void render_char(Line *current_line, uint32_t *display_buffer, int bochs_fd,
				 char text, stbtt_fontinfo *font, bool no_render) {
	int ascent, descent, baseline, lineGap;
	float scale;

	int f_x0, f_y0, f_x1, f_y1;
	stbtt_GetFontBoundingBox(font, &f_x0, &f_y0, &f_x1, &f_y1);
	scale = stbtt_ScaleForPixelHeight(font, 24);
	stbtt_GetFontVMetrics(font, &ascent, &descent, &lineGap);
	baseline = (int)(f_y1 * scale);

	if (text == '\n') {
		xpos = 0;
		yline += (ascent * scale) - (descent * scale) + (lineGap * scale);
		if (yline >= yres) {
			yline -= (ascent * scale) - (descent * scale) + (lineGap * scale);
			// memset(display_buffer, 0, (((ascent * scale) - (descent * scale)
			// + (lineGap * scale))*xres)*4);
		}
		return;
	}

	int x0, y0, x1, y1;
	int advance, lsb, height, width, xoff, yoff;
	float x_shift = xpos - (float)floor(xpos);
	stbtt_GetCodepointHMetrics(font, text, &advance, &lsb);
	stbtt_GetCodepointBox(font, text, &x0, &y0, &x1, &y1);

	if ((xpos + ((x1 - x0) * scale)) > xres) {
		xpos = 0;
		yline += (ascent * scale) - (descent * scale) + (lineGap * scale);
	}

	unsigned char *char_buf = cache[text].ptr;

	if (!char_buf) {
		cache[text].ptr = stbtt_GetCodepointBitmap(
			font, scale, scale, text, &cache[text].width, &cache[text].height,
			&cache[text].xoff, &cache[text].yoff);
		char_buf = cache[text].ptr;
	}
	width = cache[text].width;
	height = cache[text].height;
	xoff = cache[text].xoff;
	yoff = cache[text].yoff;

	current_line->bpsp_line_y = yline;
	if (!current_line->rend_begin_y) {
		current_line->rend_begin_y = (yline + baseline - height) - (y0 * scale);
	} else
		current_line->rend_begin_y =
			min(current_line->rend_begin_y,
				(yline + baseline - height) - (y0 * scale));

	current_line->rend_extent_y = (current_line->rend_extent_y > height)
									  ? current_line->rend_extent_y
									  : height;

	// note that this stomps the old data, so where character boxes overlap
	// (e.g. 'lj') it's wrong because this API is really for baking
	// character bitmaps into textures. if you want to render a sequence of
	// characters, you really need to render each bitmap to a temp buffer,
	// then "alpha blend" that into the working buffer

	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			if (char_buf[y * width + x]) {
				int ypos = (yline + y + baseline - height) - (y0 * scale);
				if (((ypos * xres) + x + (int)xpos) >= (xres * yres) ||
					((ypos * xres) + x + (int)xpos) < 0) {
					continue;
				}
				uint32_t r = char_buf[y * width + x] << 16;
				uint32_t g = char_buf[y * width + x] << 8;
				uint32_t b = char_buf[y * width + x];
				display_buffer[(ypos * xres) + x + (int)xpos] = r | g | b;
			}
		}
	}

	xpos += (advance * scale);

	if (!no_render) {
		lseek(bochs_fd, 0, SEEK_SET);
		write(bochs_fd, display_buffer, xres * yres * 4);
	}
#if 0
	for (int ch = text_begin; ch < text_sz; ch++) {
		if (text[ch] == '\n') {
			if (!first_newline) first_newline = ch;
			xpos = 0;
			yline += (ascent * scale) - (descent * scale) + (lineGap * scale);
			if (yline >= yres) {
				yline -= (ascent * scale) - (descent * scale) + (lineGap * scale);
				text_begin = first_newline+1;
				//memset(display_buffer, 0, (((ascent * scale) - (descent * scale) + (lineGap * scale))*xres)*4);
			}
			continue;
		}

		int x0, y0, x1, y1;
		int advance, lsb, height, width, xoff, yoff;
		float x_shift = xpos - (float)floor(xpos);
		stbtt_GetCodepointHMetrics(font, text[ch], &advance, &lsb);
		stbtt_GetCodepointBox(font, text[ch], &x0, &y0, &x1, &y1);

		if ((xpos+((x1-x0)*scale)) > xres) {
			xpos = 0;
			yline += (ascent * scale) - (descent * scale) + (lineGap * scale);
		}

		unsigned char *char_buf = cache[text[ch]].ptr;

		if (!char_buf) {
			cache[text[ch]].ptr = stbtt_GetCodepointBitmap(
				font, scale, scale, text[ch], &cache[text[ch]].width, &cache[text[ch]].height, &cache[text[ch]].xoff, &cache[text[ch]].yoff);
			char_buf = cache[text[ch]].ptr;
		}
		width = cache[text[ch]].width;
		height = cache[text[ch]].height;
		xoff = cache[text[ch]].xoff;
		yoff = cache[text[ch]].yoff;

		// note that this stomps the old data, so where character boxes overlap
		// (e.g. 'lj') it's wrong because this API is really for baking
		// character bitmaps into textures. if you want to render a sequence of
		// characters, you really need to render each bitmap to a temp buffer,
		// then "alpha blend" that into the working buffer

		for (int x = 0; x < width; x++) {
			for (int y = 0; y < height; y++) {
				if (char_buf[y * width + x]) {
					int ypos = (yline + y + baseline - height) - (y0 * scale);
					if (((ypos * xres) + x + (int)xpos) >= (xres*yres) || ((ypos * xres) + x + (int)xpos) < 0) {
						continue;
					}
					uint32_t r = char_buf[y*width+x]<<16;
					uint32_t g = char_buf[y*width+x]<<8;
					uint32_t b = char_buf[y*width+x];
					display_buffer[(ypos * xres) + x + (int)xpos] = r|g|b;
				}
			}
		}

		xpos += (advance * scale);
		if (text[ch + 1])
			xpos += scale *
					stbtt_GetCodepointKernAdvance(font, text[ch], text[ch + 1]);

		//STBTT_free(char_buf, font->userdata);
	}

	lseek(bochs_fd, 0, SEEK_SET);
	write(bochs_fd, display_buffer, xres * yres * 4);
#endif
}

int main() {
	int line_index = 0;
	size_t text_alloc = 0x1000;
	size_t text_sz = 0;
	char *text = (char *)malloc(text_alloc);
	size_t line_alloc = 100;
	Line *lines = (Line *)malloc(sizeof(Line) * line_alloc);
	memset((char *)lines, 0, sizeof(Line) * line_alloc);
	uint32_t *display_buffer =
		(uint32_t *)malloc(sizeof(uint32_t) * xres * yres);

	int bochs_fd = open("/dev/bochs", 0);
	if (bochs_fd < 0)
		return 1;

	int serial_fd = open("/dev/serial", 0);
	int tty_fd = open("/dev/tty", 0);
	int font_fd = open("alagard.ttf", 0);
	size_t font_sz = lseek(font_fd, 0, SEEK_END);
	lseek(font_fd, 0, SEEK_SET);
	uint8_t *ttf_buffer = malloc(font_sz);
	read(font_fd, ttf_buffer, font_sz);
	stbtt_fontinfo font;
	if (!stbtt_InitFont(&font, ttf_buffer,
						stbtt_GetFontOffsetForIndex(ttf_buffer, 0))) {
		printf("Failure\n");
		return 1;
	}

	stbtt__GetGlyphGSUBInfo(&font, NULL, 0);

	int shell_pid = 0;
	// FIXME this fork obliterates the CoW table for the subsequent fork,
	// somehow.
	if (!(shell_pid = fork())) {
		execvp("shell", NULL);
		while (1)
			;
	}

	int line_offset = 0;
	int yline_fix = 0;

	while (1) {
		if (text_sz >= text_alloc) {
			text_alloc += 0x1000;
			text = realloc(text, text_alloc);
		}
		if ((line_index + 1) > (line_alloc)) {
			line_alloc += 100;
			lines = realloc(lines, sizeof(Line) * line_alloc);
		}
		Line *current_line = &lines[line_index];
		char c = 0;
		size_t size_read = read(tty_fd, &c, 1);

#if 0
		if (c == '\b') {
			text_sz--;
			current_line->text_buf_end--;
			xpos = 0;
			yline = current_line->bpsp_line_y;
			memset((char *)&display_buffer[current_line->rend_begin_y * xres],
				   0,
				   ((current_line->rend_begin_y + current_line->rend_extent_y) *
					xres) *
					   4);
			lseek(bochs_fd, 0, SEEK_SET);
			write(bochs_fd, display_buffer, xres * yres * 4);
			for (size_t i = current_line->text_buf_begin;
				 i < current_line->text_buf_end; i++) {
				render_char(current_line, display_buffer, bochs_fd, text[i],
							&font, false);
			}
			continue;
		} else
#endif
		text[text_sz++] = c;

		current_line->text_buf_end++;
		// FIXME handle text wrap
		if (c == '\n') {
			line_index++;
			current_line = &lines[line_index];
			current_line->text_buf_begin = text_sz;
			current_line->text_buf_end = text_sz;
			if (yline >= (yres - 140)) {
				xpos = 0;
				yline = 0; // lines[line_index-1].rend_begin_y;
				memset(display_buffer, 0, xres * yres * 4);
				line_offset++;
				for (size_t line = line_offset; line < line_index; line++) {
					Line *redraw_line = &lines[line];
					redraw_line->rend_begin_y = 0;
					redraw_line->rend_extent_y = 0;
					for (size_t i = redraw_line->text_buf_begin;
						 i < redraw_line->text_buf_end; i++) {
						render_char(redraw_line, display_buffer, bochs_fd,
									text[i], &font, true);
					}
				}

				lseek(bochs_fd, 0, SEEK_SET);
				write(bochs_fd, display_buffer, xres * yres * 4);
			}
		}

		write(serial_fd, &c, 1);
		render_char(current_line, display_buffer, bochs_fd, c, &font, false);
	}

	return 0;
}
