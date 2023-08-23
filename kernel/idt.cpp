#include <kernel/idt.h>
#include <kernel/printk.h>

static InterruptHandler *s_the;

static IDTPointer idtPointer;
static IDTEntry idtEntries[256];

InterruptHandler::InterruptHandler() { s_the = this; }

InterruptHandler::~InterruptHandler() { s_the = nullptr; }

InterruptHandler *InterruptHandler::the() {
	if (!s_the) {
		printk("InterruptHandler not initialized!\n");
		while (1)
			;
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

extern "C" void interrupt_14(InterruptRegisters regs) {
	size_t faulting_address;
	asm volatile("mov %%cr2, %0" : "=r"(faulting_address));
	printk("Page fault at %x\n", faulting_address);
	printk("Error code 0x%x\n", regs.eflags);
	// Halt the CPU
	while (1) {
		asm volatile("cli");
		asm volatile("hlt");
	}
}

#define INTERRUPT_HANDLER(x)                                                   \
	extern "C" void interrupt_##x##_handler();                                 \
	asm("interrupt_" #x "_handler:");                                          \
	asm("	pusha");                                                           \
	asm("   push %gs");                                                        \
	asm("   push %fs");                                                        \
	asm("   push %es");                                                        \
	asm("   push %ds");                                                        \
	asm("	call interrupt_" #x);                                              \
	asm("   add $0x10, %esp");                                                 \
	asm("	popa");                                                            \
	asm("	iret");														       \


#define HALTING_INTERRUPT_STUB(x, msg)                                         \
	extern "C" void interrupt_##x(InterruptRegisters regs) {                   \
		printk(msg "\n");                                                      \
		printk("Error code 0x%x\n", regs.eflags);                              \
		printk("Registers EAX: 0x%x EBX: 0x%x ECX: 0x%x EDX: 0x%x\n",          \
			   regs.eax, regs.ebx, regs.ecx, regs.edx);                        \
		while (1) {                                                            \
			asm volatile("cli");                                               \
			asm volatile("hlt");                                               \
		}                                                                      \
	}

INTERRUPT_HANDLER(0)  // Divide by zero
INTERRUPT_HANDLER(1)  // Debug
INTERRUPT_HANDLER(2)  // Non-maskable interrupt
INTERRUPT_HANDLER(3)  // Breakpoint
INTERRUPT_HANDLER(4)  // Overflow
INTERRUPT_HANDLER(5)  // Bound range exceeded
INTERRUPT_HANDLER(6)  // Invalid opcode
INTERRUPT_HANDLER(7)  // Device not available
INTERRUPT_HANDLER(8)  // Double fault
INTERRUPT_HANDLER(9)  // Coprocessor segment overrun
INTERRUPT_HANDLER(10) // Invalid TSS
INTERRUPT_HANDLER(11) // Segment not present
INTERRUPT_HANDLER(12) // Stack-segment fault
INTERRUPT_HANDLER(13) // General protection fault
INTERRUPT_HANDLER(14) // Page fault
INTERRUPT_HANDLER(15) // Reserved
INTERRUPT_HANDLER(16) // x87 floating-point exception
INTERRUPT_HANDLER(17) // Alignment check
INTERRUPT_HANDLER(18) // Machine check
INTERRUPT_HANDLER(19) // SIMD floating-point exception
INTERRUPT_HANDLER(20) // Virtualization exception

HALTING_INTERRUPT_STUB(0, "Divide by zero")
HALTING_INTERRUPT_STUB(1, "Debug")
HALTING_INTERRUPT_STUB(2, "Non-maskable interrupt")
HALTING_INTERRUPT_STUB(3, "Breakpoint")
HALTING_INTERRUPT_STUB(4, "Overflow")
HALTING_INTERRUPT_STUB(5, "Bound range exceeded")
HALTING_INTERRUPT_STUB(6, "Invalid opcode")
HALTING_INTERRUPT_STUB(7, "Device not available")
HALTING_INTERRUPT_STUB(8, "Double fault")
HALTING_INTERRUPT_STUB(9, "Coprocessor segment overrun")
HALTING_INTERRUPT_STUB(10, "Invalid TSS")
HALTING_INTERRUPT_STUB(11, "Segment not present")
HALTING_INTERRUPT_STUB(12, "Stack-segment fault")
HALTING_INTERRUPT_STUB(13, "General protection fault")
// HALTING_INTERRUPT_STUB(14, "Page fault")
HALTING_INTERRUPT_STUB(15, "Reserved")
HALTING_INTERRUPT_STUB(16, "x87 floating-point exception")
HALTING_INTERRUPT_STUB(17, "Alignment check")
HALTING_INTERRUPT_STUB(18, "Machine check")
HALTING_INTERRUPT_STUB(19, "SIMD floating-point exception")
HALTING_INTERRUPT_STUB(20, "Virtualization exception")

void InterruptHandler::setup() {
	s_the = new InterruptHandler();
	idtPointer.limit = sizeof(IDTEntry) * 256 - 1;
	idtPointer.base = (size_t)&idtEntries[0];

	s_the->refresh();
	for (uint8_t i = 0; i < 255; i++) {
		s_the->setHandler(i, unhandled_interrupt);
	}

	s_the->setHandler(0, interrupt_0_handler);
	s_the->setHandler(1, interrupt_1_handler);
	s_the->setHandler(2, interrupt_2_handler);
	s_the->setHandler(3, interrupt_3_handler);
	s_the->setHandler(4, interrupt_4_handler);
	s_the->setHandler(5, interrupt_5_handler);
	s_the->setHandler(6, interrupt_6_handler);
	s_the->setHandler(7, interrupt_7_handler);
	s_the->setHandler(8, interrupt_8_handler);
	s_the->setHandler(9, interrupt_9_handler);
	s_the->setHandler(10, interrupt_10_handler);
	s_the->setHandler(11, interrupt_11_handler);
	s_the->setHandler(12, interrupt_12_handler);
	s_the->setHandler(13, interrupt_13_handler);
	s_the->setHandler(14, interrupt_14_handler);
	s_the->setHandler(15, interrupt_15_handler);
	s_the->setHandler(16, interrupt_16_handler);
	s_the->setHandler(17, interrupt_17_handler);
	s_the->setHandler(18, interrupt_18_handler);
	s_the->setHandler(19, interrupt_19_handler);
	s_the->setHandler(20, interrupt_20_handler);
}

void InterruptHandler::setHandler(uint8_t interrupt, void (*handler)()) {
	idtEntries[interrupt].setBase((uint32_t)handler);
	idtEntries[interrupt].kernel_cs = 0x08;
	idtEntries[interrupt].reserved = 0;
	idtEntries[interrupt].attributes = 0x8E;

	// TODO is there a way we can defer this to a later time? Excessive calls to
	// lidt will increase boot time. we could just hardcode the interrupts by
	// hand and only call setHandler when we need to change one.
	refresh();
}