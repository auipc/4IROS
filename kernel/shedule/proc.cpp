#include <kernel/shedule/proc.h>
#include <kernel/vfs/vfs.h>
#include <kernel/unix/ELF.h>
#include <kernel/arch/amd64/x64_sched_help.h>

#define KSTACK_SIZE (4096)
Process* current_process = nullptr;
extern TSS tss;
uint64_t s_pid = 0;

class StdinNode final : public VFSNode {
public:
	StdinNode() {
		VFSNode();
	}

	virtual int write(void* buffer, size_t size) override {
		for (size_t i = 0; i < size; i++) {
			m_buffer.push(((char*)buffer)[i]);
		}
		return size;
	}

	virtual int read(void* buffer, size_t size) override {
		memcpy(buffer, m_buffer.data()+m_position, size);
		return size;
	}

	virtual bool check_blocked() override {
		return m_position >= m_buffer.size();
	}

private:
	Vec<char> m_buffer;
};

class StdioNode final : public VFSNode {
  public:
	StdioNode() {
		VFSNode();
	}

	virtual int write(void *buffer, size_t size) override {
		(void)size;
		for (size_t i = 0; i < size; i++) {
			write_scb(((char*)buffer)[i], 7);
		}
		return size;
	}
};

Process::Process()
	: pid(s_pid++)
{
	m_file_handles.push(new FileHandle(new StdinNode()));
	m_file_handles.push(new FileHandle(new StdioNode()));
	m_file_handles.push(new FileHandle(new StdioNode()));
}

Process::Process(Process* p)
	: pid(s_pid++)
{
	m_file_handles = p->m_file_handles;
}

void Process::sched(uintptr_t rsp) {
	if (current_process == current_process->next) return;
	if (rsp)
		current_process->m_kern_stack_top = rsp;
	do {
		current_process = current_process->next;
		if (current_process->m_state == ProState::Blocked)
			current_process->poll_is_blocked();
	} while (current_process->m_state != ProState::Running);

	assert(current_process);
	ksyscall_data.stack = current_process->m_actual_kern_stack_top;
	// FIXME remove this
	write_msr(0xC0000102, (uint32_t)(uintptr_t)&ksyscall_data, (uint32_t)((uintptr_t)&ksyscall_data>>32));
	tss.rsp[0] = current_process->m_actual_kern_stack_top;
	x64_switch((void*)current_process->m_kern_stack_top,(uint64_t)current_process->m_pglv);
}

void Process::reentry() {
	assert(current_process);
	ksyscall_data.stack = current_process->m_actual_kern_stack_top;
	// FIXME remove this
	write_msr(0xC0000102, (uint32_t)(uintptr_t)&ksyscall_data, (uint32_t)((uintptr_t)&ksyscall_data>>32));
	tss.rsp[0] = current_process->m_actual_kern_stack_top;
	x64_switch((void*)current_process->m_kern_stack_top,(uint64_t)current_process->m_pglv);
}

Process* Process::create(void* func_ptr) {
	Process* proc = new Process();
	//auto current_pd = (RootPageLevel*)get_cr3();
	//RootPageLevel* pgl = Paging::the()->clone(*(RootPageLevel*)get_cr3());
	//Paging::switch_page_directory(pgl);

	uintptr_t stack_begin = (uintptr_t)kmalloc(KSTACK_SIZE);
	uintptr_t stack_end = stack_begin+KSTACK_SIZE;

	uint64_t* stp = x64_create_kernel_task_stack((uint64_t*)stack_end, (uintptr_t)func_ptr);

	proc->m_pglv = (RootPageLevel*)get_cr3();
	proc->m_stack_top = (uintptr_t)stp;
	proc->m_stack_bot = stack_begin; 
	proc->m_kern_stack_top = (uintptr_t)stp;
	proc->m_kern_stack_bot = stack_begin; 
	//Paging::switch_page_directory(current_pd);
	return proc;
}

