#include <unistd.h>

int main();

void _start() {
	int r = main();
	exit(r);
}
