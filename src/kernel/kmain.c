#include "shared.h"
#include "font.h"

int cursorX = 0;
int cursorY = 0;

void print_debug(char c) {
    __asm__ __volatile__(
        "out dx, al" :
        : "a"(c), "d"(0xE9)
    );

}

void putpx(framebuffer_info* fb, uint64_t x, uint64_t y, uint32_t col) {
    uint32_t *base = (uint32_t*)fb->base;
    base[y * fb->width + x] = col;
}

void putchar(framebuffer_info* fb, char c) {
    unsigned char* charBitmap = font8x16[(uint8_t)c];

    for (int i = 0; i < 16; ++i) {
        unsigned char row = charBitmap[i];

        for (int j = 0; j < 8; ++j) {
            if (row & (0x80u >> j)) {
                putpx(fb, (cursorX*8)+j, (cursorY*16)+i, 0x0);
            } else {
                putpx(fb, (cursorX*8)+j, (cursorY*16)+i, 0xFFFFFFFF);
            }
        }
    }

    cursorX++;
}

void kmain(framebuffer_info *fb) {
    uint32_t *base = (uint32_t*)fb->base;
    //uint64_t totalPixels = (uint64_t)fb->height * (uint64_t)fb->width;

    for (uint32_t i = 0; i < fb->height; ++i) {
        for (uint32_t j = 0; j < fb->width; ++j) {
            putpx(fb, j, i, 0xFFFFFFFF);
        }
    }

    putchar(fb, 'H');
    putchar(fb, 'e');
    putchar(fb, 'l');
    putchar(fb, 'l');
    putchar(fb, 'o');

    putchar(fb, ' ');

    putchar(fb, 'w');
    putchar(fb, 'o');
    putchar(fb, 'r');
    putchar(fb, 'l');
    putchar(fb, 'd');
    putchar(fb, '!');

    print_debug('Y');

}
