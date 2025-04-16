#include <stdbool.h>
#include <stddef.h>
#include "ramfs.h"
#include "mm/kmem.h"
#include "mm/slab.h"

#define RAMFS_MOUNT_NOT_IMPLEMENTED 3

struct slab_allocator ramfs_inode_allocator = SLAB_OF(struct ramfs_inode);
struct slab_allocator ramfs_cluster_allocator = SLAB_OF(struct ramfs_cluster);

void ramfs_init() {
    slab_allocator_init(&ramfs_inode_allocator);
    slab_allocator_init(&ramfs_cluster_allocator);
}

ssize_t ramfs_mount(struct inode *device_inode, struct dentry *mountpoint_inode) {
    (void) device_inode;
    (void) mountpoint_inode;
    return RAMFS_MOUNT_NOT_IMPLEMENTED;
}

void ramfs_lookup(struct inode *file_inode, struct dentry *dentry) {
    (void) file_inode;
    (void) dentry;
}

ssize_t ramfs_create_file_inode(
    struct inode *parent_dir,
    struct inode *out_inode,
    struct dentry *out_dentry
) {
    out_inode->type = INODE_REGULAR_FILE;
    out_inode->superblock = parent_dir->superblock;
    init_list(&out_inode->dentry_lh);
    out_dentry->inode = out_inode;
    out_dentry->mounted_inode = NULL;
    struct ramfs_inode *ramfs_inode = ramfs_inode_alloc();
    init_list(&ramfs_inode->ramfs_clusters_lh);
    out_inode->private = ramfs_inode;
    list_add_tail(&out_dentry->dentry_le, &parent_dir->dentry_lh);
    return 0;
}

// out_dentry should have initialized name
ssize_t ramfs_create_dir_inode(
    struct inode *parent_dir,
    struct inode *out_inode,
    struct dentry *out_dentry
) {
    out_inode->type = INODE_DIRECTORY;
    out_inode->superblock = parent_dir->superblock;
    init_list(&out_inode->dentry_lh);
    out_dentry->inode = out_inode;
    out_dentry->mounted_inode = NULL;
    list_add_tail(&out_dentry->dentry_le, &parent_dir->dentry_lh);
    return 0;
}

// out_dentry should have initialized name
ssize_t ramfs_create_dev_inode(
    struct inode *parent_dir,
    uint16_t device_type,
    uint16_t device_number,
    struct file_operations *device_fops,
    void *device,
    struct inode *out_inode,
    struct dentry *out_dentry
) {
    out_inode->type = INODE_DEVICE;
    out_inode->superblock = parent_dir->superblock;
    out_inode->device_type = device_type;
    out_inode->device_number = device_number;
    out_inode->device_fops = device_fops;
    out_inode->device = device;
    out_dentry->inode = out_inode;
    out_dentry->mounted_inode = NULL;
    list_add_tail(&out_dentry->dentry_le, &parent_dir->dentry_lh);
    return 0;
}

ssize_t ramfs_read(struct file *filp, void *buffer, size_t length) {
    struct ramfs_inode *ramfs_inode = filp->inode->private;
    size_t read_start_offset = filp->offset;
    size_t read_end_offset = (filp->offset + length < ramfs_inode->size) ? (filp->offset + length) : ramfs_inode->size;
    if (read_end_offset == read_start_offset) {
        return 0;
    }

    struct list_head *ramfs_clusters_le = ramfs_inode->ramfs_clusters_lh.next;
    size_t cluster_start_offset = 0;
    size_t bytes_read = 0;
    while (cluster_start_offset < read_end_offset) {
        if (ramfs_clusters_le == &ramfs_inode->ramfs_clusters_lh) {
            // No more clusters
            break;
        }
        size_t cluster_end_offset = cluster_start_offset + RAMFS_CLUSTER_CAPACITY;
        if (read_start_offset < cluster_end_offset) {
            // There is some overlap between the cluster's offset range and the buffer's offset range
            size_t copied_offset_range_start = read_start_offset > cluster_start_offset ? read_start_offset : cluster_start_offset;
            size_t copied_offset_range_end = read_end_offset < cluster_end_offset ? read_end_offset : cluster_end_offset;
            struct ramfs_cluster *cluster = container_of(
                ramfs_clusters_le,
                struct ramfs_cluster,
                ramfs_clusters_le
            );
            memcpy(
                &buffer[copied_offset_range_start - read_start_offset],
                &cluster->data[copied_offset_range_start - cluster_start_offset],
                copied_offset_range_end - copied_offset_range_start
            );
            bytes_read += (copied_offset_range_end - copied_offset_range_start);
        }
        ramfs_clusters_le = ramfs_clusters_le->next;
        cluster_start_offset += RAMFS_CLUSTER_CAPACITY;
    }
    filp->offset += bytes_read;
    return bytes_read;
}

