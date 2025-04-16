#ifndef SCHEDULER_H
#define SCHEDULER_H

#define TS_RUNNING 0x91
#define TS_WAITING 0x92
#define TS_ZOMBIE 0x93

#include <stdint.h>
#include "lib/cstd.h"
#include "lib/list.h"
#include "mm/slab.h"

typedef struct {
    uint32_t reserved0;
    uint32_t rsp0_low;
    uint32_t rsp0_high;
    uint32_t reserved1[22];
    uint16_t reserved2;
    uint16_t iomap;
} __attribute__((packed)) tss64_t;

extern tss64_t tss;
extern uint32_t pid_counter;

#define TASK_NAME_MAXLEN 256

struct task_struct {
    uint32_t pid;
    uint8_t task_state;
    uint8_t exit_code;
    uint64_t kernel_rsp; // RSP for task switch into this task
    uint64_t kernel_entry_rsp; // RSP for all interrupts from user mode to kernel mode
    void *kernel_stack_pages;
    void *pml4_page;
    uint8_t name[TASK_NAME_MAXLEN];
    struct list_head memory_ranges_lh;
    struct list_head task_struct_le;
    // struct list_head termination_wait_queue_head;
    struct list_head files_lh; // List of struct file for this task
};

ct_assert(offsetof(struct task_struct, kernel_rsp) == 8); // Update scheduler.s if this changes

extern struct task_struct *current_task_ts;
extern struct list_head task_struct_lh;
extern struct slab_allocator task_struct_allocator;
#define task_struct_alloc() slab_alloc(&task_struct_allocator)
#define task_struct_free(x) slab_free(&task_struct_allocator, x)

#define KERNEL_STACK_PAGES 2

void scheduler_init_1();
void set_segment_registers_for_userspace();
void task_yield();
void setup_kernelspace_memory(struct task_struct *process);
void free_kernelspace_memory(struct task_struct *task);
void free_task(struct task_struct *task);
void load_cr3_from(struct task_struct *process);
struct task_struct *task_struct_find(uint32_t pid);

// In assembly
void switch_to_task(struct task_struct *new_task);

#endif
