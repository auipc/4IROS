#include <kernel/PIC.h>
#include <kernel/arch/i386/kernel.h>
#include <kernel/assert.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/mem/Paging.h>
#include <kernel/mem/malloc.h>
#include <kernel/multiboot.h>
#include <kernel/printk.h>
#include <kernel/tasking/Process.h>
#include <kernel/tasking/Scheduler.h>

extern "C" void __cxa_pure_virtual() {
	// Do nothing or print an error message.
}

extern "C" void syscall_interrupt_handler();

extern "C" void syscall_interrupt(InterruptRegisters &regs) {
	uint32_t return_value = 0;
	uint32_t syscall_no = regs.eax;
	Process *current = Scheduler::the()->current();

	printk("Current process PID %d\n", Scheduler::the()->current()->pid());

	switch (syscall_no) {
	// exit
	case 1: {
		printk("Exit %d\n", current->pid());
		Scheduler::the()->kill_process(*current);
		Scheduler::schedule();
	} break;
	// dumb exec
	case 2: {
		Process *next2 = new Process("init2");
		printk("dumb exec %d\n", next2->pid());
		Scheduler::the()->add_process(*next2);
	} break;
	// fork
	case 3: {
		Process* child = current->fork(regs);
		printk("forking to %d\n", child->pid());
		Scheduler::the()->add_process(*child);
	} break;
	default:
		printk("Unknown syscall\n");
		break;
	}

	regs.eax = return_value;
}

extern "C" char _multiboot_data;
extern "C" void kernel_main(uint32_t magic, uint32_t ptr) {
	assert(magic == 0x2BADB002);
	// Kinda hacky, but kernel_main never exits.
	// Just hope the stack isn't touched.
	auto vga = VGAInterface();
	// I guess printing might be the most important function for now.
	// I've seen other kernels defer enable printing until later,
	// but maybe they have serial? Better than a black screen I guess.
	printk_use_interface(&vga);

	multiboot_info *mb = reinterpret_cast<multiboot_info *>(ptr);
	(void)mb;

	kmalloc_init();
	GDT::setup();
	PIC::setup();
	InterruptHandler::setup();
	InterruptHandler::the()->setUserHandler(0x80, &syscall_interrupt_handler);
	Paging::setup(128 * MB);

	asm volatile("sti");
	Scheduler::setup();

	while (1)
		;
}
