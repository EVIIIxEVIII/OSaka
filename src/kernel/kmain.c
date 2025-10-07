#include "shared.h"
#include "font.h"

extern void set_idt_gate(uint8_t vec, void* handler, uint16_t selector, uint8_t type_attr);
extern void load_idt();
extern void load_gdt64();
extern void keyboard_stub();
extern void isr_test_stub();

framebuffer_info* global_fb;
int cursorX = 0;
int cursorY = 0;

void print_debug(char c) {
    __asm__ __volatile__(
        "out dx, al" :
        : "a"(c), "d"(0xE9)
    );
}

static inline unsigned char inb(unsigned short port) {
    unsigned char value;
    __asm__ volatile (
        "in al, dx"
        : "=a"(value)
        : "d"(port)
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

void pic_remap_20_28(void) {
    unsigned char m = inb(0x21);
    unsigned char s = inb(0xA1);

    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    outb(0x21, m);
    outb(0xA1, s);
}

void pic_unmask_irq(unsigned irq) {
    if (irq < 8) {
        unsigned char m = inb(0x21);
        outb(0x21, m & ~(1u << irq));
    } else {
        irq -= 8;
        unsigned char s = inb(0xA1);
        outb(0xA1, s & ~(1u << irq));
    }
}

void pic_mask_all(void) {
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
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

void keyboard_handler() {
    printk("X");
}

static void clear_screen() {
    for (uint32_t i = 0; i < global_fb->height; ++i) {
        for (uint32_t j = 0; j < global_fb->width; ++j) {
            putpx(j, i, 0xFFFFFFFF);
        }
    }
}

void kmain(framebuffer_info *fb) {
    __asm__ __volatile__("cli");
    global_fb = fb;
    clear_screen();

    load_gdt64();
    set_idt_gate(0x40, isr_test_stub, 0x08, 0x8E);
    set_idt_gate(0x21, keyboard_stub, 0x08, 0x8E);
    load_idt();

    pic_remap_20_28();
    pic_mask_all();
    pic_unmask_irq(1);
    __asm__ __volatile__("sti");

    __asm__ __volatile__("int 0x21");

    printk("Hello World!");
}
