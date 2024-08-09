#pragma once
#include <stdint.h>

enum EFlags {
	// Merge this with all values?
	AlwaysSet = 1 << 1,
	InterruptEnable = 1 << 9,
};

struct InterruptReg {
	// Us
	uint64_t rax, rbx, rcx, rdx, rsi, rdi, r15, r14, r13, r12, r11, r10, r9, r8, rbp/*, rsp*/;
	// Sys
	uint64_t eip, cs, eflags;
};

inline uint64_t get_cr2() {
	uint64_t cr2;
	asm volatile("movq %%cr2, %0" : "=a"(cr2) :);
	return cr2;
}
