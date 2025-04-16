#include "drivers/device-numbers.h"
#include "drivers/zero.h"
#include "fs/vfs.h"
#include "lib/cstd.h"

struct inode *zero_inode;

ssize_t devzero_read(void *dev, uint8_t* buffer, uint64_t offset, size_t length) {
    (void) dev;
    (void) buffer;
    (void) offset;
    (void) length;
    // Fill the buffer with zeros
    memset(buffer, 0, length);
    return length;
}

ssize_t devzero_write(void *dev, uint8_t* buffer, uint64_t offset, size_t length) {
    (void) dev;
    (void) buffer;
    (void) offset;
    (void) length;
    // Do nothing and report that all bytes have been written
    return length;
}

void zero_init() {
    zero_inode = vfs_mknod(
        vfs_dev_dir_inode,
        u8p("zero"),
        DEVICE_ZERO,
        1,
        &zero_device_fops,
        NULL // Could point to some dummy struct instead
    );
}

struct file_operations zero_device_fops = {
    .read = devzero_read,
    .write = devzero_write,
};
