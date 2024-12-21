#include <gprof.h>

void mcount() {
	uint64_t *rbp = 0;
	if (!rbp)
		asm volatile("mov %%rbp, %0" : "=a"(rbp));

	uint64_t rip = *(rbp + 1);
	if (!rip)
		return;

	printf("RIP: %x\n", rip);
}
