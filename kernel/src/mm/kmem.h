#ifndef KMEM_H
#define KMEM_H
#include <stdint.h>
#include <stddef.h>

extern uint64_t hhdm_offset;
extern uint64_t kmem_start;
extern uint64_t kmem_length;
extern uint64_t kmem_total_pages;
extern uint64_t kmem_used_pages;

// Normal memory, used for page array
#define KPAGE_PAGE_ARRAY 0x81
// Normal memory, free or heap space
#define KPAGE_FREE 0x82
// Normal memory, used
#define KPAGE_USED 0x83
// Device memory
#define KPAGE_DEVICE 0x84

struct page {
    uint8_t status;
};

void kmem_init();
void *kpage_alloc(size_t num_pages);
void kpage_free(void *page, size_t num_pages);
void *dpage_alloc(size_t num_pages);

#endif
