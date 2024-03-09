#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

int main() {
	printf("Hello, World\n");
	char *addr = (char *)mmap((void *)0x60000, 0x4000);
	addr[0] = 1;
	printf("%x\n", *addr);
	char *addr1 = (char *)mmap((void *)0x50000, 0x2000);
	addr1[0] = 1;
	printf("%x\n", *addr1);

	while (1)
		;
}
