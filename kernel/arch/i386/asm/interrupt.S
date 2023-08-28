.macro DEFINE_HANDLER x
	.extern \x\()_interrupt
	.global \x\()_interrupt_handler
	\x\()_interrupt_handler:
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
		iret
.endm

.macro DEFINE_IRQ_HANDLER x
	.extern \x\()_interrupt
	.global \x\()_interrupt_handler
	\x\()_interrupt_handler:
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
		iret
.endm

DEFINE_HANDLER syscall
DEFINE_IRQ_HANDLER pit
