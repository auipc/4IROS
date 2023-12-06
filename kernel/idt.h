#pragma once
#include <kernel/stdint.h>

struct InterruptRegisters {
	uint32_t ds, es, fs, gs;
	uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
	uint32_t eip, cs, eflags;						 // Pushed by the processor.
} __attribute__((packed));

struct InterruptRegistersPF {
	uint32_t ds, es, fs, gs;
	uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
	uint32_t eip, cs, eflags;						 // Pushed by the processor.
} __attribute__((packed));

struct IDTPointer {
	uint16_t limit;
	size_t base;
} __attribute__((packed));

struct IDTEntry {
	uint16_t base_low;
	uint16_t kernel_cs;
	uint8_t reserved;
	uint8_t attributes;
	uint16_t base_high;

	inline void setBase(uint32_t base) {
		base_low = base & 0xFFFF;
		base_high = (base >> 16) & 0xFFFF;
	}
} __attribute__((packed));

static_assert(sizeof(IDTEntry) == 8, "IDTEntry must be 8 bytes");

class InterruptHandler {
  public:
	InterruptHandler();
	~InterruptHandler();

	static InterruptHandler *the();

	static void setup();

	void setHandler(uint8_t interrupt, void (*handler)());
	void setUserHandler(uint8_t interrupt, void (*handler)());

  private:
	void refresh();
};
