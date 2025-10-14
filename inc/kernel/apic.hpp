#pragma once
#include "shared/types.h"
#include "shared/shared.h"
#include "xsdt.hpp"

#define APIC_LAPIC        0
#define APIC_IO_APIC      1
#define APIC_IO_APIC_ISO  2
#define APIC_IO_APIC_NIS  3
#define APIC_LAPIC_NIS    4
#define APIC_LAPIC_AO     5
#define APIC_LOCAL_2xAPIC 9

#define MAX_CPUS   32
#define MAX_IOAPIC 4
#define MAX_ISO    32

typedef struct PACK {
    u8 entry_type;
    u8 record_len;
} APICHeader;

typedef struct PACK {
    APICHeader header;
    u16        reserved;
    u32        lapicx2_id;
    u32        flags;
    u32        acpi_id;
} APICEntryLocal2xAPIC;

typedef struct PACK {
    APICHeader header;
    u8         acpi_processor_id;
    u8         apic_id;
    u32        flags;
} APICEntryLocalAPIC;

typedef struct PACK {
    APICHeader header;
    u8         io_apic_id;
    u8         reserved;
    u32        io_apic_address;
    u32        global_system_interrupt_base;
} APICEntryIOAPIC;

typedef struct PACK {
    APICHeader header;
    u8         bus_source;
    u8         irq_source;
    u32        global_system_interrupt;
    u16        flags;
} APICEntryIOAPICSourceOverride;

typedef struct PACK {
    SDTHeader   header;
    u32         lapic_address;
    u32         pic_8259_support; // legacy not used
    byte        entries[];
} MADT;

typedef struct PACK {
    SDTHeader header;
    u32       lapic_address;
    u32       pic_8259_support; //legacy not used

    u64       lapic_count;
    u64       lapicsx2_count;
    u64       io_apic_count;
    u64       io_apic_source_overrides_count;

    APICEntryLocal2xAPIC          lapicsx2[MAX_CPUS];
    APICEntryLocalAPIC            lapics[MAX_CPUS];
    APICEntryIOAPIC               io_apics[MAX_IOAPIC];
    APICEntryIOAPICSourceOverride io_apic_source_overrides[MAX_ISO];
} APICEntries;

APICEntries parseMADT(SDTHeader* header);
