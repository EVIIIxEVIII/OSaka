global keyboard_stub
global isr_test_stub
extern keyboard_handler

section .text

%macro PUSH_REGS 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro


%macro POP_REGS 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

keyboard_stub:
    push rax
    push rdx

    ; read scancode to deassert IRQ1
    mov     dx, 0x60
    in      al, dx

    ; debug: write 'K' to QEMU port 0xE9
    mov     dx, 0xE9
    mov     al, 'K'
    out     dx, al

    ; EOI to master PIC
    mov     dx, 0x20
    mov     al, 0x20
    out     dx, al

    pop     rdx
    pop     rax
    iretq

isr_test_stub:
    push rax
    mov  al, 'I'
    out  0xE9, al
    pop  rax

    iretq
