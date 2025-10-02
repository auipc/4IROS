#include <kernel/arch/amd64/x64_sched_help.h>
#include <kernel/driver/TTY.h>
#include <kernel/shedule/proc.h>
#include <kernel/unix/ELF.h>
#include <kernel/util/HashTable.h>
#include <kernel/vfs/stddev.h>
#include <kernel/vfs/vfs.h>

#define KSTACK_SIZE (4096 * 10)
Process *current_process = nullptr;
extern TSS tss;
uint64_t s_pid = 0;

HashTable<Process *> *process_pid_map;

static TTY *shared_fake;
bool sched_enabled = true;

Process::Process(const char *name) : pid(s_pid++) {
	m_cwd = VFS::the().parse_path("/");
	m_name = strdup(name);
	m_saved_fpu = (uint8_t *)kmalloc_really_aligned(512, 16);
	if (!shared_fake)
		shared_fake = new TTY();
	m_file_handles[m_file_handle_counter++] = new FileHandle(shared_fake);
	// FIXME make a path class with an explicit constructor so I don't have to
	// keep doing this
	auto serial_path = VFS::the().parse_path("/dev/serial");
	auto path = VFS::the().parse_path("/dev/tty");
	m_file_handles[m_file_handle_counter++] = VFS::the().open_fh(path);
	m_file_handles[m_file_handle_counter++] = VFS::the().open_fh(path);

	if (!process_pid_map)
		process_pid_map = new HashTable<Process *>();

	m_cow_tbl = new HashTable<CoWPage *>();
	process_pid_map->push(pid, this);
	// FIXME clear FPU registers and state before calling this
	initialize_fpu(m_saved_fpu);
}

Process::Process(Process *p) : pid(s_pid++) {
	m_cow_tbl = new HashTable<CoWPage *>();
	copy_handles(p);
}

Process::~Process() {}

void Process::yield() {}

void kgrim() {
	asm volatile("sti");
	while (1) {
		asm volatile("nop");
	}
#if 0
	while(1) {
		asm volatile("cli");
		Process* process_list = current_process->next;
		while (process_list != current_process) {
			Process* cur = process_list;
			Process* next = process_list->next;
			Process* prev = process_list->prev;
			if (cur->state() == ProState::Dead) {
				// FIXME add a mutex for modifying the run queue or process data
				printk("Reaping process %d %s\n", cur->pid, cur->name());
				prev->next = next;
				next->prev = prev;

				// FIXME notify the processes blocked on this process that the process has been deleted
				// or have a ref counter for blocks idk
				//delete cur;
			}
			process_list = next;
		}
		asm volatile("sti");
		// FIXME yield here
		// We'll use the return value pushed onto the stack as the address we return to once yield is finished
		for (int i = 0; i < 5000; i++) {
			asm volatile("nop");
		}
		//yield();
	}
#endif
}

extern void parse_symtab_task();
// FIXME separate kernel process PIDs from user ones
void Process::init() {
	// Process *proc = Process::create_user("DOOM");
	Process *proc = Process::create_user("init");
	proc->next = proc;
	proc->prev = proc;
	current_process = proc;

	ksyscall_data.stack = proc->kern_stack_top();
	write_msr(0xC0000102, (uint32_t)(uintptr_t)&ksyscall_data,
			  (uint32_t)((uintptr_t)&ksyscall_data >> 32));
	tss.rsp[0] = proc->kern_stack_top();
	ksyscall_data.fxsave_region =
		reinterpret_cast<uint8_t **>(&current_process->m_saved_fpu);
#if 0
	// Delegated because it isn't particularly important
	auto grim = Process::create((void*)kgrim);
	auto kproc = Process::create((void*)parse_symtab_task);

	proc->next = grim;
	grim->next = kproc;
	kproc->next = proc;

	proc->prev = kproc;
	grim->prev = proc;
	kproc->prev = grim;
#endif

	x64_switch((void *)proc->kern_stack_top(),
			   (uint64_t)proc->root_page_level());
}

