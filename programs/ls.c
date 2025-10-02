#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
	char *path = ".";
	if (argc > 1) {
		path = argv[1];
	}

	DIR *d = opendir(path);
	struct dirent *dir;
	while ((dir = readdir(d))) {
		printf("%s ", dir->d_name);
	}

	printf("\n");
	return 0;
}
