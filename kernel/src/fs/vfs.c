#include "arch/asm.h"
#include "drivers/tty.h"
#include "fs/ramfs.h"
#include "fs/vfs.h"
#include "kernel/scheduler.h"
#include "lib/cstd.h"
#include "mm/slab.h"

struct slab_allocator inode_allocator = SLAB_OF(struct inode);
struct slab_allocator dentry_allocator = SLAB_OF(struct dentry);
struct slab_allocator file_allocator = SLAB_OF(struct file);

struct inode vfs_root_inode;
struct inode *vfs_dev_dir_inode;
struct inode *vfs_sys_dir_inode;
struct superblock vfs_root_superblock;
struct ramfs_superblock ramfs_root_superblock;

void vfs_init() {
    slab_allocator_init(&inode_allocator);
    slab_allocator_init(&dentry_allocator);
    slab_allocator_init(&file_allocator);

    vfs_root_inode.type = INODE_DIRECTORY;
    init_list(&vfs_root_inode.dentry_lh); // Empty list of dentries
    
    vfs_root_superblock.private = &ramfs_root_superblock;
    vfs_root_superblock.device = NULL;
    vfs_root_superblock.device_fops = NULL;
    vfs_root_superblock.ops = &ramfs_superblock_ops;
    vfs_root_inode.superblock = &vfs_root_superblock;

    vfs_dev_dir_inode = vfs_mkdir(&vfs_root_inode, u8p("dev"));
    vfs_sys_dir_inode = vfs_mkdir(&vfs_root_inode, u8p("sys"));
}

// Create file inode
struct inode *vfs_create(struct inode *parent_dir, uint8_t *name) {
    struct inode *dir_inode = inode_alloc();
    struct dentry *dir_dentry = dentry_alloc();
    strcpy(dir_dentry->name, name); // TODO: strncpy
    ssize_t create_result = parent_dir->superblock->ops->create_file_inode(parent_dir, dir_inode, dir_dentry);
    if (create_result != 0) {
        inode_free(dir_inode);
        dentry_free(dir_dentry);
        return NULL;
    }
    return dir_inode;
}

// Create directory inode
struct inode *vfs_mkdir(struct inode *parent_dir, uint8_t *name) {
    struct inode *dir_inode = inode_alloc();
    struct dentry *dir_dentry = dentry_alloc();
    strcpy(dir_dentry->name, name); // TODO: strncpy
    parent_dir->superblock->ops->create_dir_inode(parent_dir, dir_inode, dir_dentry);
    return dir_inode;
}

// Create device inode
struct inode *vfs_mknod(
    struct inode *parent_dir,
    uint8_t *name,
    uint16_t device_type,
    uint16_t device_number,
    struct file_operations *device_fops,
    void *device
) {
    struct inode *dev_inode = inode_alloc();
    struct dentry *dev_dentry = dentry_alloc();
    strcpy(dev_dentry->name, name); // TODO: strncpy
    parent_dir->superblock->ops->create_dev_inode(
        parent_dir,
        device_type,
        device_number,
        device_fops,
        device,
        dev_inode,
        dev_dentry
    );
    return dev_inode;
}

