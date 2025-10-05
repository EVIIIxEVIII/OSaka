

void kmain(void) {
    volatile char *vga = (volatile char*)0xB8000;
    const char *msg = "Kernel loaded successfully!";
    for (int i = 0; msg[i] != '\0'; ++i) {
        vga[i * 2]     = msg[i];
        vga[i * 2 + 1] = 0x0F;
    }

    //__asm__ __volatile__("ud2");
}
