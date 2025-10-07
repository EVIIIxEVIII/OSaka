; needed for legacy segmentation on the CPU, not actually used, but without it the IDT doesn't work :(
BITS 64
global load_gdt64

section .data
align 16
gdt64:
    dq 0

    dw 0
    dw 0
    db 0
    db 10011010b
    db 00100000b
    db 0

    dw 0
    dw 0
    db 0
    db 10010010b
    db 00000000b
    db 0

gdt64_end:

gdtr64:
    dw gdt64_end - gdt64 - 1
    dq gdt64

section .text
load_gdt64:
    lgdt [gdtr64]

    ; reload segment registers
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax

    push qword 0x08
    lea  rax, [rel .next]
    push rax
    retfq

.next:
    ret
