#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char stdin_buf[1024] = {};
char stdout_buf[1024] = {};

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
FILE _stderr = {.fd = 2, .eof = 0, .buffering_mode = 0, .error = 0, .pos = 0};

FILE *stdin = &_stdin;
FILE *stdout = &_stdout;
FILE *stderr = &_stderr;

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
	(void)mode;
	FILE *f = (FILE *)malloc(sizeof(FILE));
	memset((char *)f, 0, sizeof(FILE));
	f->fd = open(filename, 0);
	return f;
}

int fclose(FILE *stream) { return 0; }

int getchar() { return fgetc(stdin); }

int putchar(int ch) { return fwrite(&ch, 1, 1, stdout); }

int fgetc(FILE *stream) {
	char ch;
	if (!fread(&ch, 1, 1, stream))
		return 0;
	return ch;
}

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
	return read(stream->fd, buffer, size * count);
}

size_t fwrite(const void *buffer, size_t size, size_t count, FILE *stream) {
	switch (stream->buffering_mode) {
	case _IONBF:
		return write(stream->fd, buffer, size * count);
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

int fprintf(FILE *stream, const char *format, ...) {
	__builtin_va_list ap;
	__builtin_va_start(ap, format);
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
					precision = __builtin_va_arg(ap, int);

				precision = precision_char - '0';
				j += 2;
				c2 = format[j];

				// Skip invalid precision
				if (precision > 9)
					break;
			}

			switch (c2) {
			case 'd': {
				unsigned int value = __builtin_va_arg(ap, unsigned int);
				char buffer[10] = {};
				itoa(value, buffer, 10);
				fwrite(buffer, strlen(buffer), 1, stream);
				break;
			}
			case 'x': {
				unsigned int value = __builtin_va_arg(ap, unsigned int);
				char buffer[8];
				itoa(value, buffer, 16);
				fwrite(buffer, strlen(buffer), 1, stream);
				break;
			}
			case 's': {
				char *value = __builtin_va_arg(ap, char *);
				if (precision >= 0) {
					write(1, value, precision);
					fwrite(value, precision, 1, stream);
				} else {
					fwrite(value, strlen(value), 1, stream);
				}
				break;
			}
			case 'c': {
				char value = __builtin_va_arg(ap, int);
				fwrite(&value, 1, 1, stream);
				break;
			}
			default:
				fwrite(&c, 1, 1, stream);
				fwrite(&c2, 1, 1, stream);
				break;
			}
			break;
		}
		default:
			fwrite(&c, 1, 1, stream);
			break;
		}
	}
	__builtin_va_end(ap);
	return 0;
}

int printf(const char *format, ...) {
	__builtin_va_list ap;
	__builtin_va_start(ap, format);
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
					precision = __builtin_va_arg(ap, int);

				precision = precision_char - '0';
				j += 2;
				c2 = format[j];

				// Skip invalid precision
				if (precision > 9)
					break;
			}

			switch (c2) {
			case 'd': {
				unsigned int value = __builtin_va_arg(ap, unsigned int);
				char buffer[10] = {};
				itoa(value, buffer, 10);
				write(1, buffer, strlen(buffer));
				break;
			}
			case 'x': {
				unsigned int value = __builtin_va_arg(ap, unsigned int);
				char buffer[8];
				itoa(value, buffer, 16);
				write(1, buffer, strlen(buffer));
				break;
			}
			case 's': {
				char *value = __builtin_va_arg(ap, char *);
				if (precision >= 0) {
					write(1, value, precision);
				} else {
					write(1, value, strlen(value));
				}
				break;
			}
			case 'c': {
				char value = __builtin_va_arg(ap, int);
				write(1, &value, 1);
				break;
			}
			default:
				write(1, &c, 1);
				write(1, &c2, 1);
				break;
			}
			break;
		}
		default:
			write(1, &c, 1);
			break;
		}
	}
	__builtin_va_end(ap);
	return 0;
}

int sprintf(char *str, const char *format, ...) {}
