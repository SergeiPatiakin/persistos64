#include <stdint.h>
#include <stddef.h>
#include "arch/asm.h"
#include "kernel/limine-requests.h"
#include "drivers/tty.h"
#include "mm/kmem.h"
#include "mm/page.h"

uint64_t hhdm_offset;
uint64_t kmem_start;
uint64_t kmem_length;
uint64_t kmem_total_pages; // Total RAM-resident pages (not counting MMIO pages)
struct page *kmem_page_array;
uint64_t kmem_used_pages = 0;
uint64_t dmem_start = 0xffffc00000000000; // Address that Limine doesn't map
uint64_t dmem_used_pages = 0;

#define DMEM_MAX_PAGES 1000

void kmem_init() {
    if (hhdm_request.response == NULL) {
        panic(u8p("No HHDM response\n"));
    }
    hhdm_offset = hhdm_request.response->offset;

    if (memmap_request.response == NULL) {
        panic(u8p("No memmap response\n"));
    }

    // Choose the largest region for kmem
    uint64_t largest_region_bytes = 0;
    struct limine_memmap_entry *largest_region_entry = NULL;
    for (uint32_t i = 0; i < memmap_request.response->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap_request.response->entries[i];
        // if (entry->type != 0 && entry->type != 6 && entry->type != 7) continue;
        if (entry->type != 0) continue;
        if (entry->length > largest_region_bytes) {
            largest_region_bytes = entry->length;
            largest_region_entry = entry;
        }
    }
    if (largest_region_entry == NULL) {
        panic(u8p("No usable memory regions found"));
    }
    kmem_start = largest_region_entry->base + hhdm_offset;
    kmem_length = largest_region_entry->length;
    kmem_total_pages = kmem_length >> 12; // 4096 bytes per page
    size_t page_array_pages = (kmem_total_pages * sizeof(struct page) >> 12) + 1; // 4096 bytes per page
    kmem_page_array = (void*)kmem_start;

    // Initialize page map
    for (size_t i = 0; i < kmem_total_pages; i++) {
        if (i < page_array_pages) {
            kmem_page_array[i].status = KPAGE_PAGE_ARRAY;
        } else {
            kmem_page_array[i].status = KPAGE_FREE;
        }
    }
}

void *kpage_alloc(size_t num_pages) {
    size_t i = 0;
    size_t consecutive_free = 0;
    size_t first_page_index = 0;
    while (true) {
        if (i >= kmem_total_pages) {
            panic(u8p("Out of physical memory\n"));
        }
        if (kmem_page_array[i].status == KPAGE_FREE) {
            consecutive_free++;
            if (consecutive_free == num_pages) {
                first_page_index = i - num_pages + 1;
                break;
            }
        } else {
            consecutive_free = 0;
        }
        i++;
    }

    for (size_t j = first_page_index; j < first_page_index + num_pages; j++) {
        kmem_page_array[j].status = KPAGE_USED;
    }
    kmem_used_pages += num_pages;
    return (void*)kmem_start + (first_page_index << 12);
}

void kpage_free(void *page, size_t num_pages) {
    size_t first_page_index = (((uint64_t)page) - kmem_start) >> 12;
    for (size_t j = first_page_index; j < first_page_index + num_pages; j++) {
        kmem_page_array[j].status = KPAGE_FREE;
    }
    kmem_used_pages -= num_pages;
}

void *dpage_alloc(size_t num_pages) {
    uint64_t mem = dmem_start + dmem_used_pages * PAGE_SIZE;
    dmem_used_pages += num_pages;
    return (void*)mem;
}
