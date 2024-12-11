.global initialize_fpu
initialize_fpu:
	fninit
	fxsave (%rdi)
	ret

.global x64_switch
x64_switch:
	cli

	movq %rsi, %cr3
	movq %rdi, %rsp

	pushq %rax
	call write_eoi
	popq %rax

	swapgs
	mov %gs:16, %rax
	mov (%rax), %rax
	fxrstor (%rax)
	swapgs

	popq %rax
	popq %rbx
	popq %rcx
	popq %rdx
	popq %rsi
	popq %rdi
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %r11
	popq %r10
	popq %r9
	popq %r8
	popq %rbp
	iretq
	//pushq $0x0
	#iretq

// MORON: Since we're creating another stack, why not push to it using the accepted method? <-- This is horrible
.global x64_create_kernel_task_stack
x64_create_kernel_task_stack:
	cli
	// Switch stack
	movq %rsp, %rcx
	movq %rdi, %rsp

	// For RETURN-TO-OUTER-PRIVILEGE-LEVEL, something something RPL.
	// SS
	pushq $0x10
	// RSP
	pushq %rdi

	// Popped at end of IRET
	// eflags
	pushq $0x202
	// cs
	pushq $0x08
	pushq %rsi

	// RBP?
	pushq %rsp//$(%rsp+14*8)
	pushq $0
	pushq $0
	pushq $0
	pushq $0
	pushq $0
	pushq $0
	pushq $0
	pushq $0
	pushq $0
	pushq $0
	pushq $0
	pushq $0
	pushq $0
	pushq $0
	movq %rsp, %rax
	// Back again
	mov %rcx, %rsp
	// Returns the stack
	ret

.global x64_create_user_task_stack
x64_create_user_task_stack:
	cli
	// Switch stack
	movq %rsp, %r10
	movq %rdi, %rsp

	// For RETURN-TO-OUTER-PRIVILEGE-LEVEL, something something RPL.

	// SS
	//pushq $0x20
	pushq $0x1b
	// RSP
	pushq %rdx

	// Popped at end of IRET
	// rflags
	pushq $0x3202
	#pushq $0x3002
	// cs
	pushq $0x23
	//pushq $0x8
	// rip
	pushq %rsi

	// General registers
	// RBP?
	pushq %rdi
	pushq $0
	pushq $0
	pushq $0
	pushq $0
	pushq $0
	pushq $0
	pushq $0
	pushq $0
	pushq $0
	pushq $0
	pushq $0
	pushq $0
	pushq $0
	pushq $0
	movq %rsp, %rax
	// Back again
	mov %r10, %rsp
	// Returns the stack
	ret

.global x64_syscall_block_help
x64_syscall_block_help:
	cli
	// For RETURN-TO-OUTER-PRIVILEGE-LEVEL, something something RPL.
	// SS
	pushq $0x10
	// RSP
	pushq %rdi

	// Popped at end of IRET
	// rflags
	pushfq
	// cs
	pushq $0x08
	// rip
	pushq %rsi

	swapgs
	mov %gs:16, %rax
	mov (%rax), %rax
	fxrstor (%rax)
	swapgs

	// General registers
	// RBP?
	pushq %rbp
	pushq %r8
	pushq %r9
	pushq %r10
	pushq %r11
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	pushq %rdi
	pushq %rsi
	pushq %rdx
	pushq %rcx
	pushq %rbx
	pushq %rax
	movq %rsp, %rdi
	// Returns the stack
	call sched_sled
	ret

.global get_rip
get_rip:
	mov (%rsp), %rax
	ret
