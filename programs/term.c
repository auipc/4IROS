#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const char scancode_ascii[] = {
	0,	 0,	  '1',	'2',  '3',	'4', '5', '6', '7', '8', '9', '0',
	'-', '=', '\b', '\t', 'q',	'w', 'e', 'r', 't', 'y', 'u', 'i',
	'o', 'p', '[',	']',  '\n', 0,	 'a', 's', 'd', 'f', 'g', 'h',
	'j', 'k', 'l',	';',  '"',	'`', 0,	  '|', 'z', 'x', 'c', 'v',
	'b', 'n', 'm',	',',  '.',	'/', 0,	  0,   0,	' '};

static const char scancode_ascii_upper[] = {
	0,	 0,	  '!',	'@',  '#',	'$', '%', '^', '&', '*', '(', ')',
	'_', '+', '\b', '\t', 'Q',	'W', 'E', 'R', 'T', 'Y', 'U', 'I',
	'O', 'P', '{',	'}',  '\n', 0,	 'A', 'S', 'D', 'F', 'G', 'H',
	'J', 'K', 'L',	':',  '"',	'~', 0,	  '|', 'Z', 'X', 'C', 'V',
	'B', 'N', 'M',	'<',  '>',	'?', 0,	  0,   0,	' '};

static const char scancode_ascii_caps[] = {
	0,	 0,	  '!',	'@',  '#',	'$', '%', '^', '&', '*', '(', ')',
	'_', '+', '\b', '\t', 'Q',	'W', 'E', 'R', 'T', 'Y', 'U', 'I',
	'O', 'P', '{',	'}',  '\n', 0,	 'A', 'S', 'D', 'F', 'G', 'H',
	'J', 'K', 'L',	':',  '"',	'~', 0,	  '|', 'Z', 'X', 'C', 'V',
	'B', 'N', 'M',	'<',  '>',	'?', 0,	  0,   0,	' '};

int main() {
	int shell_pid = 0;
	if (!(shell_pid = fork())) {
		exec("shell");
	}

	int fd = open("/dev/keyboard", 0);
	uint8_t use_upper = 0;

	while (1) {
		char buf[2];
		size_t size_read = read(fd, buf, 2);
		for (int i = 0; i < size_read; i += 2) {
			if (buf[i + 1] == 42)
				use_upper = !buf[i];

			if (!buf[i] && sizeof(scancode_ascii) > buf[i + 1]) {
				if (!scancode_ascii[buf[i + 1]])
					continue;

				if (!use_upper) {
					printf("%c", scancode_ascii[buf[i + 1]]);
					write(0, &scancode_ascii[buf[i + 1]], 1);
				} else {
					printf("%c", scancode_ascii_upper[buf[i + 1]]);
					write(0, &scancode_ascii_upper[buf[i + 1]], 1);
				}
			}
		}
	}

	return 0;
}
