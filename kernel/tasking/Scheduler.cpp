#include <kernel/PIT.h>
#include <kernel/gdt.h>
#include <kernel/mem/Paging.h>
#include <kernel/tasking/Process.h>
#include <kernel/tasking/Scheduler.h>
#include <kernel/printk.h>

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

	Process *next = new Process("init");
	next->set_next(s_current->next());
	next->next()->set_prev(next);
	next->set_prev(s_current);
	s_current->set_next(next);

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
	s_current = s_current->m_next;
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
	s_current = s_current->m_next;
	auto next_stack = &s_current->m_stack_top;

	sched_asm_no_save(
		prev_stack, next_stack,
		(uintptr_t)Paging::get_physical_address(
			reinterpret_cast<void *>(s_current->page_directory())));
}
