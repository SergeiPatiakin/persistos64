#ifndef USERSPACE_H
#define USERSPACE_H

#include <stdint.h>
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

/* 64-bit ELF base types. */
typedef uint64_t Elf64_Addr;
typedef uint16_t Elf64_Half;
typedef int16_t Elf64_SHalf;
typedef uint64_t Elf64_Off;
typedef int32_t Elf64_Sword;
typedef int32_t Elf64_Word;
typedef uint64_t Elf64_Xword;
typedef int64_t Elf64_Sxword;

// ELF file header
typedef struct elf64_hdr {
    uint8_t e_ident[16];  /* ELF "magic number" */
    Elf64_Half e_type;
    Elf64_Half e_machine;
    Elf64_Word e_version;
    Elf64_Addr e_entry; /* Entry point virtual address */
    Elf64_Off e_phoff;  /* Program header table file offset */
    Elf64_Off e_shoff;  /* Section header table file offset */
    Elf64_Word e_flags;
    Elf64_Half e_ehsize;
    Elf64_Half e_phentsize; /* Size of program header entry */
    Elf64_Half e_phnum; /* Number of program header entries */
    Elf64_Half e_shentsize;
    Elf64_Half e_shnum;
    Elf64_Half e_shstrndx;
} Elf64_Ehdr;

ct_assert(sizeof(Elf64_Ehdr) == 64);

typedef struct elf64_phdr {
    Elf64_Word p_type;
    Elf64_Word p_flags;
    Elf64_Off p_offset; /* Segment file offset */
    Elf64_Addr p_vaddr; /* Segment virtual address */
    Elf64_Addr p_paddr; /* Segment physical address */
    Elf64_Xword p_filesz;   /* Segment size in file */
    Elf64_Xword p_memsz;    /* Segment size in memory */
    Elf64_Xword p_align;    /* Segment alignment, file & memory */
} Elf64_Phdr;

ct_assert(sizeof(Elf64_Phdr) == 56);

#endif
