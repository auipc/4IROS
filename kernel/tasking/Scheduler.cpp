#include <kernel/tasking/Process.h>
#include <kernel/tasking/Scheduler.h>

Scheduler *Scheduler::s_the = nullptr;

Scheduler::Scheduler() { s_the = this; }

Scheduler::~Scheduler() { s_the = nullptr; }

void next_process() {
	printk("test\n");
	Scheduler::the()->schedule();
	while(1) {
		printk("proc 1\n");
		Scheduler::the()->schedule();
	}
}

void kernel_idle() {
	printk("idle\n");
	//asm volatile("int $0x80");
	//printk("test\n");
	Scheduler::the()->schedule();
	printk("this works\n");
	Scheduler::the()->schedule();
	while(1) {
		printk("proc 2\n");
		Scheduler::the()->schedule();
	}
}

void Scheduler::setup() {
	s_the = new Scheduler();
	Process *idle = new Process((void *)kernel_idle);
	Scheduler::the()->m_current = idle;
	idle->set_next(idle);
	idle->set_prev(idle);

	Process *next = new Process((void *)next_process);
	idle->set_next(next);
	next->set_next(idle);

	asm volatile("mov %%eax, %%esp" : : "a"(Scheduler::the()->m_current->stack_top()));
	asm volatile("pop %ebp");
	asm volatile("pop %edi");
	asm volatile("pop %esi");
	asm volatile("pop %ebx");
	asm volatile("popf");
	asm volatile("ret");
	/*
	asm volatile("pop %ds");
	asm volatile("pop %es");
	asm volatile("pop %fs");
	asm volatile("pop %gs");
	asm volatile("popa");
	asm volatile("iret");*/
}

/*
extern "C" void task_switch(uintptr_t* prev, uintptr_t* next);
asm("task_switch:");
// dont want to do a bunch of stack stuff right now :/
asm("mov %esp, (%esi)");
asm("mov (%ebx), %esp");
asm("ret");
*/

static int schedules = 0;

void Scheduler::schedule() {
	if (m_current == m_current->next()) return;

	auto prev_stack = &m_current->m_stacktop;
	m_current = m_current->next();
	auto next_stack = &m_current->m_stacktop;

	//printk("prev_stack %x, next_stack %x\n", prev_stack, next_stack);

	//task_switch(prev_stack, next_stack);
	asm volatile("pushf");
	asm volatile("push %ebx");
	asm volatile("push %esi");
	asm volatile("push %edi");
	asm volatile("push %ebp");
	asm volatile("mov %%esp, %%eax":"=a"(*prev_stack));
	asm volatile("mov %%eax, %%esp"::"a"(*next_stack));
	asm volatile("pop %ebp");
	asm volatile("pop %edi");
	asm volatile("pop %esi");
	asm volatile("pop %ebx");
	asm volatile("popf"); 

	// TODO remove ugly hack
	if (++schedules > 1)
		asm volatile("add $4, %esp");

	asm volatile("ret");
}
