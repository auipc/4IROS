#pragma once
#include <kernel/Debug.h>
#include <kernel/printk.h>

#define assert(x)                                                              \
	if (!(x)) {                                                                \
		Debug::stack_trace();                                                  \
		printk("Assertion failed: file %s, line %d\n", __FILE__, __LINE__);    \
		while (1) {                                                            \
			asm volatile("cli");                                               \
			asm volatile("hlt");                                               \
		}                                                                      \
	}
