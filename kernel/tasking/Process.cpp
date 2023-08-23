#include <kernel/mem/malloc.h>
#include <kernel/string.h>
#include <kernel/tasking/Process.h>
#include <kernel/arch/i386/i386.h>

Process::Process(void *entry) {
	uintptr_t stack = (uintptr_t)kmalloc(STACK_SIZE);
	m_stack = stack;
	m_stacktop = m_stack + STACK_SIZE;
	// memset((char*)stack, 0, STACK_SIZE);
	setup(entry);
}

Process::~Process() { kfree((void *)(m_stack)); }

void Process::setup(void *entry) {
	uint32_t ebp = m_stacktop;
	// EFLAGS
	m_stacktop -= sizeof(uint32_t);
	*(uint32_t *)m_stacktop = EFlags::InterruptEnable | EFlags::AlwaysSet;
	// CS
	m_stacktop -= sizeof(uint32_t);
	*(uint32_t *)m_stacktop = 0x8;
	// EIP
	m_stacktop -= sizeof(uint32_t);
	*(uint32_t *)m_stacktop = (uint32_t)entry;
	// EDI
	m_stacktop -= sizeof(uint32_t);
	*(uint32_t *)m_stacktop = 0;
	// ESI
	m_stacktop -= sizeof(uint32_t);
	*(uint32_t *)m_stacktop = 0;
	// EBP
	m_stacktop -= sizeof(uint32_t);
	*(uint32_t *)m_stacktop = ebp;
	m_stacktop -= sizeof(uint32_t);
	// EBX
	m_stacktop -= sizeof(uint32_t);
	*(uint32_t *)m_stacktop = 0;
	// EDX
	m_stacktop -= sizeof(uint32_t);
	*(uint32_t *)m_stacktop = 0;
	// ECX
	m_stacktop -= sizeof(uint32_t);
	*(uint32_t *)m_stacktop = 0;
	// EAX
	m_stacktop -= sizeof(uint32_t);
	*(uint32_t *)m_stacktop = 0;
	// DS
	m_stacktop -= sizeof(uint32_t);
	*(uint32_t *)m_stacktop = 0x10;
	// ES
	m_stacktop -= sizeof(uint32_t);
	*(uint32_t *)m_stacktop = 0x10;
	// FS
	m_stacktop -= sizeof(uint32_t);
	*(uint32_t *)m_stacktop = 0x10;
	// GS
	m_stacktop -= sizeof(uint32_t);
	*(uint32_t *)m_stacktop = 0x10;

	m_stacktop = reinterpret_cast<uintptr_t>(m_stacktop);
}
