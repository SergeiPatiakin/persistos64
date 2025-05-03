#include "drivers/device-numbers.h"
#include "mem.h"
#include "fs/vfs.h"
#include "lib/cstd.h"

struct inode *mem_inode;

ssize_t dev_mem_read(void *dev, uint8_t* buffer, uint64_t offset, size_t length) {
    // TODO: avoid unmapped memory
    memcpy(buffer, offset, length);
    return length;
}

ssize_t dev_mem_write(void *dev, uint8_t* buffer, uint64_t offset, size_t length) {
    memcpy(offset, buffer, length);
    return length;
}

void dev_mem_init() {
    mem_inode = vfs_mknod(
        vfs_dev_dir_inode,
        u8p("mem"),
        DEVICE_KMEM,
        1,
        &dev_mem_fops,
        NULL // Could point to some dummy struct instead
    );
}

struct file_operations dev_mem_fops = {
    .read = dev_mem_read,
    .write = dev_mem_write,
};
