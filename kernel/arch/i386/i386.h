#pragma once
#include <stdint.h>

enum EFlags {
	// Merge this with all values?
	AlwaysSet = 1 << 1,
	InterruptEnable = 1 << 9,
};

inline uint32_t get_cr2() {
	uint32_t cr2;
	asm volatile("mov %%cr2, %0" : "=a"(cr2) :);
	return cr2;
}
