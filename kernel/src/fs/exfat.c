#include <stddef.h>
#include "exfat.h"
#include "drivers/device-numbers.h"
#include "drivers/tty.h"
#include "lib/cstd.h"
#include "fs/vfs.h"
#include "mm/kmem.h"
#include "mm/slab.h"

struct slab_allocator exfat_inode_allocator = SLAB_OF(struct exfat_inode);
struct slab_allocator exfat_file_cluster_allocator = SLAB_OF(struct exfat_file_cluster);

void exfat_init() {
    slab_allocator_init(&exfat_inode_allocator);
    slab_allocator_init(&exfat_file_cluster_allocator);
}

ssize_t exfat_mount(struct inode *device_inode, struct dentry *mountpoint_dentry) {
    (void) device_inode;
    (void) mountpoint_dentry;
    if (device_inode->type != INODE_DEVICE || device_inode->device_type != DEVICE_NVMEPART) {
        return -10;
    }
    struct file_operations *fops = device_inode->device_fops;
    struct nvmepart_device *dev = device_inode->device;
    void *page0 = kpage_alloc(1);
    fops->read(dev, page0, 0, 4096);
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

    struct superblock *vfs_superblock = kpage_alloc(1); // TODO: don't waste memory
    vfs_superblock->device_fops = fops;
    vfs_superblock->device = dev;
    vfs_superblock->ops = &exfat_superblock_ops;
    vfs_superblock->private = exfat_superblock;

    struct exfat_inode *exfat_inode = exfat_inode_alloc();
    exfat_inode->exfat_start_lba = exfat_superblock->cluster_heap_offset +
      (1 << exfat_superblock->sectors_per_cluster_exponent) *
      (exfat_superblock->first_cluster_of_root_directory - 2);
    exfat_inode->load_needed = true;
  
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
    struct superblock *vfs_superblock = dir_inode->superblock;
    struct exfat_superblock *exfat_superblock = vfs_superblock->private;
    struct exfat_inode *exfat_inode = dir_inode->private;
    uint32_t dir_content_lba = exfat_inode->exfat_start_lba;
    void *dir_content_page = kpage_alloc(1); // Freed at the end of the function
    
    struct dentry *file_dentry = NULL;
    uint8_t file_name_index = 0;
    while (true) { // For every directory content page
        vfs_superblock->device_fops->read(
            vfs_superblock->device,
            dir_content_page,
            dir_content_lba << exfat_superblock->bytes_per_sector_exponent, 4096
        );
        uint8_t *x = dir_content_page;
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
                bool is_directory = file_attributes & (1 << 4);
                if (is_directory) {
                    file_dentry->inode->type = INODE_DIRECTORY;
                    init_list(&file_dentry->inode->dentry_lh);
                } else {
                    file_dentry->inode->type = INODE_REGULAR_FILE;
                    file_dentry->inode->file_length = 0;
                    init_list(&child_inode->file_clusters_lh);
                }
                child_inode->load_needed = true;
            } else if (*x == 0xC0) {
                // File info entry
                uint32_t start_cluster_number = *((uint32_t*)(x + 0x14));
                ((struct exfat_inode*)(file_dentry->inode->private))->exfat_start_lba =
                    exfat_superblock->cluster_heap_offset +
                    (1 << exfat_superblock->sectors_per_cluster_exponent) *
                    (start_cluster_number - 2);
        
                if (file_dentry->inode->type == INODE_REGULAR_FILE) {
                    uint32_t file_length = *((uint32_t*)(x + 0x8)); // This is really uint64_t
                    file_dentry->inode->file_length = file_length;
                }
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
            if (x >= dir_content_page + 4096) {
                // Reached end of page
                break;
            }
            if (*x == 0) {
                // Reached end of directory entries
                break; // TODO: goto finalize;
            }
        }
        uint32_t dir_cluster_index = (
            (dir_content_lba - exfat_superblock->cluster_heap_offset) >>
            exfat_superblock->sectors_per_cluster_exponent
        ) + 2;
    
        uint32_t fat_entry_byte_offset = (
            exfat_superblock->fat_offset <<
            exfat_superblock->bytes_per_sector_exponent
        ) + 4 * dir_cluster_index;

        uint32_t fat_entry;
        vfs_superblock->device_fops->read(
            vfs_superblock->device,
            &fat_entry,
            fat_entry_byte_offset,
            4
        );
    
        if (fat_entry == 0x00000000) {
            // error("exfat_load_dir_inode: unexpected free cluster in chain\n");
            break;
        } else if (fat_entry == 0x00000001) {
            printk("exfat_load_dir_inode: unexpected cluster 0x00000001 in chain\n");
            break;
        } else if (fat_entry == 0xFFFFFFFF) {
            // End of cluster chain
            break;
        } else if (fat_entry == 0xFFFFFFF7) {
            printk("exfat_load_dir_inode: bad cluster\n");
            break;
        } else {
            uint32_t new_dir_content_lba = exfat_superblock->cluster_heap_offset +
            (1 << exfat_superblock->sectors_per_cluster_exponent) * (fat_entry - 2);
            dir_content_lba = new_dir_content_lba;
        }
    }
    
    finalize:
    exfat_inode->load_needed = false;
    kpage_free(dir_content_page, 1);
}

