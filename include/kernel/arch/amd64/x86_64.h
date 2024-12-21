#pragma once
#include <kernel/arch/amd64/kernel.h>
#include <stdint.h>

struct KSyscallData {
	uintptr_t stack;
	uintptr_t user_stack;
	uint8_t **fxsave_region;
} PACKED;

enum EFlags {
	// Merge this with all values?
	AlwaysSet = 1 << 1,
	InterruptEnable = 1 << 9,
};

struct InterruptReg {
	// Us
	uint64_t rax, rbx, rcx, rdx, rsi, rdi, r15, r14, r13, r12, r11, r10, r9, r8,
		rbp /*, rsp*/;
	// Sys
	uint64_t rip, cs, eflags, rsp, ss;
} PACKED;

struct SyscallReg {
	uint64_t rax, rbx, rcx, rdx, rsi, rdi, r15, r14, r13, r12, r11, r10, r9, r8,
		rbp /*, rsp*/;
} PACKED;

struct ExceptReg {
	// Us
	uint64_t rax, rbx, rcx, rdx, rsi, rdi, r15, r14, r13, r12, r11, r10, r9, r8,
		rbp /*, rsp*/;
	// Sys
	uint64_t error, rip, cs, eflags;
} PACKED;

inline uint64_t get_cr2() {
	uint64_t cr2;
	asm volatile("movq %%cr2, %0" : "=a"(cr2) :);
	return cr2;
}

inline bool interrupts_enabled() {
	uint64_t eflags = 0;
	asm volatile("pushf\n"
				 "pop %0"
				 : "=a"(eflags));
	return eflags & (1 << 9);
}

enum PAEPageFlags : uint64_t {
	Present = 1,
	Write = 1 << 1,
	User = 1 << 2,
	PWT = 1 << 3,
	CacheDisable = 1 << 4,
	Accessed = 1 << 5,
	Dirty = 1ull << 6ull,
	Global = 1ull << 8ull,
	ExecuteDisable = 1ull << 63ull,
};

struct TSS {
	uint32_t reserved;
	uint64_t rsp[3];
	uint32_t reserved_1;
	uint32_t reserved_2;
	uint32_t ist[14];
	uint32_t reserved_3;
	uint32_t reserved_4;
	uint32_t iomap;
} PACKED;

inline void write_msr(uint32_t msr, uint32_t low, uint32_t high) {
	asm volatile("wrmsr" ::"c"(msr), "a"(low), "d"(high));
}

static_assert(sizeof(TSS) == 104);
