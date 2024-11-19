.code32
# Declare constants for the multiboot header.
.set ALIGN,    1<<0				# align loaded modules on page boundaries
.set MEMINFO,  1<<1				# provide memory map
.set FLAGS,    ALIGN | MEMINFO	# this is the Multiboot 'flag' field
.set MAGIC,    0x1BADB002		# 'magic number' lets bootloader find the header
.set CHECKSUM, -(MAGIC + FLAGS) # checksum of above, to prove we are multiboot

# Declare a multiboot header that marks the program as a kernel.
.section .multiboot.data, "aw"
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM
.long 0
.long 0
.long 0
.long 0
.long 0
.long 0
.long 0
.long 0
.long 32

# Allocate the initial stack.
.section .bootstrap_stack, "aw", @nobits
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

# The kernel entry point.
.section .multiboot.text, "a"
.set HIGHER_HALF, 0xC0000000
.global _pml4
.global pdpte0
.global pdbad
.global ptbad
.align 4096
_pml4:
	.skip 4096
pdpte0:
	.skip 4096
pd0:
	.skip 4096
pt0:
	.skip 4096
# kernel size is limited to 2mb yay
pdbad:
	.skip 4096
ptbad:
	.skip 4096

.align 8
gdt:
# entry 0 (null)
.long 0
.long 0
# FIXME do we need the RW bit set?
# entry 1 (kcode)
.long 0
# I didn't read the spec hard enough and L is supposed to be set while DB isn't.
#.long 0x609b00
.long 0x209b00
# entry 2 (kdata)
.long 0
.long 0x409300
# entry 3 (ucode)
#.long 0
#.long 0x20fb00
# entry 4 (udata)
#.long 0
#.long 0x40f300
gdt_end:

gdtptr:
	# ptr upper
	#.long 0
	# ptr lower
	.word gdt_end - gdt - 1
	.long gdt

map_page:
	push %ebx
	movl $(pdpte0), _pml4
	orl $3, _pml4

	mov $(pdpte0), %edx
	mov %eax, %ecx
	shrl $30, %ecx
	andl $0x1ff, %ecx
	imull $8, %ecx
	addl %ecx, %edx

	movl $(pd0), (%edx)
	#shrl $12, _pml4
	#shll $12, _pml4
	orl $3, (%edx)

	mov %eax, %ecx
	shrl $21, %ecx
	andl $0x1ff, %ecx
	mov $(pd0), %edx
	imull $8, %ecx
	addl %ecx, %edx

	movl $(pt0), (%edx)
	#shrl $12, pd0
	#shll $12, pd0
	orl $3, (%edx)
	
	/*addl $((HIGHER_HALF>>21)*8), %edx
	movl $(pt0), (%edx)
	#shrl $12, pd0
	#shll $12, pd0
	orl $3, (%edx)*/

	pop %ebx

	mov %eax, %ecx
	shrl $12, %ecx
	andl $0x1ff, %ecx
	imull $8, %ecx
	mov $pt0, %edx
	addl %ecx, %edx

	movl %ebx, (%edx)
	#shrl $12, pt0
	#shll $12, pt0
	orl $3, (%edx)

	ret

.global _start
.type _start, @function
_start:
	cli
	# move multiboot header
	mov %eax, %edi
	mov %ebx, %esi
	// https://wiki.osdev.org/X86-64#How_do_I_enable_Long_Mode_.3F
	movl $(stack_top - HIGHER_HALF), %esp
	# set Page Address Extension bit
	mov $32, %eax
	movl %eax, %cr4

	# load pml4
	movl $_pml4, %eax
	#subl $0xC0000000, %eax
	movl %eax, %cr3

	mov $0, %ebx
	mov $(_kernel_end-0xC0000000), %ecx
	jmp beg
rep:
	addl $0x1000, %ebx
beg:
	push %eax
	push %ecx
	push %edx
	# virt
	mov $0xC0000000, %eax
	addl %ebx, %eax

	call map_page
	# virt
	xor %eax, %eax
	addl %ebx, %eax

	call map_page

	pop %edx
	pop %ecx
	pop %eax

	cmpl %ebx, %ecx
	jge rep

	# set long mode, execute disable, and syscall enable bits
	movl $0x901, %eax
	#orl %ebx, %ecx
	movl $0xC0000080, %ecx
	wrmsr

	# enable paging
	movl %cr0, %eax
	orl $0x80000001, %eax
	movl %eax, %cr0

	mov $0x10, %eax
	movw %eax, %ds
	movw %eax, %fs
	movw %eax, %gs
	movw %eax, %ss
	movw %eax, %es
	lgdt gdtptr
	# finish the transition into 64 bit by far returning
	pushl $0x08
	pushl $kx86_cont
	lret
