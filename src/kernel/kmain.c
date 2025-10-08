#include "shared.h"
#include "font.h"

#define PIC1 0x20
#define PIC2 0xA0
#define PIC1_COMMAND PIC1
#define PIC1_DATA    (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA    (PIC2 + 1)
#define PIC_EOI      0x20

#define ICW1_ICW4       0x01
#define ICW1_SINGLE     0x02
#define ICW1_INTERVAL4  0x04
#define ICW1_INIT       0x10

#define ICW4_8086       0x01
#define ICW4_AUTO       0x02
#define ICW4_BUF_SLAVE  0x08
#define ICW4_BUF_MASTER 0x0C
#define ICW4_SFNM       0x10

#define CASCADE_IRQ     2

extern void set_idt_gate(uint8_t vec, void* handler, uint16_t selector, uint8_t type_attr);
extern void load_idt();
extern void load_gdt64();
extern void keyboard_stub();
extern void isr_test_stub();
extern void timer_stub();

static inline uint8_t inb8(uint16_t p){ uint8_t v; __asm__ volatile("in al, dx":"=a"(v):"d"(p)); return v; }
static inline unsigned char inb(unsigned short port) {
    unsigned char value;
    __asm__ volatile (
        "in al, dx"
        : "=a"(value)
        : "d"(port)
        : "memory"
    );
    return value;
}

static inline void outb(unsigned short port, unsigned char value) {
    __asm__ volatile (
        "out dx, al"
        :
        : "d"(port), "a"(value)
    );
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

framebuffer_info* global_fb;
int cursorX = 0;
int cursorY = 0;

void print_debug(char c) {
    __asm__ __volatile__(
        "out dx, al" :
        : "a"(c), "d"(0xE9)
    );
}


void putpx(uint64_t x, uint64_t y, uint32_t col) {
    uint32_t *base = (uint32_t*)global_fb->base;
    base[y * global_fb->width + x] = col;
}

void putchar(char c) {
    unsigned char* charBitmap = font8x16[(uint8_t)c];

    for (int i = 0; i < 16; ++i) {
        unsigned char row = charBitmap[i];

        for (int j = 0; j < 8; ++j) {
            if (row & (0x80u >> j)) {
                putpx((cursorX*8)+j, (cursorY*16)+i, 0x0);
            } else {
                putpx((cursorX*8)+j, (cursorY*16)+i, 0xFFFFFFFF);
            }
        }
    }

    cursorX++;
}

void printk(const char* str) {
    int i = 0;

    while (str[i] != '\0') {
        putchar(str[i]);
        i++;
    }
}

static void clear_screen() {
    for (uint32_t i = 0; i < global_fb->height; ++i) {
        for (uint32_t j = 0; j < global_fb->width; ++j) {
            putpx(j, i, 0xFFFFFFFF);
        }
    }
}

void PIC_sendEOI(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }

    outb(PIC1_COMMAND, PIC_EOI);
}

void keyboard_handler() {
    (void)inb8(0x60);
    printk("X");
    PIC_sendEOI(1);
}

void IRQ_set_mask(uint8_t IRQline) {
    uint16_t port;
    uint8_t value;

    if (IRQline < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        IRQline -= 8;
    }

    value = inb(port) | (1 << IRQline);
    outb(port, value);
}

void IRQ_clear_mask(uint8_t IRQline) {
    uint16_t port;
    uint8_t value;

    if (IRQline < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        IRQline -= 8;
    }

    value = inb(port) & ~(1 << IRQline);
    outb(port, value);
}

void PIC_remap(int offset1, int offset2) {
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    outb(PIC1_DATA, offset1);
    io_wait();
    outb(PIC2_DATA, offset2);
    io_wait();

    outb(PIC1_DATA, 1 << CASCADE_IRQ);
    io_wait();
    outb(PIC2_DATA, 2);
    io_wait();

    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    outb(PIC1_DATA, 0x0);   // master PIC: unmask IRQ1 only
    outb(PIC2_DATA, 0x0);   // slave PIC: all masked
}

void kmain(framebuffer_info *fb) {
    global_fb = fb;
    clear_screen();

    __asm__ __volatile__("cli");
    load_gdt64();

    PIC_remap(0x20, 0x28);
    set_idt_gate(0x20, timer_stub, 0x08, 0x8E);
    set_idt_gate(0x21, keyboard_stub, 0x08, 0x8E);
    set_idt_gate(0x29, keyboard_stub, 0x08, 0x8E);

    load_idt();
    __asm__ __volatile__("sti");

    __asm__ __volatile__("int 0x20");
    __asm__ __volatile__("int 0x21");

    printk("Hello World!");
}
