#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, const char **argv) {
	while (1) {
		printf("Hello\n");
		for (int i = 0; i < 100000; i++)
			asm volatile("nop");
	}
	return 0;
}