void Process::register_signal_handler(void *ptr) {
	m_sig_handler = (uintptr_t)ptr;
}

void Process::kill_process(Process *p) {
	if (p->m_children.size())
		panic("Woah\n");
	delete p;
}

void Process::sched(uintptr_t rsp, uintptr_t user_rsp,
					bool is_timer_triggered) {
	(void)is_timer_triggered;
	if (current_process == current_process->next)
		return;
	if (rsp) {
		current_process->m_kern_stack_top = rsp;
		// FIXME need a better way of detecting when we are in the kernel
		if (user_rsp > 0x8000000000)
			current_process->m_stack_top = user_rsp;
		current_process->m_saved_user_stack_ksyscall = ksyscall_data.user_stack;
	}
	Process *old = current_process;
	sched_enabled = false;
	asm volatile("sti");
	do {
		current_process = current_process->next;
		if (current_process->m_state == ProState::Blocked) {
			current_process->poll_is_blocked();
		}
	} while (current_process->m_state != ProState::Running);
	asm volatile("cli");
	sched_enabled = true;
	if (current_process == old) {
		current_process = old;
		return;
	}

	assert(current_process);
	ksyscall_data.stack = current_process->m_actual_kern_stack_top;
	ksyscall_data.user_stack = current_process->m_saved_user_stack_ksyscall;
	ksyscall_data.fxsave_region =
		reinterpret_cast<uint8_t **>(&current_process->m_saved_fpu);
	// FIXME remove this
	write_msr(0xC0000102, (uint32_t)(uintptr_t)&ksyscall_data,
			  (uint32_t)((uintptr_t)&ksyscall_data >> 32));
	tss.rsp[0] = current_process->m_actual_kern_stack_top;
	x64_switch((void *)current_process->m_kern_stack_top,
			   (uint64_t)current_process->m_pglv);
}

// FIXME POSIX mandates we queue signals (also that the queue acts as a set)
void Process::send_signal(int sig) {
	(void)sig;
	if (m_in_signal)
		return;
	m_in_signal = true;
	printk("send_signal\n");
	m_signal_saved_kern_stack_top = m_kern_stack_top;
	m_signal_saved_actual_kern_stack_top = m_actual_kern_stack_top;
	// FIXME free this
	uint64_t *new_stack = (uint64_t *)kmalloc_really_aligned(PAGE_SIZE, 16);
	uint64_t *new_stack_top = (uint64_t *)(((uint64_t)new_stack) + PAGE_SIZE);
	uint64_t *stp = x64_create_user_task_stack((uint64_t *)(new_stack_top),
											   (uintptr_t)(m_sig_handler),
											   (uint64_t *)(m_stack_top - 128));
	m_kern_stack_top = (uint64_t)stp;
	m_actual_kern_stack_top = (uint64_t)new_stack_top;

	if (m_state == ProState::Blocked) {
		m_state = ProState::Running;
		was_blocked = true;
	}
}

void Process::sigret() {
	if (was_blocked) {
		m_state = ProState::Blocked;
		was_blocked = false;
	}

	m_in_signal = false;
	m_kern_stack_top = m_signal_saved_kern_stack_top;
	m_actual_kern_stack_top = m_signal_saved_actual_kern_stack_top;
	Process::sched(0);
}

#if 0
void Process::sched(uintptr_t rsp) {
	if (current_process == current_process->next)
		return;
	if (rsp) {
		current_process->m_kern_stack_top = rsp;
		current_process->m_saved_user_stack_ksyscall = ksyscall_data.user_stack;
	}
	Process *old = current_process;
	disable_timer();
	asm volatile("sti");
	do {
		current_process = current_process->next;
		printk("%d\n", current_process->pid);
		if (current_process->m_state == ProState::Blocked) {
			current_process->poll_is_blocked();
		}
	} while (current_process->m_state != ProState::Running);
	asm volatile("cli");
	enable_timer();
	if (current_process == old) {
		current_process = old;
		return;
	}

	assert(current_process);
	ksyscall_data.stack = current_process->m_actual_kern_stack_top;
	ksyscall_data.user_stack = current_process->m_saved_user_stack_ksyscall;
	ksyscall_data.fxsave_region =
		reinterpret_cast<uint8_t **>(&current_process->m_saved_fpu);
	// FIXME remove this
	write_msr(0xC0000102, (uint32_t)(uintptr_t)&ksyscall_data,
			  (uint32_t)((uintptr_t)&ksyscall_data >> 32));
	tss.rsp[0] = current_process->m_actual_kern_stack_top;
	x64_switch((void *)current_process->m_kern_stack_top,
			   (uint64_t)current_process->m_pglv);
}
#endif

