#ifndef _LIBC_DIRENT_H
#define _LIBC_DIRENT_H
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <sys/types.h>
#define PATH_MAX 256

struct dirent;

typedef struct _DIR {
	int fd;
	struct dirent **dirlist;
	int dircount;
} DIR;

struct dirent {
	ino_t d_ino;
	char d_name[PATH_MAX];
};

DIR *opendir(const char *path);
struct dirent *readdir(DIR *);
#ifdef __cplusplus
};
#endif
#endif
