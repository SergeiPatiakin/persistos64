#include <stdbool.h>
#include "arch/asm.h"
#include "mm/slab.h"
#include "mm/kmem.h"
#include "mm/page.h"
#include "drivers/tty.h"
#include "lib/cstd.h"
#include "lib/list.h"

// Try allocating on existing page, return NULL if failed
void *slab_try_alloc(struct slab_allocator *allocator) {
    for (
        struct list_head *page_le = allocator->slab_page_lh.next;
        page_le != &allocator->slab_page_lh;
        page_le = page_le->next
    ) {
        struct slab_page_header *header = container_of(page_le, struct slab_page_header, slab_page_le);
        // Performance optimization: quickly detect completely full pages
        if (
            header->free_bitmaps[0] == 0 &&
            header->free_bitmaps[1] == 0 &&
            header->free_bitmaps[2] == 0 &&
            header->free_bitmaps[3] == 0
        ) {
            continue;
        }
        for (uint8_t i = 0; i < 4; i++) {
            for (uint8_t j = 0; j < 64; j++) {
                if ((header->free_bitmaps[i] >> j) & 0x1) {
                    uint8_t slot_number = ((i << 6) + j);
                    header->free_bitmaps[i] ^= (1LL << j);
                    return (void*)header + sizeof(struct slab_page_header) + slot_number * allocator->object_size;
                }
            }
        }
    }
    return NULL;
}

void* slab_alloc(struct slab_allocator *allocator) {
    void* address = slab_try_alloc(allocator);
    if (address){
        goto finalize;
    }
    // No free slots found on any page, time to allocate a new page
    struct slab_page_header *new_header = kpage_alloc(1);
    memset(new_header, 0, PAGE_SIZE);
    list_add(&new_header->slab_page_le, &allocator->slab_page_lh);
    for (uint8_t i = 0; i < 4; i++) {
        for (uint8_t j = 0; j < 64; j++) {
            uint8_t slot_number = ((i << 6) + j);
            if (slot_number >= allocator->objects_per_page) {
                goto no_more_slots;
            }
            new_header->free_bitmaps[i] |= (1LL << j);
        }
    }
    
    no_more_slots:
    // Allocation will now surely succeed
    address = slab_try_alloc(allocator);
    if (!address) {
        panic(u8p("Slab allocation unexpectedly failed"));
    }

    finalize:
    // allocator->allocated_objects++; // For debugging
    // memset(address, 0x11, allocator->object_size); // For debugging
    return address;
}

void slab_free(struct slab_allocator *allocator, void* address) {
    // allocator->freed_objects++; // For debugging
    // memset(address, 0x50, allocator->object_size); // For debugging
    struct slab_page_header *page_header = (void*)((uint64_t)address & 0xFFFFFFFFFFFFF000);
    uint16_t page_offset = (address - sizeof(struct slab_page_header) - (void*)page_header);
    uint16_t slot_number =  page_offset / allocator->object_size;
    uint16_t slot_offset = page_offset % allocator->object_size;
    if (slot_offset != 0) {
        panic(u8p("invalid slab free"));
    }
    uint8_t slot_number_div64 = slot_number >> 6;
    uint8_t slot_number_mod64 = slot_number % 64;
    page_header->free_bitmaps[slot_number_div64] |= (1LL << slot_number_mod64);
}

void slab_allocator_init(struct slab_allocator *allocator) {
    init_list(&allocator->slab_page_lh);
}