void Process::reentry() {
	assert(current_process);
	ksyscall_data.stack = current_process->m_actual_kern_stack_top;
	ksyscall_data.user_stack = current_process->m_saved_user_stack_ksyscall;
	ksyscall_data.fxsave_region =
		reinterpret_cast<uint8_t **>(&current_process->m_saved_fpu);
	// FIXME remove this
	write_msr(0xC0000102, (uint32_t)(uintptr_t)&ksyscall_data,
			  (uint32_t)((uintptr_t)&ksyscall_data >> 32));
	tss.rsp[0] = current_process->m_actual_kern_stack_top;
	x64_switch((void *)current_process->m_kern_stack_top,
			   (uint64_t)current_process->m_pglv);
}

void Process::collapse_cow() {}

#if 0
void Process::resolve_cow_recurse(RootPageLevel *level, Process *current,
								  uintptr_t fault_addr) {
	if (!current)
		return;
	CoWInfo *inf = current->cow_table->get(fault_addr);
	if (!inf)
		return;
	Paging::the()->resolve_cow_fault(*level, *current->root_page_level(),
									 fault_addr, true);
	current->cow_table->remove(fault_addr);
	for (size_t i = 0; i < current->m_children.size(); i++) {
		resolve_cow_recurse(level, current->m_children[i], fault_addr);
	}
}

#endif
extern char *temp_area[6];
int Process::resolve_cow(Process *current, uintptr_t fault_addr) {
	CoWPage **cow_page = current->m_cow_tbl->get(fault_addr);
	if (!cow_page)
		return 0;

	auto &pdpt = current->m_pglv->entries[(fault_addr >> 39) & 0x1ff];
	auto pdpte_lvl = pdpt.fetch();
	auto &pdpte = pdpte_lvl->entries[(fault_addr >> 30) & 0x1ff];
	auto pdt_lvl = pdpte.fetch();
	auto &pdt = pdt_lvl->entries[(fault_addr >> 21) & 0x1ff];
	auto pt_lvl = pdt.fetch();
	auto &pt = pt_lvl->entries[(fault_addr >> 12) & 0x1ff];

	if (((*cow_page)->refs) == 1) {
		(*cow_page)->refs--;
		current->m_cow_tbl->remove(fault_addr);
		printk("Removing page\n");
		pt.pdata |= PAEPageFlags::Write;
		pdt.commit(*pt_lvl.ptr());
		delete *cow_page;
		current->m_cow_tbl->remove(fault_addr);
		asm volatile("invlpg %0" ::"m"(fault_addr));
		return 1;
	}

	auto free_page = Paging::the()->allocator()->find_free_page();
	Paging::the()->map_page(*(RootPageLevel *)get_cr3(),
							(uintptr_t)temp_area[0], (*cow_page)->skel.addr());
	Paging::the()->map_page(*(RootPageLevel *)get_cr3(),
							(uintptr_t)temp_area[1], free_page);
	memcpy((void *)temp_area[1], (void *)temp_area[0],
		   sizeof(char) * PAGE_SIZE);
	pt.copy_flags((*cow_page)->skel.pdata);
	printk("old %x new %x\n", pt.addr(), free_page);
	pt.set_addr(free_page);
	pt.pdata |= PAEPageFlags::Write;

	pdt.commit(*pt_lvl.ptr());
	(*cow_page)->refs--;
	current->m_cow_tbl->remove(fault_addr);
	asm volatile("invlpg %0" ::"m"(fault_addr));
	return 1;
}

