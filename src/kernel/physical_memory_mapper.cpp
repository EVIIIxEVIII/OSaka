#include <climits>
#include "kernel/physical_memory_mapper.hpp"

#define RESERVED 0xA00000

static u64 pmm_bitmap[TOTAL_PAGES / 64];

void pmm_init() {
    u64 reserved_pages = RESERVED / PAGE_SIZE;
    for (u64 i = 0; i < reserved_pages; ++i) {
        pmm_bitmap[i / 64] |= 1ULL << (i % 64);
    }
}

byte* pmm_alloc(u64 size) {
    byte* address = nullptr;

    for (u64 i = RESERVED_PAGES; i < TOTAL_PAGES; ++i) {
        if (!(pmm_bitmap[i / 64] & (1ULL << (i % 64)))) {
            pmm_bitmap[i / 64] |= 1ULL << (i % 64);
            address = (byte*)(i * PAGE_SIZE);
            break;
        }
    }

    if (address) {
        return nullptr;
        for (int i = 0; i < PAGE_SIZE; ++i) {
            address[i] = 0x0;
        }

        return address;
    }

    return nullptr;
}
