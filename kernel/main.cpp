#include <kernel/PIC.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/mem/Paging.h>
#include <kernel/mem/malloc.h>
#include <kernel/printk.h>
#include <kernel/tasking/Scheduler.h>

extern "C" void __cxa_pure_virtual() {
	// Do nothing or print an error message.
}

extern "C" void syscall_interrupt_handler();

extern "C" void syscall_interrupt(InterruptRegisters &regs) {
	asm volatile("cli");
	uint32_t syscall_no = regs.eax;
	bool schedule_away = false;

	switch (syscall_no) {
	case 1: {
		Process *current = Scheduler::the()->current();
		current->prev()->set_next(current->next());
		current->next()->set_prev(current->prev());
		// FIXME There's a chance the kernel stack is still used
		// High chance
		delete current;
		schedule_away = true;
	} break;
	default:
		printk("Unknown syscall\n");
		break;
	}

	printk("EAX: 0x%x\n", regs.eax);
	printk("ESP: 0x%x\n", regs.esp);
	printk("DS: 0x%x, ES: 0x%x, FS: 0x%x, GS: 0x%x\n", regs.ds, regs.es,
		   regs.fs, regs.gs);
	printk("Syscall interrupt!\n");
	asm volatile("sti");
	if (schedule_away) {
		Scheduler::schedule_no_save();
		assert(false);
	}
}

extern "C" void kernel_main() {
	// Kinda hacky, but kernel_main never exits.
	// Just hope the stack isn't touched.
	auto vga = VGAInterface();
	// I guess printing might be the most important function for now.
	// I've seen other kernels defer enable printing until later,
	// but maybe they have serial? Better than a black screen I guess.
	printk_use_interface(&vga);
	kmalloc_init();
	PIC::enable();
	GDT::setup();
	printk("We're running! %d\n", 100);
	printk("We're running!\n");
	printk("We're running!\n");
	printk("We're running!\n");
	InterruptHandler::setup();
	printk("lol 0x%x\n", &syscall_interrupt_handler);
	InterruptHandler::the()->setUserHandler(0x80, &syscall_interrupt_handler);
	Paging::setup();

	asm volatile("sti");
	printk("We're running!\n");
	Paging::current_page_directory()->map_page(0x8000, 0x8000, false);

	Scheduler::setup();

	while (1)
		;
}
