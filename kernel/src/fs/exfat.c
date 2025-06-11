// Exfat filesystem
#include <stddef.h>
#include "exfat.h"
#include "drivers/device-numbers.h"
#include "drivers/tty.h"
#include "lib/cstd.h"
#include "fs/vfs.h"
#include "mm/kmem.h"
#include "mm/slab.h"

struct slab_allocator exfat_inode_allocator = SLAB_OF(struct exfat_inode);
struct slab_allocator exfat_cluster_allocator = SLAB_OF(struct exfat_cluster);

void exfat_init() {
    slab_allocator_init(&exfat_inode_allocator);
    slab_allocator_init(&exfat_cluster_allocator);
}

uint64_t cluster_index_to_lba(uint32_t cluster_index, struct exfat_superblock *esb) {
    return esb->cluster_heap_offset +
        (1 << esb->sectors_per_cluster_exponent) *
        (cluster_index - 2);
}

uint64_t cluster_index_to_device_offset(uint32_t cluster_index, struct exfat_superblock *esb) {
    return cluster_index_to_lba(cluster_index, esb) << (esb->bytes_per_sector_exponent);
}

ssize_t exfat_mount(struct inode *device_inode, struct dentry *mountpoint_dentry) {
    (void) device_inode;
    (void) mountpoint_dentry;
    if (device_inode->type != INODE_DEVICE || device_inode->device_type != DEVICE_NVMEPART) {
        return -10;
    }
    struct device_operations *ops = device_inode->device_ops;
    struct nvmepart_device *dev = device_inode->device;
    void *page0 = kpage_alloc(1);
    ops->read(dev, page0, 0, 4096);
    if (memcmp("EXFAT", page0 + 3, 5) != 0) {
        kpage_free(page0, 1);
        return -11;
    }

    struct exfat_superblock *exfat_superblock = kpage_alloc(1); // TODO: don't waste memory
    exfat_superblock->fat_offset = *((uint32_t*)(page0 + 0x50));
    exfat_superblock->fat_length = *((uint32_t*)(page0 + 0x54));
    exfat_superblock->cluster_heap_offset = *((uint32_t*)(page0 + 0x58));
    exfat_superblock->first_cluster_of_root_directory = *((uint32_t*)(page0 + 0x60));
    exfat_superblock->bytes_per_sector_exponent = *((uint8_t*)(page0 + 0x6C));
    exfat_superblock->sectors_per_cluster_exponent = *((uint8_t*)(page0 + 0x6D));
    int8_t pages_per_cluster_exponent = exfat_superblock->bytes_per_sector_exponent
        + exfat_superblock->sectors_per_cluster_exponent
        - 12;
    exfat_superblock->pages_per_cluster = pages_per_cluster_exponent > 0
        ? 1L << pages_per_cluster_exponent
        : 1;

    struct superblock *vfs_superblock = kpage_alloc(1); // TODO: don't waste memory
    vfs_superblock->device_ops = ops;
    vfs_superblock->device = dev;
    vfs_superblock->ops = &exfat_superblock_ops;
    vfs_superblock->private = exfat_superblock;

    struct exfat_inode *exfat_inode = exfat_inode_alloc();
    exfat_inode->first_cluster_index = exfat_superblock->first_cluster_of_root_directory;
    exfat_inode->load_needed = true;
    exfat_inode->no_fat_chain = false; // TODO: can the root directory have NoFatChain? 
    exfat_inode->dentry_cluster_index = 0; // No dentry for the root directory
    exfat_inode->dentry_cluster_offset = 0;
    init_list(&exfat_inode->cluster_chain_lh);
  
    struct inode *vfs_root_inode = inode_alloc();
    vfs_root_inode->type = INODE_DIRECTORY;
    vfs_root_inode->superblock = vfs_superblock;
    vfs_root_inode->private = exfat_inode;
    init_list(&vfs_root_inode->dentry_lh);
  
    mountpoint_dentry->mounted_inode = vfs_root_inode;
    kpage_free(page0, 1);
    return 0;
}

