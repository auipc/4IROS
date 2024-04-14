.section .data
_stack_end:
.org 0x2000
_stack_base:

// https://developer.arm.com/documentation/100933/0100/AArch64-exception-vector-table
vec:
	b sp0_synchronus // S
	.balign 128
	b sp0_irq // IRQ
	.balign 128
	b sp0_fiq // FIQ
	.balign 128
	b sp0_serror // serror
	.balign 128

	b spx_synchronus // S
	.balign 128
	b spx_irq // IRQ
	.balign 128
	b spx_fiq // FIQ
	.balign 128
	b spx_serror // serror
	.balign 128

	b aarch64_synchronus // S
	.balign 128
	b aarch64_irq // IRQ
	.balign 128
	b aarch64_fiq // FIQ
	.balign 128
	b aarch64_serror // serror
	.balign 128

	b trap // S
	.balign 128
	b trap // IRQ
	.balign 128
	b trap // FIQ
	.balign 128
	b trap // serror

aarch64_fiq:
	b aarch64_fiq

aarch64_serror:
	b aarch64_serror

aarch64_irq:
	b aarch64_irq

aarch64_synchronus:
	b aarch64_synchronus

spx_fiq:
	b spx_fiq

spx_serror:
	b spx_serror

spx_irq:
	b spx_irq

spx_synchronus:
	//eret
	b spx_synchronus

sp0_fiq:
	b sp0_fiq

sp0_serror:
	b sp0_serror

sp0_irq:
	b sp0_irq

sp0_synchronus:
	b sp0_synchronus

trap:
	b trap

.section .text, "a"
.global _start
.align 4
.extern kernel_init
_start:
	ldr x0, =_stack_base
	mov sp, x0
	
	// pause other CPU cores
	mrs x0, MPIDR_EL1
	and x0, x0, 0xFF
	cbnz x0, loop

	// drop down to EL1
	mov x0, xzr
	orr x0, x0, #(1<<10)
	msr SCR_EL3, x0

	msr SCTLR_EL1, xzr
	ldr x0, =cont
	msr ELR_EL3, x0

	mov x0, sp
	msr SP_EL1, x0

	ldr x0, =0x05
	msr SPSR_EL3, x0

	ldr x0, =vec
	msr VBAR_EL1, x0
	msr VBAR_EL2, x0
	msr VBAR_EL3, x0

	eret
loop:
	wfi
	b loop

.global cont
cont:
	bl kernel_init
