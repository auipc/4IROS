#ifndef _LIBC_STDIO_H
#define _LIBC_STDIO_H
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stddef.h>

#define EOF -1
typedef size_t fpos_t;
typedef struct _FILE {
	int fd;
	struct {
		char* buf;
		size_t buf_idx;
		size_t buf_sz;
	} buffer;
	int buffering_mode;
	uint8_t eof;
	uint8_t error;
	fpos_t pos;
} FILE;

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

// no buffering
#define _IONBF 0
// full buffering
#define _IOFBF 1
// line buffered
#define _IOLBF 2

int setvbuf(FILE* stream, char* buf, int mode, size_t size);
size_t ftell(FILE* stream);
int feof(FILE* stream);
int ferror(FILE* stream);
int fflush(FILE* stream);
int getchar(void);
int putchar(int ch);
int fputc(int ch, FILE* stream);
int ungetc(int ch, FILE* stream);
int fgetc(FILE* stream);
FILE* fopen(const char* filename, const char* mode);
int fclose(FILE* stream);
int fseek(FILE* stream, long offset, int origin);
size_t fread(void* buffer, size_t size, size_t count, FILE* stream);
size_t fwrite(const void* buffer, size_t size, size_t count, FILE* stream);
char* fgets(char* str, int count, FILE* stream);
int sprintf (char * str, const char * format, ... );
int fprintf(FILE* stream, const char *format, ...);
int puts(const char *str);
int fputs(const char *str, FILE* stream);
// Sidenote: printf can fail? I'd like to see the C code that has error checking
// for every print call...
int printf(const char *format, ...);
#ifdef __cplusplus
};
#endif
#endif
