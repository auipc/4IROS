#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv) {
	if (argc < 2) return 1;
	int fd = open(argv[1], 0);
	if (fd < 0) return 1;
	int sz = lseek(fd, 0, SEEK_END);
	if (!sz) return 1;
	lseek(fd, 0, SEEK_SET);
	char* buf = (char*)malloc(sizeof(char)*sz);
	read(fd, buf, sz);
	for (int i = 0; i < sz; i++)
		fwrite(&buf[i], 1, 1, stdout);
	return 0;
}
