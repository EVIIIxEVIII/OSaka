#include "shared/shared.h"
#include "shared/types.h"
#include "kernel/apic.hpp"
#include "kernel/console.hpp"
#include "kernel/xsdt.hpp"
#include "kernel/scan_codes.hpp"
#include "kernel/physical_memory_mapper.hpp"
#include "kernel/virtual_memory_mapper.hpp"

BootData* global_boot_data;
constexpr u64 KERNEL_ADDR = 0x100000;
constexpr u64 STACK_SIZE = 32768;

extern "C" void set_idt_gate(u8 vec, void* handler, u16 selector, u8 type_attr);
extern "C" void load_idt();
extern "C" void load_gdt64();
extern "C" void keyboard_stub();
extern "C" void page_fault_stub();

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    asm volatile (
        "in al, dx"
        : "=a"(value)
        : "d"(port)
    );
    return value;
}

static inline void outb(u16 port, u8 value) {
    asm volatile (
        "out dx, al"
        :
        : "d"(port), "a"(value)
    );
}

u64 compare_mem(const void* dest, const void* src, u64 count) {
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
    *(volatile uint32_t*)(apic_base) = offset;
    *(volatile uint32_t*)(apic_base + 0x10) = val;
}

uint32_t read_ioapic_register(const uintptr_t apic_base, const uint8_t offset)
{
    *(volatile uint32_t*)(apic_base) = offset;
    return *(volatile uint32_t*)(apic_base + 0x10);
}

static inline uint64_t rdmsr(uint32_t msr) {
    u32 lo, hi;
    asm volatile ("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}

uintptr_t get_lapic_base(void) {
    u64 val = rdmsr(0x1B);           // IA32_APIC_BASE
    return (uintptr_t)(val & 0xFFFFF000); // bits 12–51 = physical base
}

static inline void lapic_eoi(void) {
    u64 apic_base = get_lapic_base();

    volatile uint32_t *lapic = (volatile uint32_t *)(apic_base);
    lapic[0xB0 / 4] = 0;
}

void setup_keyboard(const APICEntryIOAPIC* io_apic, const APICEntryLocalAPIC* lapic) {
    uint8_t irq     = 1;
    uint8_t vector  = 0x21;
    uintptr_t ioapic_base = io_apic->io_apic_address;

    uint64_t rte = 0;
    rte |= vector;                      // bits 0–7: vector
    rte |= (0ULL << 8);                 // bits 8–10: delivery mode = Fixed
    rte |= (0ULL << 11);                // bit 11: destination mode = Physical
    rte |= (0ULL << 13);                // bit 13: polarity = 0 (active-high)
    rte |= (0ULL << 15);                // bit 15: trigger = 0 (edge)
    rte |= (0ULL << 16);                // bit 16: mask = 0 (unmasked)
    rte |= ((u64)lapic->apic_id << 56); // bits 56–63: destination APIC ID

    write_ioapic_register(ioapic_base, 0x10 + 2 * irq,     (uint32_t)rte);
    write_ioapic_register(ioapic_base, 0x10 + 2 * irq + 1, (uint32_t)(rte >> 32));

    outb(0xA1, 0xFF);
    outb(0x21, 0xFF);
    set_idt_gate(0x21, (void*)keyboard_stub, 0x08, 0x8E);
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

BootData* copy_boot_data(BootData* boot_data, byte* memory) {
    RSDP* rsdp_copy = (RSDP*)memory;
    *rsdp_copy = *(boot_data->rsdp);
    memory += sizeof(RSDP);

    FramebufferInfo* fb_copy = (FramebufferInfo*)memory;
    *fb_copy = boot_data->fb;
    memory += sizeof(FramebufferInfo);


    BootData* dst = (BootData*) memory;
    dst->rsdp = rsdp_copy;
    dst->fb = *fb_copy;
    dst->kernel_size = boot_data->kernel_size;

    return dst;
}

void setup_interrupt_table(BootData* boot_data) {
    asm volatile("cli");
    load_gdt64();

    RSDP* rsdp = boot_data->rsdp;
    XSDT* xsdt = (XSDT*)rsdp->xsdt_address;
    u32 table_entries = (xsdt->header.length - sizeof(SDTHeader)) / 8;

    APICEntries apicEntries{};
    for (u32 i = 0; i < table_entries; ++i) {
        SDTHeader* header = (SDTHeader*)xsdt->entry[i];

        if (compare_mem(header->signature, "APIC", 4) == 0) {
            apicEntries = parseMADT(header);
        }
    }

    APICEntryIOAPIC io_apic = apicEntries.io_apics[0];
    APICEntryLocalAPIC lapic = apicEntries.lapics[0];

    setup_keyboard(&io_apic, &lapic);

    load_idt();
    asm volatile("sti");
}

void turn_on_virtual_memory(u64* page_table_base) {
    outb(0xE9, 'A');
    asm volatile(
        "mov cr3, rax"
        :
        : "a"(page_table_base)
        : "memory"
    );
    outb(0xE9, 'B');
}

extern "C" void page_fault_handler() {
    outb(0xE9, 'F');
}

void walk_page_table(u64 virt) {
    u64 pml4_i = (virt >> 39) & 0x1FF;
    u64 pdpt_i = (virt >> 30) & 0x1FF;
    u64 pd_i   = (virt >> 21) & 0x1FF;
    u64 pt_i   = (virt >> 12) & 0x1FF;

    u64 *pml4 = vmm_get_base();
    printk("pml4: %x \n", pml4);
    u64 *pdpt = (u64 *)(pml4[pml4_i] & 0xFFFFF000);
    printk("pdpt: %x \n", pdpt);
    u64 *pd   = (u64 *)(pdpt[pdpt_i] & 0xFFFFF000);
    printk("pd: %x \n", pd);
    u64 *pt   = (u64 *)(pd[pd_i] & 0xFFFFF000);
    printk("pt: %x \n", pt);

    u64 entry = pt[pt_i];
    printk("Entry for %x = %x\n", virt, entry);
}

extern "C" void kmain(BootData* temp_boot_data) {
    {
        pmm_init();
        vmm_init();
    }

    byte* reserved_memory = pmm_alloc(10 * PAGE_SIZE);
    vmm_identity_map((u64)reserved_memory, 10 * PAGE_SIZE);

    BootData* boot_data = copy_boot_data(temp_boot_data, reserved_memory);
    global_boot_data = boot_data;

    console_init(&boot_data->fb);
    clear_screen(0x0);

    u64 kernel_code_size = (boot_data->kernel_size + PAGE_SIZE - 1) & ~(PAGE_SIZE-1);
    u64 kernel_size = (kernel_code_size + STACK_SIZE + 2*PAGE_SIZE - 1) & ~(PAGE_SIZE-1);
    vmm_identity_map(KERNEL_ADDR, kernel_size);

    printk("Turning on Virtual memory\n");

    vmm_identity_map((u64)get_lapic_base(), PAGE_SIZE);
    setup_interrupt_table(boot_data);
    turn_on_virtual_memory(vmm_get_base());

    printk("Frame buffer is now mapped!\n");

    //volatile byte* page = pmm_alloc(1);
    //map_page((u64)page, (u64)page);
    //vmm_identity_map((u64)page, PAGE_SIZE);

    //byte* virtual_memory = vmm_map(10);
    //if (!virtual_memory) {
    //    printk("Failed to allocate virtual memory!\n");
    //} else {
    //    printk("Virtual memory location: %x \n", virtual_memory);
    //}

    //byte* page_table_base = vmm_get_base();
    //printk("Page table base: %x \n", page_table_base);

    for (;;) { asm volatile("hlt"); }
}
