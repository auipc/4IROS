#include <kernel/Debug.h>
#include <kernel/PIC.h>
#include <kernel/Syscall.h>
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
#include <kernel/util/Spinlock.h>
#include <kernel/vfs/vfs.h>

Spinlock cxa_spinlock;

extern "C" int __cxa_guard_acquire(long long int *) {
	cxa_spinlock.acquire();
	return 1;
}

extern "C" void __cxa_guard_release(long long int *) { cxa_spinlock.release(); }

extern "C" void __cxa_pure_virtual() {
	// Do nothing or print an error message.
}

extern "C" void syscall_interrupt_handler();

extern "C" void syscall_interrupt(InterruptRegisters &regs) {
	Syscall::handler(regs);
}

void test_fs() {
	Vec<const char *> zero_path;
	zero_path.push("dev");
	zero_path.push("zero");
	VFSNode *zero_file = VFS::the().open(zero_path);
	assert(zero_file);
	uint8_t zero_value = 0xFF;
	zero_file->read(&zero_value, sizeof(uint8_t) * 1);
	assert(zero_file == 0);
}

extern "C" void kernel_main(uint32_t magic, uint32_t mb_ptr_phys) {
	assert(magic == 0x2BADB002);

	multiboot_info_t *mb = reinterpret_cast<multiboot_info_t *>(
		0xc03fe000 + (mb_ptr_phys % PAGE_SIZE));

	kmalloc_temp_init();
	Paging::setup((mb->mem_lower + mb->mem_upper) * 1024);
	kmalloc_init();

	printk_use_interface(new VGAInterface());

	printk("%d\n", (mb->mem_lower + mb->mem_upper));

	GDT::setup();
	PIC::setup();
	InterruptHandler::setup();
	InterruptHandler::the()->setUserHandler(0x80, &syscall_interrupt_handler);

	asm volatile("sti");
	printk("fbaddr %x\n", mb->framebuffer_addr);
	Scheduler::setup();

	while (1)
		;
}
