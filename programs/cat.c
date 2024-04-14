#include <stdio.h>
#include <unistd.h>

int main() {
	char buffer[0x1000] = {};
	int fd = open("lol.c", 0);
	int sz = read(fd, buffer, 0x1000);
	printf("%.*s", sz, buffer);

	return 0;
}
