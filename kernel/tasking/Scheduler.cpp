#include <kernel/tasking/Process.h>
#include <kernel/tasking/Scheduler.h>

Scheduler *Scheduler::s_the = nullptr;

Scheduler::Scheduler() { s_the = this; }

Scheduler::~Scheduler() { s_the = nullptr; }

void kernel_idle() {
	printk("test\n");
	while (1)
		;
}

void Scheduler::setup() {
	s_the = new Scheduler();
	Process *idle = new Process((void *)kernel_idle);
	s_the->m_current = idle;
	idle->set_next(idle);
	idle->set_prev(idle);

	asm volatile("mov %%eax, %%esp" : : "a"(idle->stack_top()));
	asm volatile("pop %ds");
	asm volatile("pop %es");
	asm volatile("pop %fs");
	asm volatile("pop %gs");
	asm volatile("popa");
	asm volatile("iret");
}