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
	swapgs
	mov %gs:16, %rax
	mov (%rax), %rax
	fxsave (%rax)
.endm

.macro popregs
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
.endm

.global interrupt_0_begin
interrupt_0_begin:
	cli
	pushregs
	call div_interrupt
	popregs
	addq $8, %rsp
	sti
	iretq

.global interrupt_5_begin
interrupt_5_begin:
	cli
	pushregs
	call unhandled_interrupt
	popregs
	sti
	iretq

.global interrupt_6_begin
interrupt_6_begin:
	cli
	pushregs
	call unhandled_interrupt
	popregs
	sti
	iretq

.global interrupt_7_begin
interrupt_7_begin:
	cli
	pushregs
	call unhandled_interrupt
	popregs
	sti
	iretq

.global interrupt_8_begin
interrupt_8_begin:
	cli
	pushregs
	call df_interrupt
	popregs
	addq $8, %rsp
	sti
	iretq

.global interrupt_10_begin
interrupt_10_begin:
	cli
	pushregs
	call tss_interrupt
	popregs
	addq $8, %rsp
	sti
	iretq

.global interrupt_11_begin
interrupt_11_begin:
	cli
	pushregs
	call unhandled_interrupt
	popregs
	sti
	iretq

.global interrupt_12_begin
interrupt_12_begin:
	cli
	pushregs
	call unhandled_interrupt
	popregs
	sti
	iretq

.global interrupt_13_begin
interrupt_13_begin:
	cli
	pushregs
	call gpf_interrupt
	popregs
	addq $8, %rsp
	sti
	iretq

.global interrupt_14_begin
interrupt_14_begin:
	cli
	pushregs
	call pf_interrupt
	popregs
	addq $8, %rsp
	sti
	iretq

.global interrupt_16_begin
interrupt_16_begin:
	cli
	pushregs
	call unhandled_interrupt
	popregs
	sti
	iretq

.global interrupt_65_begin
interrupt_65_begin:
	cli
	pushregs
	call ps2kbd_interrupt
	popregs
	sti
	iretq


.global interrupt_78_begin
interrupt_78_begin:
	cli
	pushregs
	call write_eoi
	popregs
	sti
	iretq


.global interrupt_48_begin
interrupt_48_begin:
	cli
	pushregs
	call timer_interrupt
	popregs
	sti
	iretq
