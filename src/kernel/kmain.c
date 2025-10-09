#include "shared.h"
#include "font.h"

uefi_data* global_uefi_data;
framebuffer_info* global_fb;

int cursorX = 0;
int cursorY = 0;

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

void keyboard_handler() {
    (void)inb8(0x60);
    printk("X");
}

void kmain(uefi_data *uefi) {
    global_uefi_data = uefi;
    global_fb = uefi->fb;

    clear_screen();

    __asm__ __volatile__("cli");
    load_gdt64();

    set_idt_gate(0x20, timer_stub, 0x08, 0x8E);
    set_idt_gate(0x21, keyboard_stub, 0x08, 0x8E);

    load_idt();
    __asm__ __volatile__("sti");

    __asm__ __volatile__("int 0x20");
    __asm__ __volatile__("int 0x21");

    printk("Hello World!");
}