void exfat_lookup(struct inode *inode, struct dentry *dentry) {
    (void) dentry;
    struct exfat_inode *exfat_inode = inode->private;
    if (!exfat_inode->load_needed) {
        return;
    }
    if (inode->type == INODE_DIRECTORY) {
        exfat_load_dir_inode(inode);
    } else if (inode->type == INODE_REGULAR_FILE) {
        exfat_load_file_inode(inode);
    }
}

void exfat_load_dir_inode(struct inode *dir_inode) {
    exfat_load_cluster_chain(dir_inode);
    struct superblock *vfs_superblock = dir_inode->superblock;
    struct exfat_superblock *exfat_superblock = vfs_superblock->private;
    struct exfat_inode *exfat_inode = dir_inode->private;
    void *cluster_buffer = kpage_alloc(exfat_superblock->pages_per_cluster); // Freed at the end of the function
    uint64_t bytes_per_cluster = 1LL << (exfat_superblock->bytes_per_sector_exponent + exfat_superblock->sectors_per_cluster_exponent);

    struct dentry *file_dentry = NULL;
    uint8_t file_name_index = 0;
    for (
        struct list_head *cluster_chain_le = exfat_inode->cluster_chain_lh.next;
        cluster_chain_le != &exfat_inode->cluster_chain_lh;
        cluster_chain_le = cluster_chain_le->next
    ) {
        struct exfat_cluster *cluster = (void*)cluster_chain_le - offsetof(struct exfat_cluster, cluster_chain_le);
        uint32_t cluster_index = cluster->cluster_index;
        vfs_superblock->device_ops->read(
            vfs_superblock->device,
            cluster_buffer,
            cluster_index_to_device_offset(cluster_index, exfat_superblock),
            bytes_per_cluster
        );
        uint8_t *x = cluster_buffer;
        while (true) { // For every directory entry
            if (*x == 0x83) {
                // Volume Label
            } else if (*x == 0x81) {
                // Allocation Bitmap
            } else if (*x == 0x82) {
                // Up-case table
            } else if (*x == 0x85) {
                // File entry
                file_dentry = dentry_alloc();
                file_dentry->inode = NULL;
                file_dentry->mounted_inode = NULL;
                file_name_index = 0;
                memset(file_dentry->name, 0, DENTRY_MAX_NAME_LENGTH + 1);
                struct inode *vfs_inode = inode_alloc();
                vfs_inode->superblock = vfs_superblock;
                struct exfat_inode *child_inode = exfat_inode_alloc();
                vfs_inode->private = child_inode;
                file_dentry->inode = vfs_inode;
                list_add_tail(&file_dentry->dentry_le, &dir_inode->dentry_lh);
                
                uint16_t file_attributes = *((uint16_t*)(x + 4));
                init_list(&child_inode->cluster_chain_lh);
                bool is_directory = file_attributes & (1 << 4);
                if (is_directory) {
                    file_dentry->inode->type = INODE_DIRECTORY;
                    init_list(&file_dentry->inode->dentry_lh);
                } else {
                    file_dentry->inode->type = INODE_REGULAR_FILE;
                    file_dentry->inode->file_length = 0;
                }
                child_inode->load_needed = true;
            } else if (*x == 0xC0) {
                // Stream Extension Entry
                uint32_t start_cluster_number = *((uint32_t*)(x + 0x14));
                ((struct exfat_inode*)(file_dentry->inode->private))->first_cluster_index = start_cluster_number;
                ((struct exfat_inode*)(file_dentry->inode->private))->dentry_cluster_index = cluster_index;
                ((struct exfat_inode*)(file_dentry->inode->private))->dentry_cluster_offset = (void*)x - cluster_buffer;
                
                uint32_t file_length = *((uint32_t*)(x + 0x8)); // This is really uint64_t
                ((struct exfat_inode*)(file_dentry->inode->private))->size = file_length;
                if (file_dentry->inode->type == INODE_REGULAR_FILE) {
                    file_dentry->inode->file_length = file_length;
                }

                uint8_t general_secondary_flags = *((uint8_t*)(x + 0x1));
                bool no_fat_chain = general_secondary_flags & (1 << 1);
                ((struct exfat_inode*)(file_dentry->inode->private))->no_fat_chain = no_fat_chain;
            } else if (*x == 0xC1) {
                if (file_dentry == NULL) {
                    // Should never happen
                } else {
                    int i = 0;
                    for (; i < 15; i++) {
                        if (file_name_index + i > DENTRY_MAX_NAME_LENGTH) {
                            break;
                        }
                        file_dentry->name[file_name_index + i] = *(x + 2 * (i + 1));
                    }
                    file_name_index += i;
                }
            } else {
                // Unknown entry
            }
            x += 0x20;
            if ((void*)x >= cluster_buffer + bytes_per_cluster) {
                // Reached end of page
                break;
            }
            if (*x == 0) {
                // Reached end of directory entries
                break; // TODO: goto finalize;
            }
        }
    }
    
    exfat_inode->load_needed = false;
    kpage_free(cluster_buffer, exfat_superblock->pages_per_cluster);
}

