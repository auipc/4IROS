#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

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
	int serial_fd = open("/dev/serial", 0);
	int fd = open("/dev/tty", 0);
	while (1) {
		char c;
		size_t size_read = read(fd, &c, 1);
		write(serial_fd, &c, 1);
	}
}
#endif

int main() {
	int pid = 0;
	if (!(pid = fork())) {
		execvp("term", NULL);
		while(1);
	}

#if 0
	if (!fork()) {
		const char* argv[] = {"linuxxdoom", 0};
		execvp("linuxxdoom", argv);
		while(1);
	}
#endif

#ifdef HERETIC
	if (!fork()) {
		heretic();
	}
#endif

	char *line_buffer = malloc(4096);
	char *line_buffer_ptr = line_buffer;
	int fd = open("/dev/serial", 0);
	while (1) {
		unsigned char c;
		size_t size_read = read(fd, &c, 1);
		if ((line_buffer_ptr - line_buffer) > 4096)
			continue;
		*line_buffer_ptr = c;
		printf("%c", c);
		fflush(stdout);
		if (*line_buffer_ptr == '\r')
			*line_buffer_ptr = '\n';

		if (*line_buffer_ptr == 127) {
			char bksp = '\b';
			write(fd, &bksp, 1);
			if (line_buffer_ptr > line_buffer)
				line_buffer_ptr--;
			*line_buffer_ptr = '\0';
			continue;
		}

		if (*line_buffer_ptr == '\n') {
			fwrite(line_buffer, 1, (line_buffer_ptr+1) - line_buffer, stdin);
			fflush(stdin);
			line_buffer_ptr = line_buffer;
		} else
			line_buffer_ptr++;
	}

#if 0
	// waitpid(pid, NULL, 0);
	int fd = open("/dev/keyboard", 0);
	// printf("%d\n", errno);
	if (fd < 0)
		return 1;
	uint8_t use_upper = 0;

	char* line_buffer = malloc(4096);
	char* line_buffer_ptr = line_buffer;

	while (1) {
		char buf[2];
		size_t size_read = read(fd, buf, 2);
		if (((int)size_read) == -1) {
			printf("errno %d\n", errno);
			break;
		}
		for (int i = 0; i < size_read; i += 2) {
			if (buf[i + 1] == 42)
				use_upper = !buf[i];

			if (buf[i]) continue;
			if (!scancode_ascii[buf[i + 1]])
				continue;

			if (buf[i + 1] >= sizeof(scancode_ascii)) continue;

			if ((line_buffer_ptr-line_buffer) > 4096) continue;
			if (!use_upper) {
				printf("%c", scancode_ascii[buf[i + 1]]);
				*line_buffer_ptr = scancode_ascii[buf[i + 1]];
			} else {
				printf("%c", scancode_ascii_upper[buf[i + 1]]);
				*line_buffer_ptr = scancode_ascii_upper[buf[i + 1]];
			}

			if (*line_buffer_ptr == '\b') {
				if (line_buffer_ptr > line_buffer)
					line_buffer_ptr--;
				*line_buffer_ptr = '\0';
				continue;
			}

			if (*line_buffer_ptr == '\n') {
				fwrite(line_buffer, 1, (line_buffer_ptr-line_buffer)+1, stdin);
				line_buffer_ptr = line_buffer;
			} else line_buffer_ptr++;
#if 0
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
#endif
		}
	}
#endif
	while (1)
		;
}
