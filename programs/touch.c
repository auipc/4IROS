#include <fcntl.h>
#include <unistd.h>

int main(int argc, const char **argv) {
	if (argc < 2)
		return 1;
	if (open(argv[1], O_WRONLY) < 0)
		return 1;
	return 0;
}
