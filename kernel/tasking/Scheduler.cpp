#include <kernel/tasking/Process.h>
#include <kernel/tasking/Scheduler.h>
#include <kernel/idt.h>

Scheduler *Scheduler::s_the = nullptr;

Scheduler::Scheduler() { s_the = this; }

Scheduler::~Scheduler() { s_the = nullptr; }

void next_process() {
	printk("test\n");
	while(1) {
		//printk("proc 1\n");
		asm volatile("int $0x7F");
	}
}

void kernel_idle() {
	printk("idle\n");
	printk("this works\n");
	while(1) {
		//printk("proc 2\n");
		asm volatile("int $0x7F");
	}
}


extern "C" void _schedule() {
	Scheduler::schedule();
}

extern "C" void schedule_handler();
asm("schedule_handler:");
asm("	pusha");
asm("   push %gs");
asm("   push %fs");
asm("   push %es");
asm("   push %ds");
// TODO UGLY HACK
asm("	call _schedule");
asm("   add $0x10, %esp");
asm("	popa");
asm("	iret");

void Scheduler::setup() {
	s_the = new Scheduler();
	Process *idle = new Process((void *)kernel_idle);
	Scheduler::the()->m_current = idle;
	idle->set_next(idle);
	idle->set_prev(idle);

	Process *next = new Process((void *)next_process);
	idle->set_next(next);
	next->set_next(idle);

	InterruptHandler::the()->setHandler(0x7F, schedule_handler);

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

void Scheduler::schedule() {
	if (Scheduler::the()->current() == Scheduler::the()->current()->next()) return;

	auto prev_stack = &Scheduler::the()->current()->m_stacktop;
	Scheduler::the()->set_current(Scheduler::the()->current()->next());
	auto next_stack = &Scheduler::the()->current()->m_stacktop;

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
	asm volatile("ret");
}
