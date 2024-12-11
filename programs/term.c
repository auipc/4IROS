#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

static const uint16_t xres = 1280;
static const uint16_t yres = 800;
static const int SPACING = 10;
void render_text(uint32_t* display_buffer, int bochs_fd, char* text, size_t text_sz, stbtt_fontinfo* font) {
	int i,j,ascent, descent,baseline, lineGap;
	float scale, xpos=2; // leave a little padding in case the character extends left
			
	int yline = 0;
	int f_x0, f_y0, f_x1, f_y1;
	stbtt_GetFontBoundingBox(font, &f_x0, &f_y0, &f_x1, &f_y1);
	scale = stbtt_ScaleForPixelHeight(font, 50);
	stbtt_GetFontVMetrics(font, &ascent,&descent,lineGap);
	baseline = (int) (f_y1*scale);
	for (int x = 0; x < xres; x++) {
		display_buffer[((int)baseline*xres)+x+(int)xpos] = (1<<16)-1;
	}

	for (int ch = 0; ch < text_sz; ch++) {
		if (text[ch] == '\n') {
			xpos = 0;
			yline += (ascent*scale)-(descent*scale)+(lineGap*scale);
			continue;
		}

		int x0, y0, x1, y1;
		int advance,lsb,height,width,xoff,yoff;
		float x_shift = xpos - (float) floor(xpos);
		stbtt_GetCodepointHMetrics(font, text[ch], &advance, &lsb);
		stbtt_GetCodepointBox(font, text[ch], &x0, &y0, &x1, &y1);
		unsigned char* char_buf = stbtt_GetCodepointBitmap(font, scale, scale, text[ch], &width, &height, &xoff, &yoff);
		// note that this stomps the old data, so where character boxes overlap (e.g. 'lj') it's wrong
		// because this API is really for baking character bitmaps into textures. if you want to render
		// a sequence of characters, you really need to render each bitmap to a temp buffer, then
		// "alpha blend" that into the working buffer

		for (int x = 0; x < width; x++) {
			for (int y = 0; y < height; y++) {
				if (char_buf[y*width+x]) {
					int ypos = (yline+y+baseline-height)-(y0*scale);
					display_buffer[(ypos*xres)+x+(int)xpos] = (1<<24)-1;
				}
			}
		}


		xpos += (advance * scale);
		if (text[ch+1])
			xpos += scale*stbtt_GetCodepointKernAdvance(font, text[ch],text[ch+1]);

		STBTT_free(char_buf, font->userdata);
	}

	lseek(bochs_fd, 0, SEEK_SET);
	write(bochs_fd, display_buffer, xres*yres*4);
}

asm("lolfdsjkldf:");
asm("movq $0xfffffffffffff00b, %rax");
asm("movq $0xfffffffffffff001,  %rbx");
asm("movq $0xfffffffffffff002,  %rcx");
asm("movq $0xfffffffffffff003,  %rdx");
asm("movq $0xfffffffffffff004,  %rdi");
asm("movq $0xfffffffffffff005,  %rsi");
asm("jmp lolfdsjkldf");

extern void lolfdsjkldf();

int main() {
	char* text = (char*)malloc(0x1000);
	size_t text_sz = 0;
	uint32_t* display_buffer = (uint32_t*)malloc(sizeof(uint32_t)*xres*yres);

	int bochs_fd = open("/dev/bochs", 0);
	if (bochs_fd < 0)
		return 1;

	int font_fd = open("jetbrains.ttf", 0);
	size_t font_sz = lseek(font_fd, 0, SEEK_END);
	lseek(font_fd, 0, SEEK_SET);
	uint8_t* ttf_buffer = malloc(font_sz);
	//printf("%x\n", ttf_buffer);
	read(font_fd, ttf_buffer, font_sz);
	stbtt_fontinfo font;
	if(!stbtt_InitFont(&font, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer,0))) {
		printf("Failure\n");
		return 1;
	}

	int shell_pid = 0;
	// FIXME this fork obliterates the CoW table for the subsequent fork, somehow.
	if (!(shell_pid = fork())) {
		execvp("shell", NULL);
		//execvp("farbview", NULL);
		while (1)
			;
	}

	while (1) {
		char c = 0;
		size_t size_read = read(1, &c, 1);
		text[text_sz++] = c;
		render_text(display_buffer, bochs_fd, text, text_sz, &font);
	}

	return 0;
}
