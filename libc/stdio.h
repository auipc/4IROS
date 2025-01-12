#ifndef _LIBC_STDIO_H
#define _LIBC_STDIO_H
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#ifdef __cplusplus
extern "C" {
#endif

#define BUFSIZ 1024
#define EOF -1
typedef size_t fpos_t;
typedef struct _FILE {
	int fd;
	struct {
		char *buf;
		size_t buf_idx;
		size_t buf_sz;
	} buffer;
	int buffering_mode;
	uint8_t eof;
	uint8_t error;
	fpos_t pos;
} FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

// no buffering
#define _IONBF 0
// full buffering
#define _IOFBF 1
// line buffered
#define _IOLBF 2

void clearerr(FILE* stream);
int fileno(FILE* stream);
int setvbuf(FILE *stream, char *buf, int mode, size_t size);
size_t ftell(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
int fflush(FILE *stream);
int getchar(void);
int sscanf(const char* s, const char* format, ...);
int putchar(int ch);
int fputc(int ch, FILE *stream);
int ungetc(int ch, FILE *stream);
int fgetc(FILE *stream);
FILE *fopen(const char *filename, const char *mode);
int fclose(FILE *stream);
int fseek(FILE *stream, long offset, int origin);
size_t fread(void *buffer, size_t size, size_t count, FILE *stream);
size_t fwrite(const void *buffer, size_t size, size_t count, FILE *stream);
char *fgets(char *str, int count, FILE *stream);
int vfprintf(FILE *stream, const char *format, __builtin_va_list list);
int vsprintf(char *str, const char *format, __builtin_va_list list);
int sprintf(char *str, const char *format, ...);
int snprintf(char *str, size_t size, const char *format, ...);
int vsnprintf(char *str, size_t size, const char *format, __builtin_va_list list);
int fprintf(FILE *stream, const char *format, ...);
int puts(const char *str);
int fputs(const char *str, FILE *stream);
// Sidenote: printf can fail? I'd like to see the C code that has error checking
// for every print call...
int printf(const char *format, ...);
#ifdef __cplusplus
};
#endif
#endif
