#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
	int fd = open("Makefile", 0);
	if (fd < 0)
		return 1;
	off_t file_sz = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	char *buffer = (char *)malloc(sizeof(char) * file_sz);
	int sz = read(fd, buffer, 0x1000);
	printf("%d", sz);
	if (sz < 0)
		return 1;
	printf("%.*s", sz, buffer);
	return 0;
}