ssize_t ramfs_write(struct file *filp, void *buf, size_t length) {
    uint8_t *buffer = buf;
    struct ramfs_inode *ramfs_inode = filp->inode->private;
    
    if (length == 0) {
        return 0;
    }

    // Ensure the inode contains enough clusters
    size_t write_start_offset = filp->offset;
    size_t write_end_offset = filp->offset + length;
    if (write_end_offset > ramfs_inode->size) {
        ramfs_set_size(filp, write_end_offset);
    }

    struct list_head *ramfs_clusters_le = ramfs_inode->ramfs_clusters_lh.next;
    size_t cluster_start_offset = 0;
    while (cluster_start_offset < write_end_offset) {
        size_t cluster_end_offset = cluster_start_offset + RAMFS_CLUSTER_CAPACITY;
        if (write_start_offset < cluster_end_offset) {
            // There is some overlap between the cluster's offset range and the buffer's offset range
            size_t copied_offset_range_start = write_start_offset > cluster_start_offset ? write_start_offset : cluster_start_offset;
            size_t copied_offset_range_end = write_end_offset < cluster_end_offset ? write_end_offset : cluster_end_offset;
            struct ramfs_cluster *cluster = container_of(
                ramfs_clusters_le,
                struct ramfs_cluster,
                ramfs_clusters_le
            );
            memcpy(
                &cluster->data[copied_offset_range_start - cluster_start_offset],
                &buffer[copied_offset_range_start - write_start_offset],
                copied_offset_range_end - copied_offset_range_start
            );
        }
        ramfs_clusters_le = ramfs_clusters_le->next;
        cluster_start_offset += RAMFS_CLUSTER_CAPACITY;
    }
    return length;
}

void ramfs_drop_cluster(struct ramfs_cluster *cluster) {
    list_del(&cluster->ramfs_clusters_le);
    ramfs_cluster_free(cluster);
}

ssize_t ramfs_set_size(struct file *filp, size_t size) {
    struct ramfs_inode *ramfs_inode = filp->inode->private;
    struct list_head *ramfs_clusters_le = ramfs_inode->ramfs_clusters_lh.next;
    size_t cluster_start_offset = 0;
    // Create new clusters if needed
    do {
        if (ramfs_clusters_le == &ramfs_inode->ramfs_clusters_lh) {
            // Add another cluster and memset data to zeroes
            struct ramfs_cluster *cluster = ramfs_cluster_alloc();
            list_add_tail(&cluster->ramfs_clusters_le, &ramfs_inode->ramfs_clusters_lh);
            memset(cluster->data, 0, RAMFS_CLUSTER_CAPACITY);
        } else {
            // Go to next cluster
            ramfs_clusters_le = ramfs_clusters_le->next;
        }
        cluster_start_offset += RAMFS_CLUSTER_CAPACITY;
    } while (cluster_start_offset < size);
    // ramfs_clusters_le now points at the first unneeded cluster (if there is one)
    
    // Delete excess clusters if needed
    while (ramfs_clusters_le != &ramfs_inode->ramfs_clusters_lh) {
        struct ramfs_cluster *cluster = container_of(
            ramfs_clusters_le,
            struct ramfs_cluster,
            ramfs_clusters_le
        );
        ramfs_clusters_le = ramfs_clusters_le->next;
        ramfs_drop_cluster(cluster);
    }
    ramfs_inode->size = size;
    return 0;
}

struct filesystem_ops ramfs_superblock_ops = {
    .mount = ramfs_mount,
    .lookup = ramfs_lookup,
    .create_file_inode = ramfs_create_file_inode,
    .create_dir_inode = ramfs_create_dir_inode,
    .create_dev_inode = ramfs_create_dev_inode,
    .read = ramfs_read,
    .write = ramfs_write,
    .set_size = ramfs_set_size,
};
