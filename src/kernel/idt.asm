BITS 64
global load_idt
global set_idt_gate

section .data
align 16
idt_table:
    times 256 dq 0, 0
idtr:
    dw idtr_table_end - idt_table - 1
    dq idt_table
idtr_table_end:


section .text

load_idt:
    mov al, 'A'
    out 0xE9, al

    lidt [idtr]
    ret

; void set_idt_gate(uint8_t vec, void* handler, uint16_t selector, uint8_t type_attr)
; rdi = vec
; rsi = handler
; rdx = selector
; rcx = type_attr
set_idt_gate:
; The layout of a IDT table entry is:
; 2 bytes for the first 16 bits of the handler address
; 2 bytes for the selector used to specify the ring and the type of intererupt
; 1 bytes for the ist 0 for no stack switch and 1 for switch to a kernel stack (0 for now)
; 1 bytes for the type attribute
; 2 bytes for the next 16 bits of the handler address
; 4 bytes for the last 32 bits of the handler address
; 4 bytes are reserved so they are 0

    mov al, 'B'
    out 0xE9, al

    movzx rax, dil
    shl   rax, 4 ; convert the index to byte offset (16 bytes per entry)
    lea   r8,  [idt_table]
    add   r8,  rax

    mov   rax, rsi

    mov   word [r8 + 0], ax ; low 16 bits

    mov   word [r8 + 2], dx

    mov   byte [r8 + 4], 0

    mov   byte [r8 + 5], cl

    shr   rax, 16
    mov   word [r8 + 6], ax ; mid 16 bits (for 32 bit systems)

    shr   rax, 16
    mov   dword [r8 + 8], eax ; high 32 bits (for 64 bit systems)

    mov   dword [r8 + 12], 0
    ret


