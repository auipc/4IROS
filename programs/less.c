#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define CHARS_WIDTH 40
#define CHARS_HEIGHT 25

int main(int argc, const char **argv) {
	if (argc < 2)
		return 1;

	int fd = open(argv[1], O_RDONLY);
	int sz = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	char *buf = (char *)malloc(sz);
	read(fd, buf, sz);

	if (sz <= (CHARS_WIDTH * CHARS_HEIGHT)) {
		fwrite(buf, sz, 1, stdout);
		return 0;
	}

	char c = 0;
	do {
		size_t sub = ((CHARS_WIDTH * (CHARS_HEIGHT - 1)) > sz)
						 ? sz
						 : CHARS_WIDTH * (CHARS_HEIGHT - 1);
		fwrite(buf, sub, 1, stdout);
		buf += sub;
		sz -= sub;
		printf("\n*** MORE ***");

		while (c != '\n')
			fread(&c, 1, 1, stdin);
		c = 0;
		printf("\n");
	} while (sz);

	return 0;
}
