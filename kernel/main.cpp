#include <kernel/stdint.h>
#include <kernel/mem/malloc.h>
#include <kernel/printk.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/PIC.h>

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
	kmalloc_init();
	PIC::enable();
	GDT::setup();
	printk_use_interface(new VGAInterface());
	printk("We're running! %d\n", 100);
	printk("We're running!\n");
	printk("We're running!\n");
	printk("We're running!\n");
	InterruptHandler::setup();
	printk("lol\n");
	InterruptHandler::the()->setHandler(0x80, syscall_interrupt_handler);/*[]() {
		printk("Intentional interrupt!\n");
	});*/

	asm volatile("sti");
	asm volatile("int $0x80");

	while (1)
		;
}
