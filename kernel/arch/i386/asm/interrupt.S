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

.macro DEFINE_EXCEPTION_HANDLER x
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
		addl $8, %esp
		sti
		iret
.endm

.macro DEFINE_EXCEPTION_ERROR_CODE_HANDLER x
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
		addl $12, %esp
		sti
		iret
.endm

DEFINE_HANDLER syscall
DEFINE_HANDLER ps2kbd

DEFINE_EXCEPTION_HANDLER div 
DEFINE_EXCEPTION_HANDLER df
DEFINE_EXCEPTION_ERROR_CODE_HANDLER invalidtss
DEFINE_EXCEPTION_ERROR_CODE_HANDLER gpf
DEFINE_EXCEPTION_ERROR_CODE_HANDLER pf

.extern pit_interrupt
.global pit_interrupt_handler
pit_interrupt_handler:
	cli
	pusha
	pushw %gs
	mov %esp, %eax
	call pit_interrupt
	mov (%esp), %ds
	mov (%esp), %fs
	mov (%esp), %es
	popw %gs
	popa
	sti
	iret
