bits 64
default rel

global _start
extern kmain

section .text.boot
_start:
    mov dx, 0xE9
    mov al, 'X'
    out dx, al

    mov     rsp, stack_top

    call    kmain

    mov dx, 0xE9
    mov al, 'X'
    out dx, al

    .hang:
        cli
        hlt
        jmp     .hang

section .bss
align 16
    resb 16384
align 16
stack_top:

