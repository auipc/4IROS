.global _start
_start:
     popq %rdi
     popq %rsi
     call main
     mov %rax, %rdi
1:
;     mov $1, %rax
;     syscall
     mov $1, %rdi
     call exit
     jmp 1b
     retq
