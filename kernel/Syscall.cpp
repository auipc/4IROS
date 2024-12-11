#include <kernel/Syscall.h>
#include <kernel/arch/amd64/x64_sched_help.h>
#include <kernel/mem/Paging.h>
#include <kernel/printk.h>
#include <kernel/shedule/proc.h>
#include <kernel/vfs/stddev.h>
#include <kernel/vfs/vfs.h>
#include <priv/common.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

extern HashTable<Process*>* process_pid_map;
namespace Syscall {

static void sys$exit(Process *current, int status, [[maybe_unused]] int* errno) {
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
	current->exit_code = status;
	current->kill();
	Paging::the()->release(*current->root_page_level());
	Process::sched(0);
}

static size_t sys$fork(Process *current, SyscallReg *regs, [[maybe_unused]] int* errno) {
	Process *child = current->fork(regs);
	child->next = current_process->next;
	child->prev = current_process;
	current_process->next = child;
	printk("Run queue: \n");
	printk("%d", current_process->pid);
	Process* p = current_process->next;
	while (p != current_process) {
		p = p->next;
		if (p->state() != ProState::Dead) {
			printk(", %d", p->pid);
		}
	}
	printk("\n");
	return child->pid;
}

// FIXME If the NUL terminator is missing we could end up accidentally reading
// kernel memory if it directly bordered userspace memory. We should probably
// turn on the write protect bit in CR0 so we don't overwrite read only user
// memory.
// FIXME Make verify_buffer work with CoW
static bool verify_buffer(RootPageLevel *pd, uintptr_t buffer_addr,
						  size_t size) {
	uintptr_t start_addr = buffer_addr - (buffer_addr % PAGE_SIZE);

	for (uintptr_t i = start_addr; i < size + (PAGE_SIZE - 1); i += PAGE_SIZE) {
		if (pd->get_page_flags(buffer_addr + i) &
			(PAEPageFlags::Present | PAEPageFlags::User))
			return false;
	}
	return true;
}

FileHandle *stdin_filehandle;
FileHandle *stdout_filehandle;

KSyscallData data;
// FIXME: Two processes can share the same VFSNode which causes problems (i.e.
// processes might snipe values from the keyboard buffer and the value is lost
// for another process reading it).
static int sys$read(Process *current, int fd, const char *buffer,
					size_t count, [[maybe_unused]] int* errno) {
	if (!verify_buffer(current->root_page_level(), (uintptr_t)buffer, count)) {
		*errno = EFAULT;
		return -1;
	}

	if ((size_t)fd > (current->m_file_handles.size() - 1)) {
		*errno = EBADF;
		return -1;
	}

	auto fh = current->m_file_handles[fd];
	if (fd == 0) {
		if (!stdin_filehandle)
			stdin_filehandle = new FileHandle(new StdDev(false));
		fh = stdin_filehandle;
	} else if (fd == 1) {
		if (!stdout_filehandle)
			stdout_filehandle = new FileHandle(new StdDev(true));
		fh = stdout_filehandle;
	}
	fh->block_if_required(count);

	current->block_on_handle(fh);
	printk("check_blocked_begin\n");
	uintptr_t rip = get_rip();
	
	while (fh->check_blocked()) {
		printk("check_blocked\n");
		uint64_t *sp;
		asm volatile("mov %%rsp, %0" : "=a"(sp) :);
		x64_syscall_block_help(sp, rip);
	}
	printk("check_blocked_end\n");

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

static size_t sys$write(Process *current, uint32_t handle,
						uintptr_t buffer_addr, size_t length, [[maybe_unused]] int* errno) {
	(void)handle;
	auto current_pd = current->root_page_level();

	if (!verify_buffer(current_pd, buffer_addr, length)) {
		printk("Invalid buffer\n");
		*errno = EFAULT;
		return -1;
	}

	if (current->m_file_handles.size() - 1 < handle) {
		printk("Bad FD\n");
		return -1;
	}

	if (handle == 0) {
		// printk("Write Handle %x\n", current->m_file_handles[handle]);
		if (!stdin_filehandle)
			stdin_filehandle = new FileHandle(new StdDev(false));
		stdin_filehandle->write(reinterpret_cast<void *>(buffer_addr), length);
		return length;
	} else if (handle == 1) {
		if (!stdout_filehandle)
			stdout_filehandle = new FileHandle(new StdDev(true));
		stdout_filehandle->write(reinterpret_cast<void *>(buffer_addr), length);
		return length;
	}
	current->m_file_handles[handle]->write(
		reinterpret_cast<void *>(buffer_addr), length);

	return length;
}

static int sys$open(Process *current, const char *buffer, int flags, [[maybe_unused]] int* errno) {
	(void)flags;
	if (!verify_buffer(current->root_page_level(), (uintptr_t)buffer,
					   PAGE_SIZE)) {
		*errno = EFAULT;
		return -1;
	}

	auto path = VFS::the().parse_path(buffer);
	FileHandle *handle = VFS::the().open_fh(path);
	if (!handle) {
		*errno = ENOENT;
		return -1;
	}

	current->m_file_handles.push(handle);
	int fd = current->m_file_handles.size() - 1;
	return fd;
}

static int sys$lseek(Process *current, int fd, off_t offset, int whence, [[maybe_unused]] int* errno) {
	if ((size_t)fd > (current->m_file_handles.size() - 1)) {
		*errno = EBADF;
		return -1;
	}

	if (whence > SEEK_END) {
		*errno = EINVAL;
		return -1;
	}

	auto fh = current->m_file_handles[fd];
	fh->seek(offset, static_cast<SeekMode>(whence));

	return fh->tell();
}

static void *sys$mmap(Process *current, void *address, size_t length, [[maybe_unused]] int* errno) {
	auto current_pd = current->root_page_level();
	size_t addr = reinterpret_cast<size_t>(address);
	if (!length)
		return nullptr;

	uintptr_t free_page = Paging::the()->allocator()->find_free_pages((length+4095)/PAGE_SIZE);

	for (size_t extent = 0; extent < (length+4095)/PAGE_SIZE; extent++) {
		if (current_pd->get_page_flags(addr+(extent*PAGE_SIZE)) & (PAEPageFlags::Present))
			return nullptr;

		Paging::the()->map_page(*current_pd, addr+(extent*PAGE_SIZE), free_page+(extent*PAGE_SIZE),
								PAEPageFlags::User | PAEPageFlags::Present |
									PAEPageFlags::Write);
	}
#if 0
	for (size_t page = Paging::page_align(addr);
		 page < Paging::page_align(addr_end); page += PAGE_SIZE) {
		if (current_pd->get_page_flags(page) & (PAEPageFlags::Present))
			return nullptr;

		uintptr_t free_page = Paging::the()->allocator()->find_free_page();
		Paging::the()->map_page(*current_pd, page, free_page,
								PAEPageFlags::User | PAEPageFlags::Present |
									PAEPageFlags::Write);
	}
#endif

	return (void*)addr;
}

static size_t sys$exec(Process *current, const char *buffer,
					   const char **argv, [[maybe_unused]] int* errno) {
	if (!verify_buffer(current->root_page_level(), (uintptr_t)buffer, 255)) {
		*errno = EFAULT;
		return -1;
	}
	printk("exec %s\n", buffer);

	size_t argc = 0;
	if (argv) {
		while (argv[argc++])
			;
		argc--;
	}

	char** argv_copy = nullptr;
	if (argc) {
		argv_copy = new char*[argc];
		for (size_t i = 0; i < argc; i++) {
			size_t arg_sz = strlen(argv_copy[i]);
			argv_copy[i] = new char[arg_sz];
			memcpy(argv_copy[i], argv[i], sizeof(char)*arg_sz);
		}
	}
	// Lazily just kill the process, copy the pid, and file descriptors.
	auto proc = Process::create_user(buffer, argc, const_cast<const char**>(argv_copy));
	if (!proc)
		return -1;

	auto next = current->next;
	auto prev = current->prev;
	auto parent = current->parent();
	auto pid = current->pid;
	current->dont_goto_me = true;
	current->kill();
	printk("killing original %x\n", current->pid);
	current->collapse_cow();

	for (size_t i = 0; i < current->m_file_handles.size(); i++) {
		proc->m_file_handles.push(current->m_file_handles[i]);
	}
	// This kinda sucks
	*current = *proc;
	current->next = next;
	current->prev = prev;
	current->set_parent(parent);
	current->pid = pid;

	// FIXME preserve children
	Process::reentry();
	return 0;
}

extern "C" void sched_sled(uintptr_t rsp) { printk("sched_sled %x\n", rsp); Process::sched(rsp); }

static int sys$sleep(Process *current, size_t ms, [[maybe_unused]] int* errno) {
	printk("block_help\n");
	Process *proc = current;
	proc->block_on_sleep(ms);
	uintptr_t rip = get_rip();
	asm volatile("sti");
	if (proc->state() == ProState::Blocked) {
		uint64_t *sp;
		asm volatile("mov %%rsp, %0" : "=a"(sp) :);
		x64_syscall_block_help(sp, rip);
	}
	asm volatile("cli");

	return 0;
}

static int sys$waitpid(Process *current, pid_t pid, int *wstatus, int options, [[maybe_unused]] int* errno) {
	// FIXME: Implement the functionality for pid <= 0
	assert(pid > 0);
	(void)options;

	printk("block_help\n");
#if 0
	Process *proc = current;

	// FIXME infinite loop
	asm volatile("sti");
	while ((proc->next || proc->state() == ProState::Dead) &&
		   proc->pid != pid) {
		proc = proc->next;
	}
	asm volatile("cli");
#endif
	printk("waitpid begin\n");
	Process** p = process_pid_map->get(pid);
	if (!p) {
		printk("Waitpid failed %d\n", pid);
		return -1;
	}

	current->block_on_proc(*p);
	uintptr_t rip = get_rip();
	if ((*p)->state() == ProState::Running) {
		uint64_t *sp;
		asm volatile("mov %%rsp, %0" : "=a"(sp) :);
		x64_syscall_block_help(sp, rip);
	}
	auto current_pgl = (PageLevel *)get_cr3();
	Paging::switch_page_directory(current->root_page_level());
	*wstatus = (*p)->exit_code;
	Paging::switch_page_directory(current_pgl);
	printk("Waitpid end %d\n", pid);
	return pid;
}

void handler(SyscallReg *regs) {
	// auto current_pgl = (PageLevel*)get_cr3();
	// Paging::switch_page_directory(Paging::the()->kernel_root_directory());

	current_process->m_saved_user_stack_ksyscall = ksyscall_data.user_stack;
	// :)
	decltype(regs->rax) return_value = 0;
	decltype(regs->rax) syscall_no = regs->rax;
	// FIXME This breaks one of the rules POSIX sets for errno. However, I'm lazy.
	int errno = 0;
	Process *current = current_process;

	switch (syscall_no) {
	case SYS_EXIT: {
		sys$exit(current, (int)regs->rbx, &errno);
	} break;
	case SYS_FORK: {
		return_value = sys$fork(current, regs, &errno);
	} break;
	case SYS_OPEN: {
		return_value =
			sys$open(current, reinterpret_cast<const char *>(regs->rbx),
					 static_cast<int>(regs->rdx), &errno);
	} break;
	case SYS_WRITE: {
		return_value = sys$write(current, regs->rbx, regs->rdx, regs->rdi, &errno);
	} break;
	case SYS_READ: {
		return_value = sys$read(current, static_cast<int>(regs->rbx),
								reinterpret_cast<char *>(regs->rdx),
								static_cast<size_t>(regs->rdi), &errno);
	} break;
	case SYS_LSEEK: {
		return_value = sys$lseek(current, static_cast<int>(regs->rbx),
								 static_cast<off_t>(regs->rdx),
								 static_cast<int>(regs->rdi), &errno);
	} break;
	case SYS_MMAP: {
		return_value = reinterpret_cast<size_t>(
			sys$mmap(current, reinterpret_cast<void *>(regs->rbx), regs->rdx, &errno));
	} break;
	case SYS_EXEC: {
		return_value =
			sys$exec(current, reinterpret_cast<const char *>(regs->rbx),
					 reinterpret_cast<const char **>(regs->rdx), &errno);
	} break;
	case SYS_WAITPID: {
		return_value = static_cast<size_t>(sys$waitpid(
			current, static_cast<pid_t>(regs->rbx),
			reinterpret_cast<int *>(regs->rdx), static_cast<int>(regs->rdi), &errno));
	} break;
	case SYS_SLEEP: {
		return_value = static_cast<size_t>(
			sys$sleep(current, static_cast<size_t>(regs->rbx), &errno));
	} break;
	default:
		printk("Unknown syscall %d\n", syscall_no);
		break;
	}

	// FIXME This breaks one of the rules POSIX sets for errno. However, I'm lazy.
	regs->rbx = errno;
	regs->rax = return_value;
	write_msr(0xC0000102, (uint32_t)(uintptr_t)&ksyscall_data,
			  (uint32_t)((uintptr_t)&ksyscall_data >> 32));

	// Paging::switch_page_directory(current_pgl);
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

extern "C" void syscall_proxy(SyscallReg *regs) { Syscall::handler(regs); }
