#include "kernel/virtual_memory_mapper.hpp"
#include "kernel/console.hpp"
#include "kernel/physical_memory_mapper.hpp"

#define VM_RANGE_SIZE (PAGE_SIZE * 10)

static VmRangeTable vm_range_table{};
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

static void map_reserved_pages() {
    for (u64 addr = 0x0; addr < RESERVED; addr += PAGE_SIZE) {
        map_page(addr, addr);
    }
}

void vmm_init() {
    map_reserved_pages();

    VmRange* vm_range = (VmRange*)pmm_alloc(VM_RANGE_SIZE);
    vm_range->start = 0x0;
    vm_range->end = (u64)RESERVED;
    vm_range->flag = VM_RESERVED;
    vm_range->next = nullptr;

    VmRange* vm_range_next = vm_range+1;
    vm_range_next->start = (u64)RESERVED;
    vm_range_next->end = (u64)((1ULL << 48) - 1);
    vm_range_next->flag = VM_FREE;
    vm_range_next->next = nullptr;
    vm_range->next = vm_range_next;

    vm_range_table.base = vm_range;
    vm_range_table.count = 2;
    vm_range_table.capacity = VM_RANGE_SIZE / sizeof(VmRange);
}

byte* vmm_get_base() {
    return (byte*)vm_range_table.base;
}

static VmRange find_vm_range(u64 size) {
    VmRange* current;
    u64 aligned_size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    VmRange result;
    result.flag = VM_UNSET;

    for (u64 i = 0; i < vm_range_table.count; ++i) {
        current = vm_range_table.base + i;
        u64 segment_size = current->end - current->start;

        if (segment_size >= aligned_size && current->flag == VM_FREE) {
            VmRange* new_range = vm_range_table.base + vm_range_table.count;

            new_range->start = current->start + aligned_size;
            new_range->end = current->end;
            new_range->flag = VM_FREE;
            new_range->next = current->next;

            current->next = new_range;
            current->end = current->start + aligned_size;
            current->flag = VM_USED;

            vm_range_table.count++;
            result = *new_range;
            break;
        }
    }

    return result;
}

// TODO: add coalescing
byte* vmm_map(u64 size) {
    u64 aligned_size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    u64 pages = aligned_size / PAGE_SIZE;

    VmRange vm_range = find_vm_range(size);
    if (vm_range.flag == VM_UNSET) {
        printk("Failed to allocate virtual pages! \n");
        return nullptr;
    }

    u64 physical_memory = (u64)pmm_alloc(size, ALLOC_GENERAL);
    if (!physical_memory) {
        printk("Failed to allocate physical pages! \n");
        return nullptr;
    }

    for (u64 i = 0; i < pages; ++i) {
        u64 vitual_mem = vm_range.start + (PAGE_SIZE * i);
        u64 phys_mem = physical_memory + (PAGE_SIZE * i);
        map_page(vitual_mem, phys_mem);
    }

    return (byte*)vm_range.start;
}

void vmm_free(u64 virt) {


}