void __kprocess_failsafe_panic() {
	panic("A kernel process wasn't properly killed");
}

Process *Process::create(void *func_ptr) {
	Process *proc = new Process("kernel_proc");
	proc->m_is_kernel_task = true;
	// auto current_pd = (RootPageLevel*)get_cr3();
	// RootPageLevel* pgl = Paging::the()->clone(*(RootPageLevel*)get_cr3());
	// Paging::switch_page_directory(pgl);

	uintptr_t stack_begin = (uintptr_t)kmalloc_really_aligned(KSTACK_SIZE, 16);
	uintptr_t stack_end = stack_begin + KSTACK_SIZE;
	stack_end -= 8;
	*(uint64_t *)stack_end = (uint64_t)&__kprocess_failsafe_panic;

	uint64_t *stp = x64_create_kernel_task_stack((uint64_t *)stack_end,
												 (uintptr_t)func_ptr);
	// Push failsafe return val onto the stack
	// This prevents kernel processes from returning to whatever garbage is on
	// the stack
	proc->m_pglv = (RootPageLevel *)get_cr3();
	proc->m_stack_bot = stack_begin;
	proc->m_stack_top = (uintptr_t)stp;
	proc->m_kern_stack_bot = stack_begin;
	proc->m_kern_stack_top = (uintptr_t)stp;
	proc->m_actual_kern_stack_top = stack_end;
	return proc;
}

Process *Process::create_user(const char *path, size_t argc,
							  const char **argv) {
	auto path_vec = VFS::the().parse_path(path);
	auto exec_file = VFS::the().open(path_vec);
	if (!exec_file)
		return nullptr;
	Process *proc = new Process(path);
	auto file_sz = exec_file->size();
	char *buffer = new char[file_sz];
	exec_file->read(buffer, file_sz);
	ELFHeader64 *ehead = reinterpret_cast<ELFHeader64 *>(buffer);
	const ELFSectionHeader64 *esecheads =
		reinterpret_cast<const ELFSectionHeader64 *>(buffer + ehead->phtable);

	auto current_pd = (RootPageLevel *)get_cr3();
	Pair<RootPageLevel *, void *> pgl = Paging::the()->clone(
		*(RootPageLevel *)Paging::the()->kernel_root_directory());

	proc->m_pglv_virt = pgl.right;

	Paging::the()->switch_page_directory(pgl.left);

	for (int i = 0; i < ehead->phnum; i++) {
		const auto sechead = esecheads[i];
		if (sechead.seg != 1)
			continue;
		uint64_t flags =
			PAEPageFlags::Present | PAEPageFlags::Write | PAEPageFlags::User;
		// Disable execution if no ELF executable flag
		// flags |= ((uint64_t)!(sechead.flags & 1ull))<<63ull;
		// Writable? Ideally, panic if both eXecute and Writable are enabled.
		// flags |= (!!(sechead.flags & 2ull))<<1ull;

		auto vaddr = sechead.vaddr; // 0x500000+sechead.off-0x1000;
		for (size_t j = 0; j < sechead.memsz; j += PAGE_SIZE) {
			auto paddr = Paging::the()->allocator()->find_free_page();
			Paging::the()->map_page_user(*pgl.left, vaddr + j, paddr,
										 PAEPageFlags::User | flags);
		}
		memset((char *)vaddr, 0, sechead.memsz);
		memcpy((char *)vaddr, buffer + sechead.off, sechead.filesz);
	}

	uintptr_t stack_begin = 0x8000000000 + 0x400000;
	uintptr_t stack_end = stack_begin + STACK_SIZE;

	uintptr_t kstack_begin = (uintptr_t)kmalloc_really_aligned(KSTACK_SIZE, 16);
	uintptr_t kstack_end = kstack_begin + KSTACK_SIZE;

	for (int i = 0; i < STACK_SIZE + 4096; i += 4096) {
		uintptr_t stack_page = Paging::the()->allocator()->find_free_page();
		Paging::the()->map_page_user(*pgl.left, stack_begin + i, stack_page,
									 PAEPageFlags::User | PAEPageFlags::Write |
										 PAEPageFlags::Present);
		memset((char *)(stack_begin + i), 0, PAGE_SIZE);
	}

	size_t argv_ptr_size = sizeof(uint64_t) * argc;
	stack_end -= argv_ptr_size;
	for (size_t i = 0; i < argc; i++) {
		stack_end -= strlen(argv[i]) + 1;
	}
	uintptr_t argv_begin = stack_end;
	uintptr_t argv_ptr = argv_begin + argv_ptr_size;
	for (size_t i = 0; i < argc; i++) {
		*(uint64_t *)(argv_begin + (sizeof(uint64_t) * i)) = argv_ptr;
		memcpy((void *)argv_ptr, argv[i], strlen(argv[i]));
		argv_ptr += strlen(argv[i]) + 1;
	}
	stack_end -= stack_end % 16;

	stack_end -= 8;
	*(uint64_t *)stack_end = argv_begin;
	stack_end -= 8;
	*(uint64_t *)stack_end = argc;

	uint64_t *stp = x64_create_user_task_stack((uint64_t *)kstack_end,
											   (uintptr_t)(ehead->entry),
											   (uint64_t *)stack_end);

	proc->m_pglv = pgl.left;
	proc->m_stack_bot = stack_begin;
	proc->m_stack_top = stack_end;
	proc->m_kern_stack_bot = kstack_begin;
	proc->m_kern_stack_top = (uintptr_t)stp;
	proc->m_actual_kern_stack_top = kstack_end;
	Paging::switch_page_directory(current_pd);
	delete[] buffer;
	return proc;
}

