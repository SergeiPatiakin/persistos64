#include <stdint.h>
#include "cstd.h"
#include <persistos.h>

// Free block
#define KHEAP_STATUS_FREE 0x1
// Last block
#define KHEAP_STATUS_FOOTER 0x10
// Just for debugging, doesn't do anything
#define KHEAP_MAGIC 0x77777777

struct block_meta {
    uint32_t magic;
    uint32_t status;
    struct block_meta *prev;
    struct block_meta *next;
};
#define META_SIZE sizeof(struct block_meta)

struct block_meta *first_block_meta;
void *program_break;

void libc_init() {
    first_block_meta = brk(NULL);
    program_break = brk((void*)first_block_meta + 4096);
    
    struct block_meta *footer_block_meta = first_block_meta + 1;
    
    first_block_meta->magic = KHEAP_MAGIC;
    first_block_meta->prev = NULL;
    first_block_meta->next = footer_block_meta;
    
    footer_block_meta->magic = KHEAP_MAGIC;
    footer_block_meta->prev = first_block_meta;
    footer_block_meta->next = NULL;
    footer_block_meta->status = KHEAP_STATUS_FOOTER;
}

void *malloc(size_t size) {
      // Align to 8 bytes
    size_t padded_size = (size & 0x7) ? ((size | 0x7) + 1) : size;
    struct block_meta *current_block_meta = first_block_meta;
    struct block_meta *prev_block_meta = NULL;
    while (!(current_block_meta->status & KHEAP_STATUS_FOOTER)) {
        if (
            (current_block_meta->status & KHEAP_STATUS_FREE) &&
            (void*)current_block_meta->next - (void*)current_block_meta >= padded_size + META_SIZE
        ) {
            // Use current block
            current_block_meta->status &= ~KHEAP_STATUS_FREE;
            if ((void*)current_block_meta->next - (void*)current_block_meta > padded_size + 2 * META_SIZE) {
                // Split current block.
                // Create new free block from unused part of current block
                struct block_meta *new_block_meta = (void*)current_block_meta + META_SIZE + padded_size;
                new_block_meta->status = KHEAP_STATUS_FREE;
                new_block_meta->magic = KHEAP_MAGIC;
                
                // Link new_block_meta with the following block
                new_block_meta->next = current_block_meta->next;
                if (current_block_meta->next) {
                    current_block_meta->next->prev = new_block_meta;
                }
                
                // Link new_block_meta with current_block_meta
                new_block_meta->prev = current_block_meta;
                current_block_meta->next = new_block_meta;
            }
            return ((void*)current_block_meta) + META_SIZE;
        }
        prev_block_meta = current_block_meta;
        current_block_meta = current_block_meta->next;
    }
    // Turn the footer block into a regular block, and create a new footer block
    struct block_meta *new_footer_block_meta = (void*)current_block_meta + META_SIZE + padded_size;
    if ((void*)new_footer_block_meta + META_SIZE > program_break) {
        program_break = brk((void*)new_footer_block_meta + META_SIZE);
    }

    new_footer_block_meta->magic = 0x77777777;

    new_footer_block_meta->prev = current_block_meta;

    current_block_meta->next = new_footer_block_meta;
    new_footer_block_meta->next = NULL;

    current_block_meta->status = 0;
    new_footer_block_meta->status = KHEAP_STATUS_FOOTER;

    return (void*)current_block_meta + META_SIZE;
}

void puts(uint8_t* s) {
    write(1, s, strlen(s));
}

void free(void *obj) {
    struct block_meta *block = obj - META_SIZE;
    if (block->magic != KHEAP_MAGIC) {
        puts("free: Bad magic\n");
        exit(10);
    }
    // block->prev and block->next are guaranteed to be non-null
    if (!(block->prev->status & KHEAP_STATUS_FREE) && !(block->next->status & KHEAP_STATUS_FREE)) {
        block->status = block->status | KHEAP_STATUS_FREE;
    } else if ((block->prev->status & KHEAP_STATUS_FREE) && !(block->next->status & KHEAP_STATUS_FREE)) {
        // Delete this block, extend prev block
        block->prev->next = block->next;
        block->magic = 0;
        if (block->next) {
            block->next->prev = block->prev;
        }
    } else if (!(block->prev->status & KHEAP_STATUS_FREE) && (block->next->status & KHEAP_STATUS_FREE)) {
        // Delete next block, free and extend this block
        struct block_meta *tmp_following = block->next->next; // Guaranteed to be not null
        block->next->magic = 0;
        block->next = tmp_following;
        block->status |= KHEAP_STATUS_FREE;
        if (tmp_following) {
            tmp_following->prev = block;
        }
    } else { // (block->prev->flags & KHEAP_BLOCK_FREE) & (block->next && block->next->flags & KHEAP_BLOCK_FREE)
        // Delete this block, delete next block, extend prev block
        struct block_meta *tmp_following = block->next->next;
        block->prev->next = tmp_following;
        if (tmp_following) {
            tmp_following->prev = block->prev;
        }
        block->magic = 0;
        block->next->magic = 0;
    }
}

struct FILE std_files[3] = {
    {._fd = 0},
    {._fd = 1},
    {._fd = 2},
};
struct FILE* stdin = std_files;
struct FILE* stdout = &std_files[1];
struct FILE* stderr = &std_files[2];

void fputs(uint8_t* s, struct FILE* file) {
    write(file->_fd, s, strlen(s));
}

bool is_error(ssize_t x) {
    return (x >= -4095) && (x <= -1);
}
