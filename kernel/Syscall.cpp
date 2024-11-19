#include <kernel/Syscall.h>
#include <kernel/mem/Paging.h>
#include <kernel/vfs/vfs.h>
#include <kernel/shedule/proc.h>
#include <kernel/printk.h>
#include <priv/common.h>
#include <sys/types.h>
#include <unistd.h>
#include <kernel/arch/amd64/x64_sched_help.h>

namespace Syscall {

#if 0
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
#endif

static void sys$exit(Process *current, int status) {
#if 0
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
#endif
	(void)status;
	printk("exit\n");
	current->kill();
	Process::sched(0);
}

static size_t sys$fork(Process *current, SyscallReg* regs) {
	Process *child = current->fork(regs);
	child->next = current_process->next;
	child->prev = current_process;
	current_process->next = child;
	return child->pid;
}

// FIXME If the NUL terminator is missing we could end up accidentally reading kernel memory 
// if it directly bordered userspace memory. We should probably turn on the write protect bit in CR0 so we don't 
// overwrite read only user memory.
static bool verify_buffer(RootPageLevel *pd, uintptr_t buffer_addr, size_t size) {
	uintptr_t start_addr = buffer_addr - (buffer_addr % PAGE_SIZE);

	for (uintptr_t i = start_addr; i < size + (PAGE_SIZE-1); i += PAGE_SIZE) {
		if (pd->get_page_flags(buffer_addr + i) & (PAEPageFlags::Present | PAEPageFlags::User))
			return false;
	}
	return true;
}

// FIXME: Two processes can share the same VFSNode which causes problems (i.e.
// processes might snipe values from the keyboard buffer and the value is lost
// for another process reading it).
static int sys$read(Process *current, int fd, const char *buffer,
					size_t count) {
	if (!verify_buffer(current->root_page_level(), (uintptr_t)buffer, count))
		return -1;

	if ((size_t)fd > (current->m_file_handles.size() - 1))
		return -1;

	auto fh = current->m_file_handles[fd];
	fh->block_if_required(count);

	current->block_on_handle(fh);
	uintptr_t rip = get_rip();
	if (fh->check_blocked()) {
		uint64_t* sp;
		asm volatile("mov %%rsp, %0":"=a"(sp):);
		x64_syscall_block_help(sp, rip);
	}

#if 0
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
#endif

	return fh->read((void *)buffer, count);
}

static size_t sys$write(Process *current, uint32_t handle, uintptr_t buffer_addr,
						size_t length) {
	(void)handle;
	auto current_pd = current->root_page_level();

	if (!verify_buffer(current_pd, buffer_addr, length)) {
		printk("Invalid buffer\n");
		return -1;
	}

	if (current->m_file_handles.size() - 1 < handle) {
		printk("Bad FD\n");
		return -1;
	}

	current->m_file_handles[handle]->write(
		reinterpret_cast<void *>(buffer_addr), length);

	return length;
}

static int sys$open(Process *current, const char *buffer, int flags) {
	(void)flags;
	if (!verify_buffer(current->root_page_level(), (uintptr_t)buffer, PAGE_SIZE))
		return -1;

	auto path = VFS::the().parse_path(buffer);
	FileHandle *handle = VFS::the().open_fh(path);
	if (!handle)
		return -1;

	current->m_file_handles.push(handle);
	int fd = current->m_file_handles.size() - 1;
	return fd;
}

static int sys$lseek(Process* current, int fd, off_t offset, int whence) {
	if ((size_t)fd > (current->m_file_handles.size() - 1))
		return -1;

	if (whence > SEEK_END)
		return -1;

	auto fh = current->m_file_handles[fd];
	fh->seek(offset, static_cast<SeekMode>(whence));

	return fh->tell();
}

static void *sys$mmap(Process *current, void *address, size_t length) {
	auto current_pd = current->root_page_level();
	size_t addr = reinterpret_cast<size_t>(address);
	if (!length)
		return nullptr;

	size_t addr_end = (addr + length + PAGE_SIZE - 1);

	for (size_t page = Paging::page_align(addr);
		 page < Paging::page_align(addr_end); page += PAGE_SIZE) {
		if (current_pd->get_page_flags(page) & (PAEPageFlags::Present))
			return nullptr;

		uintptr_t free_page = Paging::the()->allocator()->find_free_page();
		Paging::the()->map_page(*current_pd, page, free_page, PAEPageFlags::User | PAEPageFlags::Present | PAEPageFlags::Write);
	}

	return address;
}

static size_t sys$exec(Process *current, const char *buffer) {
	if (!verify_buffer(current->root_page_level(), (uintptr_t)buffer, 255))
		return -1;

	printk("exec\n");
	// Lazily just kill the process, copy the pid, and file descriptors.
	auto proc = Process::create_user(buffer);
	auto next = current->next;
	auto prev = current->prev;

	// This kinda sucks
	*current = *proc;
	current->next = next;
	current->prev = prev;
	Process::reentry();
	return 0;
}

extern "C" void sched_sled(uintptr_t rsp) {
	Process::sched(rsp);
}

static int sys$waitpid(Process *current, pid_t pid, int *wstatus, int options) {
	// FIXME: Implement the functionality for pid <= 0
	assert(pid > 0);
	(void)wstatus;
	(void)options;

	Process* proc = current;

	while ((proc->next || proc->state() == ProState::Dead) && proc->pid != pid) {
		proc = proc->next;
	}

	current->block_on_proc(proc);
	uintptr_t rip = get_rip();
	printk("proc_state %d\n", proc->state());
	if (proc->state() == ProState::Running) {
		uint64_t* sp;
		asm volatile("mov %%rsp, %0":"=a"(sp):);
		x64_syscall_block_help(sp, rip);
	}
	return pid;
}

void handler(SyscallReg* regs) {
	//auto current_pgl = (PageLevel*)get_cr3();
	//Paging::switch_page_directory(Paging::the()->kernel_root_directory());

	// :)
	decltype(regs->rax) return_value = 0;
	decltype(regs->rax) syscall_no = regs->rax;
	Process* current = current_process;

	switch (syscall_no) {
	case SYS_EXIT: {
		sys$exit(current, (int)regs->rbx);
	} break;
	case SYS_FORK: {
		return_value = sys$fork(current, regs);
	} break;
	case SYS_OPEN: {
		return_value =
			sys$open(current, reinterpret_cast<const char *>(regs->rbx),
					 static_cast<int>(regs->rdx));
	} break;
	case SYS_WRITE: {
		return_value = sys$write(current, regs->rbx, regs->rdx, regs->rdi);
	} break;
	case SYS_READ: {
		return_value = sys$read(current, static_cast<int>(regs->rbx),
								reinterpret_cast<char *>(regs->rdx),
								static_cast<size_t>(regs->rdi));
	} break;
	case SYS_LSEEK: {
		return_value = sys$lseek(current, static_cast<int>(regs->rbx),
								static_cast<off_t>(regs->rdx),
								static_cast<int>(regs->rdi));
	} break;
	case SYS_MMAP: {
		return_value = reinterpret_cast<size_t>(
			sys$mmap(current, reinterpret_cast<void *>(regs->rbx), regs->rdx));
	} break;
	case SYS_EXEC: {
		return_value =
			sys$exec(current, reinterpret_cast<const char *>(regs->rbx));
	} break;
	case SYS_WAITPID: {
		return_value = static_cast<size_t>(sys$waitpid(
			current, static_cast<pid_t>(regs->rbx),
			reinterpret_cast<int *>(regs->rdx), static_cast<int>(regs->rdi)));
	} break;
	default:
		printk("Unknown syscall %d\n", syscall_no);
		break;
	}

	regs->rax = return_value;
	write_msr(0xC0000102, (uint32_t)(uintptr_t)&ksyscall_data, (uint32_t)((uintptr_t)&ksyscall_data>>32));
	//Paging::switch_page_directory(current_pgl);
#if 0
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
#endif
}

} // namespace Syscall

extern "C" void syscall_proxy(SyscallReg* regs) {
	Syscall::handler(regs);
}
