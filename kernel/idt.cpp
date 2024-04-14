#include <kernel/Debug.h>
#include <kernel/arch/i386/i386.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/printk.h>
#include <kernel/tasking/Process.h>
#include <kernel/tasking/Scheduler.h>

enum class Exceptions {
	DivError = 0,
	Debug = 1,
	NMI = 2,
	Breakpoint = 3,
	Overflow = 4,
	// Do modern compilers even emit the instruction to trigger this?
	BoundRangeExceeded = 5,
	InvalidOp = 6,
	// Another i386 era exception that will never trigger!
	DeviceNotAvailable = 7,
	DoubleFault = 8,
	// Unused
	CoprocessorSegOverrun = 9,
	InvalidTSS = 10,
	SegmentNotPresent = 11,
	StackSegFault = 12,
	GeneralProtFault = 13,
	PageFault = 14,
	Reserved = 15,
	x87FP = 16,
	AlignCheck = 17,
	MachineCheck = 18,
	COUNT,
};

static_assert(static_cast<uint8_t>(Exceptions::COUNT) <= 256);

enum class PageFaultBits {
	User = (1 << 2),
};

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

extern "C" void div_interrupt(InterruptRegisters regs) {
	printk("[[Div error]]\n");
	printk("CS %x\n", regs.cs);
	if (regs.cs == USER_CS) {
		Process *current = Scheduler::the()->current();
		printk("Killing PID %d\n", current->pid());
		Scheduler::the()->kill_process(*current);
		Scheduler::the()->schedule((uint32_t *)regs.esp);
	}
	while (1)
		;
}

extern "C" void gpf_interrupt(InterruptRegistersFault regs) {
	printk("[[General Protection Fault]]\n");
	if (regs.error != 0) {
		printk("Caused by segment %d\n", regs.error);
	}

	while (1)
		;
}

extern "C" void df_interrupt(InterruptRegisters) {
	printk("[[Double Fault]]");
	while (1)
		;
}

extern "C" void invalidtss_interrupt(InterruptRegisters) {
	printk("[[Invalid TSS]]");
	while (1)
		;
}

extern "C" void pf_interrupt(InterruptRegistersFault regs) {
	uint32_t fault_addr = get_cr2();

	// Paging::the()->resolve_fault(Paging::current_page_directory(),
	// fault_addr);
	if (regs.cs == USER_CS &&
		(regs.error & static_cast<uint32_t>(PageFaultBits::User))) {
		Process *current = Scheduler::the()->current();
		printk("Current PTR %x\n", current);
		printk("Current PID %x\n", current->pid());
		printk("[[Page fault in userspace while accessing address 0x%x. "
			   "Killing PID "
			   "%d]]\n",
			   fault_addr, current->pid());
		printk("Error code 0x%x\n", regs.error);
		Debug::stack_trace();
		Scheduler::the()->kill_process(*current);
		Scheduler::the()->schedule((uint32_t *)regs.esp);
		while (1)
			;
	} else {
		printk("[[Unrecoverable kernel page fault while accessing address "
			   "0x%x]]\n",
			   fault_addr);
		printk("EIP: %x\n", regs.eip);
		printk("Error code 0x%x\n", regs.error);
		Debug::stack_trace();
		// We PF'd in kernel, halt the CPU.
		while (1) {
			asm volatile("cli");
			asm volatile("hlt");
		}
	}
}

extern "C" void div_interrupt_handler();
extern "C" void gpf_interrupt_handler();
extern "C" void df_interrupt_handler();
extern "C" void invalidtss_interrupt_handler();
extern "C" void pf_interrupt_handler();

void InterruptHandler::setup() {
	s_the = new InterruptHandler();
	idtPointer.limit = sizeof(IDTEntry) * 255;
	idtPointer.base = (size_t)&idtEntries[0];

	s_the->refresh();
	for (uint8_t i = 0; i < 255; i++) {
		s_the->set_handler(i, unhandled_interrupt, false);
	}

	s_the->set_handler(static_cast<uint8_t>(Exceptions::DivError),
					   div_interrupt_handler, false);
	s_the->set_handler(static_cast<uint8_t>(Exceptions::GeneralProtFault),
					   gpf_interrupt_handler, false);
	s_the->set_handler(static_cast<uint8_t>(Exceptions::DoubleFault),
					   df_interrupt_handler, false);
	s_the->set_handler(static_cast<uint8_t>(Exceptions::InvalidTSS),
					   invalidtss_interrupt_handler, false);
	s_the->set_handler(static_cast<uint8_t>(Exceptions::PageFault),
					   pf_interrupt_handler, false);

	s_the->refresh();
}

void InterruptHandler::set_handler(uint8_t interrupt, void (*handler)(),
								   bool reload) {
	idtEntries[interrupt].setBase(reinterpret_cast<uintptr_t>(handler));
	idtEntries[interrupt].kernel_cs = 0x08;
	idtEntries[interrupt].reserved = 0;
	idtEntries[interrupt].attributes = 0x8E;
	if (reload)
		refresh();
}

void InterruptHandler::set_user_handler(uint8_t interrupt, void (*handler)()) {
	idtEntries[interrupt].setBase(reinterpret_cast<uintptr_t>(handler));
	idtEntries[interrupt].kernel_cs = 0x08;
	idtEntries[interrupt].reserved = 0;
	idtEntries[interrupt].attributes = 0xEF;
	refresh();
}
