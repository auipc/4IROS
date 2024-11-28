#pragma once
#include <stddef.h>
#include <stdint.h>

#define KB 1024
#define MB (1024 * KB)

#define VIRTUAL_ADDRESS 0xC0000000
#define PAGE_SIZE 4096L
#define PAGE_ALIGN_MASK ~(PAGE_SIZE - 1)

#define PACKED __attribute__((packed))
#define BIT48_MAX (0xffffffffffff)

struct KernelBootInfo {
	uintptr_t kmap_start;
	uintptr_t kmap_end;
};

struct SymTab {
	uintptr_t addr;
	size_t size;
	const char *func_name;
};

extern uint64_t global_waiter_time;
