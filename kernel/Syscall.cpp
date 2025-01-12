#include <errno.h>
#include <kernel/Syscall.h>
#include <kernel/arch/amd64/x64_sched_help.h>
#include <kernel/arch/x86_common/IO.h>
#include <kernel/mem/Paging.h>
#include <kernel/printk.h>
#include <kernel/shedule/proc.h>
#include <kernel/vfs/stddev.h>
#include <kernel/vfs/vfs.h>
#include <priv/common.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

extern HashTable<Process *> *process_pid_map;
namespace Syscall {

static void sys$exit(Process *current, int status,
					 [[maybe_unused]] int *errno) {
	//printk("exit %d\n", status);
	current->exit_code = status;
	current->kill();
	Paging::the()->switch_page_directory(Paging::the()->s_kernel_page_directory);
	Paging::the()->release(*current->root_page_level());
	Process::sched(0);
}

static size_t sys$fork(Process *current, SyscallReg *regs,
					   [[maybe_unused]] int *errno) {
	Process *child = current->fork(regs);
	child->next = current_process->next;
	child->prev = current_process;
	current_process->next = child;
	info("Run queue: \n");
	printk("%d", current_process->pid);
	Process *p = current_process->next;
	while (p != current_process) {
		p = p->next;
		if (p->state() != ProState::Dead) {
			printk(", %d(%s[%d])", p->pid, p->name(), p->state());
		}
	}
	printk("\n");
	printk("fork %d\n", child->pid);
	return child->pid;
}

// FIXME If the NUL terminator is missing we could end up accidentally reading
// kernel memory if it directly bordered userspace memory. We should probably
// turn on the write protect bit in CR0 so we don't overwrite read only user
// memory.
// FIXME Make verify_buffer work with CoW
static bool verify_buffer(RootPageLevel *pd, uintptr_t buffer_addr,
						  size_t size) {
	uintptr_t start_addr = buffer_addr & ~(PAGE_SIZE - 1);

	for (uintptr_t i = start_addr; i < size + (PAGE_SIZE - 1); i += PAGE_SIZE) {
		auto flags = pd->get_page_flags(buffer_addr + i);
		if (!(flags & PAEPageFlags::Present) || !(flags & PAEPageFlags::User))
			return false;
	}
	return true;
}

static bool verify_buffer_write(Process *current, RootPageLevel *pd,
								uintptr_t buffer_addr, size_t size) {
	(void)current;
	uintptr_t start_addr = buffer_addr & ~(PAGE_SIZE - 1);

	for (uintptr_t addr = start_addr;
		 addr < start_addr + size + (PAGE_SIZE - 1); addr += PAGE_SIZE) {
		auto flags = pd->get_page_flags(addr);

		info("%x %x %d %d %d %d\n", addr, current->cow_table->get(addr),
			   flags, (flags & PAEPageFlags::Present),
			   (flags & PAEPageFlags::User), (flags & PAEPageFlags::Write));
		if (!(flags & PAEPageFlags::Present) || !(flags & PAEPageFlags::User) ||
			!(flags & PAEPageFlags::Write)) {
			if (!current->cow_table->get(addr))
				return false;
		}
	}

	for (uintptr_t addr = start_addr; addr < size + (PAGE_SIZE - 1);
		 addr += PAGE_SIZE) {
		info("reslve cow\n");
		Process::resolve_cow(current, addr);
	}

	return true;
}

KSyscallData data;
// FIXME: Two processes can share the same VFSNode which causes problems (i.e.
// processes might snipe values from the keyboard buffer and the value is lost
// for another process reading it).
static int sys$read(Process *current, int fd, const char *buffer, size_t count,
					[[maybe_unused]] int *errno) {
	// FIXME check writable
	if (!verify_buffer(current->root_page_level(), (uintptr_t)buffer, count)) {
		*errno = EFAULT;
		printk("verify buffer failed %x\n", buffer);
		return -1;
	}

	if ((size_t)fd > (current->m_file_handles.size() - 1)) {
		printk("Bad FD\n");
		*errno = EBADF;
		return -1;
	}

	auto fh = current->m_file_handles[fd];
	if (!fh || !fh->node()) {
		printk("NO FILE\n");
		return -1;
	}
	fh->block_if_required(count);

	// FIXME block_on_handle crashes
	current->block_on_handle_read(fh);
#if 0
	uintptr_t rip = get_rip();

	while (fh->check_blocked()) {
		uint64_t *sp;
		asm volatile("mov %%rsp, %0" : "=a"(sp) :);
		x64_syscall_block_help(sp, rip);
	}
#else
	asm volatile("sti");
	while (fh->check_blocked()) {
	}
	asm volatile("cli");
#endif

	current->set_state(ProState::Running);

	return fh->read((void *)buffer, count);
}

// FIXME If writing a large length causes the node to block (i.e. needs more space) 
// and there's no way to rectify it, it'll block forever.
// The fix is moving blocking code to drivers that need it
static size_t sys$write(Process *current, uint32_t handle,
						uintptr_t buffer_addr, size_t length,
						[[maybe_unused]] int *errno) {
	auto current_pd = current->root_page_level();

	if (!verify_buffer(current_pd, buffer_addr, length)) {
		printk("Invalid buffer\n");
		*errno = EFAULT;
		return -1;
	}

	if (current->m_file_handles.size() - 1 < handle) {
		printk("File handle not open!\n");
		return -1;
	}

	// FIXME stderr is fucked
	if (handle == 2) {
		for (size_t i = 0; i < length; i++) {
			printk("%c", ((char*)(buffer_addr))[i]);
		}
		return length;
	}

	if (!current->m_file_handles[handle]) panic("Missing file handle");

	auto fh = current->m_file_handles[handle];
	current->block_on_handle_write(fh, length);
	asm volatile("sti");
	while (fh && fh->check_blocked_write(length)) {
	}
	asm volatile("cli");

	current->set_state(ProState::Running);

	fh->write(
		reinterpret_cast<void *>(buffer_addr), length);

	return length;
}

static int sys$open(Process *current, const char *buffer, int flags,
					[[maybe_unused]] int *errno) {
	(void)flags;
	if (!verify_buffer(current->root_page_level(), (uintptr_t)buffer,
					   PAGE_SIZE)) {
		info("Invalid buffer\n");
		*errno = EFAULT;
		return -1;
	}

	auto path = VFS::the().parse_path(buffer);
	FileHandle *handle = VFS::the().open_fh(path);
	if (!handle) {
		if (flags & (O_WRONLY|O_CREAT)) {
			printk("open %x\n", VFS::the().get_root_fs()->mounted_filesystem()->create(buffer));
			handle = VFS::the().open_fh(path);
		} else {
			printk("NO SUCH FILE %s\n", buffer);
			*errno = ENOENT;
			return -1;
		}
	}

	if (flags & O_APPEND) {
		handle->seek(0, (SeekMode)SEEK_END);
	} else if (flags & O_WRONLY) {
		handle->node()->set_size(0);
	}

	// FIXME use any type of key map to store file handles (so close works)
	current->m_file_handles.push(handle);

	int fd = current->m_file_handles.size() - 1;
	return fd;
}

static int sys$lseek(Process *current, int fd, off_t offset, int whence,
					 [[maybe_unused]] int *errno) {
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

static void *sys$mmap(Process *current, void *address, size_t length,
					  [[maybe_unused]] int *errno) {
	auto current_pd = current->root_page_level();
	uintptr_t addr = reinterpret_cast<uintptr_t>(address);
	if (!length)
		return nullptr;

	uintptr_t free_page = Paging::the()->allocator()->find_free_pages(
		(length + 4095) / PAGE_SIZE);

	for (size_t extent = 0; extent < (length + 4095) / PAGE_SIZE; extent++) {
		if (current_pd->get_page_flags(addr + (extent * PAGE_SIZE)) &
			(PAEPageFlags::Present))
			return nullptr;

		Paging::the()->map_page_user(*current_pd, addr + (extent * PAGE_SIZE),
								free_page + (extent * PAGE_SIZE),
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

	return (void *)addr;
}

static size_t sys$exec(Process *current, const char *buffer, const char **argv,
					   [[maybe_unused]] int *errno) {
	if (!verify_buffer(current->root_page_level(), (uintptr_t)buffer, 255)) {
		*errno = EFAULT;
		return -1;
	}
	info("exec %s\n", buffer);

	size_t argc = 0;
	if (argv) {
		while (argv[argc++])
			;
		argc--;
	}

	char **argv_copy = nullptr;
	if (argc) {
		argv_copy = new char*[argc];
		for (size_t i = 0; i < argc; i++) {
			size_t arg_sz = strlen(argv[i]) + 1;
			argv_copy[i] = new char[arg_sz];
			argv_copy[i][arg_sz - 1] = '\0';
			memcpy(argv_copy[i], argv[i], sizeof(char) * arg_sz);
		}
	}

	for (size_t i = 0; i < argc; i++) {
		info("%s\n", argv_copy[i]);
	}

	// Lazily just kill the process, copy the pid, and file descriptors.
	auto proc = Process::create_user(buffer, argc,
									 const_cast<const char **>(argv_copy));
	if (!proc)
		return -1;

	auto next = current->next;
	auto prev = current->prev;
	auto parent = current->parent();
	auto pid = current->pid;
	current->dont_goto_me = true;
	// current->kill();
	// info("killing original %x\n", current->pid);
	current->collapse_cow();

	proc->m_file_handles = current->m_file_handles;
	// This kinda sucks
	*current = *proc;
	current->next = next;
	current->prev = prev;
	current->set_parent(parent);
	current->pid = pid;
	current->set_state(ProState::Running);

	// FIXME preserve children
	Process::reentry();
	return 0;
}

extern "C" void sched_sled(uintptr_t rsp) { Process::sched(rsp); }

static int sys$sleep(Process *current, size_t ms, [[maybe_unused]] int *errno) {
	info("block_help\n");
	Process *proc = current;
	proc->block_on_sleep(ms);
	uintptr_t rip = get_rip();
	while (proc->state() == ProState::Blocked) {
		uint64_t *sp;
		asm volatile("mov %%rsp, %0" : "=a"(sp) :);
		x64_syscall_block_help(sp, rip);
	}

	return 0;
}

static int sys$waitpid(Process *current, pid_t pid, int *wstatus, int options,
					   [[maybe_unused]] int *errno) {
	// FIXME: Implement the functionality for pid <= 0
	assert(pid > 0);
	(void)options;

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
	Process **p = process_pid_map->get(pid);
	if (!p) {
		printk("Waitpid failed %d\n", pid);
		return -1;
	}

	current->block_on_proc(*p);
	//uintptr_t rip = get_rip();
	asm volatile("sti");
	while (current->state() == ProState::Blocked) {
		current->poll_is_blocked();
	}
	asm volatile("cli");
	auto current_pgl = (PageLevel *)get_cr3();
	Paging::switch_page_directory(current->root_page_level());
	*wstatus = (*p)->exit_code;
	Paging::switch_page_directory(current_pgl);
	//printk("waitpid end on process %s[%d] state is %d %d\n", (*p)->name(), (*p)->pid, (*p)->state(), ProState::Dead);
	return pid;
}

int timeofday() {
	outb(0x70, 00);
	uint8_t seconds_bcd = inb(0x71);
	uint8_t seconds = (((seconds_bcd >> 4) & 0xf) * 10) + (seconds_bcd & 0xf);
	outb(0x70, 2);
	uint8_t minutes_bcd = inb(0x71);
	uint8_t minutes = (((minutes_bcd >> 4) & 0xf) * 10) + (minutes_bcd & 0xf);
	outb(0x70, 4);
	uint8_t hour_bcd = inb(0x71);
	uint8_t hour = (((hour_bcd >> 4) & 0xf) * 10) + (hour_bcd & 0xf);
	outb(0x70, 7);
	uint8_t day_bcd = inb(0x71);
	uint8_t day = (((day_bcd >> 4) & 0xf) * 10) + (day_bcd & 0xf);
	outb(0x70, 8);
	uint8_t month_bcd = inb(0x71);
	uint8_t month = (((month_bcd >> 4) & 0xf) * 10) + (month_bcd & 0xf);
	outb(0x70, 9);
	uint8_t years_bcd = inb(0x71);
	int years = (((years_bcd >> 4) & 0xf) * 10) + (years_bcd & 0xf);
	return ((29 + years) * 31556926) + ((11 + month) * 2629743) +
				 ((day - 1) * 86400) + ((hour + 2) * 3600) +
				 ((minutes + 47) * 60) + seconds;
}

static int sys$gettimeofday(Process *current, timeval *tv, void *,
							[[maybe_unused]] int *errno) {
	if (!verify_buffer(current->root_page_level(), (uintptr_t)tv, PAGE_SIZE)) {
		*errno = EFAULT;
		return -1;
	}

	return timeofday();
}

static int sys$kill(Process* current, pid_t pid, [[maybe_unused]] int *errno) {
	(void)current;
	Process **p = process_pid_map->get(pid);
	if (!p) {
		info("Kill couldn't find pid %d\n", pid);
		*errno = ESRCH;
		return -1;
	}

	info("sys$kill\n");
	if ((*p)->state() != ProState::Dead) {
		(*p)->exit_code = 129;
		(*p)->kill();
		//Paging::the()->release(*(*p)->root_page_level());
	}
	return 0;
}

void handler(SyscallReg *regs) {
	// auto current_pgl = (PageLevel*)get_cr3();
	// Paging::switch_page_directory(Paging::the()->kernel_root_directory());

	current_process->m_saved_user_stack_ksyscall = ksyscall_data.user_stack;
	// :)
	decltype(regs->rax) return_value = 0;
	decltype(regs->rax) syscall_no = regs->rax;
	// FIXME This breaks one of the rules POSIX sets for errno. However, I'm
	// lazy.
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
		return_value =
			sys$write(current, regs->rbx, regs->rdx, regs->rdi, &errno);
	} break;
	case SYS_READ: {
		//printk("sys$read\n");
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
		return_value = reinterpret_cast<size_t>(sys$mmap(
			current, reinterpret_cast<void *>(regs->rbx), regs->rdx, &errno));
	} break;
	case SYS_EXEC: {
		return_value =
			sys$exec(current, reinterpret_cast<const char *>(regs->rbx),
					 reinterpret_cast<const char **>(regs->rdx), &errno);
	} break;
	case SYS_WAITPID: {
		return_value = static_cast<size_t>(
			sys$waitpid(current, static_cast<pid_t>(regs->rbx),
						reinterpret_cast<int *>(regs->rdx),
						static_cast<int>(regs->rdi), &errno));
	} break;
	case SYS_SLEEP: {
		return_value = static_cast<size_t>(
			sys$sleep(current, static_cast<size_t>(regs->rbx), &errno));
	} break;
	case SYS_GETTIMEOFDAY: {
		return_value = static_cast<size_t>(
			sys$gettimeofday(current, reinterpret_cast<timeval *>(regs->rbx),
							 reinterpret_cast<void *>(regs->rdx), &errno));
	} break;
	case SYS_KILL: {
		return_value = static_cast<size_t>(
			sys$kill(current, static_cast<pid_t>(regs->rbx), &errno));
	} break;
	default:
		info("Unknown syscall %d\n", syscall_no);
		break;
	}

	// FIXME This breaks one of the rules POSIX sets for errno. However, I'm
	// lazy.
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
		info("Unknown syscall\n");
		break;
	}

	regs.eax = return_value;
#endif
}

} // namespace Syscall

extern "C" void syscall_proxy(SyscallReg *regs) { Syscall::handler(regs); }
