#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

int main() { 
	printf("ls\n");
	uint64_t x = 2;
	uint64_t y = 3;
	while(1) {
		uint64_t lol = x+y;
		x = y;
		y = lol;
	}
	return 0; 
}
