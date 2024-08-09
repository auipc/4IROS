#include <kernel/Syscall.h>
#include <kernel/idt.h>
#include <kernel/mem/Paging.h>
#include <kernel/printk.h>
#include <kernel/tasking/Process.h>
#include <kernel/tasking/Scheduler.h>
#include <libc/priv/common.h>
#include <sys/types.h>

namespace Syscall {

static bool verify_buffer(PageDirectory *pd, uintptr_t buffer_addr, size_t size) {
	uintptr_t start_addr = buffer_addr - (buffer_addr % PAGE_SIZE);

	for (uintptr_t i = start_addr; i < PAGE_SIZE + size; i += PAGE_SIZE) {
		if (!pd->is_mapped(buffer_addr + i))
			return false;
		if (!pd->is_user_page(buffer_addr + i))
			return false;
	}
	return true;
}

static void sys$exit(Process *current, int status) {
	Scheduler::the()->sched_spinlock.acquire();
	auto alert = current->alert();
	if (alert.alert_status_ptr) {
		PageDirectory *old = current->page_directory();
		Paging::switch_page_directory(alert.listener->page_directory());
		*alert.alert_status_ptr = status;
		Paging::switch_page_directory(old);
	}

	Scheduler::the()->kill_process(*current);
	Scheduler::the()->sched_spinlock.release();
	Scheduler::schedule(0);
}

static size_t sys$exec(Process *current, const char *buffer) {
	if (!verify_buffer(current->page_directory(), (uintptr_t)buffer, 255))
		return -1;

	auto vp = VFS::the().parse_path(buffer);
	VFSNode *binary = VFS::the().open(vp);
	if (!binary)
		return 1;

	Process *p = new Process(binary);
	// Lazily just kill the process, copy the pid, and file descriptors.
	p->set_pid(current->pid());
	for (size_t i = 0; i < current->m_file_descriptors->size(); i++) {
		p->m_file_descriptors =
			current
				->m_file_descriptors; //->push((*current->m_file_descriptors)[i]);
	}

	p->set_alert(current->alert());
	current->set_state(Process::States::Dead);
	Scheduler::the()->add_process(*p);
	Scheduler::schedule(0);
	assert(false);
	return 0;
}

static size_t sys$fork(Process *current, InterruptRegisters &regs) {
	Process *child = current->fork(regs);
	size_t pid = child->pid();
	Scheduler::the()->add_process(*child);
	return pid;
}

extern "C" void *read_eip();
asm("read_eip:");
asm("pop %eax");
asm("jmp *%eax");

static size_t sys$write(Process *current, uint32_t handle, uintptr_t buffer_addr,
						size_t length) {
	auto current_pd = current->page_directory();

	if (!verify_buffer(current_pd, buffer_addr, length))
		return -1;

	if (current->m_file_descriptors->size() - 1 < handle)
		return -1;

	(*current->m_file_descriptors)[handle]->write(
		reinterpret_cast<void *>(buffer_addr), length);

	return length;
}

static int sys$open(Process *current, const char *buffer, int flags) {
	(void)flags;
	if (!verify_buffer(current->page_directory(), (uintptr_t)buffer, PAGE_SIZE))
		return -1;

	auto path = VFS::the().parse_path(buffer);
	FileHandle *handle = VFS::the().open_fh(path);
	if (!handle)
		return -1;

	current->m_file_descriptors->push(handle);
	int fd = current->m_file_descriptors->size() - 1;
	return fd;
}

// FIXME: Two processes can share the same VFSNode which causes problems (i.e.
// processes might snipe values from the keyboard buffer and the value is lost
// for another process reading it).
static int sys$read(Process *current, int fd, const char *buffer,
					size_t count) {
	if (!verify_buffer(current->page_directory(), (uintptr_t)buffer, count))
		return -1;

	if ((size_t)fd > (current->m_file_descriptors->size() - 1))
		return -1;

	auto fh = (*current->m_file_descriptors)[fd];
	fh->block_if_required(count);

	// FIXME: Are we 100% sure the stack isn't getting mangled? We're relying on
	// that assumption (which probably isn't true on other optimization levels).
	void *eip = read_eip();
	if (fh->check_blocked()) {
		// Set the process state to blocked so it won't get scheduled again,
		// then save registers.
		current->set_state(Process::States::Blocked);
		current->m_blocked_source = fh;

		// FIXME: Move me (or allow nested syscall interrupts)
		asm volatile("pushf");
		asm volatile("pushl %cs");
		asm volatile("push %0" ::"a"(eip));
		asm volatile("pusha");
		asm volatile("pushw %ds");
		uint32_t esp;
		asm volatile("mov %%esp, %%eax" : "=a"(esp));
		Scheduler::schedule((uint32_t *)esp);
		while (1)
			;
	}

	return fh->read((void *)buffer, count);
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

static int sys$waitpid(Process *current, pid_t pid, int *wstatus, int options) {
	(void)options;
	// FIXME: Implement the functionality for pid <= 0
	assert(pid > 0);
	auto wait_proc = current;
	auto end = current;

	do {
		if ((pid_t)wait_proc->pid() == pid) {
			break;
		}
		wait_proc = wait_proc->next();
	} while (wait_proc != end);

	if (wait_proc == end)
		return -1;

	if (wstatus &&
		verify_buffer(current->page_directory(), (size_t)wstatus, 4)) {
		wait_proc->set_alert(Process::ProcessAlert{current, wstatus});
	}

	auto throughtheloop = [](Process *actual_current, pid_t seek_pid) {
		Scheduler::the()->sched_spinlock.acquire();
		auto start = actual_current;
		auto end = actual_current;

		do {
			if ((pid_t)start->pid() == seek_pid &&
				start->state() != Process::States::Dead)
				return true;
			start = start->next();
		} while (start != end);
		Scheduler::the()->sched_spinlock.release();
		return false;
	};

	void *eip = read_eip();

	while (throughtheloop(Scheduler::the()->current(), pid)) {
		// FIXME: Move me (or allow nested syscall interrupts)
		asm volatile("pushf");
		asm volatile("pushl %cs");
		asm volatile("push %0" ::"a"(eip));
		asm volatile("pusha");
		asm volatile("pushw %ds");
		uint32_t esp;
		asm volatile("mov %%esp, %%eax" : "=a"(esp));
		Scheduler::schedule((uint32_t *)esp);
		while (1)
			;
	}

	return pid;
}

void handler(InterruptRegisters &regs) {
	size_t return_value = 0;
	uint32_t syscall_no = regs.eax;
	Process *current = Scheduler::the()->current();

	switch (syscall_no) {
	case SYS_EXIT: {
		sys$exit(current, (int)regs.ebx);
	} break;
	case SYS_EXEC: {
		return_value =
			sys$exec(current, reinterpret_cast<const char *>(regs.ebx));
	} break;
	case SYS_FORK: {
		return_value = sys$fork(current, regs);
	} break;
	case SYS_WRITE: {
		return_value = sys$write(current, regs.ebx, regs.ecx, regs.edx);
	} break;
	case SYS_OPEN: {
		return_value =
			sys$open(current, reinterpret_cast<const char *>(regs.ebx),
					 static_cast<int>(regs.ecx));
	} break;
	case SYS_READ: {
		return_value = sys$read(current, static_cast<int>(regs.ebx),
								reinterpret_cast<char *>(regs.ecx),
								static_cast<size_t>(regs.edx));
	} break;
	case SYS_MMAP: {
		return_value = reinterpret_cast<size_t>(
			sys$mmap(current, reinterpret_cast<void *>(regs.ebx), regs.ecx));
	} break;
	case SYS_WAITPID: {
		return_value = static_cast<size_t>(sys$waitpid(
			current, static_cast<pid_t>(regs.ebx),
			reinterpret_cast<int *>(regs.ecx), static_cast<int>(regs.edx)));
	} break;
	default:
		printk("Unknown syscall\n");
		break;
	}

	regs.eax = return_value;
}

} // namespace Syscall
