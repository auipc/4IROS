#include <kernel/tasking/Process.h>
#include <kernel/tasking/Scheduler.h>
#include <kernel/idt.h>

Scheduler *Scheduler::s_the = nullptr;
Process *Scheduler::s_current = nullptr;

Scheduler::Scheduler() { s_the = this; }

Scheduler::~Scheduler() { s_the = nullptr; }

void next_process2() {
	while(1) {
		printk("proc 2\n");
		asm volatile("int $0x7F");
	}
}

void next_process() {
	while(1) {
		printk("proc 1\n");
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
asm("	call _schedule");
asm("   add $0x10, %esp");
asm("	popa");
asm("	iret");

void Scheduler::setup() {
	s_the = new Scheduler();
	Process *idle = new Process((void *)kernel_idle);
	Scheduler::the()->s_current = idle;
	idle->set_next(idle);
	idle->set_prev(idle);

	Process *next = new Process((void *)next_process);
	idle->set_next(next);
	next->set_next(idle);

	Process *next2 = new Process((void *)next_process2);
	next->set_next(next2);
	next2->set_next(idle);

	InterruptHandler::the()->setHandler(0x7F, schedule_handler);

	asm volatile("mov %%eax, %%esp" : : "a"(Scheduler::the()->s_current->m_stacktop));
	//asm volatile("mov %%eax, %%cr3" : : "a"(Scheduler::the()->s_current->page_directory()));
	asm volatile("pop %ebp");
	asm volatile("pop %edi");
	asm volatile("pop %esi");
	asm volatile("pop %ebx");
	asm volatile("popf");
	asm volatile("ret");
	/*asm volatile("pop %ds");
	asm volatile("pop %es");
	asm volatile("pop %fs");
	asm volatile("pop %gs");
	asm volatile("popa");
	asm volatile("iret");*/
}

extern "C" void sched_asm(uintptr_t* prev_stack, uintptr_t* next_stack);
asm("sched_asm:");
asm("mov %eax, %ecx");
asm("pushf");
asm("push %ebx");
asm("push %esi");
asm("push %edi");
asm("push %ebp");
asm("mov %esp,%eax");
asm("mov %eax,(%ecx)");
asm("mov (%edx),%eax");
asm("mov %eax,%esp");
asm("pop %ebp");
asm("pop %edi");
asm("pop %esi");
asm("pop %ebx");
asm("popf");
asm("ret");

/*
void test(uintptr_t* prev_stack, uintptr_t* next_stack) {
	asm volatile("pushf");
	asm volatile("push %ebx");
	asm volatile("push %esi");
	asm volatile("push %edi");
	asm volatile("push %ebp");
	asm volatile("mov %%esp, %%eax":"=a"(*prev_stack));
	asm volatile("mov %%eax, %%esp"::"a"(*next_stack));
	//asm volatile("mov %%eax, %%cr3" : : "a"(Scheduler::the()->current()->page_directory()));
	asm volatile("pop %ebp");
	asm volatile("pop %edi");
	asm volatile("pop %esi");
	asm volatile("pop %ebx");
	asm volatile("popf");
	asm volatile("ret");
}*/

// FIXME This only works on -O1-3 due to needing a certain stack layout.
// Try to figure out how to fix this later
void Scheduler::schedule() {
	if (s_current == s_current->m_next) return;
	auto prev_stack = &s_current->m_stacktop;
	s_current = s_current->m_next;
	auto next_stack = &s_current->m_stacktop;

	printk("prev_stack %x next_stack %x\n", prev_stack, next_stack);

	sched_asm(prev_stack, next_stack);
}
