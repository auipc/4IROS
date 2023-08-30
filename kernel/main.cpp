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
	asm volatile("cli");
	uint32_t syscall_no = regs.eax;
	bool schedule_away = false;

	printk("Current process PID %d\n", Scheduler::the()->current()->pid());

	switch (syscall_no) {
	// exit
	case 1: {
		/*
		Process *current = Scheduler::the()->current();
		current->prev()->set_next(current->next());
		current->next()->set_prev(current->prev());
		// FIXME There's a chance the kernel stack is still used
		// High chance
		delete current;
		schedule_away = true;
		*/
	} break;
	case 2: {
		Process *next2 = new Process("init2");
		next2->set_next(Scheduler::the()->current()->next());
		next2->next()->set_prev(next2);
		next2->set_prev(Scheduler::the()->current());
		Scheduler::the()->current()->set_next(next2);
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

	MultiBootInfo *mb = reinterpret_cast<MultiBootInfo *>(
		reinterpret_cast<uintptr_t>(&_multiboot_data) + VIRTUAL_ADDRESS);
	printk("MB ptr %x\n", ptr);
	printk("MB mods count %d\n", mb->mods_count);
	printk("MB mods addr 0x%x\n", mb->mods_addr);

	kmalloc_init();
	GDT::setup();
	printk("We're running! %d\n", 100);
	printk("We're running!\n");
	printk("We're running!\n");
	printk("We're running!\n");
	PIC::setup();
	InterruptHandler::setup();
	printk("lol 0x%x\n", &syscall_interrupt_handler);
	InterruptHandler::the()->setUserHandler(0x80, &syscall_interrupt_handler);
	Paging::setup();

	printk("We're running!\n");
	asm volatile("sti");
	Scheduler::setup();

	while (1)
		;
}
