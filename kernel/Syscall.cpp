#include <kernel/Syscall.h>
#include <kernel/mem/Paging.h>
#include <kernel/printk.h>
#include <kernel/tasking/Process.h>
#include <kernel/tasking/Scheduler.h>

namespace Syscall {

enum {
	SYS_EXIT = 1,
	SYS_EXEC = 2,
	SYS_FORK = 3,
	SYS_WRITE = 4,
};

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
	default:
		printk("Unknown syscall\n");
		break;
	}

	regs.eax = return_value;
}

} // namespace Syscall
