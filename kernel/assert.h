#pragma once
#include <kernel/Debug.h>
#include <kernel/printk.h>

#define assert(x)                                                              \
	if (!(x)) {                                                                \
		printk("Assertion failed: file %s, line %d\n", __FILE__, __LINE__);    \
		Debug::stack_trace();                                                  \
		while (1) {                                                            \
			asm volatile("cli");                                               \
			asm volatile("hlt");                                               \
		}                                                                      \
	}

#define assert_eq(lhs, rhs)                                                    \
	if (lhs != rhs) {                                                          \
		printk("Assertion failed: file %s, line %d\n%d == %d\n", __FILE__,     \
			   __LINE__, lhs, rhs);                                            \
		Debug::stack_trace();                                                  \
		while (1) {                                                            \
			asm volatile("cli");                                               \
			asm volatile("hlt");                                               \
		}                                                                      \
	}
