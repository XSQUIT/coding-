.global _start
.intel_syntax noprefix

.section .bss
    .lcomm buffer, 20
.section .text
_start:
    //sys-write
    mov rdi, 8
    mov rsi, rdi
    add rsi, rsi

    add rsi, 48

    lea rbx, [rip + buffer]
    mov [rbx], sil
    mov byte ptr [rbx+1], 10

    mov rax, 1
    mov rdi, 1
    lea rsi, [rip + buffer]
    mov rdx, 2
    syscall
    
    syscall
    //exit
    mov rax, 60
    mov rdi, 0
    syscall

    


