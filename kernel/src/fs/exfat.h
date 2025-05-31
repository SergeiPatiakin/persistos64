#ifndef EXFAT_H
#define EXFAT_H
#include <stdint.h>
#include <stdbool.h>
#include "fs/vfs.h"
#include "lib/list.h"

extern struct slab_allocator exfat_inode_allocator;
#define exfat_inode_alloc() slab_alloc(&exfat_inode_allocator)
#define exfat_inode_free(x) slab_free(&exfat_inode_allocator, x)

extern struct slab_allocator exfat_cluster_allocator;
#define exfat_cluster_alloc() slab_alloc(&exfat_cluster_allocator)
#define exfat_cluster_free(x) slab_free(&exfat_cluster_allocator, x)

struct exfat_cluster {
    uint32_t cluster_index;
    struct list_head cluster_chain_le;
};

struct exfat_inode {
    uint32_t first_cluster_index;
    bool load_needed;
    bool no_fat_chain;
    uint64_t dentry_cluster_index;
    uint64_t dentry_cluster_offset;
    uint64_t size;
    struct list_head cluster_chain_lh;
};

struct exfat_superblock {
    uint32_t fat_offset;
    uint32_t fat_length;
    uint32_t cluster_heap_offset;
    uint32_t first_cluster_of_root_directory;
    uint8_t bytes_per_sector_exponent;
    uint8_t sectors_per_cluster_exponent;
    uint32_t pages_per_cluster; // For convenience
};

extern struct filesystem_ops exfat_superblock_ops;

void exfat_init();
void exfat_load_dir_inode(struct inode *dir_inode);
void exfat_load_file_inode(struct inode *file_inode);
void exfat_load_cluster_chain(struct inode *inode);

#endif
