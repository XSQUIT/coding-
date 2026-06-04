.global _start
.intel_syntax noprefix

_start:
    //sys-write
    mov rax, 1
    mov rdi, 1
    lea rsi, [int]
    mov rdx, 2
    syscall
    //exit
    mov rax, 60
    mov rdi, 0
    syscall

    
int:
    mov rdi, 8
    mov rsi, rdi
    add rsi, rdi
    .int rsi


