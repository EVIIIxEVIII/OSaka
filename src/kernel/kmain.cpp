#include "shared.h"
#include "font.h"
#include "types.h"

BootData* global_boot_data;
FramebufferInfo* global_fb;

int cursorX = 0;
int cursorY = 0;

typedef struct PACK {
  char Signature[4];
  uint32_t Length;
  uint8_t Revision;
  uint8_t Checksum;
  char OEMID[6];
  char OEMTableID[8];
  uint32_t OEMRevision;
  uint32_t CreatorID;
  uint32_t CreatorRevision;
} SDT_HDR;

typedef struct PACK {
    u8  ACPIProcessorId;
    u8  APICId;
    u32 Flags;
} APICEntryLocalAPIC;

typedef struct PACK {
    u8  IOAPICId;
    u8  Reserved;
    u32 IOAPICAddress;
    u32 GlobalSystemInterruptBase;
} APICEntryIOAPIC;

typedef struct PACK {
    u8 EntryType;
    u8 RecordLen;
    union {
        APICEntryIOAPIC ApicEntryIOApic;
        APICEntryLocalAPIC ApicEntryLocalApic;
    };
} APICEntry;

typedef struct PACK {
    SDT_HDR   Header;
    u32       LAPIC;
    u32       PIC8259Support;
    APICEntry Entries[];
} APIC;

extern "C" void set_idt_gate(u8 vec, void* handler, u16 selector, u8 type_attr);
extern "C" void load_idt();
extern "C" void load_gdt64();
extern "C" void keyboard_stub();
extern "C" void isr_test_stub();
extern "C" void timer_stub();

static inline void outb(u16 port, u8 value) {
    __asm__ volatile (
        "out dx, al"
        :
        : "d"(port), "a"(value)
    );
}

void print_debug(char c) {
    __asm__ __volatile__(
        "out dx, al" :
        : "a"(c), "d"(0xE9)
    );
}

void putpx(u64 x, u64 y, u32 col) {
    uint32_t *base = (uint32_t*)global_fb->base;
    base[y * global_fb->width + x] = col;
}

void putchar(char c) {
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

void printk(const char* str) {
    int i = 0;

    while (str[i] != '\0') {
        putchar(str[i]);
        i++;
    }
}

static void clear_screen() {
    for (u32 i = 0; i < global_fb->height; ++i) {
        for (u32 j = 0; j < global_fb->width; ++j) {
            putpx(j, i, 0xFFFFFFFF);
        }
    }
}

void keyboard_handler() {
    printk("X");
}



extern "C" void kmain(BootData* boot_data) {
    global_boot_data = boot_data;
    global_fb = &boot_data->fb; // TODO: make it aligned

    clear_screen();

    __asm__ __volatile__("cli");
    load_gdt64();

    outb(0xA1, 0xFF);
    outb(0x21, 0xFF);

    set_idt_gate(0x20, (void*)timer_stub, 0x08, 0x8E);
    set_idt_gate(0x21, (void*)keyboard_stub, 0x08, 0x8E);

    load_idt();
    __asm__ __volatile__("sti");

    __asm__ __volatile__("int 0x20");
    __asm__ __volatile__("int 0x21");

    printk("Hello World!");
}
