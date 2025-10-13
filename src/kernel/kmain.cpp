#include "shared.h"
#include "apic.hpp"
#include "types.h"
#include "console.hpp"
#include "xsdt.hpp"

BootData* global_boot_data;

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

uint8_t scancode_to_ascii[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'','`', 0,'\\',
    'z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' '
};
static u8 break_next = 0;
extern "C" void keyboard_handler() {
    while (inb(0x64) & 1) {
        uint8_t sc = inb(0x60);
        if (sc == 0xF0) {
            break_next = 1;
        } else if (!break_next) {
            printk("%c", scancode_to_ascii[sc]);
        } else {
            break_next = 0;
        }
    }
    lapic_eoi();
}

extern "C" void kmain(BootData* boot_data) {
    global_boot_data = boot_data;
    console_set_fb(&boot_data->fb);
    clear_screen(0xFFFFFFFF);

    __asm__ __volatile__("cli");
    load_gdt64();

    outb(0xA1, 0xFF);
    outb(0x21, 0xFF);

    set_idt_gate(0x21, (void*)keyboard_stub, 0x08, 0x8E);

    RSDP* rsdp = boot_data->rsdp;
    printk("Rsdp sig: %.8s \n", rsdp->signature);
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

    printk("I/O APIC id: %u \n", io_apic.io_apic_id);
    printk("I/O APIC address: %u \n", io_apic.io_apic_address);
    printk("I/O APIC reserved: %u \n", io_apic.reserved);
    printk("I/O APIC global system interrupt base: %u \n", io_apic.global_system_interrupt_base);

    printk("I/O APIC ");

    APICEntryLocalAPIC lapic = apicEntries.lapics[0];

    printk("LAPIC processor id: %u \n", lapic.acpi_processor_id);
    printk("LAPIC apic id: %u \n", lapic.apic_id);
    printk("LAPIC flag: %u \n", lapic.flags);
    printk("LAPIC address: %u \n", get_lapic_base());

    uint8_t irq     = 1;
    uint8_t vector  = 0x21;
    uintptr_t ioapic_base = io_apic.io_apic_address;

    uint64_t rte = 0;
    rte |= vector;                    // bits 0–7: vector
    rte |= (0ULL << 8);               // bits 8–10: delivery mode = Fixed
    rte |= (0ULL << 11);              // bit 11: destination mode = Physical

    rte |= (0ULL << 13);              // bit 13: polarity = 0 (active-high)

    rte |= (0ULL << 15);              // bit 15: trigger = 0 (edge)
    rte |= (0ULL << 16);              // bit 16: mask = 0 (unmasked)
    rte |= ((u64)lapic.apic_id << 56); // bits 56–63: destination APIC ID

    write_ioapic_register(ioapic_base, 0x10 + 2 * irq,     (uint32_t)rte);
    write_ioapic_register(ioapic_base, 0x10 + 2 * irq + 1, (uint32_t)(rte >> 32));

    load_idt();
    __asm__ __volatile__("sti");


    for (;;) {
        asm volatile("hlt");
    }
}
