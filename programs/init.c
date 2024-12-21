#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
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
	int pid = 0;
	char *buf = (char *)mmap((void *)0x990000, 4096); //[2]
	if (!(pid = fork())) {
		execvp("farbview", NULL);
		// char* argv[] = {"frotz", "TRINITY.DAT", 0};
		// execvp("frotz", argv);
		while (1)
			;
	}

	// waitpid(pid, NULL, 0);
	int fd = open("/dev/keyboard", 0);
	// printf("%d\n", errno);
	if (fd < 0)
		return 1;
	uint8_t use_upper = 0;

	*buf = 0x39;
	printf("%d\n", buf[0]);
#if 0
	while (1) {
		//char* buf = (char*)mmap(0x900000, 4096); //[2]
		size_t size_read = read(fd, buf, 2);
		printf("size_read %d\n", size_read);
		if (((int)size_read) == -1) {
			printf("errno %d\n", errno);
			break;
		}
		for (int i = 0; i < size_read; i += 2) {
			if (buf[i + 1] == 42)
				use_upper = !buf[i];

			if (!buf[i] && sizeof(scancode_ascii) > buf[i + 1]) {
				if (!scancode_ascii[buf[i + 1]])
					continue;

				if (!use_upper) {
					printf("%c", scancode_ascii[buf[i + 1]]);
					fwrite(&scancode_ascii[buf[i + 1]], 1, 1, stdin);
				} else {
					printf("%c", scancode_ascii_upper[buf[i + 1]]);
					fwrite(&scancode_ascii_upper[buf[i + 1]], 1, 1, stdin);
				}
			}
		}
	}
#endif
	while (1)
		;
}
