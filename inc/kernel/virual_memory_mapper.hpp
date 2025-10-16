#pragma once
#include "shared/types.h"

typedef struct {
    byte* page_global_directory;
    byte* page_upper_directory;
    byte* page_middle_directory;
    byte* page_table;
} PageTables;


typedef struct VmRange {
    u64 start;
    u64 end;
    VmRange* next;
} VmRange;

void vmm_init();
byte* vmm_map(u64 size);



