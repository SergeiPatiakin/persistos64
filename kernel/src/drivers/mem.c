// Driver for /dev/mem, a pseudodevice for reading the current process's memory space
// (kernel mode and user mode)
#include "drivers/device-numbers.h"
#include "mem.h"
#include "fs/vfs.h"
#include "lib/cstd.h"

struct inode *dev_mem_inode;

ssize_t dev_mem_read(void *dev, uint8_t* buffer, uint64_t offset, size_t length) {
    (void) dev;
    // TODO: avoid unmapped memory
    memcpy(buffer, (void*)offset, length);
    return length;
}

ssize_t dev_mem_write(void *dev, uint8_t* buffer, uint64_t offset, size_t length) {
    (void) dev;
    memcpy((void*)offset, buffer, length);
    return length;
}

void dev_mem_init() {
    dev_mem_inode = vfs_mknod(
        vfs_dev_dir_inode,
        u8p("mem"),
        DEVICE_MEM,
        1,
        &mem_device_ops,
        NULL // Could point to some dummy struct instead
    );
}

struct device_operations mem_device_ops = {
    .read = dev_mem_read,
    .write = dev_mem_write,
    .ioctl = NULL,
};
