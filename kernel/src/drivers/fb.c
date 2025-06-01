#include <stdint.h>
#include "fs/vfs.h"
#include "drivers/device-numbers.h"
#include "kernel/limine-requests.h"
#include "fb.h"

struct limine_framebuffer *framebuffer;
ssize_t framebuffer_length;
struct file_operations fb_device_fops;

void fb_init() {
    framebuffer = framebuffer_request.response->framebuffers[0];
    framebuffer_length = framebuffer->pitch * framebuffer->height * framebuffer->bpp;
    vfs_mknod(
        vfs_dev_dir_inode,
        u8p("fb0"),
        DEVICE_FRAMEBUFFER,
        1,
        &fb_device_fops,
        NULL
    );
}

ssize_t fb_read(void *dev, uint8_t* buffer, uint64_t offset, size_t length) {
    return -1;
}

ssize_t fb_write(void *dev, uint8_t* buffer, uint64_t offset, size_t length) {
    if (offset > framebuffer_length) {
        return 0;
    }
    ssize_t bytes_to_copy = offset + length > framebuffer_length
        ? framebuffer_length - offset
        : length;
    memcpy(framebuffer->address + offset, buffer, bytes_to_copy);
    return bytes_to_copy;
}

struct file_operations fb_device_fops = {
    .read = fb_read,
    .write = fb_write,
};
