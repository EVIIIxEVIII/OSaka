#include "kernel/virual_memory_mapper.hpp"
#include "kernel/physical_memory_mapper.hpp"


typedef enum PageTableEntryFlags {
    PAGE_TABLE_ENTRY_CLEAR_FLAGS = ~((1ULL << 12) - 1),
    PAGE_TABLE_ENTRY_PRESENT_BIT = (1 << 0),
    PAGE_TABLE_ENTRY_READ_WRITE_BIT = (1 << 1),
    PAGE_TABLE_ENTRY_USER_SUPERVISOR_BIT = (1 << 2),
} PageTableEntryFlags;

static VmRange vm_range{};
static u64* page_global_directory = (u64*)pmm_alloc(4096);

void vmm_init() {
    vm_range.start = 0x0;
    vm_range.end = (1ULL << 48) - 1;
    vm_range.next = nullptr;
}

byte* vmm_map(u64 size) {
    VmRange current = vm_range;

    for (;;) {



    }

}

u64* get_or_alloc_next(u64* table, u64 index) {
    if (table[index] & 1) {
        return (u64*)table[index];
    }

    u64* next_table_address = (u64*)pmm_alloc(4096);
    u64 next_table_entry = (u64)next_table_address;

    next_table_entry &= PAGE_TABLE_ENTRY_CLEAR_FLAGS;
    next_table_entry |= PAGE_TABLE_ENTRY_PRESENT_BIT;
    next_table_entry |= PAGE_TABLE_ENTRY_READ_WRITE_BIT;
    table[index] = next_table_entry;

    return next_table_address;
}

void map_page(u64 virt, u64 phys) {
    u64 page_global_dir_i = (virt >> 39) & 0x1FF;
    u64 page_upper_dir_i = (virt >> 30) & 0x1FF;
    u64 page_middle_dir_i = (virt >> 21) & 0x1FF;
    u64 page_table_i = (virt >> 12) & 0x1FF;


    u64* page_upper_dir = get_or_alloc_next(page_global_directory, page_global_dir_i);
    u64* page_middle_dir = get_or_alloc_next(page_upper_dir, page_upper_dir_i);
    u64* page_table = get_or_alloc_next(page_middle_dir, page_middle_dir_i);

    u64 page_table_entry = page_table[page_table_i];


}
