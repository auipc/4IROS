#pragma once
#include <kernel/arch/i386/kernel.h>
#include <kernel/stdint.h>

// https://wiki.osdev.org/Getting_to_Ring_3#The_TSS
struct TSS {
	uint32_t prev_tss; // The previous TSS - with hardware task switching these
					   // form a kind of backward linked list.
	uint32_t esp0; // The stack pointer to load when changing to kernel mode.
	uint32_t ss0;  // The stack segment to load when changing to kernel mode.
	// Everything below here is unused.
	uint32_t esp1; // esp and ss 1 and 2 would be used when switching to rings 1
				   // or 2.
	uint32_t ss1;
	uint32_t esp2;
	uint32_t ss2;
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t es;
	uint32_t cs;
	uint32_t ss;
	uint32_t ds;
	uint32_t fs;
	uint32_t gs;
	uint32_t ldt;
	uint16_t trap;
	uint16_t iomap_base;
} PACKED;

struct GDTPointer {
	uint16_t limit;
	uint32_t base;
} PACKED;

struct GDTEntry {
	uint16_t limit_low;
	uint32_t base_low : 24;
	struct {
		uint8_t accessed : 1;
		uint8_t rw : 1;
		uint8_t dc : 1;
		uint8_t ex : 1;
		uint8_t type : 1;
		uint8_t priv : 2;
		uint8_t present : 1;
	} PACKED access;
	struct {
		uint8_t limit_high : 4;
		uint8_t zero : 2;
		uint8_t size : 1;
		uint8_t granularity : 1;
	} PACKED flags;
	uint8_t base_high;
} PACKED;

class GDT {
  public:
	static void setup();
};
