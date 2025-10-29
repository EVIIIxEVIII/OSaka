global keyboard_stub
global page_fault_stub

extern keyboard_handler
extern page_fault_handler

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
    PUSH_REGS
    call keyboard_handler
    POP_REGS
    iretq

page_fault_stub:
    PUSH_REGS

    call page_fault_handler

    POP_REGS
    iretq

