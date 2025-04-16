#ifndef VFS_H
#define VFS_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "lib/cstd.h"
#include "lib/list.h"

#define INODE_DIRECTORY 0x61
#define INODE_DEVICE 0x62
#define INODE_REGULAR_FILE 0x63

#define DENTRY_MAX_NAME_LENGTH 63

struct inode;
struct file;
struct dentry;
struct superblock;
struct file_operations;
struct task_struct;

struct filesystem_ops {
    // Returns error code
    ssize_t (*mount)(struct inode *device_inode, struct dentry *mountpoint_dentry);
    void (*lookup)(struct inode *inode, struct dentry *dentry);
    // Returns error code
    ssize_t (*create_file_inode)(
        struct inode *parent_inode,
        struct inode *out_inode,
        struct dentry *out_dentry
    );
    ssize_t (*create_dir_inode)(
        struct inode *parent_inode,
        struct inode *out_inode,
        struct dentry *out_dentry
    );
    ssize_t (*create_dev_inode)(
        struct inode *parent_inode,
        uint16_t device_type,
        uint16_t device_number,
        struct file_operations *device_fops,
        void *device,
        struct inode *out_inode,
        struct dentry *out_dentry
    );

    ssize_t (*read)(struct file *filp, void *buffer, size_t length);
    ssize_t (*write)(struct file *filp, void *buffer, size_t length);
    ssize_t (*set_size)(struct file *filp, size_t length);
};

struct superblock {
    void *private;
    struct filesystem_ops *ops;
    struct file_operations *device_fops; // Device file operations for underlying device
    void *device; // Underlying device
};

struct inode {
    uint8_t type;
    struct superblock *superblock;
    void *private; // FS-specific. Probably a pointer to a FS-specific inode struct
    
    union {
        // INODE_DIRECTORY
        struct {
            struct list_head dentry_lh; // List of children inodes
        };
    
        // INODE_DEVICE
        struct {
            uint16_t device_type;
            uint16_t device_number;
            struct file_operations *device_fops;
            void *device;
        };

        // INODE_REGULAR_FILE
        struct {
            size_t file_length;
        };
    };
};

struct dentry {
    uint8_t name[DENTRY_MAX_NAME_LENGTH + 1];
    struct inode* mounted_inode;
    struct inode* inode;
    struct list_head dentry_le;
};

extern struct inode vfs_root_inode;
extern struct inode *vfs_dev_dir_inode;

struct vfs_lookup_result {
    uint8_t status;
    struct inode *inode;
    struct dentry *dentry;
    struct inode *parent_directory;
    uint8_t *name_start; // Only filled for VFS_RESOLVE_SUCCESS_DOESNT_EXIST
    size_t name_length; // Only filled for VFS_RESOLVE_SUCCESS_DOESNT_EXIST
};

#define VFS_RESOLVE_SUCCESS_EXISTS 0
#define VFS_RESOLVE_SUCCESS_DOESNT_EXIST 1

#define VFS_RESOLVE_ERR_DOESNT_EXIST 3
#define VFS_RESOLVE_ERR_NOT_A_DIR 4

struct file_operations {
    ssize_t (*write)(void *dev, uint8_t* buffer, uint64_t offset, size_t length);
    ssize_t (*read)(void *dev, uint8_t* buffer, uint64_t offset, size_t length);
};

struct file {
    uint32_t fd; // File descriptor number
    struct inode *inode;
    uint64_t offset;
    struct list_head files_le; // List of struct file associated with a task_struct, sorted by fd
};

extern struct slab_allocator inode_allocator;
#define inode_alloc() slab_alloc(&inode_allocator)
#define inode_free(x) slab_free(&inode_allocator, x)

extern struct slab_allocator dentry_allocator;
#define dentry_alloc() slab_alloc(&dentry_allocator)
#define dentry_free(x) slab_free(&dentry_allocator, x)

extern struct slab_allocator file_allocator;
#define file_alloc() slab_alloc(&file_allocator)
#define file_free(x) slab_free(&file_allocator, x)


#define O_CREAT 0x1
#define O_TRUNCATE 0x2

void vfs_init();
struct inode *vfs_mknod(
    struct inode *dir,
    uint8_t *name,
    uint16_t device_type,
    uint16_t device_number,
    struct file_operations *fops,
    void *device
);
struct inode *vfs_mkdir(
    struct inode *dir,
    uint8_t *name
);
struct inode *vfs_create(struct inode *parent_dir, uint8_t *name);
void vfs_resolve(uint8_t *path, struct vfs_lookup_result *result);
ssize_t vfs_read(struct file *filp, void *buffer, size_t length);
ssize_t vfs_write(struct file *filp, void *buffer, size_t length);
ssize_t vfs_ftruncate(struct file *filp, size_t size);
struct file *filp_find(struct task_struct *process, uint32_t fd);
struct file *filp_insert_point_find(struct task_struct *process, uint32_t fd);

extern struct inode *vfs_sys_dir_inode;

#endif
