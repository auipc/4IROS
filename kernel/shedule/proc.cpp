#include <kernel/arch/amd64/x64_sched_help.h>
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

Process::Process() : pid(s_pid++) {
	m_saved_fpu = (uint8_t *)kmalloc_really_aligned(512, 16);
	m_file_handles.push(new FileHandle(new StdDev(false)));
	m_file_handles.push(new FileHandle(new StdDev(true)));
	m_file_handles.push(new FileHandle(new StdDev(true)));
	if (!process_pid_map)
		process_pid_map = new HashTable<Process *>();

	cow_table = new HashTable<CoWInfo>();
	process_pid_map->push(pid, this);
	// FIXME clear FPU registers and state before calling this
	initialize_fpu(m_saved_fpu);
}

Process::Process(Process *p) : pid(s_pid++) {
	m_file_handles = p->m_file_handles;
	cow_table = new HashTable<CoWInfo>();
}

Process::~Process() {
	delete cow_table;
	cow_table = nullptr;
}

void Process::init() {
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
	x64_switch((void *)proc->kern_stack_top(),
			   (uint64_t)proc->root_page_level());
}

void Process::kill_process(Process *p) {
	if (p->m_children.size())
		panic("Woah\n");
	delete p;
}

void Process::sched(uintptr_t rsp) {
	if (current_process == current_process->next)
		return;
	if (rsp) {
		current_process->m_kern_stack_top = rsp;
		current_process->m_saved_user_stack_ksyscall = ksyscall_data.user_stack;
	}
	Process* old = current_process;
	ioapic_set_ioredir_val(21, 1 << 16);
	asm volatile("sti");
	do {
		current_process = current_process->next;
		if (current_process->m_state == ProState::Blocked) {
			current_process->poll_is_blocked();
		}
	} while (current_process->m_state != ProState::Running);
	asm volatile("cli");
	ioapic_set_ioredir_val(21, 48);
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

void Process::collapse_cow() {
}

void Process::resolve_cow_recurse(RootPageLevel* level, Process* current, uintptr_t fault_addr) {
	if (!current) return;
	CoWInfo* inf = current->cow_table->get(fault_addr);
	if (!inf) return;
	Paging::the()->resolve_cow_fault(*level, *current->root_page_level(), fault_addr, true);
	current->cow_table->remove(fault_addr);
	for (size_t i = 0; i < current->m_children.size(); i++) {
		resolve_cow_recurse(level, current->m_children[i], fault_addr);
	}
}

int Process::resolve_cow(Process* current, uintptr_t fault_addr) {
	RootPageLevel* pglv = current->root_page_level();
	printk("%x %d\n", fault_addr, current->pid);
	CoWInfo* inf = current->cow_table->get(fault_addr);
	printk("%x\n", inf);
	if (!inf) return 0;
	if (inf->owner == pglv) {
		for (size_t i = 0; i < current->m_children.size(); i++) {
			resolve_cow_recurse(pglv, current->m_children[i], fault_addr);
		}
		current->cow_table->remove(fault_addr);
		return 1;
	}

	Paging::the()->resolve_cow_fault(*inf->owner, *pglv, fault_addr);
	current->cow_table->remove(fault_addr);
	return 1;
}

Process *Process::create(void *func_ptr) {
	Process *proc = new Process();
	// auto current_pd = (RootPageLevel*)get_cr3();
	// RootPageLevel* pgl = Paging::the()->clone(*(RootPageLevel*)get_cr3());
	// Paging::switch_page_directory(pgl);

	uintptr_t stack_begin = (uintptr_t)kmalloc_really_aligned(KSTACK_SIZE, 16);
	uintptr_t stack_end = stack_begin + KSTACK_SIZE;

	uint64_t *stp = x64_create_kernel_task_stack((uint64_t *)stack_end,
												 (uintptr_t)func_ptr);

	proc->m_pglv = (RootPageLevel *)get_cr3();
	proc->m_stack_top = (uintptr_t)stp;
	proc->m_stack_bot = stack_begin;
	proc->m_kern_stack_top = (uintptr_t)stp;
	proc->m_kern_stack_bot = stack_begin;
	// Paging::switch_page_directory(current_pd);
	return proc;
}

Process *Process::create_user(const char *path, size_t argc,
							  const char **argv) {
	(void)argv;
	Process *proc = new Process();
	auto path_vec = VFS::the().parse_path(path);
	auto exec_file = VFS::the().open(path_vec);
	if (!exec_file)
		return nullptr;
	auto file_sz = exec_file->size();
	char *buffer = new char[file_sz];
	exec_file->read(buffer, file_sz);
	ELFHeader64 *ehead = reinterpret_cast<ELFHeader64 *>(buffer);
	const ELFSectionHeader64 *esecheads =
		reinterpret_cast<const ELFSectionHeader64 *>(buffer + ehead->phtable);

	auto current_pd = (RootPageLevel *)get_cr3();
	RootPageLevel *pgl = Paging::the()->clone_for_fork(
		*(RootPageLevel *)Paging::the()->kernel_root_directory(), true);
	Paging::the()->switch_page_directory(pgl);

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
			Paging::the()->map_page(*pgl, vaddr + j, paddr,
									PAEPageFlags::User | flags);
		}
		memset((char *)vaddr, 0, sechead.memsz);
		memcpy((char *)vaddr, buffer + sechead.off, sechead.filesz);
	}

	uintptr_t stack_begin = 0x400000;
	uintptr_t stack_end = stack_begin + STACK_SIZE;

	uintptr_t kstack_begin = (uintptr_t)kmalloc_really_aligned(KSTACK_SIZE, 16);
	uintptr_t kstack_end = kstack_begin + KSTACK_SIZE;

	for (int i = 0; i < STACK_SIZE + 4096; i += 4096) {
		uintptr_t stack_page = Paging::the()->allocator()->find_free_page();
		Paging::the()->map_page(*pgl, stack_begin + i, stack_page,
								PAEPageFlags::User | PAEPageFlags::Write |
									PAEPageFlags::Present);
		memset((char*)(stack_begin+i), 0, PAGE_SIZE);
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
	stack_end -= stack_end % 8;

	stack_end -= 8;
	*(uint64_t *)stack_end = argv_begin;
	stack_end -= 8;
	*(uint64_t *)stack_end = argc;

	uint64_t *stp = x64_create_user_task_stack((uint64_t *)kstack_end,
											   (uintptr_t)(ehead->entry),
											   (uint64_t *)stack_end);

	proc->m_pglv = pgl;
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
	Process *new_proc = new Process();
	new_proc->m_parent = this;
	m_children.push(this);
	new_proc->m_pglv = Paging::the()->clone_for_fork_shallow_cow(*m_pglv, cow_table, new_proc->cow_table);

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
	new_proc->m_file_handles.clear();
	for (size_t i = 0; i < m_file_handles.size(); i++) {
		new_proc->m_file_handles.push(new FileHandle(*m_file_handles[i]));
	}
	return new_proc;
}

void Process::block_on_proc(Process *p) {
	m_blocker.blocked_on = Blocker::ProcessBlock;
	m_blocker.blocked_process = p;
	m_state = ProState::Blocked;
}

void Process::block_on_sleep(size_t ms) {
	m_blocker.blocked_on = Blocker::SleepBlock;
	m_blocker.sleep_waiter_ms = global_waiter_time + ms;
	m_state = ProState::Blocked;
}

void Process::block_on_handle(FileHandle *handle) {
	m_blocker.blocked_on = Blocker::FileIOBlock;
	m_blocker.blocked_handle = handle;
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
		if (global_waiter_time >= m_blocker.sleep_waiter_ms) {
			m_state = ProState::Running;
		}
		break;
	case Blocker::FileIOBlock:
		if (!m_blocker.blocked_handle->check_blocked()) {
			m_state = ProState::Running;
		}
		break;
	default:
		panic("FIXME");
		break;
	}
}
