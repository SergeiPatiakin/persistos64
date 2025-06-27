// Driver for the framebuffer device
#include <stdint.h>
#include "fs/vfs.h"
#include "drivers/device-numbers.h"
#include "kernel/limine-requests.h"
#include "include/persistos-headers.h"
#include "fb.h"

struct limine_framebuffer *framebuffer;
size_t framebuffer_length;
struct device_operations fb_device_ops;

void fb_init() {
    framebuffer = framebuffer_request.response->framebuffers[0];
    framebuffer_length = framebuffer->pitch * framebuffer->height * framebuffer->bpp;
    vfs_mknod(
        vfs_dev_dir_inode,
        u8p("fb0"),
        DEVICE_FRAMEBUFFER,
        1,
        &fb_device_ops,
        NULL
    );
}

ssize_t fb_read(void *dev, uint8_t* buffer, uint64_t offset, size_t length) {
    (void) dev;
    (void) buffer;
    (void) offset;
    (void) length;
    return -1;
}

ssize_t fb_write(void *dev, uint8_t* buffer, uint64_t offset, size_t length) {
    (void) dev;
    if (offset > framebuffer_length) {
        return 0;
    }
    ssize_t bytes_to_copy = offset + length > framebuffer_length
        ? framebuffer_length - offset
        : length;
    memcpy(framebuffer->address + offset, buffer, bytes_to_copy);
    return bytes_to_copy;
}

ssize_t fb_ioctl(void *dev, uint64_t cmd, uint64_t arg) {
    (void) dev;
    switch (cmd) {
        case 1: {
            struct fb_info *info = (void*)arg;
            info->fb_width = framebuffer->width;
            info->fb_height = framebuffer->height;
            info->fb_pitch = framebuffer->pitch;
            return 0;
        }
        default: {
            return -1;
        }
    }
}

struct device_operations fb_device_ops = {
    .read = fb_read,
    .write = fb_write,
    .ioctl = fb_ioctl,
};
