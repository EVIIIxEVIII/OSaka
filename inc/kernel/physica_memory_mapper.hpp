#pragma once

#include "shared/types.h"
#define PAGE_SIZE 4096
#define TOTAL_MEM (64 * 1024 * 1024) // TODO: get from UEFI
#define TOTAL_PAGES (TOTAL_MEM / PAGE_SIZE)

static u64 pmm_bitmap[TOTAL_PAGES / 64];

void  physical_memory_mapper_init();
byte* physical_memory_mapper_alloc();


