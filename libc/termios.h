#ifndef _LIBC_TERMIOS_H
#define _LIBC_TERMIOS_H
#include <stdint.h>

struct winsize {
	unsigned short ws_row;
	unsigned short ws_col;
	unsigned short ws_xpixel;
	unsigned short ws_ypixel;
};

// FIXME these are bogus values
#define ECHO (0)
#define ECHONL (1)
#define ICRNL (2)
#define INLCR (3)
#define IGNCR (4)
#define CSIZE (5)
#define ISTRIP (6)
#define CS8 (7)
#define ICANON (8)
#define IXON (9)
#define ONLCR (10)
#define VMIN (11)
#define VTIME (12)
#define VERASE (13)
#define ISIG (14)
#define IEXTEN (15)
#define BRKINT (16)
#define PARMRK (17)
#define NOFLSH (18)
#define TCSETAW (19)
#define NCCS 255

#define TCSANOW 0

struct termios {
	int c_iflag;
	int c_oflag;
	int c_cflag;
	int c_lflag;
	char c_cc[NCCS];
};

#define B0 0
#define B50 0
#define B75 0
#define B110 0
#define B134 0
#define B150 0
#define B200 0
#define B300 0
#define B600 0
#define B1200 0
#define B1800 0
#define B2400 0
#define B4800 0
#define B9600 0

#define CBAUD 0

int tcsetattr(int fildes, int optional_actions,
			  const struct termios *termios_p);

int tcgetattr(int fildes, struct termios *termios_p);

#endif
