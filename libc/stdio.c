#include <ctype.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char stdin_buf[1024] = {};
char stdout_buf[1024] = {};

#define STDIO_NOLBF
#ifdef STDIO_NOLBF
FILE _stdin = {.fd = 0,
			   .eof = 0,
			   .buffer = {.buf = stdin_buf, .buf_idx = 0, .buf_sz = 1024},
			   .buffering_mode = _IONBF,
			   .error = 0,
			   .pos = 0};
FILE _stdout = {.fd = 1,
				.eof = 0,
				.buffer = {.buf = stdout_buf, .buf_idx = 0, .buf_sz = 1024},
				.buffering_mode = _IONBF,
				.error = 0,
				.pos = 0};
#else
FILE _stdin = {.fd = 0,
			   .eof = 0,
			   .buffer = {.buf = stdin_buf, .buf_idx = 0, .buf_sz = 1024},
			   .buffering_mode = _IOLBF,
			   .error = 0,
			   .pos = 0};
FILE _stdout = {.fd = 1,
				.eof = 0,
				.buffer = {.buf = stdout_buf, .buf_idx = 0, .buf_sz = 1024},
				.buffering_mode = _IOLBF,
				.error = 0,
				.pos = 0};
#endif
FILE _stderr = {.fd = 2, .eof = 0, .buffering_mode = 0, .error = 0, .pos = 0};

FILE *stdin = &_stdin;
FILE *stdout = &_stdout;
FILE *stderr = &_stderr;

int fileno(FILE* stream) {
	return stream->fd;
}

int setvbuf(FILE *stream, char *buf, int mode, size_t size) {
	stream->buffering_mode = mode;
	stream->buffer.buf = buf;
	stream->buffer.buf_sz = size;
}

size_t ftell(FILE *stream) {
	// FIXME pos is wrong 9 times out of 10...
	return stream->pos;
}

int ferror(FILE *stream) {
	// FIXME
	return 0;
}

int feof(FILE *stream) {
	// FIXME
	return 0;
}

int fflush(FILE *stream) {
	switch (stream->buffering_mode) {
	case _IOLBF:
		write(stream->fd, stream->buffer.buf, stream->buffer.buf_idx);
		stream->buffer.buf_idx = 0;
		break;
	case _IOFBF:
		break;
	}
	return 0;
}

FILE *fopen(const char *filename, const char *mode) {
	int flags = 0;
	while (*mode) {
		switch (*mode) {
			case 'w':
				flags |= O_WRONLY;
				break;
			default:
				break;
		}
		mode++;
	}

	int fd = open(filename, flags);
	if (fd < 0) return NULL;
	FILE *f = (FILE *)malloc(sizeof(FILE));
	memset((char *)f, 0, sizeof(FILE));
	f->fd = fd;
	return f;
}

int fclose(FILE *stream) { return 0; }

int getchar() { return fgetc(stdin); }

#if 0
int putchar(int ch) { 
	fwrite(&ch, 1, 1, stdout); 
	return ch;
}
#endif
int putchar(int ch) { return fwrite(&ch, 1, 1, stdout); }

int fgetc(FILE *stream) {
	unsigned char ch;
	if (!fread(&ch, 1, 1, stream))
		return 0;
	return ch;
}

#if 0
int fputc(int ch, FILE *stream) { 
	fwrite((unsigned char *)&ch, 1, 1, stream);
	return ch; 
}
#endif
int fputc(int ch, FILE *stream) { return fwrite((char *)&ch, 1, 1, stream); }

int ungetc(int ch, FILE *stream) {
	// FIXME
}

int fseek(FILE *stream, long offset, int origin) {
	int pos = lseek(stream->fd, offset, origin);
	stream->pos = pos;
	return (pos > 0) ? 0 : -1;
}

size_t fread(void *buffer, size_t size, size_t count, FILE *stream) {
	int sz = read(stream->fd, buffer, size * count);
	stream->pos += sz;
	return sz;
}

size_t fwrite(const void *buffer, size_t size, size_t count, FILE *stream) {
	switch (stream->buffering_mode) {
	case _IONBF:
		int sz = write(stream->fd, buffer, size * count);
		stream->pos += sz;
		return sz;
	case _IOLBF:
		if (!stream->buffer.buf)
			return 0;
		if (!stream->buffer.buf_sz)
			return 0;
		if (size * count >= (stream->buffer.buf_sz - stream->buffer.buf_idx)) {
			return 0;
		}
		memcpy(&stream->buffer.buf[stream->buffer.buf_idx], buffer,
			   size * count);
		stream->buffer.buf_idx += size * count;
		for (size_t i = 0; i < size; i++) {
			if (((char *)buffer)[i] == '\n') {
				write(stream->fd, stream->buffer.buf, stream->buffer.buf_idx);
				stream->buffer.buf_idx = 0;
			}
		}
		break;
	case _IOFBF:
		break;
	}

	stream->pos += size*count;
	return size * count;
}

