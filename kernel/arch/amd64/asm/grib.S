.section .text
.global x64_switch
x64_switch:
	cli
	movq %rsi, %cr3
	movq %rdi, %rsp
	/*pushq $0x202
	pushq $0x08
	pushq $0x0*/
	iretq
	//pushq $0x0
	#iretq

// MORON: Since we're creating another stack, why not push to it using the accepted method? <-- Bogus reasoning for something that lowers code size :)
.global x64_create_task_stack
x64_create_task_stack:
	cli
	// Switch stack
	movq %rsp, %rcx
	movq %rdi, %rsp

	// For RETURN-TO-OUTER-PRIVILEGE-LEVEL, something something RPL.
	// SS
	pushq $0x10
	// RSP
	pushq %rsp

	// Popped at end of IRET
	pushq $0x202
	pushq $0x08
	pushq $0xFFFFF

	movq %rsp, %rax
	// Back again
	mov %rcx, %rsp
	// Returns the stack
	ret
