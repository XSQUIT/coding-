.global _start
.intel_syntax noprefix

_start:

        mov rax, 1
        mov rdi, 1
        lea rsi, [hello_world]
        mov rdx, 14
        syscall

        
        mov rdi, 8
        mov rax, 60
        
        syscall

hello_world:
        .asciz "Hello, World!\n"