void exfat_load_file_inode(struct inode *file_inode) {
    exfat_load_cluster_chain(file_inode);
    struct exfat_inode *exfat_inode = file_inode->private;
    exfat_inode->load_needed = false;
}

void exfat_load_cluster_chain(struct inode *inode) {
    struct superblock *vfs_superblock = inode->superblock;
    struct exfat_superblock *exfat_superblock = vfs_superblock->private;
    struct exfat_inode *exfat_inode = inode->private;
    uint32_t cluster_index = exfat_inode->first_cluster_index;

    uint64_t bytes_per_cluster = 1LL << (exfat_superblock->bytes_per_sector_exponent + exfat_superblock->sectors_per_cluster_exponent);
    bool is_root_directory = exfat_inode->first_cluster_index == exfat_superblock->first_cluster_of_root_directory;
    for (
        uint32_t byte_offset = 0;
        byte_offset < exfat_inode->size
        || is_root_directory; // We lack size info about the root dir, so just keep reading the cluster chain
        byte_offset += bytes_per_cluster
    ) {
        struct exfat_cluster *cluster = exfat_cluster_alloc();
        list_add_tail(&cluster->cluster_chain_le, &exfat_inode->cluster_chain_lh);
        cluster->cluster_index = cluster_index;
    
        if (exfat_inode->no_fat_chain) {
            cluster_index++;
        } else {
            uint32_t fat_entry_byte_offset = (
                exfat_superblock->fat_offset <<
                exfat_superblock->bytes_per_sector_exponent
            ) + 4 * cluster_index;

            uint32_t fat_entry;
            ssize_t device_bytes_read = vfs_superblock->device_ops->read(
                vfs_superblock->device,
                u8p(&fat_entry),
                fat_entry_byte_offset,
                4
            );
            if (device_bytes_read != 4) {
                printk("exfat: could not read FAT entry from backing device\n");
                break;
            }
        
            if (fat_entry == 0x00000000) {
                printk("exfat: unexpected zeroed FAT entry in chain\n");
                break;
            } else if (fat_entry == 0x00000001) {
                printk(u8p("exfat: unexpected cluster 0x00000001 in chain\n"));
                break;
            } else if (fat_entry == 0xFFFFFFFF) {
                // End of cluster chain
                break;
            } else if (fat_entry == 0xFFFFFFF7) {
                printk(u8p("exfat: bad cluster\n"));
                break;
            } else {
                cluster_index = fat_entry;
            }
        }
    }
}

