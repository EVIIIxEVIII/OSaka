#include <stdarg.h>
#include "kernel/console.hpp"
#include "shared/types.h"
#include "kernel/font.hpp"

void console_set_fb(FramebufferInfo *p) { fb = *p; }

void putpx(u64 x, u64 y, u32 col) {
    uint32_t *base = (uint32_t*)fb.base;
    base[y * fb.width + x] = col;
}

void clear_screen(u64 color) {
    for (u32 i = 0; i < fb.height; ++i) {
        for (u32 j = 0; j < fb.width; ++j) {
            putpx(j, i, color);
        }
    }
}

void putchar(char c) {
    if (c == '\n') {
        cursorX = 0;
        cursorY++;
        return;
    }

    unsigned char* charBitmap = font8x16[(u8)c];
    for (u8 i = 0; i < 16; ++i) {
        u8 row = charBitmap[i];

        for (u8 j = 0; j < 8; ++j) {
            if (row & (0x80u >> j)) {
                putpx((cursorX*8)+j, (cursorY*16)+i, 0x0);
            } else {
                putpx((cursorX*8)+j, (cursorY*16)+i, 0xFFFFFFFF);
            }
        }
    }

    cursorX++;
}

static void print_dec_sig(i64 num) {
    char buf[32];
    int i = 0;
    if (num == 0) { putchar('0'); return; }
    if (num < 0) { putchar('-'); num = -num; }
    while (num) { buf[i++] = '0' + (num % 10); num /= 10; }
    while (i--) putchar(buf[i]);
}

static void print_dec_unsig(u64 num) {
    char buf[32];
    int i = 0;
    if (num == 0) { putchar('0'); return; }
    if (num < 0) { putchar('-'); num = -num; }
    while (num) { buf[i++] = '0' + (num % 10); num /= 10; }
    while (i--) putchar(buf[i]);
}

static void print_hex(u64 num) {
    const char *hex = "0123456789ABCDEF";
    putchar('0'); putchar('x');
    for (int i = (sizeof(num) * 8) - 4; i >= 0; i -= 4)
        putchar(hex[(num >> i) & 0xF]);
}

void printk(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            putchar(*fmt);
            continue;
        }
        fmt++;

        i32 precision = -1;
        if (*fmt == '.') {
            fmt++;
            precision = 0;
            while (*fmt >= '0' && *fmt <= '9') {
                precision = precision * 10 + (*fmt - '0');
                fmt++;
            }
        }

        switch(*fmt) {
            case 'c': {
                putchar(va_arg(args, int));
                break;
            }

            case 's': {
                char *s = va_arg(args, char*);
                i32 count = 0;
                while (*s && (precision <= 0 || count < precision)) {
                    putchar(*s++);
                    count++;
                }
                break;
            }
            case 'd':
                print_dec_sig(va_arg(args, i64));
                break;

            case 'u':
                print_dec_unsig(va_arg(args, u64));
                break;

            case 'x':
                print_hex(va_arg(args, unsigned int));
                break;

            case '%':
                putchar('%');
                break;

            default:
                putchar('?');
                break;
        }
    }

    va_end(args);
}
