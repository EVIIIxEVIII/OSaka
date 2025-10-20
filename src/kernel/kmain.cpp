#include "shared/shared.h"
#include "shared/types.h"
#include "kernel/apic.hpp"
#include "kernel/console.hpp"
#include "kernel/xsdt.hpp"
#include "kernel/scan_codes.hpp"
#include "kernel/physical_memory_mapper.hpp"

BootData* global_boot_data;

#define PAGE_TABLE_ADDR 0xA00000;

extern "C" void set_idt_gate(u8 vec, void* handler, u16 selector, u8 type_attr);
extern "C" void load_idt();
extern "C" void load_gdt64();
extern "C" void keyboard_stub();
extern "C" void isr_test_stub();
extern "C" void timer_stub();

#define LAPIC_BASE 0xFEE00000
#define LAPIC_EOI  0xB0
static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ __volatile__ (
        "in al, dx"
        : "=a"(value)
        : "d"(port)
    );
    return value;
}

static inline void outb(u16 port, u8 value) {
    __asm__ volatile (
        "out dx, al"
        :
        : "d"(port), "a"(value)
    );
}

u64 compareMem(const void* dest, const void* src, u64 count) {
    byte* dest_b = (byte*) dest;
    byte* src_b = (byte*) src;

    u64 ans = 0;
    for (u64 i = 0; i < count; ++i) {
        if (dest_b[i] != src_b[i]) {
            ans++;
        }
    }

    return ans;
}

void write_ioapic_register(const uintptr_t apic_base, const uint8_t offset, const uint32_t val)
{
    /* tell IOREGSEL where we want to write to */
    *(volatile uint32_t*)(apic_base) = offset;
    /* write the value to IOWIN */
    *(volatile uint32_t*)(apic_base + 0x10) = val;
}

uint32_t read_ioapic_register(const uintptr_t apic_base, const uint8_t offset)
{
    /* tell IOREGSEL where we want to read from */
    *(volatile uint32_t*)(apic_base) = offset;
    /* return the data from IOWIN */
    return *(volatile uint32_t*)(apic_base + 0x10);
}

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t lo, hi;
    __asm__ volatile ("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}

uintptr_t get_lapic_base(void) {
    uint64_t val = rdmsr(0x1B);           // IA32_APIC_BASE
    return (uintptr_t)(val & 0xFFFFF000); // bits 12–51 = physical base
}

static inline void lapic_eoi(void) {
    uint64_t apic_base = LAPIC_BASE;

    volatile uint32_t *lapic = (volatile uint32_t *)(apic_base);
    lapic[0xB0 / 4] = 0;
}

void setup_keyboard(const APICEntryIOAPIC* io_apic, const APICEntryLocalAPIC* lapic) {
    uint8_t irq     = 1;
    uint8_t vector  = 0x21;
    uintptr_t ioapic_base = io_apic->io_apic_address;

    uint64_t rte = 0;
    rte |= vector;                    // bits 0–7: vector
    rte |= (0ULL << 8);               // bits 8–10: delivery mode = Fixed
    rte |= (0ULL << 11);              // bit 11: destination mode = Physical
    rte |= (0ULL << 13);              // bit 13: polarity = 0 (active-high)
    rte |= (0ULL << 15);              // bit 15: trigger = 0 (edge)
    rte |= (0ULL << 16);              // bit 16: mask = 0 (unmasked)
    rte |= ((u64)lapic->apic_id << 56); // bits 56–63: destination APIC ID

    write_ioapic_register(ioapic_base, 0x10 + 2 * irq,     (uint32_t)rte);
    write_ioapic_register(ioapic_base, 0x10 + 2 * irq + 1, (uint32_t)(rte >> 32));
}

extern "C" void keyboard_handler() {
    while (inb(0x64) & 1) {
        uint8_t sc = inb(0x60);
        if (sc < 128) {
            printk("%c", scancode_to_ascii[sc]);
        }
    }
    lapic_eoi();
}

typedef struct {
    u64* page_global_directory;
    u64* page_upper_directory;
    u64* page_middle_directory;
    u64* page_table;
} PageTables;

extern "C" void kmain(BootData* boot_data) {
    global_boot_data = boot_data;
    console_set_fb(&boot_data->fb);
    clear_screen(0xFFFFFFFF);
    pmm_init();

    __asm__ __volatile__("cli");
    load_gdt64();

    outb(0xA1, 0xFF);
    outb(0x21, 0xFF);

    set_idt_gate(0x21, (void*)keyboard_stub, 0x08, 0x8E);

    RSDP* rsdp = boot_data->rsdp;
    XSDT* xsdt = (XSDT*)rsdp->xsdt_address;
    u32 table_entries = (xsdt->header.length - sizeof(SDTHeader)) / 8;

    APICEntries apicEntries{};
    for (u32 i = 0; i < table_entries; ++i) {
        SDTHeader* header = (SDTHeader*)xsdt->entry[i];

        if (compareMem(header->signature, "APIC", 4) == 0) {
            apicEntries = parseMADT(header);
        }
    }

    APICEntryIOAPIC io_apic = apicEntries.io_apics[0];
    APICEntryLocalAPIC lapic = apicEntries.lapics[0];

    setup_keyboard(&io_apic, &lapic);
    load_idt();
    __asm__ __volatile__("sti");

    byte* memory = pmm_alloc(7);
    printk("Memory location: %x \n", memory);

    memory = pmm_alloc(7);
    printk("Memory location: %x \n", memory);

    memory = pmm_alloc(4097);
    printk("Memory location: %x \n", memory);

    memory = pmm_alloc(7);
    printk("Memory location: %x \n", memory);
    for (;;) { asm volatile("hlt"); }
}
