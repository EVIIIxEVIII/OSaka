#pragma once

#include "shared/types.h"
#define PAGE_SIZE 4096
#define TOTAL_MEM (64 * 1024 * 1024) // TODO: get from UEFI
#define TOTAL_PAGES (TOTAL_MEM / PAGE_SIZE)
#define RESERVED 0xA00000
#define RESERVED_PAGES (RESERVED / PAGE_SIZE)

void  pmm_init();
byte* pmm_alloc(u64 size);


