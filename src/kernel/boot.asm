global _start

section .text
    _start:
        mov rsp, stack_top
        call kmain

        .hang:
            hlt
            jmp .hang

section .bss
    align 16
    stack:
        resb 16384

    align 16
    stack_top:
