// TODO save segment regs
.macro pushregs
	pushq %rbp
	movq %rsp, %rbp
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
.endm

.macro popregs
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
.endm

.global syscall_entry
syscall_entry:
	swapgs
	mov %rsp, %gs:8
	mov %gs:0, %rsp
	pushregs
	mov %gs:16, %rax
	mov (%rax), %rax
	fxsave (%rax)
	swapgs
	call syscall_proxy
	swapgs
	mov %gs:16, %rax
	mov (%rax), %rax
	fxrstor (%rax)
	popregs
	mov %gs:8, %rsp
	swapgs
	sysretq
