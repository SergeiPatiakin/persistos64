#include <stdint.h>
#include "arch/asm.h"
#include "arch/gdt.h"
#include "fs/elf.h"
#include "fs/tar.h"
#include "fs/vfs.h"
#include "lib/cstd.h"
#include "lib/list.h"
#include "kernel/limine-requests.h"
#include "kernel/scheduler.h"
#include "mm/kmem.h"
#include "mm/map.h"
#include "mm/page.h"
#include "mm/slab.h"
#include "mm/userspace.h"


struct slab_allocator userspace_memory_range_allocator = SLAB_OF(struct userspace_memory_range);

void userspace_init() {
    slab_allocator_init(&userspace_memory_range_allocator);
}

// Given process->pml4_page is already set up, map a userspace page
void* map_user_page(struct task_struct *process, void* user_space_address) {
    void *kernel_space_address = kpage_alloc(1);
    memset(kernel_space_address, 0, PAGE_SIZE);
    set_page_mapping(process->pml4_page, user_space_address, kernel_space_address - hhdm_offset, false);
    return kernel_space_address;
}

// Must be called after setup_kernelspace_memory
void setup_userspace_memory(struct task_struct *process) {
    struct list_head *memory_ranges_lh = &process->memory_ranges_lh;
    for (
        struct list_head *memory_ranges_le = memory_ranges_lh->next;
        memory_ranges_le != memory_ranges_lh;
        memory_ranges_le = memory_ranges_le->next
    ) {
        struct userspace_memory_range *range = container_of(memory_ranges_le, struct userspace_memory_range, memory_ranges_le);
        for (void* page_address = (void*)(range->start & PAGE_ADDRESS_MASK); page_address < (void*)range->end; page_address += 0x1000) {
            map_user_page(process, page_address);
        }
    }
}

// Frees userspace pages and PT, PD and PTPD pages
// Does not free PML4 page
void free_userspace_memory(struct task_struct *process) {
    uint64_t *pml4_entries = process->pml4_page;
    for (uint64_t *pml4_entry = pml4_entries; pml4_entry < pml4_entries + 256; pml4_entry++) {
        if ((*pml4_entry & PAGE_ADDRESS_MASK) == 0) continue;
        uint64_t *ptpd_entries = (void*)(*pml4_entry & PAGE_ADDRESS_MASK) + hhdm_offset;
        for (uint64_t *ptpd_entry = ptpd_entries; ptpd_entry < ptpd_entries + 512; ptpd_entry++) {
            if ((*ptpd_entry & PAGE_ADDRESS_MASK) == 0) continue;
            uint64_t *pd_entries = (void*)(*ptpd_entry & PAGE_ADDRESS_MASK) + hhdm_offset;
            for (uint64_t *pd_entry = pd_entries; pd_entry < pd_entries + 512; pd_entry++) {
                if ((*pd_entry & PAGE_ADDRESS_MASK) == 0) continue;
                uint64_t *pt_entries = (void*)(*pd_entry & PAGE_ADDRESS_MASK) + hhdm_offset;
                for (uint64_t *pt_entry = pt_entries; pt_entry < pt_entries + 512; pt_entry++) {
                    if ((*pt_entry & PAGE_ADDRESS_MASK) == 0) continue;
                    void* page = (void*)(*pt_entry & PAGE_ADDRESS_MASK) + hhdm_offset;
                    kpage_free(page, 1);
                    *pt_entry = 0;
                }
                kpage_free(pt_entries, 1);
                *pd_entry = 0;
            }
            kpage_free(pd_entries, 1);
            *ptpd_entry = 0;
        }
        kpage_free(ptpd_entries, 1);
        *pml4_entry = 0;
    }

    // Free list process->memory_ranges_lh
    struct list_head *memory_ranges_le = process->memory_ranges_lh.next;
    while(memory_ranges_le != &process->memory_ranges_lh) {
        struct userspace_memory_range *range = container_of(memory_ranges_le, struct userspace_memory_range, memory_ranges_le);
        memory_ranges_le = memory_ranges_le->next;
        userspace_memory_range_free(range);
    }
    init_list(&process->memory_ranges_lh);
}

// Load ELF64 to current process
// Returns 0 on success
void load_elf64(struct file *filp, struct loader_result *loader_result_out) {
    struct elf64_hdr elf_file_header;
    vfs_read(filp, &elf_file_header, sizeof(struct elf64_hdr));
    
    void *largest_end_address = NULL;
    for (uint32_t i = 0; i < elf_file_header.e_phnum; i++) {
        struct elf64_phdr segment_header;
        filp->offset = elf_file_header.e_phoff + i * sizeof(struct elf64_phdr);
        vfs_read(filp, &segment_header, sizeof(struct elf64_phdr));
        if (!segment_header.p_vaddr && !segment_header.p_memsz) {
            // Skip null
            continue;
        }
        struct userspace_memory_range *memory_range = userspace_memory_range_alloc(); // Freed in task_free
        memory_range->type = USERSPACE_MEMRANGE_NORMAL;
        memory_range->start = segment_header.p_vaddr;
        memory_range->end = segment_header.p_vaddr + segment_header.p_memsz;
        if ((void*)(memory_range->end) > largest_end_address) {
            largest_end_address = (void*)(memory_range->end);
        }
        list_add_tail(&memory_range->memory_ranges_le, &current_task_ts->memory_ranges_lh);
    }

    // 1 page of heap
    struct userspace_memory_range *heap_memory_range = userspace_memory_range_alloc(); // Freed in task_free
    heap_memory_range->type = USERSPACE_MEMRANGE_HEAP;
    heap_memory_range->start = ((uint64_t)largest_end_address & PAGE_OFFSET_MASK) ? (((uint64_t)largest_end_address | PAGE_OFFSET_MASK) + 1) : (uint64_t)largest_end_address;
    heap_memory_range->end = heap_memory_range->start + PAGE_SIZE;
    list_add_tail(&heap_memory_range->memory_ranges_le, &current_task_ts->memory_ranges_lh);

    // 1MB of stack
    struct userspace_memory_range *stack_memory_range = userspace_memory_range_alloc(); // Freed in task_free
    stack_memory_range->type = USERSPACE_MEMRANGE_STACK;
    stack_memory_range->start = 0x00007FFFFFF00000;
    stack_memory_range->end = 0x0000800000000000;
    list_add_tail(&stack_memory_range->memory_ranges_le, &current_task_ts->memory_ranges_lh);
    
    setup_userspace_memory(current_task_ts);

    for (uint32_t i = 0; i < elf_file_header.e_phnum; i++) {
        struct elf64_phdr segment_header;
        filp->offset = elf_file_header.e_phoff + i * sizeof(struct elf64_phdr);
        vfs_read(filp, &segment_header, sizeof(struct elf64_phdr));
        filp->offset = segment_header.p_offset;
        vfs_read(filp, (void*)segment_header.p_vaddr, segment_header.p_filesz);
    }

    loader_result_out->user_entry_rip = elf_file_header.e_entry;
    loader_result_out->user_entry_rsp = 0x0000800000000000;
}
