#include "kernel/physical_memory_mapper.hpp"

#define RESERVED 0xA00000

static u64 pmm_bitmap[TOTAL_PAGES / 64];

static bool is_free(u64 page) {
    return!(pmm_bitmap[page / 64] & (1ULL << (page % 64)));
}

static void set_reserved(u64 page) {
    pmm_bitmap[page / 64] |= 1ULL << (page % 64);
}

void pmm_init() {
    u64 reserved_pages = RESERVED / PAGE_SIZE;
    for (u64 i = 0; i < reserved_pages; ++i) {
        pmm_bitmap[i / 64] |= 1ULL << (i % 64);
    }
}

byte* pmm_alloc(u64 size) {
    u64 aligned_size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    u64 pages = aligned_size / PAGE_SIZE;

    u64 candidates = 0;
    u64 segment_start = UINT64_MAX;

    for (u64 i = RESERVED_PAGES; i < TOTAL_PAGES; ++i) {
        if (is_free(i)) {
            if(candidates++ == pages) {
                segment_start = i - pages;
                break;
            }
        } else {
            candidates = 0;
        }
    }

    if (segment_start == UINT64_MAX) {
        return nullptr;
    }

    for (u64 i = 0; i < pages; ++i) {
        set_reserved(segment_start + i);
    }

    byte* address = (byte*)(segment_start * PAGE_SIZE);
    for (u64 i = 0; i < pages * PAGE_SIZE; ++i) {
        address[i] = 0x0;
    }

    return address;
}
