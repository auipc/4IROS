.macro DEFINE_HANDLER x
	.extern \x\()_interrupt
	.global \x\()_interrupt_handler
	\x\()_interrupt_handler:
		cli
		pusha
		pushl %gs
		pushl %fs
		pushl %es
		pushl %ds
		mov %esp, %eax
		call \x\()_interrupt
		popl %ds
		popl %es
		popl %fs
		popl %gs
		popa
		sti
		iret
.endm

DEFINE_HANDLER syscall

.extern pit_interrupt
.global pit_interrupt_handler
pit_interrupt_handler:
	cli
	pusha
	pushw %gs
	mov %esp, %ecx
	call pit_interrupt
	mov (%esp), %gs
	mov (%esp), %fs
	mov (%esp), %es
	popw %ds
	popa
	sti
	iret