void vfs_resolve(uint8_t *path, struct vfs_lookup_result *lookup_result) {
    uint8_t *buf = path;
    while(*buf == '/') buf++;
    struct inode *inode = &vfs_root_inode;
    struct dentry *cur_dentry = NULL;
    struct inode *parent_inode = &vfs_root_inode;
    while (*buf != 0) {
        // At this point, we are trying to traverse into inode.
        // It had better be a directory
        if (inode->type != INODE_DIRECTORY) {
            lookup_result->status = VFS_RESOLVE_ERR_NOT_A_DIR;
            return;
        }
        
        // Treat multiple consecutive slashes as single slash
        while (*buf == '/') buf++;
        uint8_t *path_fragment_start = buf;
        while (*buf != '/' && *buf != 0) buf++;
        // First character after end of path fragment
        uint8_t *path_fragment_end = buf;
        if (path_fragment_end == path_fragment_start) {
            // Trailing slash at end of directory
            break;
        }

        // Search directory for matching dentry
        bool found_match = false;
        for (
            struct list_head *dentry_le = inode->dentry_lh.next;
            dentry_le != &inode->dentry_lh;
            dentry_le = dentry_le->next
        ) {
            struct dentry *dentry = container_of(dentry_le, struct dentry, dentry_le);
            if (strklcmp(dentry->name, path_fragment_start, path_fragment_end - path_fragment_start) == 0) {
                // Found match
                found_match = true;
                parent_inode = inode;
                cur_dentry = dentry;
                if (dentry->mounted_inode) {
                    inode = dentry->mounted_inode;
                } else {
                    inode = dentry->inode;
                }
                
                // For disk based filesystems
                inode->superblock->ops->lookup(inode, dentry /* sus */ );
                break;
            }
        }
        if (!found_match) {
            if (*buf == 0) {
                // This is the last fragment
                lookup_result->status = VFS_RESOLVE_SUCCESS_DOESNT_EXIST;
                lookup_result->dentry = cur_dentry;
                lookup_result->parent_directory = inode;
                lookup_result->name_start = path_fragment_start;
                lookup_result->name_length = path_fragment_end - path_fragment_start;
                return;
            } else {
                lookup_result->status = VFS_RESOLVE_ERR_DOESNT_EXIST;
                return;
            }
        }
    }
    lookup_result->inode = inode;
    lookup_result->dentry = cur_dentry;
    lookup_result->parent_directory = parent_inode;
    lookup_result->status = VFS_RESOLVE_SUCCESS_EXISTS;
    return;
}

ssize_t vfs_write(struct file *filp, void *buffer, size_t length) {
    if (filp->inode->type == INODE_DEVICE) {
        ssize_t bytes_written = filp->inode->device_fops->write(
            filp->inode->device,
            buffer,
            filp->offset,
            length
        );
        if (bytes_written != -1) {
            filp->offset += bytes_written;
        }
        return bytes_written;
    } else if (filp->inode->type == INODE_REGULAR_FILE) {
        if (!filp->inode->superblock) {
            panic(u8p("vfs_write: no supernode\n")); // Should not happen
        }
        ssize_t bytes_written = filp->inode->superblock->ops->write(filp, buffer, length);
        if (bytes_written != -1) {
            filp->offset += bytes_written;
        }
        return bytes_written;
    } else if (filp->inode->type == INODE_DIRECTORY) {
        printk(u8p("vfs_write: not implemented for directories\n"));
        return -1;
    }
    panic(u8p("vfs_write: unknown inode type\n"));
    return -1;
}

ssize_t vfs_read(struct file *filp, void *buffer, size_t length) {
    if (filp->inode->type == INODE_DEVICE) {
        ssize_t bytes_read = filp->inode->device_fops->read(
            filp->inode->device,
            buffer,
            filp->offset,
            length
        );
        filp->offset += bytes_read;
        return bytes_read;
    } else if (filp->inode->type == INODE_REGULAR_FILE) {
        if (!filp->inode->superblock) {
            // printk("vfs_read: no supernode\n"); // Should not happen
            return -1;
        }
        return filp->inode->superblock->ops->read(filp, buffer, length);
    } else if (filp->inode->type == INODE_DIRECTORY) {
        // printk("vfs_read: not implemented for directories\n");
        return -1;
    } else {
        // printk("vfs_read: unknown inode type\n");
        return -1;
    }
}

ssize_t vfs_ftruncate(struct file *filp, size_t size) {
    if (filp->inode->type == INODE_DEVICE) {
        return -1;
    } else if (filp->inode->type == INODE_REGULAR_FILE) {
        if (!filp->inode->superblock) {
            return -1;
        }
        return filp->inode->superblock->ops->set_size(filp, size);
    } else if (filp->inode->type == INODE_DIRECTORY) {
        return -1;
    } else {
        return -1;
    }
}

struct file *filp_find(struct task_struct *process, uint32_t fd) {
    list_for_each(files_le, process->files_lh) {
        struct file *candidate_filp = container_of(files_le, struct file, files_le);
        if (candidate_filp->fd == fd) {
            return candidate_filp;
        }
    }
    return NULL;
}

// Find first filp with fd larger than given fd
struct file *filp_insert_point_find(struct task_struct *process, uint32_t fd) {
    list_for_each(files_le, process->files_lh) {
        struct file *candidate_filp = container_of(files_le, struct file, files_le);
        if (candidate_filp->fd > fd) {
            return candidate_filp;
        }
    }
    return NULL;
}
