#ifndef _LIBC_TERMIO_H
#define _LIBC_TERMIO_H
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <termios.h>

struct termio {
	int c_iflag;
	int c_oflag;
	int c_cflag;
	int c_lflag;
	char c_cc[NCCS];
};

#ifdef __cplusplus
};
#endif
#endif
