#include <stdbool.h>
#include "map.h"
#include "drivers/tty.h"
#include "lib/cstd.h"
#include "mm/kmem.h"
#include "mm/page.h"

#define PAGE_DIRECTORY_ATTRIBUTES 0x27
#define PAGE_TABLE_ATTRIBUTES 0xE7
#define PAGE_TABLE_MMIO_ATTRIBUTES 0xFF

// Must not be used for memory mapped by Limine, because Limine uses 2MB pages which we don't want to deal with
void set_page_mapping(void *pml4_page, void* virt_address, void* phys_address, bool is_mmio) {
    if ((uint64_t)phys_address & 4095) {
        panic(u8p("phys_address must be page-aligned"));
    }
    uint64_t *pml4_entries = pml4_page;
    
    uint64_t *ptpd_entries = (void*)((pml4_entries[((uint64_t)virt_address >> 39) & 0x1FF] & PAGE_ADDRESS_MASK) + hhdm_offset);
    if (ptpd_entries == (void*)hhdm_offset) {
        ptpd_entries = kpage_alloc(1);
        memset(ptpd_entries, 0, PAGE_SIZE);
        pml4_entries[((uint64_t)virt_address >> 39) & 0x1FF] = ((uint64_t)ptpd_entries - hhdm_offset) | PAGE_DIRECTORY_ATTRIBUTES;
    }

    uint64_t *pd_entries = (void*)((ptpd_entries[((uint64_t)virt_address >> 30) & 0x1FF] & PAGE_ADDRESS_MASK) + hhdm_offset);
    if (pd_entries == (void*)hhdm_offset) {
        pd_entries = kpage_alloc(1);
        memset(pd_entries, 0, PAGE_SIZE);
        ptpd_entries[((uint64_t)virt_address >> 30) & 0x1FF] = ((uint64_t)pd_entries - hhdm_offset) | PAGE_DIRECTORY_ATTRIBUTES;
    }

    uint64_t *pt_entries = (uint64_t*)((pd_entries[((uint64_t)virt_address >> 21) & 0x1FF] & PAGE_ADDRESS_MASK) + hhdm_offset);
    if (pt_entries == (void*)hhdm_offset) {
        pt_entries = kpage_alloc(1);
        memset(pt_entries, 0, PAGE_SIZE);
        pd_entries[((uint64_t)virt_address >> 21) & 0x1FF] = ((uint64_t)pt_entries - hhdm_offset) | PAGE_DIRECTORY_ATTRIBUTES;
    }

    pt_entries[((uint64_t)virt_address >> 12) & 0x1FF] = ((uint64_t)phys_address) | (
        is_mmio ? PAGE_TABLE_MMIO_ATTRIBUTES : PAGE_TABLE_ATTRIBUTES
    );
}
