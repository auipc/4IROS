.global _start
_start:
     popq %rdi
     popq %rsi
     call main
     mov %rax, %rdi
1:
     mov $1, %rax
     syscall
     jmp 1b
     retq
