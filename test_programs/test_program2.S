.global _start
.section .text
_start:
	mov $3, %eax
	int $0x80
	cmpl $0, %eax
	je .l
1:
	mov $1, %eax
	int $0x80
	jmp 1b
.l:
	jmp .l
