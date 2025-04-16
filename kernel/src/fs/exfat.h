#ifndef EXFAT_H
#define EXFAT_H
#include <stdint.h>
#include <stdbool.h>
#include "fs/vfs.h"
#include "lib/list.h"

extern struct slab_allocator exfat_inode_allocator;
#define exfat_inode_alloc() slab_alloc(&exfat_inode_allocator)
#define exfat_inode_free(x) slab_free(&exfat_inode_allocator, x)

extern struct slab_allocator exfat_file_cluster_allocator;
#define exfat_file_cluster_alloc() slab_alloc(&exfat_file_cluster_allocator)
#define exfat_file_cluster_free(x) slab_free(&exfat_file_cluster_allocator, x)

struct exfat_file_cluster {
    uint32_t byte_offset; // Byte offset from start of file
    uint64_t device_offset; // Byte offset in backing device
    struct list_head file_clusters_le;
};

struct exfat_inode {
    uint32_t exfat_start_lba;
    bool load_needed;
    struct list_head file_clusters_lh; // List of content pages (for a regular file only)
};

struct exfat_superblock {
    uint32_t fat_offset;
    uint32_t fat_length;
    uint32_t cluster_heap_offset;
    uint32_t first_cluster_of_root_directory;
    uint8_t bytes_per_sector_exponent;
    uint8_t sectors_per_cluster_exponent;
};

extern struct filesystem_ops exfat_superblock_ops;

void exfat_init();
void exfat_load_dir_inode(struct inode *dir_inode);
void exfat_load_file_inode(struct inode *file_inode);

#endif