char *fgets(char *str, int count, FILE *stream) {
	int c = 0;
	while (1) {
		fread(str++, 1, 1, stream);
		c++;
		if (c >= count)
			break;
		if (*(str - 1) == '\n')
			break;
	}

	str[c] = '\0';
	return str;
}

int puts(const char *str) { return fwrite(str, strlen(str), 1, stdout); }

int fputs(const char *str, FILE *stream) {
	return fwrite(str, strlen(str), 1, stream);
}

typedef void(write_func_t)(void* args, char* b, size_t len);

void funcprintf(write_func_t func, void* write_func_args, const char* format, va_list ap) {
	for (size_t j = 0; j < strlen(format); j++) {
		char c = format[j];
		switch (c) {
		case '%': {
			j++;
			int precision = -1;
			char c2 = format[j];
			if (c2 == '.') {
				char precision_char = format[j + 1];
				if (precision_char == '*')
					precision = va_arg(ap, int);

				precision = precision_char - '0';
				j += 2;
				c2 = format[j];

				// Skip invalid precision
				if (precision > 9)
					break;
			}

			switch (c2) {
			case 'd': {
				unsigned int value = va_arg(ap, unsigned int);
				char buffer[10] = {};
				itoa(value, buffer, 10);
				func(write_func_args, buffer, strlen(buffer));
				//fwrite(buffer, strlen(buffer), 1, stream);
				break;
			}
			case 'x': {
				unsigned int value = va_arg(ap, unsigned int);
				char buffer[8];
				itoa(value, buffer, 16);
				func(write_func_args, buffer, strlen(buffer));
				break;
			}
			case 's': {
				char *value = va_arg(ap, char *);
				if (precision >= 0) {
					func(write_func_args, value, precision);
				} else {
					func(write_func_args, value, strlen(value));
				}
				break;
			}
			case 'c': {
				char value = va_arg(ap, int);
				func(write_func_args, &value, 1);
				break;
			}
			default:
				func(write_func_args, &c, 1);
				func(write_func_args, &c2, 1);
				break;
			}
			break;
		}
		default:
			func(write_func_args, &c, 1);
			break;
		}
	}
}

static void iowrite(void* args, char* buf, size_t sz) {
	fwrite(buf, sz, 1, (FILE*)args);
}

int fprintf(FILE *stream, const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	funcprintf(iowrite, (void*)stream, format, ap);
	va_end(ap);
	return 0;
}

int vfprintf(FILE *stream, const char *format, __builtin_va_list list) {
	funcprintf(iowrite, (void*)stream, format, list);
	return 0;
}

int printf(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	funcprintf(iowrite, (void*)stdout, format, ap);
	va_end(ap);
	return 0;
}

struct bwrite_args {
	char* out;
	size_t idx;
	size_t lim;
};

static void bwrite(void* args, char* buf, size_t sz) {
	struct bwrite_args* bargs = (struct bwrite_args*)args;
	if ((size_t)(bargs->out+bargs->idx+sz) >= bargs->lim) return;
	memcpy(bargs->out+bargs->idx, buf, sz);
	bargs->idx += sz;
}

int vsprintf(char *str, const char *format, __builtin_va_list list) {
	struct bwrite_args bargs; 
	bargs.out = str;
	bargs.idx = 0;
	funcprintf(bwrite, (void*)&bargs, format, list);
	return 0;
}

int sprintf(char *str, const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	struct bwrite_args bargs; 
	bargs.out = str;
	bargs.idx = 0;
	bargs.lim = 0;
	funcprintf(bwrite, (void*)&bargs, format, ap);
	va_end(ap);
	return 0;
}

int vsnprintf(char *str, size_t size, const char *format, __builtin_va_list list) {
	struct bwrite_args bargs; 
	bargs.out = str;
	bargs.idx = 0;
	bargs.lim = size;
	funcprintf(bwrite, (void*)&bargs, format, list);
	return 0;
}

int snprintf(char *str, size_t size, const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	vsnprintf(str, size, format, ap);
	va_end(ap);
	return 0;
}

int sscanf(const char* s, const char* format, ...) {
	va_list ap;
	va_start(ap, format);

	int match = 0;

	while (*format) {
		switch (*format) {
			case '%': {
				format++;
				switch (*format) {
					case 'd': {
						size_t n = 0;
						while (isdigit(*s)) {
							if (!*s) return EOF;
							n *= 10;
							n += *s - '0';
							s++;
						}
						int* p = va_arg(ap, int*);
						*p = n;
						match++;
					} break;
				}
			} break;
			default:
				s++;
				if (!*s) return EOF;
				break;
		}
		format++;
	}
	va_end(ap);
	return match;
}
