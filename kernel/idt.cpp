#include <kernel/idt.h>
#include <kernel/printk.h>

static InterruptHandler* s_the;

static IDTPointer idtPointer;
static IDTEntry idtEntries[256];

InterruptHandler::InterruptHandler() {
	s_the = this;
}

InterruptHandler::~InterruptHandler() {
	s_the = nullptr;
}

InterruptHandler* InterruptHandler::the() {
	if (!s_the) {
		printk("InterruptHandler not initialized!\n");
		while(1);
	}

	return s_the;
}

void InterruptHandler::refresh() {
	asm volatile("lidt %0" : : "m"(idtPointer));
}

extern "C" void unhandled_interrupt() {
	printk("Unhandled interrupt\n");
	// Halt the CPU
	while (1) {
		asm volatile("cli");
		asm volatile("hlt");
	}
}

extern "C" void division_by_zero() {
	printk("Division by zero!\n");
	// Halt the CPU
	while (1) {
		asm volatile("cli");
		asm volatile("hlt");
	}
}

extern "C" void page_fault() {
	size_t faulting_address;
	asm volatile("mov %%cr2, %0" : "=r"(faulting_address));
	printk("Page fault at %x\n", faulting_address);
	printk("Page fault!\n");
	// Halt the CPU
	while (1) {
		asm volatile("cli");
		asm volatile("hlt");
	}
}

extern "C" void unhandled_interrupt_handler();
asm("unhandled_interrupt_handler:");
asm("	pusha");
/*asm("	pushw %ds");
asm("    pushw %es");
asm("    pushw %fs");
asm("    pushw %gs");
asm("    pushw %ss");
asm("    pushw %ss");
asm("    pushw %ss");
asm("    pushw %ss");
asm("    pushw %ss");
asm("    popw %ds");
asm("    popw %es");
asm("    popw %fs");
asm("    popw %gs");*/
asm("	call unhandled_interrupt");
/*asm("	popw %gs");
asm("    popw %gs");
asm("    popw %fs");
asm("    popw %es");
asm("    popw %ds");*/
asm("	popa");
asm("	iret");

void InterruptHandler::setup() {
	s_the = new InterruptHandler();
	idtPointer.limit = sizeof(IDTEntry) * 256 - 1;
	idtPointer.base = (size_t)&idtEntries[0];

	s_the->refresh();
	for (uint8_t i = 0; i < 255; i++) {
		s_the->setHandler(i, unhandled_interrupt_handler);
	}
}

void InterruptHandler::setHandler(uint8_t interrupt, void (*handler)()) {
	idtEntries[interrupt].setBase((uint32_t)handler);
	idtEntries[interrupt].kernel_cs = 0x08;
	idtEntries[interrupt].reserved = 0;
	idtEntries[interrupt].attributes = 0x8E;

	// TODO is there a way we can defer this to a later time? Excessive calls to lidt will increase boot time.
	// we could just hardcode the interrupts by hand and only call setHandler when we need to change one.
	refresh();
}