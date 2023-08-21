#include <kernel/PIC.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/mem/Paging.h>
#include <kernel/mem/malloc.h>
#include <kernel/printk.h>
#include <kernel/stdint.h>

extern "C" void __cxa_pure_virtual() {
	// Do nothing or print an error message.
}

extern "C" void syscall_interrupt_handler();
asm("syscall_interrupt_handler:");
asm("	pusha");
asm("	call syscall_interrupt");
asm("	popa");
asm("	iret");

extern "C" void syscall_interrupt(InterruptRegisters regs) {
	printk("ESP: 0x%x\n", regs.esp);
	printk("Syscall interrupt!\n");
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
	printk("lol\n");
	InterruptHandler::the()->setHandler(0x80, syscall_interrupt_handler); /*[]()
	 { printk("Intentional interrupt!\n");
	 });*/
	Paging::setup();

	asm volatile("sti");
	asm volatile("int $0x80");
	asm volatile("int $0x80");
	asm volatile("int $0x80");
	asm volatile("int $0x80");

	while (1)
		;
}
