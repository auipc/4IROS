#include <kernel/arch/i386/i386.h>
#include <kernel/mem/malloc.h>
#include <kernel/mem/Paging.h>
#include <kernel/string.h>
#include <kernel/tasking/ELF.h>
#include <kernel/tasking/Process.h>
#include <kernel/test_program.h>
#include <kernel/test_program2.h>

static uint32_t s_pid = 0;
extern "C" void task_switch_shim();

Process::Process(void *entry, bool userspace)
	: m_pid(s_pid++), m_userspace(userspace) {
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

Process::Process(const char *elf_file) : m_pid(s_pid++), m_userspace(true) {
	(void)elf_file;
	ELF *elf;
	elf = new ELF((char *)test_program2, test_program2_len);

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
	elf->load_sections(m_page_directory);

	setup(reinterpret_cast<void *>(elf->program_entry()));
	delete elf;
}

Process::Process(Process& process, InterruptRegisters &regs) 
	: m_pid(s_pid++), m_userspace(true)
{
	m_page_directory = process.m_page_directory->clone();

	Paging::switch_page_directory(m_page_directory);
	//uintptr_t stack = (uintptr_t)kmalloc(STACK_SIZE);
	m_stack_base = process.m_stack_base;
	m_stack_top = m_stack_base + STACK_SIZE;
	
	m_user_stack_base = 0x10000000;
	m_page_directory->map_range(m_user_stack_base, USER_STACK_SIZE,
								PageFlags::USER);
	m_user_stack_top = 0x10000000 + USER_STACK_SIZE;

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
	*(uint32_t *)m_stack_top = regs.eflags;
	// CS
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = m_userspace ? 0x1b : 0x08;
	// EIP
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = (uint32_t)regs.eip;
	// EDI
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = regs.edi;
	// ESI
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = regs.esi;
	// EBP
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = ebp;
	m_stack_top -= sizeof(uint32_t);
	// EBX
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = regs.ebx;
	// EDX
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = regs.edx;
	// ECX
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = regs.ecx;
	// EAX (it's zero for the return value of fork)
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = 0;

	if (m_userspace) {
		// DS, ES, FS, GS
		m_stack_top -= sizeof(uint16_t);
		*(uint16_t *)m_stack_top = 0x23;
	} else {
		// DS, ES, FS, GS
		m_stack_top -= sizeof(uint16_t);
		*(uint16_t *)m_stack_top = 0x10;
	}

	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = (uint32_t)task_switch_shim;
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = 0;
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = 0;
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = 0;
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = 0;

	Paging::switch_page_directory(Paging::s_kernel_page_directory);
}

Process::~Process() {
	if (m_userspace) {
		kfree((void *)(m_user_stack_base));
	}

	kfree((void *)m_stack_base);

	// shouldn't be using the page directory we're about to delete
	assert(Paging::current_page_directory() != m_page_directory);

	// FIXME deleting page directories
}

asm("task_switch_shim:");
asm("sti");
asm("mov (%esp), %ds");
asm("mov (%esp), %es");
asm("mov (%esp), %fs");
asm("mov (%esp), %gs");
asm("addl $2, %esp");
asm("popa");
asm("push %eax");
// This will erronously trigger on our first process.
// Don't really care all that much.
asm("mov $0x20, %eax");
asm("out %al, $0x20");
asm("out %al, $0xA0");
asm("pop %eax");
asm("iret");

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
	*(uint32_t *)m_stack_top = m_userspace ? 0x1b : 0x08;
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
		*(uint16_t *)m_stack_top = 0x23;
	} else {
		// DS, ES, FS, GS
		m_stack_top -= sizeof(uint16_t);
		*(uint16_t *)m_stack_top = 0x10;
	}

	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = (uint32_t)task_switch_shim;
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = 0;
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = 0;
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = 0;
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = 0;

	// m_stack_top = reinterpret_cast<uintptr_t>(m_stack_top);
}

Process* Process::fork(InterruptRegisters &regs) {
	return new Process(*this, regs);
}