// FIXME SSE complains because the stack is misaligned. -mstackrealign is a
// bandaid fix
Process *Process::fork(SyscallReg *regs) {
	uintptr_t kstack_begin = (uintptr_t)kmalloc_really_aligned(KSTACK_SIZE, 16);
	uint64_t *kstack_end = (uint64_t *)(kstack_begin + KSTACK_SIZE);
	// Process* new_proc = new Process(this);
	Process *new_proc = new Process(m_name);
	new_proc->m_parent = this;
	m_children.push(this);

#if 1
	Pair<RootPageLevel *, void *> pgl =
		Paging::the()->cow_clone(m_cow_tbl, new_proc->m_cow_tbl, *m_pglv);
#else
	Pair<RootPageLevel *, void *> pgl =
		Paging::the()->clone_for_fork_test(*m_pglv);
#endif
	new_proc->m_pglv = pgl.left;
	new_proc->m_pglv_virt = pgl.right;

	new_proc->m_stack_bot = m_stack_bot;
	new_proc->m_stack_top = (uintptr_t)ksyscall_data.user_stack;
	new_proc->m_actual_kern_stack_top = (uintptr_t)kstack_end;

	kstack_end--;
	*kstack_end = 0x1b;
	kstack_end--;
	*kstack_end = new_proc->m_stack_top;
	kstack_end--;
	*kstack_end = regs->r11;
	kstack_end--;
	*kstack_end = 0x23;
	kstack_end--;
	*kstack_end = regs->rcx;
	kstack_end--;
	*kstack_end = regs->rbp;

	kstack_end--;
	*kstack_end = regs->r8;
	kstack_end--;
	*kstack_end = regs->r9;
	kstack_end--;
	*kstack_end = regs->r10;
	kstack_end--;
	*kstack_end = regs->r11;
	kstack_end--;
	*kstack_end = regs->r12;
	kstack_end--;
	*kstack_end = regs->r13;
	kstack_end--;
	*kstack_end = regs->r14;
	kstack_end--;
	*kstack_end = regs->r15;
	kstack_end--;
	*kstack_end = regs->rdi;
	kstack_end--;
	*kstack_end = regs->rsi;
	kstack_end--;
	*kstack_end = regs->rdx;
	kstack_end--;
	*kstack_end = regs->rcx;
	kstack_end--;
	*kstack_end = regs->rbx;
	// RAX
	kstack_end--;
	*kstack_end = 0;

	new_proc->m_kern_stack_bot = kstack_begin;
	new_proc->m_kern_stack_top = (uintptr_t)kstack_end;
	new_proc->clear_file_handles();
	for (size_t i = 0; i < m_file_handle_counter; i++) {
		auto handle = new FileHandle(m_file_handles[i]->node());
		handle->seek(m_file_handles[i]->tell(), SEEK_SET);
		new_proc->push_handle(handle);
	}
	return new_proc;
}

