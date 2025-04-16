#ifndef USERSPACE_H
#define USERSPACE_H

#include "lib/list.h"
#include "mm/slab.h"
#include "kernel/scheduler.h"

struct file;

// In C
void setup_userspace_memory(struct task_struct *process);
extern void* mapped_user_code_page;
extern void* mapped_user_stack_page;

#define USERSPACE_MEMRANGE_NORMAL 0xA1
#define USERSPACE_MEMRANGE_HEAP 0xA2
#define USERSPACE_MEMRANGE_STACK 0xA3

struct userspace_memory_range {
    uint64_t start; // Must be page aligned.
    uint64_t end; // First byte after end of memory. Must be page aligned
    uint8_t type;
    struct list_head memory_ranges_le;
};

extern struct slab_allocator userspace_memory_range_allocator;
#define userspace_memory_range_alloc() slab_alloc(&userspace_memory_range_allocator)
#define userspace_memory_range_free(x) slab_free(&userspace_memory_range_allocator, x)

struct loader_result {
    uint64_t user_entry_rip;
    uint64_t user_entry_rsp;
};

void userspace_init();
void free_userspace_memory(struct task_struct *process);
void load_elf64(struct file *filp, struct loader_result *loader_result_out);
void* map_user_page(struct task_struct *process, void* user_space_address);

#endif
