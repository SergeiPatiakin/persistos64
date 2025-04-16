#include <stddef.h>
#include "arch/asm.h"
#include "kernel/scheduler.h"
#include "mm/kmem.h"
#include "mm/page.h"
#include "mm/slab.h"
#include "mm/userspace.h"

struct task_struct *current_task_ts = NULL;
struct list_head task_struct_lh; // List of task_struct sorted by pid
tss64_t tss;

uint32_t pid_counter = 1;

struct slab_allocator task_struct_allocator = SLAB_OF(struct task_struct);

void scheduler_init_1() {
    init_list(&task_struct_lh);
    slab_allocator_init(&task_struct_allocator);
}

void set_tss_for(struct task_struct *process) {
    tss.rsp0_low = process->kernel_entry_rsp;
    tss.rsp0_high = process->kernel_entry_rsp >> 32;
}

// Get the next task_struct after ts. Wraps around
struct task_struct *next_task_struct(struct task_struct* ts) {
	struct list_head *task_struct_le;
	task_struct_le = ts->task_struct_le.next;
	if (task_struct_le == &task_struct_lh) {
		// Wrap around
		task_struct_le = task_struct_le->next;
	}
    struct task_struct *result = container_of(task_struct_le, struct task_struct, task_struct_le);
    return result;
}

void task_yield() {
	// Find next runnable task
	struct task_struct *t = current_task_ts;
	do {
		t = next_task_struct(t);

		if (t == current_task_ts) {
			// There are no running tasks. Idle the CPU until the next interrupt.
			halt_until_any_interrupt();
		}
	} while (t->task_state != TS_RUNNING);
	switch_to_task(t);
}

// Setup kernelspace memory and PML4. Switch to the new PML4.
// Must be called before setup_userspace_memory
void setup_kernelspace_memory(struct task_struct *task) {
    void* kernel_stack_pages = kpage_alloc(KERNEL_STACK_PAGES);
    task->kernel_stack_pages = kernel_stack_pages;
    task->kernel_entry_rsp = (uint64_t)kernel_stack_pages + PAGE_SIZE * KERNEL_STACK_PAGES;

    uint64_t cr3 = read_cr3();
    uint64_t *old_pml4_entries = (void*)(cr3 + hhdm_offset);
    uint64_t *new_pml4_entries = kpage_alloc(1);
    memset(new_pml4_entries, 0, PAGE_SIZE);

    // Copy pml4 entries for upper half
    for (uint16_t i = 256; i < 512; i++) {
        new_pml4_entries[i] = old_pml4_entries[i];
    }

    task->pml4_page = new_pml4_entries;
}

void free_kernelspace_memory(struct task_struct *task) {
    kpage_free(task->pml4_page, 1);
    kpage_free(task->kernel_stack_pages, KERNEL_STACK_PAGES);
}

// Perform final cleanup for a task after kernelspace memory and userspace memory has been freed
void free_task(struct task_struct *task) {
	// Free structs tracking memory ranges
	for (
		struct list_head *memory_ranges_le = task->memory_ranges_lh.next;
		memory_ranges_le != &task->memory_ranges_lh;
	) {
		struct list_head *next_memory_ranges_le = memory_ranges_le->next;
		struct userspace_memory_range *range = container_of(
			memory_ranges_le,
			struct userspace_memory_range,
			memory_ranges_le
		);
		userspace_memory_range_free(range);
		memory_ranges_le = next_memory_ranges_le;
	}
	list_del(&task->task_struct_le);
	task_struct_free(task);
}

void load_cr3_from(struct task_struct *process) {
    // Switch to new PML4
    asm volatile ("mov %0, %%cr3" :: "a" (process->pml4_page - hhdm_offset));
}

struct task_struct *task_struct_find(uint32_t pid) {
    list_for_each(task_struct_le, task_struct_lh) {
        struct task_struct *candidate_task_struct = container_of(
			task_struct_le,
			struct task_struct,
			task_struct_le
		);
        if (candidate_task_struct->pid == pid) {
            return candidate_task_struct;
        }
    }
    return NULL;
}
