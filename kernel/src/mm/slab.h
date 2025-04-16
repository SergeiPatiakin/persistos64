#ifndef SLAB_H
#define SLAB_H
#include <stdint.h>
#include "lib/list.h"

struct slab_allocator {
    uint16_t object_size;
    uint16_t objects_per_page;
    struct list_head slab_page_lh;
};

struct slab_page_header {
    uint64_t free_bitmaps[4]; // 1 for free
    struct list_head slab_page_le;
};

void* slab_alloc(struct slab_allocator *allocator);
void slab_free(struct slab_allocator *allocator, void* address);

#define SLAB_AVAILABLE_PAGESIZE (4096 - sizeof(struct slab_page_header))

#define SLAB_OF(t) { \
    .object_size = sizeof(t), \
    .objects_per_page = SLAB_AVAILABLE_PAGESIZE / sizeof(t), \
};

void slab_allocator_init(struct slab_allocator *allocator);

#endif
