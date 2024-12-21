#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, const char **argv) {
	printf("Test\n");
	printf("Hello\n");
	while (1) {
		printf("Hello\n");
		char s[1];
		fgets(s, 1, stdin);
	}
	return 0;
}
