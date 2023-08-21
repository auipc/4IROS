#pragma once
#include <kernel/printk.h>

#define assert(x)                                                              \
	if (!(x)) {                                                                \
		printk("Assertion failed: %s, line %d\n", #x, __LINE__);               \
		while (1) {                                                            \
			asm volatile("cli");                                               \
			asm volatile("hlt");                                               \
		}                                                                      \
	}
