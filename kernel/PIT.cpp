#include <kernel/PIC.h>
#include <kernel/PIT.h>
#include <kernel/arch/i386/IO.h>
#include <kernel/idt.h>
#include <kernel/printk.h>
#include <kernel/tasking/Scheduler.h>

extern "C" void pit_interrupt_handler();
extern "C" void pit_interrupt() {
	if (Scheduler::the()) {
		Scheduler::the()->schedule();
	}
	// lol
	outb(0x20, 0x20);
	outb(0xA0, 0x20);
}

void PIT::setup() {
	uint16_t freq = 1193181 / 20;
	printk("Setting up clock with tick frequency %d\n", freq);
	// Channel 0 is connected directly to IRQ0
	InterruptHandler::the()->setHandler(0x50, pit_interrupt_handler);
	outb(0x43, 0x36);

	outb(0x40, freq & 0xFF);
	outb(0x40, (freq >> 8) & 0xFF);
	PIC::enable(0);
}