ssize_t exfat_read(struct file *filp, void *buffer, size_t length) {
    struct exfat_inode* exfat_inode = filp->inode->private;
    struct superblock *vfs_superblock = filp->inode->superblock;
    struct exfat_superblock *exfat_superblock = vfs_superblock->private;
    void *cluster_buffer = kpage_alloc(exfat_superblock->pages_per_cluster);
    uint64_t bytes_per_cluster = 1LL << (exfat_superblock->bytes_per_sector_exponent + exfat_superblock->sectors_per_cluster_exponent);

    size_t read_start_offset = filp->offset;
    size_t read_end_offset = (filp->offset + length < filp->inode->file_length) ? (filp->offset + length) : filp->inode->file_length;
    if (read_end_offset == read_start_offset) {
        return 0;
    }

    uint64_t cluster_start_offset = 0; // File offset of start of cluster
    size_t bytes_read = 0;
    for (
        struct list_head *cluster_chain_le = exfat_inode->cluster_chain_lh.next;
        cluster_chain_le != &exfat_inode->cluster_chain_lh &&
        cluster_start_offset < read_end_offset;
        cluster_chain_le = cluster_chain_le->next,
        cluster_start_offset += bytes_per_cluster
    ) {
        size_t cluster_end_offset = cluster_start_offset + bytes_per_cluster;
        
        if (read_start_offset < cluster_end_offset) {
            // There is some overlap between the cluster's offset range and the buffer's offset range
            size_t copied_offset_range_start = read_start_offset > cluster_start_offset ? read_start_offset : cluster_start_offset;
            size_t copied_offset_range_end = read_end_offset < cluster_end_offset ? read_end_offset : cluster_end_offset;
            struct exfat_cluster *cluster = (void*)cluster_chain_le - offsetof(struct exfat_cluster, cluster_chain_le);

            ssize_t chunk_bytes_read = filp->inode->superblock->device_ops->read(
                filp->inode->superblock->device,
                cluster_buffer,
                cluster_index_to_device_offset(cluster->cluster_index, exfat_superblock) + (copied_offset_range_start - cluster_start_offset),
                copied_offset_range_end - copied_offset_range_start
            );
            memcpy(buffer, cluster_buffer, chunk_bytes_read);
            bytes_read += chunk_bytes_read;
            buffer += chunk_bytes_read;
        }
    }
    filp->offset += bytes_read;
    kpage_free(cluster_buffer, exfat_superblock->pages_per_cluster);
    return bytes_read;
}

ssize_t exfat_write(struct file *filp, void *buffer, size_t length) {
    struct exfat_inode* exfat_inode = filp->inode->private;
    struct superblock *vfs_superblock = filp->inode->superblock;
    struct exfat_superblock *exfat_superblock = vfs_superblock->private;
    uint64_t bytes_per_cluster = 1LL << (exfat_superblock->bytes_per_sector_exponent + exfat_superblock->sectors_per_cluster_exponent);

    size_t total_bytes_written = 0;
    uint64_t cluster_byte_offset = 0;
    for (
        struct list_head *cluster_chain_le = exfat_inode->cluster_chain_lh.next;
        cluster_chain_le != &exfat_inode->cluster_chain_lh;
        cluster_chain_le = cluster_chain_le->next,
        cluster_byte_offset += bytes_per_cluster
    ) {
        struct exfat_cluster *cluster = (void*)cluster_chain_le - offsetof(struct exfat_cluster, cluster_chain_le);
        if (filp->offset < cluster_byte_offset) {
            continue;
        }
        uint16_t cluster_offset = (filp->offset - cluster_byte_offset) & (bytes_per_cluster - 1); // From 0 to bytes_per_cluster
    
        size_t bytes_to_write = (length - total_bytes_written) < (bytes_per_cluster - cluster_offset) ? (length - total_bytes_written) : (bytes_per_cluster - cluster_offset);
        bytes_to_write = bytes_to_write < (filp->inode->file_length - filp->offset) ? bytes_to_write : (filp->inode->file_length - filp->offset);
        if (bytes_to_write == 0) {
            // We can't read any more (we have hit end of file or end of output buffer)
            goto finalize;
        }
    
        size_t bytes_written = filp->inode->superblock->device_ops->write(
            filp->inode->superblock->device,
            buffer,
            cluster_index_to_device_offset(cluster->cluster_index, exfat_superblock),
            bytes_to_write
        );
        total_bytes_written += bytes_written;
        buffer += bytes_written;
        filp->offset += bytes_written;
    }
    finalize:
    return total_bytes_written;
}

