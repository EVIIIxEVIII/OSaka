.intel_syntax noprefix
.global _start
.extern kmain

.section .text.boot
_start:
    cli
    mov  rsp, [0x104060]

    mov   rax, 0xB8000
    mov   word ptr [rax], 0x0F4F   # 'O' white on black
    add   rax, 2
    mov   word ptr [rax], 0x0F4B

    call kmain

    .hang:
        hlt
        jmp .hang

.section .bss
.balign 16
    .skip 16384
.balign 16
stack_top:

