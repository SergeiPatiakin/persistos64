#ifndef RAMFS_H
#define RAMFS_H
#include <stdint.h>
#include "fs/vfs.h"
#include "lib/list.h"
#include "mm/slab.h"

extern struct slab_allocator ramfs_inode_allocator;
#define ramfs_inode_alloc() slab_alloc(&ramfs_inode_allocator)
#define ramfs_inode_free(x) slab_free(&ramfs_inode_allocator, x)

extern struct slab_allocator ramfs_cluster_allocator;
#define ramfs_cluster_alloc() slab_alloc(&ramfs_cluster_allocator)
#define ramfs_cluster_free(x) slab_free(&ramfs_cluster_allocator, x)

struct ramfs_superblock {};

struct ramfs_inode {
    struct list_head ramfs_clusters_lh;
    size_t size;
};

#define RAMFS_CLUSTER_CAPACITY 128

struct ramfs_cluster {
    uint8_t data[RAMFS_CLUSTER_CAPACITY];
    struct list_head ramfs_clusters_le;
};

extern struct filesystem_ops ramfs_superblock_ops;

void ramfs_init();
ssize_t ramfs_set_size(struct file *filp, size_t size);

#endif
