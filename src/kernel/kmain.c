

void print_debug() {
    __asm__ __volatile__("mov dx, 0xE9");
    __asm__ __volatile__("mov al, 'X'");
    __asm__ __volatile__("out dx, al");
}

void kmain(void) {

    print_debug();

}
