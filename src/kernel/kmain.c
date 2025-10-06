#include "shared.h"

void print_debug(char c) {
    __asm__ __volatile__(
        "out dx, al" :
        : "a"(c), "d"(0xE9)
    );

}

void kmain(struct framebuffer_info *fb) {
    uint32_t *base = (uint32_t*)fb->base;

    for (int i = 0; i < 100000; ++i) {
        base[i] = 0xFFFFFFFF;
    }

    print_debug('Y');

}