void exfat_load_file_inode(struct inode *file_inode) {
    struct superblock *vfs_superblock = file_inode->superblock;
    struct exfat_superblock *exfat_superblock = vfs_superblock->private;
    struct exfat_inode *exfat_inode = file_inode->private;
    uint32_t file_content_lba = exfat_inode->exfat_start_lba;
  
    for (uint32_t byte_offset = 0; byte_offset < file_inode->file_length; byte_offset += 4096) {
        struct exfat_file_cluster *cluster = exfat_file_cluster_alloc();
        list_add_tail(&cluster->file_clusters_le, &exfat_inode->file_clusters_lh);
        cluster->byte_offset = byte_offset;
        cluster->device_offset = (uint64_t)file_content_lba << exfat_superblock->bytes_per_sector_exponent;
        uint32_t cluster_index = (
            (file_content_lba - exfat_superblock->cluster_heap_offset)
            >> exfat_superblock->sectors_per_cluster_exponent
        ) + 2;
    
        uint32_t fat_entry_byte_offset = (
            exfat_superblock->fat_offset <<
            exfat_superblock->bytes_per_sector_exponent
        ) + 4 * cluster_index;

        uint32_t fat_entry;
        vfs_superblock->device_fops->read(
            vfs_superblock->device,
            &fat_entry,
            fat_entry_byte_offset,
            4
        );
    
        if (fat_entry == 0x00000000) {
            // error("exfat_load_file_inode: unexpected free cluster in chain\n");
            break;
        } else if (fat_entry == 0x00000001) {
            printk("exfat_load_file_inode: unexpected cluster 0x00000001 in chain\n");
            break;
        } else if (fat_entry == 0xFFFFFFFF) {
            // End of cluster chain
            break;
        } else if (fat_entry == 0xFFFFFFF7) {
            printk("exfat_load_file_inode: bad cluster\n");
            break;
        } else {
            uint32_t new_file_content_lba = exfat_superblock->cluster_heap_offset +
            (1 << exfat_superblock->sectors_per_cluster_exponent) * (fat_entry - 2);
            file_content_lba = new_file_content_lba;
        }
    }
    exfat_inode->load_needed = false;
}

ssize_t exfat_read(struct file *filp, void *buf, size_t length) {
    struct exfat_inode* exfat_inode = filp->inode->private;
    void *content_buffer = kpage_alloc(1);
    size_t total_bytes_read = 0;
    for (
        struct list_head *file_clusters_le = exfat_inode->file_clusters_lh.next;
        file_clusters_le != &exfat_inode->file_clusters_lh;
        file_clusters_le = file_clusters_le->next
    ) {
        struct exfat_file_cluster *cluster = (void*)file_clusters_le - offsetof(struct exfat_file_cluster, file_clusters_le);
        if (filp->offset < cluster->byte_offset) {
            continue;
        }
        uint16_t page_offset = (filp->offset - cluster->byte_offset) % 4096; // From 0 to 4095

        size_t bytes_to_read = (length - total_bytes_read) < (4096 - page_offset) ? (length - total_bytes_read) : (4096 - page_offset);
        bytes_to_read = bytes_to_read < (filp->inode->file_length - filp->offset) ? bytes_to_read : (filp->inode->file_length - filp->offset);
        if (bytes_to_read == 0) {
            // We can't read any more (we have hit end of file or end of output buffer)
            goto finalize;
        }

        ssize_t bytes_read = filp->inode->superblock->device_fops->read(
            filp->inode->superblock->device,
            content_buffer,
            cluster->device_offset + page_offset,
            bytes_to_read
        );
        memcpy(buf, content_buffer, bytes_read);
        total_bytes_read += bytes_read;
        buf += bytes_read;
        filp->offset += bytes_read;
    }
    finalize:
    kpage_free(content_buffer, 1);
    return total_bytes_read;
}

ssize_t exfat_write(struct file *filp, void *buffer, size_t length) {
    struct exfat_inode* exfat_inode = filp->inode->private;
    size_t total_bytes_written = 0;
    for (
        struct list_head *file_clusters_le = exfat_inode->file_clusters_lh.next;
        file_clusters_le != &exfat_inode->file_clusters_lh;
        file_clusters_le = file_clusters_le->next
    ) {
        struct exfat_file_cluster *cluster = (void*)file_clusters_le - offsetof(struct exfat_file_cluster, file_clusters_le);
        if (filp->offset < cluster->byte_offset) {
            continue;
        }
        uint16_t page_offset = (filp->offset - cluster->byte_offset) % 4096; // From 0 to 4095
    
        size_t bytes_to_write = (length - total_bytes_written) < (4096 - page_offset) ? (length - total_bytes_written) : (4096 - page_offset);
        bytes_to_write = bytes_to_write < (filp->inode->file_length - filp->offset) ? bytes_to_write : (filp->inode->file_length - filp->offset);
        if (bytes_to_write == 0) {
            // We can't read any more (we have hit end of file or end of output buffer)
            goto finalize;
        }
    
        size_t bytes_written = filp->inode->superblock->device_fops->write(
            filp->inode->superblock->device,
            buffer,
            cluster->device_offset + page_offset,
            bytes_to_write
        );
        total_bytes_written += bytes_written;
        buffer += bytes_written;
        filp->offset += bytes_written;
    }
    finalize:
    return total_bytes_written;
  }

ssize_t exfat_set_size(struct file *filp, size_t size) {
    (void) filp;
    (void) size;
    return -1;
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
    struct file_operations *device_fops,
    void *device,
    struct inode *out_inode,
    struct dentry *out_dentry
) {
    (void) parent_dir;
    (void) device_type;
    (void) device_number;
    (void) device_fops;
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
