#include "kernel/virtual_memory_mapper.hpp"
#include "kernel/console.hpp"
#include "kernel/physical_memory_mapper.hpp"

#include <assert.h>

static VmRangeTable vm_range_table{};
static u64* pml4;
constexpr u64 VM_RANGE_SIZE = (PAGE_SIZE * 10);
constexpr u64 MEM_ALIGN(u64 addr) {
    return ((addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
}

u64* get_pml4_va() {
    return (u64*)(
        (510LL << 39) |
        (510LL << 30) |
        (510LL << 21) |
        (510LL << 12)
    );
}

u64* get_pdpt_va(u64 virt) {
    u64 pml4_i = (virt >> 39) & 0x1FF;

    u64 recursive_addr =
        (510LL << 39) |
        (pml4_i  << 30);

    return (u64*)recursive_addr;
}

u64* get_pd_va(u64 virt) {
    u64 pml4_i = (virt >> 39) & 0x1FF;
    u64 pdpt_i = (virt >> 30) & 0x1FF;

    u64 recursive_addr =
        (510LL << 39) |
        (pml4_i  << 30) |
        (pdpt_i  << 21);

    return (u64*)recursive_addr;
}

u64* get_pt_va(u64 virt) {
    u64 pml4_i = (virt >> 39) & 0x1FF;
    u64 pdpt_i = (virt >> 30) & 0x1FF;
    u64 pd_i   = (virt >> 21) & 0x1FF;

    u64 recursive_addr =
        (510LL << 39) |
        (pml4_i  << 30) |
        (pdpt_i  << 21) |
        (pd_i    << 12);

    return (u64*)recursive_addr;
}

u64* get_pte_va(u64 virt) {
    u64 pml4_i = (virt >> 39) & 0x1FF;
    u64 pdpt_i = (virt >> 30) & 0x1FF;
    u64 pd_i   = (virt >> 21) & 0x1FF;
    u64 pt_i   = (virt >> 12) & 0x1FF;

    u64 recursive_addr =
        (510LL << 39) |
        (pml4_i  << 30) |
        (pdpt_i  << 21) |
        (pd_i    << 12) |
        (pt_i * 8);

    return (u64*)recursive_addr;
}

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

void map_page(u64 virt, u64 phys) {
    u64 pml4_i = (virt >> 39) & 0x1FF;
    u64 pdpt_i = (virt >> 30) & 0x1FF;
    u64 pd_i   = (virt >> 21) & 0x1FF;
    u64 pt_i   = (virt >> 12) & 0x1FF;

    u64* pdpt = get_or_alloc_next(pml4, pml4_i);
    u64* pd   = get_or_alloc_next(pdpt, pdpt_i);
    u64* pt   = get_or_alloc_next(pd, pd_i);

    u64 page_table_entry = phys;
    page_table_entry |= PAGE_TABLE_ENTRY_PRESENT_BIT;
    page_table_entry |= PAGE_TABLE_ENTRY_READ_WRITE_BIT;

    pt[pt_i] = page_table_entry;
}

void vmm_identity_map(u64 addr, u64 size) {
    u64 aligned_addr = addr & ~(PAGE_SIZE - 1);
    u64 aligned_size = MEM_ALIGN(size + (addr - aligned_addr));

    for (u64 i = 0; i < vm_range_table.count; ++i) {
        VmRange* current = vm_range_table.base + i;

        if (current->flag == VM_FREE && current->start <= aligned_addr && current->end >= (aligned_addr + aligned_size)) {
            VmRange* used = &vm_range_table.base[vm_range_table.count++];
            VmRange* after = &vm_range_table.base[vm_range_table.count++];

            used->start = aligned_addr;
            used->end = aligned_addr + aligned_size;
            used->flag = VM_USED;

            after->start = used->end;
            after->end = current->end;
            after->flag = VM_FREE;

            current->end = used->start;

            used->next = after;
            after->next = current->next;
            current->next = used;

            for (u64 j = 0; j < aligned_size / PAGE_SIZE; ++j) {
                u64 addr = aligned_addr + (j * PAGE_SIZE);
                map_page(addr, addr);
            }
            break;
        }
    }
}

static void vmm_init_recursive_pdpt() {
    u64 recursive_entry = (u64)pml4;

    recursive_entry |= PAGE_TABLE_ENTRY_PRESENT_BIT;
    recursive_entry |= PAGE_TABLE_ENTRY_READ_WRITE_BIT;

    pml4[510] = (u64)recursive_entry;
}

void vmm_init() {
    pml4 = (u64*)pmm_alloc(4096);
    vmm_init_recursive_pdpt();

    VmRange* vm_range = (VmRange*)pmm_alloc(VM_RANGE_SIZE);

    vm_range->start = 0x0;
    vm_range->end = (u64)(1ULL << 48);
    vm_range->flag = VM_FREE;
    vm_range->next = nullptr;

    vm_range_table.base = vm_range;
    vm_range_table.count = 1;
    vm_range_table.capacity = VM_RANGE_SIZE / sizeof(VmRange);

    vmm_identity_map((u64)vm_range, VM_RANGE_SIZE);
}

u64* vmm_get_base() {
    return (u64*)pml4;
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

    u64 physical_memory = (u64)pmm_alloc(size);
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
