#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

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

//#define HERETIC
#ifdef HERETIC
void heretic() {
	int serial_fd = open("/dev/serial", O_WRONLY);
	int fd = open("/dev/tty", O_RDONLY);
	while (1) {
		char c;
		size_t size_read = read(fd, &c, 1);
		write(serial_fd, &c, 1);
	}
}
#endif

int main() {
#ifdef HERETIC
	if (!fork()) {
		heretic();
		return 127;
	}
#endif

	if (!fork()) {
#if 0
		char l = '\n';
		write(0, &l, 1);
#endif
		//const char* lol[] = {"doom", "-file", "bw-pal.wad", 0};
		//execvp("DOOM", lol);
		//const char* lol[] = {"test", 0};
		//execvp("test", lol);
		const char* targ[] = {"term", 0};
		execvp("term", targ);
		return 127;
	}

	return 0;
}
