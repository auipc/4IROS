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

void next_process2() {
	while (1) {
		printk("proc 2\n");
	}
}

void next_process() {
	while (1) {
		printk("proc 1\n");
	}
}

void kernel_idle() {
	PIT::setup();
	printk("idle\n");
	printk("this works\n");
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

	Process *next = new Process("init");
	Scheduler::the()->add_process(*next);

	Process *next2 = new Process("init");
	Scheduler::the()->add_process(*next2);

	printk("init2\n");

	tss_entry.esp0 = Scheduler::the()->s_current->m_stack_top;

	asm volatile("mov %%eax, %%esp"
				 :
				 : "a"(Scheduler::the()->s_current->m_stack_top));
	asm volatile(
		"mov %%eax, %%cr3"
		:
		: "a"((uintptr_t)Paging::get_physical_address(reinterpret_cast<void *>(
			Scheduler::the()->s_current->page_directory()))));
	asm volatile("pop %ebp");
	asm volatile("pop %edi");
	asm volatile("pop %esi");
	asm volatile("pop %ebx");
	asm volatile("ret");
	/*asm volatile("pop %ds");
	asm volatile("pop %es");
	asm volatile("pop %fs");
	asm volatile("pop %gs");
	asm volatile("popa");
	asm volatile("iret");*/
}

void Scheduler::reaper() {
	while (1) {
		Process *node = s_current;
		Process *end = s_current;
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

		yield();
	}
}

void Scheduler::yield() { Scheduler::schedule(); }

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

extern "C" void sched_asm(uintptr_t *prev_stack, uintptr_t *next_stack,
						  uintptr_t cr3);
asm("sched_asm:");
asm("mov %ecx, %esi");
asm("mov %eax, %ecx");
asm("push %ebx");
asm("push %esi");
asm("push %edi");
asm("push %ebp");
asm("mov %esp,%eax");
asm("mov %eax,(%ecx)");
asm("mov %esi, %cr3");
asm("mov (%edx),%eax");
asm("mov %eax,%esp");
asm("pop %ebp");
asm("pop %edi");
asm("pop %esi");
asm("pop %ebx");
asm("ret");

extern "C" void sched_asm_no_save(uintptr_t *prev_stack, uintptr_t *next_stack,
								  uintptr_t cr3);
asm("sched_asm_no_save:");
asm("mov %ecx, %esi");
asm("mov %eax, %ecx");
asm("mov %esi, %cr3");
asm("mov (%edx),%eax");
asm("mov %eax,%esp");
asm("pop %ebp");
asm("pop %edi");
asm("pop %esi");
asm("pop %ebx");
asm("ret");

void Scheduler::schedule() {
	if (s_current == s_current->m_next)
		return;
	auto prev_stack = &s_current->m_stack_top;
	do {
		s_current = s_current->m_next;
		// skip over dead or blocked processes
	} while (s_current->m_state == Process::States::Dead ||
			 s_current->m_state == Process::States::Blocked);
	auto next_stack = &s_current->m_stack_top;

	tss_entry.esp0 = Scheduler::the()->s_current->m_stack_top;

	// esoteric enough?
	printk("%d -> %d\n", s_current->prev()->m_pid, s_current->m_pid);

	sched_asm(prev_stack, next_stack,
			  (uintptr_t)Paging::get_physical_address(
				  reinterpret_cast<void *>(s_current->page_directory())));
}

void Scheduler::schedule_no_save() {
	if (s_current == s_current->m_next)
		return;
	auto prev_stack = &s_current->m_stack_top;
	do {
		s_current = s_current->m_next;
		// skip over dead or blocked processes
	} while (s_current->m_state == Process::States::Dead ||
			 s_current->m_state == Process::States::Blocked);
	auto next_stack = &s_current->m_stack_top;

	sched_asm_no_save(
		prev_stack, next_stack,
		(uintptr_t)Paging::get_physical_address(
			reinterpret_cast<void *>(s_current->page_directory())));
}
