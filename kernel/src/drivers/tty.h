#ifndef VT_H
#define VT_H

#include <stdint.h>
#include <stddef.h>
#include <lib/cstd.h>
#include "keyboard.h"

#define VT_MAX_WIDTH 240
#define VT_MAX_HEIGHT 135

#define VT_INPUT_BUFFER_LENGTH 4096

#define VT_ESCAPE_NONE 0
#define VT_ESCAPE_ESC 1 // ^[
#define VT_ESCAPE_ESC_PAREN 2 // ^[[

struct vt_escape_state {
    uint8_t type;
};

struct vt_device {
    uint16_t cursor_row;
    uint16_t cursor_column;
    uint16_t terminal_width;
    uint16_t terminal_height;
    struct vt_escape_state escape_state;
    uint8_t terminal_buffer[VT_MAX_WIDTH * VT_MAX_HEIGHT];
    uint8_t repaint_flags[VT_MAX_WIDTH * VT_MAX_HEIGHT / 8 + 1];
    struct limine_framebuffer *framebuffer;
    uint32_t input_rb_head_idx;
    uint32_t input_rb_tail_idx;
    uint32_t device_number;
    int8_t input_rb[VT_INPUT_BUFFER_LENGTH];
};

extern struct vt_device tty1;
extern struct vt_device tty2;
extern struct vt_device tty3;
extern struct vt_device *active_vt_device;
extern struct inode *tty1_inode;
extern struct inode *tty2_inode;
extern struct inode *tty3_inode;

extern struct file_operations tty_device_fops;

void terminal_init_1();
void terminal_init_2();
void vt_repaint_terminal(struct vt_device *vt_device);
ssize_t vt_write(void *dev, uint8_t *buffer, uint64_t offset, size_t length);
ssize_t vt_read(void *dev, uint8_t *buffer, uint64_t offset, size_t length);
void vt_update_input(struct vt_device *vt_device, struct keyboard_event keyboard_event);

void printk(uint8_t* data);
void panic(uint8_t *message);

void printk_uint8(uint8_t data);
void printk_uint16(uint16_t data);
void printk_uint32(uint32_t data);
void printk_uint64(uint64_t data);

void set_active_vt(struct vt_device *vt);

#endif
