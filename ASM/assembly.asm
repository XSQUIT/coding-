section .data:
    hello: db "Hello World", 10
    len: $-hello

section .text
    mov rax, 1
    