uint64_t exfat_clusters_needed(uint64_t file_size, struct exfat_superblock *esb) {
    if (file_size == 0) {
        return 0;
    }
    return (
        (file_size - 1)
        >> (esb->bytes_per_sector_exponent + esb->sectors_per_cluster_exponent)
    ) + 1;
}

ssize_t exfat_set_size(struct file *filp, size_t size) {
    (void) filp;
    (void) size;
    struct superblock *vfs_superblock = filp->inode->superblock;
    struct exfat_superblock *exfat_superblock = (struct exfat_superblock*)(vfs_superblock->private);
    struct exfat_inode *exfat_inode = (struct exfat_inode*)(filp->inode->private);
    uint64_t current_num_clusters = exfat_clusters_needed(filp->inode->file_length, exfat_superblock);
    uint64_t new_num_clusters = exfat_clusters_needed(size, exfat_superblock);
    if (new_num_clusters != current_num_clusters) {
        return -1;
    }
    uint8_t buffer[32];
    vfs_superblock->device_ops->read(
        vfs_superblock->device,
        &buffer,
        cluster_index_to_device_offset(exfat_inode->dentry_cluster_index, exfat_superblock) + exfat_inode->dentry_cluster_offset,
        32
    );
    if (*((uint32_t*)(buffer + 0x8)) != filp->inode->file_length) {
        printk("exfat: file length inconsistency\n");
        return -3;
    }
    *((uint32_t*)(buffer + 0x8)) = size; // really uint64_t
    vfs_superblock->device_ops->write(
        vfs_superblock->device,
        &buffer,
        cluster_index_to_device_offset(exfat_inode->dentry_cluster_index, exfat_superblock) + exfat_inode->dentry_cluster_offset,
        32
    );
    filp->inode->file_length = size;

    return 0;
}

ssize_t exfat_create_file_inode(
    struct inode *parent_dir,
    struct inode *out_inode,
    struct dentry *out_dentry
) {
    (void) parent_dir;
    (void) out_inode;
    (void) out_dentry;
    return -1;
}

// out_dentry should have initialized name
ssize_t exfat_create_dir_inode(
    struct inode *parent_dir,
    struct inode *out_inode,
    struct dentry *out_dentry
) {
    (void) parent_dir;
    (void) out_inode;
    (void) out_dentry;
    return -1;
}

// out_dentry should have initialized name
ssize_t exfat_create_dev_inode(
    struct inode *parent_dir,
    uint16_t device_type,
    uint16_t device_number,
    struct device_operations *device_ops,
    void *device,
    struct inode *out_inode,
    struct dentry *out_dentry
) {
    (void) parent_dir;
    (void) device_type;
    (void) device_number;
    (void) device_ops;
    (void) device;
    (void) out_inode;
    (void) out_dentry;
    return -1;
}

struct filesystem_ops exfat_superblock_ops = {
    .mount = exfat_mount,
    .lookup = exfat_lookup,
    .create_file_inode = exfat_create_file_inode,
    .create_dir_inode = exfat_create_dir_inode,
    .create_dev_inode = exfat_create_dev_inode,
    .read = exfat_read,
    .write = exfat_write,
    .set_size = exfat_set_size,
};
