#include "kernel/virual_memory_mapper.hpp"
#include "kernel/physical_memory_mapper.hpp"

#define IDENTITY_MAP (RESERVED + 2*sizeof(VmRange))

static VmRangeTable vm_range_table{};

static VmRange* vm_range_root{};
static u64* page_global_directory = (u64*)pmm_alloc(4096);

static u64* get_or_alloc_next(u64* table, u64 index) {
    if (table[index] & PAGE_TABLE_ENTRY_PRESENT_BIT) {
        u64 entry = table[index];
        return (u64*)(entry & PAGE_TABLE_ENTRY_CLEAR_FLAGS);
    }

    u64* next_table_address = (u64*)pmm_alloc(4096);
    u64 next_table_entry = (u64)next_table_address;

    next_table_entry &= PAGE_TABLE_ENTRY_CLEAR_FLAGS;
    next_table_entry |= PAGE_TABLE_ENTRY_PRESENT_BIT;
    next_table_entry |= PAGE_TABLE_ENTRY_READ_WRITE_BIT;
    table[index] = next_table_entry;

    return next_table_address;
}

static void map_page(u64 virt, u64 phys) {
    u64 page_global_dir_i = (virt >> 39) & 0x1FF;
    u64 page_upper_dir_i = (virt >> 30) & 0x1FF;
    u64 page_middle_dir_i = (virt >> 21) & 0x1FF;
    u64 page_table_i = (virt >> 12) & 0x1FF;

    u64* page_upper_dir = get_or_alloc_next(page_global_directory, page_global_dir_i);
    u64* page_middle_dir = get_or_alloc_next(page_upper_dir, page_upper_dir_i);
    u64* page_table = get_or_alloc_next(page_middle_dir, page_middle_dir_i);

    u64 page_table_entry = phys;
    page_table_entry |= PAGE_TABLE_ENTRY_PRESENT_BIT;
    page_table_entry |= PAGE_TABLE_ENTRY_READ_WRITE_BIT;

    page_table[page_table_i] = page_table_entry;
}

//static u64

static void map_identity_pages() {
    for (u64 addr = 0x0; addr < IDENTITY_MAP; addr += PAGE_SIZE) {
        map_page(addr, addr);
    }
}

void vmm_init() {

    map_identity_pages();

    VmRange* vm_range = (VmRange*)pmm_alloc(4096);
    vm_range->start = 0x0;
    vm_range->end = (u64)IDENTITY_MAP;
    vm_range->flag = VM_RESERVED;
    vm_range->next = nullptr;

    VmRange* vm_range_next = vm_range+1;
    vm_range_next->start = (u64)IDENTITY_MAP;
    vm_range_next->end = (u64)((1ULL << 48) - 1);
    vm_range_next->flag = VM_FREE;
    vm_range_next->next = nullptr;

    vm_range->next = vm_range_next;
    vm_range_root = vm_range;

    vm_range_table.base = vm_range;
    vm_range_table.count = 2;
    vm_range_table.capacity = PAGE_SIZE / sizeof(VmRange);
}

byte* vmm_map(u64 size) {


}

void vmm_free(u64 virt) {


}