Process* Process::create_user(const char* path) {
	Process* proc = new Process();
	auto path_vec = VFS::the().parse_path(path);
	auto exec_file = VFS::the().open(path_vec);
	auto file_sz = exec_file->size();
	char* buffer = new char[file_sz];
	exec_file->read(buffer, file_sz);
	ELFHeader64* ehead = reinterpret_cast<ELFHeader64*>(buffer);
	const ELFSectionHeader64* esecheads = reinterpret_cast<const ELFSectionHeader64*>(buffer + ehead->phtable);

	auto current_pd = (RootPageLevel*)get_cr3();
	RootPageLevel* pgl = Paging::the()->clone_for_fork(*(RootPageLevel*)current_pd, true);
	Paging::the()->switch_page_directory(pgl);

	for (int i = 0; i < ehead->phnum; i++) {
		const auto sechead = esecheads[i];
		if (sechead.seg != 1) continue;
		uint64_t flags = PAEPageFlags::Present | PAEPageFlags::Write | PAEPageFlags::User;
		// Disable execution if no ELF executable flag 
		//flags |= ((uint64_t)!(sechead.flags & 1ull))<<63ull;
		// Writable? Ideally, panic if both eXecute and Writable are enabled.
		//flags |= (!!(sechead.flags & 2ull))<<1ull;

		auto vaddr = sechead.vaddr;//0x500000+sechead.off-0x1000;
		printk("vaddr. %x sz %x\n", vaddr, sechead.memsz);
		for (size_t j = 0; j < sechead.memsz; j += PAGE_SIZE) {
			auto paddr = Paging::the()->allocator()->find_free_page();
			Paging::the()->map_page(*pgl, vaddr+j, paddr, PAEPageFlags::User | flags);
		}
		memset((char*)vaddr, 0, sechead.memsz);
		memcpy((char*)vaddr, buffer+sechead.off, sechead.filesz);
		printk("vaddr %x\n", vaddr);
	}

	uintptr_t stack_begin = 0x400000;
	uintptr_t stack_end = stack_begin+STACK_SIZE;

	uintptr_t kstack_begin = (uintptr_t)kmalloc(KSTACK_SIZE);
	uintptr_t kstack_end = kstack_begin+KSTACK_SIZE;

	for (int i = 0; i < STACK_SIZE+4096; i+=4096) {
		uintptr_t stack_page = Paging::the()->allocator()->find_free_page();
		Paging::the()->map_page(*pgl, stack_begin+i, stack_page, PAEPageFlags::User | PAEPageFlags::Write | PAEPageFlags::Present);
	}
	uint64_t* stp = x64_create_user_task_stack((uint64_t*)kstack_end, (uintptr_t)(ehead->entry), (uint64_t*)stack_end);

	proc->m_pglv = pgl;
	proc->m_stack_bot = stack_begin;
	proc->m_stack_top = stack_end;
	proc->m_kern_stack_bot = kstack_begin;
	proc->m_kern_stack_top = (uintptr_t)stp;
	proc->m_actual_kern_stack_top = kstack_end;
	Paging::switch_page_directory(current_pd);
	return proc;
}

// FIXME SSE complains because the stack is misaligned. -mstackrealign is a bandaid fix
Process* Process::fork(SyscallReg* regs) {
	printk("fork :)\n");
	(void)regs;
	uintptr_t kstack_begin = (uintptr_t)kmalloc(KSTACK_SIZE);
	uint64_t* kstack_end = (uint64_t*)(kstack_begin+KSTACK_SIZE);
	uint64_t* copy_stack_end = (uint64_t*)regs;
	//Process* new_proc = new Process(this);
	Process* new_proc = new Process();
	new_proc->m_pglv = Paging::the()->clone_for_fork(*m_pglv);

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

	// Skip over RAX and RBP
	copy_stack_end += 2;
	for (int i = 0; i < 13; i++) {
		kstack_end--;
		*kstack_end = *copy_stack_end;
		copy_stack_end++;
	}
	// RAX
	kstack_end--;
	*kstack_end = 0;

	new_proc->m_kern_stack_bot = kstack_begin;
	new_proc->m_kern_stack_top = (uintptr_t)kstack_end;
	return new_proc;
}

void Process::block_on_proc(Process* p) {
	m_blocker.blocked_on = Blocker::ProcessBlock;
	m_blocker.blocked_process = p;
	m_state = ProState::Blocked;
}

void Process::block_on_handle(FileHandle* handle) {
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