void Process::block_on_proc(Process *p) {
	m_blocker.blocked_on = Blocker::ProcessBlock;
	m_blocker.blocked_process = p;
	m_state = ProState::Blocked;
}

void Process::block_on_sleep(size_t us) {
	m_blocker.blocked_on = Blocker::SleepBlock;
	m_blocker.sleep_waiter_us = global_waiter_time + us;
	m_state = ProState::Blocked;
}

void Process::block_on_handle_read(FileHandle *handle) {
	m_blocker.blocked_on = Blocker::FileIOReadBlock;
	m_blocker.file.blocked_set = new Vec<FileHandle *>();
	(*m_blocker.file.blocked_set).push(handle);
	m_state = ProState::Blocked;
}

void Process::block_on_handle_read_mul(Vec<FileHandle *> handles) {
	m_blocker.blocked_on = Blocker::FileIOReadBlock;
	m_blocker.file.blocked_set = new Vec<FileHandle *>();
	for (size_t i = 0; i < handles.size(); i++) {
		(*m_blocker.file.blocked_set).push(handles[i]);
	}
	m_state = ProState::Blocked;
}

void Process::block_on_handle_write(FileHandle *handle, size_t sz) {
	m_blocker.blocked_on = Blocker::FileIOWriteBlock;
	m_blocker.file.blocked_set = new Vec<FileHandle *>();
	(*m_blocker.file.blocked_set).push(handle);
	m_blocker.file.blocked_sz = sz;
	m_state = ProState::Blocked;
}

void Process::poll_is_blocked() {
	switch (m_blocker.blocked_on) {
	case Blocker::ProcessBlock:
		if (m_blocker.blocked_process->m_state == ProState::Dead) {
			m_state = ProState::Running;
		}
		break;
	case Blocker::SleepBlock:
		// Oh, waiter. Check, please.
		if (global_waiter_time >= m_blocker.sleep_waiter_us) {
			m_state = ProState::Running;
		}
		break;
	case Blocker::FileIOReadBlock:
		for (size_t i = 0; i < m_blocker.file.blocked_set->size(); i++) {
			if (!(*m_blocker.file.blocked_set)[i]->node()->check_blocked()) {
				m_state = ProState::Running;
				delete m_blocker.file.blocked_set;
			}
		}
		break;
	case Blocker::FileIOWriteBlock:
		if (!m_blocker.file.blocked_set) {
			m_state = ProState::Running;
			break;
		}
		// AFAIK there's nothing that would cause a write block for multiple
		// files.
		if (!m_blocker.file.blocked_set->size() ||
			!(*m_blocker.file.blocked_set)[0]->node()->check_blocked_write(
				m_blocker.file.blocked_sz)) {
			m_state = ProState::Running;
			delete m_blocker.file.blocked_set;
		}
		break;
	default:
		panic("FIXME");
		break;
	}
}

void Process::del_handle(int fd) {
	delete m_file_handles[fd];
	m_file_handles[fd] = nullptr;
}

int Process::push_handle(FileHandle *handle) {
	assert(handle);
	// Start reusing handles
	if ((m_file_handle_counter) >= FD_LIM) {
		for (size_t i = 0; i < FD_LIM; i++) {
			if (!m_file_handles[i]) {
				m_file_handle_counter++;
				m_file_handles[i] = handle;
				return i;
			}
		}
		panic("Out of file descriptors! Report this to the process?\n");
	}
	int handle_index = m_file_handle_counter++;
	m_file_handles[handle_index] = handle;
	return handle_index;
}
