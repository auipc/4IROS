#include "kernel/driver/PS2Keyboard.h"
#include <kernel/PIT.h>
#include <kernel/gdt.h>
#include <kernel/mem/Paging.h>
#include <kernel/printk.h>
#include <kernel/tasking/Process.h>
#include <kernel/tasking/Scheduler.h>

Scheduler *Scheduler::s_the = nullptr;
Process *Scheduler::s_current = nullptr;

Scheduler::Scheduler() { s_the = this; }

Scheduler::~Scheduler() { s_the = nullptr; }

void kernel_idle() {
	printk("idle\n");
	PIT::setup();
	while (1) {
	}
}

extern TSS tss_entry;
void Scheduler::setup() {
	s_the = new Scheduler();
	Process *idle = new Process((void *)kernel_idle);
	Scheduler::the()->s_current = idle;
	idle->set_next(idle);
	idle->set_prev(idle);

	Process *kreaper = new Process((void *)reaper);
	// sneaky
	kreaper->set_state(Process::States::Blocked);
	Scheduler::the()->add_process(*kreaper);
	Scheduler::the()->m_kreaper_process = kreaper;

	auto path = VFS::the().parse_path("init");
	Process *next = new Process(VFS::the().open(path));
	printk("init pid %d\n", next->m_pid);
	Scheduler::the()->add_process(*next);

	tss_entry.esp0 = Scheduler::the()->s_current->m_stack_top;

	asm volatile(
		"mov %%eax, %%esp" ::"a"(Scheduler::the()->s_current->m_stack_top));
	asm volatile("mov %%eax, %%cr3" ::"a"(Paging::get_physical_address(
		Scheduler::the()->s_current->m_page_directory)));
	asm volatile("mov (%esp), %ds");
	asm volatile("mov (%esp), %es");
	asm volatile("mov (%esp), %fs");
	asm volatile("mov (%esp), %gs");
	asm volatile("add $2, %esp");
	asm volatile("popa");
	asm volatile("iret");
}

void Scheduler::reaper() {
	while (1) {
		Scheduler::the()->sched_spinlock.acquire();
		Process *node = s_current;
		Process *end = s_current;
		// First iteration checks first process and goes to p->next.
		// Loop in the linked list until we get back to the first process we
		// checked.
		do {
			if (node->m_state != Process::States::Dead) {
				node = node->next();
				continue;
			}

			Process *process = node;
			node = node->next();
			process->prev()->set_next(process->next());
			process->next()->set_prev(process->prev());
			delete process;
		} while (node != end);

		Scheduler::the()->sched_spinlock.release();
		// No more processes to kill.
		Scheduler::the()->m_kreaper_process->set_state(
			Process::States::Blocked);
	}
}

// We could prevent the reaper from running by blocking it
void Scheduler::kill_process(Process &p) {
	// wake up the reaper, if blocked.
	// this is only for first time use.
	if (m_kreaper_process->state() == Process::States::Blocked)
		m_kreaper_process->set_state(Process::States::Active);
	p.m_state = Process::States::Dead;
}

void Scheduler::add_process(Process &p) {
	p.set_next(s_current->next());
	p.next()->set_prev(&p);
	p.set_prev(s_current);
	s_current->set_next(&p);
}

extern "C" void sched_asm(uintptr_t *next_stack, uintptr_t cr3);

asm("sched_asm:");
asm("mov %edx, %cr3");
asm("mov (%eax), %esp");
asm("mov (%esp), %ds");
asm("mov (%esp), %es");
asm("mov (%esp), %fs");
asm("mov (%esp), %gs");
asm("add $2, %esp");
asm("popa");
asm("push %eax");
asm("mov $0x20, %eax");
asm("out %al, $0x20");
asm("out %al, $0xA0");
asm("pop %eax");
asm("iret");

int scheds = 0;
void Scheduler::schedule(uint32_t *esp) {
	Scheduler::the()->sched_spinlock.acquire();
	auto prev_stack = &s_current->m_stack_top;
	// s_current->m_stack_top = s_current->m_stack_base + STACK_SIZE;
	do {
		s_current = s_current->m_next;
		if (s_current->m_blocked_source) {
			if (!s_current->m_blocked_source->check_blocked())
				s_current->set_state(Process::States::Active);
		}
		// skip over dead or blocked processes
	} while (s_current->m_state == Process::States::Dead ||
			 s_current->m_state == Process::States::Blocked);
	auto next_stack = &s_current->m_stack_top;

	if (prev_stack == next_stack)
		return;

	if (esp)
		*prev_stack = reinterpret_cast<uintptr_t>(esp);

	// If in userspace, reset the kernel stack every context switch. Otherwise,
	// a stack overflow WILL occur!
	if (s_current->m_userspace)
		tss_entry.esp0 = Scheduler::the()->s_current->m_stack_base + STACK_SIZE;

	Scheduler::the()->sched_spinlock.release();
	sched_asm(next_stack,
			  (uintptr_t)Paging::get_physical_address(
				  reinterpret_cast<void *>(s_current->page_directory())));
}
