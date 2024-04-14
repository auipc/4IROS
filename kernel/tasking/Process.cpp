#include <kernel/arch/i386/i386.h>
#include <kernel/gdt.h>
#include <kernel/mem/Paging.h>
#include <kernel/mem/malloc.h>
#include <kernel/tasking/ELF.h>
#include <kernel/tasking/Process.h>
#include <kernel/vfs/stddev.h>
#include <kernel/vfs/vfs.h>
#include <string.h>

static uint32_t s_pid = 0;

Process::Process(void *entry, bool userspace)
	: m_pid(s_pid++), m_userspace(userspace) {
	m_file_descriptors = new Vec<FileHandle *>();
	uintptr_t stack = (uintptr_t)kmalloc(STACK_SIZE);
	m_page_directory = Paging::the()->kernel_page_directory()->clone();
	m_stack_base = stack;
	m_stack_top = m_stack_base + STACK_SIZE;

	if (m_userspace) {
		m_user_stack_base = 0x10000000;
		m_page_directory->map_range(m_user_stack_base, USER_STACK_SIZE,
									PageFlags::USER);
		m_user_stack_top = 0x10000000 + USER_STACK_SIZE;
	}
	// memset((char*)stack, 0, STACK_SIZE);
	setup(entry);
}

// FIXME: Load binary outside of the constructor
Process::Process(VFSNode *executable) : m_pid(s_pid++), m_userspace(true) {
	m_file_descriptors = new Vec<FileHandle *>();
	m_file_descriptors->push(new FileHandle(new StdDev()));
	m_file_descriptors->push(new FileHandle(new StdDev(true)));
	m_file_descriptors->push(new FileHandle(new StdDev(true)));

	ELF *elf;
	elf = new ELF(executable);

	m_page_directory = Paging::the()->kernel_page_directory()->clone();

	// Allocate kernel stack
	uintptr_t stack = (uintptr_t)kmalloc(STACK_SIZE);
	m_stack_base = stack;
	m_stack_top = m_stack_base + STACK_SIZE;

	if (m_userspace) {
		m_user_stack_base = 0x10000000;
		m_page_directory->map_range(
			m_user_stack_base, USER_STACK_SIZE + PAGE_SIZE, PageFlags::USER);
		m_user_stack_top = 0x10000000 + USER_STACK_SIZE;
	}
	elf->load_sections(m_page_directory);

	auto old_pd = Paging::current_page_directory();
	Paging::switch_page_directory(m_page_directory);
	setup(reinterpret_cast<void *>(elf->program_entry()));
	Paging::switch_page_directory(old_pd);
	delete elf;
}

Process::Process(Process &parent, InterruptRegisters &regs)
	: m_pid(s_pid++), m_userspace(true) {
	m_file_descriptors = new Vec<FileHandle *>();
	for (size_t i = 0; i < parent.m_file_descriptors->size(); i++)
		m_file_descriptors->push((*parent.m_file_descriptors)[i]);

	m_page_directory = parent.m_page_directory->clone();

	PageDirectory *old = Paging::current_page_directory();
	Paging::switch_page_directory(m_page_directory);

	uintptr_t stack = (uintptr_t)kmalloc(STACK_SIZE);
	m_stack_base = stack;
	m_stack_top = m_stack_base + STACK_SIZE;
	memcpy((void *)m_stack_base, (void *)parent.m_stack_base, STACK_SIZE);

	m_user_stack_base = 0x10000000;
	/*m_page_directory->map_range(m_user_stack_base, USER_STACK_SIZE,
								PageFlags::USER);*/
	m_user_stack_top = 0x10000000 + USER_STACK_SIZE;

	if (m_userspace) {
		// If we're returning to an outer privilege level these get popped.
		m_stack_top -= sizeof(uint32_t);
		*(uint32_t *)m_stack_top = 0x23;
		m_stack_top -= sizeof(uint32_t);
		*(uint32_t *)m_stack_top = regs.user_esp; // m_user_stack_top;
	}

	// EFLAGS
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = regs.eflags;
	// CS
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = m_userspace ? 0x1b : 0x08;
	// EIP
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = (uint32_t)regs.eip;

	// FIXME: something reverses the order of popa.

	// EAX
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = 0;
	// ECX
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = regs.ecx;
	// EDX
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = regs.edx;
	// EBX
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = regs.ebx;

	// Increment ESP by 4; (* Skip next 4 bytes of stack *)
	m_stack_top -= sizeof(uint32_t);

	// EBP
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = regs.ebp;

	// ESI
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = regs.esi;

	// EDI
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = regs.edi;

	if (m_userspace) {
		// DS, ES, FS, GS
		m_stack_top -= sizeof(uint16_t);
		*(uint16_t *)m_stack_top = USER_DS;
	} else {
		// DS, ES, FS, GS
		m_stack_top -= sizeof(uint16_t);
		*(uint16_t *)m_stack_top = KERN_DS;
	}

	Paging::switch_page_directory(old);
}

Process::~Process() {
	if (m_userspace) {
		// unmap the userspace stack
		m_page_directory->unmap_range(m_user_stack_base, USER_STACK_SIZE);
	}
	kfree((void *)m_stack_base);
	// shouldn't be using the page directory we're about to delete
	assert(Paging::current_page_directory() != m_page_directory);

	// FIXME deleting page directories
}

void Process::setup(void *entry) {
	uint32_t ebp = m_stack_top;

	if (m_userspace) {
		// If we're returning to an outer privilege level these get popped.
		m_stack_top -= sizeof(uint32_t);
		*(uint32_t *)m_stack_top = 0x23;
		m_stack_top -= sizeof(uint32_t);
		*(uint32_t *)m_stack_top = m_user_stack_top;
	}

	// EFLAGS
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = EFlags::InterruptEnable | EFlags::AlwaysSet;
	// CS
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = m_userspace ? USER_CS : KERN_CS;
	// EIP
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = (uint32_t)entry;
	// EDI
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = 0;
	// ESI
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = 0;
	// EBP
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = ebp;

	// Increment ESP by 4; (* Skip next 4 bytes of stack *)
	m_stack_top -= sizeof(uint32_t);

	// EBX
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = 0;
	// EDX
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = 0;
	// ECX
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = 0;
	// EAX
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = 0;

	if (m_userspace) {
		// DS, ES, FS, GS
		m_stack_top -= sizeof(uint16_t);
		*(uint16_t *)m_stack_top = USER_DS;
	} else {
		// DS, ES, FS, GS
		m_stack_top -= sizeof(uint16_t);
		*(uint16_t *)m_stack_top = KERN_DS;
	}
}

Process *Process::fork(InterruptRegisters &regs) {
	return new Process(*this, regs);
}
