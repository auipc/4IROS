#include <kernel/arch/i386/i386.h>
#include <kernel/mem/malloc.h>
#include <kernel/string.h>
#include <kernel/tasking/Process.h>
#include <kernel/tasking/ELF.h>
#include <kernel/test_program.h>

static uint32_t s_pid = 0;

Process::Process(void *entry, bool userspace)
	: m_pid(s_pid++), m_userspace(userspace) {
	uintptr_t stack = (uintptr_t)kmalloc(STACK_SIZE);
	m_page_directory = Paging::the()->kernel_page_directory()->clone();
	m_stack_base = stack;
	m_stack_top = m_stack_base + STACK_SIZE;

	if (m_userspace) {
		m_user_stack_base = 0x10000000;
		m_page_directory->map_range(m_user_stack_base, USER_STACK_SIZE, true);
		m_user_stack_top = 0x10000000 + USER_STACK_SIZE;
	}
	// memset((char*)stack, 0, STACK_SIZE);
	setup(entry);
}

Process::Process(const char *elf_file) 
	: m_pid(s_pid++)
	, m_userspace(true)
{
	(void)elf_file;
	ELF* elf = new ELF((char*)test_program, test_program_len);
	uintptr_t stack = (uintptr_t)kmalloc(STACK_SIZE);
	m_page_directory = Paging::the()->kernel_page_directory()->clone();
	m_stack_base = stack;
	m_stack_top = m_stack_base + STACK_SIZE;

	if (m_userspace) {
		m_user_stack_base = 0x10000000;
		m_page_directory->map_range(m_user_stack_base, USER_STACK_SIZE, true);
		m_user_stack_top = 0x10000000 + USER_STACK_SIZE;
	}
	elf->load_sections(m_page_directory);

	printk("lol %x\n", m_page_directory);
	printk("lol %x\n", elf->program_entry());

	setup(reinterpret_cast<void*>(elf->program_entry()));
}

Process::~Process() {
	if (m_userspace) {
		kfree((void *)(m_user_stack_base));
	}

	kfree((void *)(m_stack_base));
}

extern "C" void task_switch_shim();
asm("task_switch_shim:");
asm("sti");
asm("pop %ds");
asm("pop %es");
asm("pop %fs");
asm("pop %gs");
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
		// DS
		m_stack_top -= sizeof(uint32_t);
		*(uint32_t *)m_stack_top = 0x23;
		// ES
		m_stack_top -= sizeof(uint32_t);
		*(uint32_t *)m_stack_top = 0x23;
		// FS
		m_stack_top -= sizeof(uint32_t);
		*(uint32_t *)m_stack_top = 0x23;
		// GS
		m_stack_top -= sizeof(uint32_t);
		*(uint32_t *)m_stack_top = 0x23;
	} else {
		// DS
		m_stack_top -= sizeof(uint32_t);
		*(uint32_t *)m_stack_top = 0x10;
		// ES
		m_stack_top -= sizeof(uint32_t);
		*(uint32_t *)m_stack_top = 0x10;
		// FS
		m_stack_top -= sizeof(uint32_t);
		*(uint32_t *)m_stack_top = 0x10;
		// GS
		m_stack_top -= sizeof(uint32_t);
		*(uint32_t *)m_stack_top = 0x10;
	}

	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = (uint32_t)task_switch_shim;
	m_stack_top -= sizeof(uint32_t);
	*(uint32_t *)m_stack_top = EFlags::InterruptEnable | EFlags::AlwaysSet;
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
