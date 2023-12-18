#include <kernel/PIC.h>
#include <kernel/PIT.h>
#include <kernel/arch/i386/IO.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/mem/Paging.h>
#include <kernel/printk.h>
#include <kernel/tasking/Process.h>
#include <kernel/tasking/Scheduler.h>

extern "C" void sched_asm(uintptr_t *prev_stack, uintptr_t *next_stack,
						  uintptr_t cr3);

extern TSS tss_entry;
extern "C" void pit_interrupt_handler();
extern "C" __attribute__((fastcall)) void pit_interrupt(uint32_t *esp) {
	if (Scheduler::the()) {
		Scheduler::the()->schedule(esp);
	}

	PIC::eoi(0x20);
}

void PIT::setup() {
	uint16_t freq = 1193181 / 20;

	// Channel 0 is connected directly to IRQ0
	InterruptHandler::the()->setHandler(0x50, pit_interrupt_handler);
	outb(0x43, 0x36);

	outb(0x40, freq & 0xFF);
	outb(0x40, (freq >> 8) & 0xFF);
	PIC::enable(0);
}
