#pragma once
#include <kernel/printk.h>

#define assert(x)                                                              \
	if (!(x)) {                                                                \
		printk("Assertion failed: file %s, line %d\n", __FILE__, __LINE__);    \
		while (1) {                                                            \
			asm volatile("cli");                                               \
			asm volatile("hlt");                                               \
		}                                                                      \
	}
