#include <kernel/Syscall.h>
#include <kernel/mem/Paging.h>
#include <kernel/printk.h>
#include <kernel/tasking/Process.h>
#include <kernel/tasking/Scheduler.h>
#include <priv/common.h>

namespace Syscall {

// FIXME: Exit codes
static void sys$exit(Process *current) {
	printk("Exit %d\n", current->pid());
	Scheduler::the()->kill_process(*current);
	Scheduler::schedule(0);
}

static size_t sys$exec(Process *current) {
	(void)current;
	// Process *next2 = new Process("init2");
	// Scheduler::the()->add_process(*next2);
	return 0;
}

static size_t sys$fork(Process *current, InterruptRegisters &regs) {
	Process *child = current->fork(regs);
	size_t pid = child->pid();
	printk("forking to %d from %d\n", child->pid(), current->pid());
	return pid;
}

static size_t sys$write(Process *current, uint32_t handle, size_t buffer_addr,
						size_t length) {
	auto current_pd = current->page_directory();
	PrintInterface *interface = printk_get_interface();
	for (size_t i = 0; i < length; i++) {
		// FIXME don't check the page constantly
		if (current_pd->is_mapped(buffer_addr + i) &&
			current_pd->is_user_page(buffer_addr + i) && handle == 1) {
			interface->write_character(
				*(reinterpret_cast<char *>(buffer_addr + i)));
		}
	}

	return length;
}

static void *sys$mmap(Process *current, void *address, size_t length) {
	auto current_pd = current->page_directory();
	size_t addr = reinterpret_cast<size_t>(address);
	if (!length)
		return nullptr;

	size_t addr_end = (addr + length + PAGE_SIZE - 1);

	for (size_t page = Paging::page_align(addr);
		 page < Paging::page_align(addr_end); page += PAGE_SIZE) {
		if (current_pd->is_mapped(page))
			return nullptr;
	}

	current_pd->map_range(addr, length, PageFlags::USER);
	printk("mmap\n");
	return address;
}

void handler(InterruptRegisters &regs) {
	size_t return_value = 0;
	uint32_t syscall_no = regs.eax;
	Process *current = Scheduler::the()->current();

	switch (syscall_no) {
	// exit
	case SYS_EXIT: {
		sys$exit(current);
	} break;
	// exec
	case SYS_EXEC: {
		return_value = sys$exec(current);
	} break;
	// fork
	case SYS_FORK: {
		return_value = sys$fork(current, regs);
	} break;
	// write
	case SYS_WRITE: {
		return_value = sys$write(current, regs.ebx, regs.ecx, regs.edx);
	} break;
	case SYS_MMAP: {
		return_value = reinterpret_cast<size_t>(
			sys$mmap(current, reinterpret_cast<void *>(regs.ebx), regs.ecx));
	} break;
	default:
		printk("Unknown syscall\n");
		break;
	}

	regs.eax = return_value;
}

} // namespace Syscall
