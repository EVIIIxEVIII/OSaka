#include "kernel/apic.hpp"
#include "kernel/console.hpp"

APICEntries parseMADT(SDTHeader* entry) {
    printk("Header: %.4s \n", entry->signature);

    MADT* madt = (MADT*) entry;

    APICEntries apic;
    apic.header = madt->header;
    apic.pic_8259_support = madt->pic_8259_support;

    apic.lapic_count = 0;
    apic.io_apic_source_overrides_count = 0;
    apic.io_apic_count = 0;

    printk("Local APIC addr: %u\n", madt->lapic_address);
    printk("Legacy 8259: %d\n", madt->pic_8259_support);

    uint32_t remaining = madt->header.length - sizeof(MADT);
    printk("Entires Length: %d\n", remaining);

    uint8_t* ptr = (uint8_t*)madt->entries;
    uint8_t* end = (uint8_t*)madt + madt->header.length;

    while (ptr < end) {
        APICHeader* header = (APICHeader*)ptr;
        u8 type = header->entry_type;
        u8 len = header->record_len;

        printk("Type: %d, Length: %d\n", type, len);
        switch (type) {
            case APIC_IO_APIC: {
                apic.io_apics[apic.io_apic_count++] = *(APICEntryIOAPIC*)ptr;
                break;
            }

            case APIC_IO_APIC_ISO: {
                apic.io_apic_source_overrides[apic.io_apic_source_overrides_count++] = *(APICEntryIOAPICSourceOverride*)ptr;
                break;
            }

            case APIC_LAPIC: {
                 apic.lapics[apic.lapic_count++] = *(APICEntryLocalAPIC*)ptr;
                 break;
            }

            case APIC_LOCAL_2xAPIC: {
                apic.lapicsx2[apic.lapicsx2_count++] = *(APICEntryLocal2xAPIC*)ptr;
                break;
            }
        }

        ptr += header->record_len;
    }

    printk("Found %d IO APICs\n", apic.io_apic_count);
    printk("Found %d IO APIC ISOs\n", apic.io_apic_source_overrides_count);
    printk("Found %d LAPICS\n", apic.lapic_count);
    printk("Found %d 2xLAPICS\n", apic.lapicsx2_count);

    return apic;
}


