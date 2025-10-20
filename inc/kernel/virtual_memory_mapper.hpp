#pragma once
#include "shared/types.h"

typedef struct {
    byte* page_global_directory;
    byte* page_upper_directory;
    byte* page_middle_directory;
    byte* page_table;
} PageTables;

typedef enum {
    VM_RESERVED,
    VM_FREE,
    VM_UNSET,
    VM_USED,
} VmRangeFlags;

typedef struct VmRange {
    u64 start;
    u64 end;
    VmRangeFlags flag;
    VmRange* next;
} VmRange;

typedef enum PageTableEntryFlags {
    PAGE_TABLE_ENTRY_CLEAR_FLAGS = ~((1ULL << 12) - 1),
    PAGE_TABLE_ENTRY_PRESENT_BIT = (1 << 0),
    PAGE_TABLE_ENTRY_READ_WRITE_BIT = (1 << 1),
    PAGE_TABLE_ENTRY_USER_SUPERVISOR_BIT = (1 << 2),
} PageTableEntryFlags;

typedef struct {
    VmRange* base;
    u64 count;
    u64 capacity;
} VmRangeTable;

void vmm_init();
byte* vmm_map(u64 size);



