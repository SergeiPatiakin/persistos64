#include <stdbool.h>
#include "arch/asm.h"
#include "drivers/device-numbers.h"
#include "drivers/tty.h"
#include "drivers/font.h"
#include "fs/vfs.h"
#include "lib/limine.h"
#include "lib/cstd.h"
#include "kernel/limine-requests.h"
#include "kernel/scheduler.h"

struct vt_device tty1;
struct vt_device tty2;
struct vt_device tty3;
struct vt_device *active_vt_device;
struct inode *tty1_inode;
struct inode *tty2_inode;
struct inode *tty3_inode;

uint32_t vt_background_color(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t device_number) {
    uint32_t red = (device_number == 1 ? 60 : 0) + (uint32_t)x * 128 / width;
    uint32_t green = (device_number == 2 ? 60 : 0);
    uint32_t blue = (device_number == 3 ? 60 : 0) + (uint32_t)y * 128 / height;
    uint32_t color = (red << 16) + (green << 8) + blue;
    return color;
}

uint32_t desktop_color(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    uint32_t blue = (uint32_t)x * 255 / width;
    uint32_t red = (uint32_t)y * 255 / height;
    uint32_t green = (red + blue) / 2;
    uint32_t color = (red << 16) + (green << 8) + blue;
    return color;
}

void repaint_desktop(struct limine_framebuffer *framebuffer) {
    volatile uint32_t *fb_ptr = framebuffer->address;
    volatile uint64_t fb_dword_pitch = framebuffer->pitch / 4; // Framebuffer pitch in dwords
    for (uint16_t y = 0; y < framebuffer->height; y++) {
        for (uint16_t x = 0; x < framebuffer->width; x++) {
            fb_ptr[y * fb_dword_pitch + x] = desktop_color(x, y, framebuffer->width, framebuffer->height);
        }
    }
}

void vt_device_init(
    struct vt_device *vt,
    struct limine_framebuffer *framebuffer,
    uint32_t device_number
) {
    uint16_t terminal_width = (framebuffer->width / 16 <= VT_MAX_WIDTH) ? (framebuffer->width / 16) : VT_MAX_WIDTH;
    uint16_t terminal_height = (framebuffer->height / 16 <= VT_MAX_HEIGHT) ? (framebuffer->height / 16) : VT_MAX_HEIGHT;

    memset(vt, 0, sizeof(struct vt_device));
    memset(vt->repaint_flags, 0xFF, VT_MAX_WIDTH * VT_MAX_HEIGHT / 8 + 1);
    vt->framebuffer = framebuffer;
    vt->terminal_width = terminal_width;
    vt->terminal_height = terminal_height;
    vt->escape_state.type = VT_ESCAPE_NONE;
    vt->device_number = device_number;
}

void set_active_vt(struct vt_device *vt) {
    active_vt_device = vt;
    memset(active_vt_device->repaint_flags, 0xFF, VT_MAX_WIDTH * VT_MAX_HEIGHT / 8 + 1);
    repaint_desktop(active_vt_device->framebuffer);
    vt_repaint_terminal(active_vt_device);
}

// Early terminal initialization
void terminal_init_1() {
    // Ensure we got a framebuffer.
    if (
        framebuffer_request.response == NULL ||
        framebuffer_request.response->framebuffer_count < 1
    ) {
        halt_forever();
    }

    // Fetch the first framebuffer.
    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    vt_device_init(&tty1, framebuffer, 1);
    vt_device_init(&tty2, framebuffer, 2);
    vt_device_init(&tty3, framebuffer, 3);
    set_active_vt(&tty1);
}

void terminal_init_2() {
    tty1_inode = vfs_mknod(
        vfs_dev_dir_inode,
        u8p("tty1"),
        DEVICE_TTY,
        tty1.device_number,
        &tty_device_fops,
        &tty1
    );
    tty2_inode = vfs_mknod(
        vfs_dev_dir_inode,
        u8p("tty2"),
        DEVICE_TTY,
        tty2.device_number,
        &tty_device_fops,
        &tty2
    );
    tty3_inode = vfs_mknod(
        vfs_dev_dir_inode,
        u8p("tty3"),
        DEVICE_TTY,
        tty3.device_number,
        &tty_device_fops,
        &tty3
    );
}

void vt_set_char(
    struct vt_device *vt_device,
    uint8_t character,
    uint16_t row,
    uint16_t column
) {
    uint32_t terminal_buffer_index = row * VT_MAX_WIDTH + column;
    vt_device->terminal_buffer[terminal_buffer_index] = character;
    vt_device->repaint_flags[terminal_buffer_index >> 3] |= (1 << (terminal_buffer_index % 8));
}

void vt_set_cursor(
    struct vt_device *vt_device,
    uint16_t row,
    uint16_t column
) {
    uint32_t old_terminal_buffer_index = vt_device->cursor_row * VT_MAX_WIDTH + vt_device->cursor_column;
    uint32_t new_terminal_buffer_index = row * VT_MAX_WIDTH + column;
    vt_device->cursor_row = row;
    vt_device->cursor_column = column;
    vt_device->repaint_flags[old_terminal_buffer_index >> 3] |= (1 << (old_terminal_buffer_index % 8));
    vt_device->repaint_flags[new_terminal_buffer_index >> 3] |= (1 << (new_terminal_buffer_index % 8));
}

#define VT_TEXT_COLOR 0x00ff00

void vt_repaint_char(
    struct vt_device *vt_device,
    uint16_t row,
    uint16_t column
) {
    uint8_t glyph_idx;
    uint8_t ascii_char = vt_device->terminal_buffer[row * VT_MAX_WIDTH + column];
    if (ascii_char < 32 || ascii_char > 32 + 95) {
        glyph_idx = 0; // Display non-printable chars as space
    } else {
        glyph_idx = ascii_char - 32;
    }
    volatile uint32_t *fb_ptr = vt_device->framebuffer->address;
    volatile uint64_t fb_dword_pitch = vt_device->framebuffer->pitch / 4; // Framebuffer pitch in dwords
    for (uint8_t i = 0; i < 16; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            // Compute glyph pixel
            uint8_t b = Font16x16[glyph_idx * 32 + i * 2];
            uint16_t pixel_x = 16 * column + j;
            uint16_t pixel_y = 16 * row + i;
            uint32_t color = ((b >> (7 - j)) & 0x1)
                ? VT_TEXT_COLOR
                : vt_background_color(pixel_x, pixel_y, vt_device->framebuffer->width, vt_device->framebuffer->height, vt_device->device_number);
            // Override with cursor pixel if applicable
            if (
                row == vt_device->cursor_row &&
                column == vt_device->cursor_column &&
                (i == 0 || i == 15 || j == 0)
            ) {
                color = VT_TEXT_COLOR;
            }
            // Write pixel
            fb_ptr[pixel_y * fb_dword_pitch + pixel_x] = color;
        }
        for (uint8_t j = 0; j < 8; j++) {
            // Compute glyph pixel
            uint8_t b = Font16x16[glyph_idx * 32 + i * 2 + 1];
            uint16_t pixel_x = 16 * column + j + 8;
            uint16_t pixel_y = 16 * row + i;
            uint32_t color = ((b >> (7 - j)) & 0x1)
                ? VT_TEXT_COLOR
                : vt_background_color(pixel_x, pixel_y, vt_device->framebuffer->width, vt_device->framebuffer->height, vt_device->device_number);
            // Override with cursor pixel if applicable
            if (
                row == vt_device->cursor_row &&
                column == vt_device->cursor_column &&
                (i == 0 || i == 15 || j == 7)
            ) {
                color = VT_TEXT_COLOR;
            }
            // Write pixel
            fb_ptr[pixel_y * fb_dword_pitch + pixel_x] = color;
        }
    }
}

void vt_repaint_terminal(struct vt_device *vt_device) {
    for (uint16_t i = 0; i < vt_device->terminal_height; i++) {
        for (uint16_t j = 0; j < vt_device->terminal_width; j++) {
            uint32_t terminal_buffer_index = i * VT_MAX_WIDTH + j;
            if (vt_device->repaint_flags[terminal_buffer_index >> 3] & (1 << (terminal_buffer_index % 8))) {
                vt_repaint_char(vt_device, i, j);
                vt_device->repaint_flags[terminal_buffer_index >> 3] ^= (1 << (terminal_buffer_index % 8));
            }
        }
    }
}


// Does not handle repaint
void vt_shift_rows(struct vt_device *vt_device) {
	// Shift all but last row
	for (size_t y = 0; y < vt_device->terminal_height - 1; y++) {
		for (size_t x = 0; x < vt_device->terminal_width; x++) {
			const size_t source_index = VT_MAX_WIDTH * (y + 1) + x;
            uint8_t character = vt_device->terminal_buffer[source_index];
            vt_set_char(vt_device, character, y, x);
		}
	}
	// Clear last row
	for (size_t x = 0; x < vt_device->terminal_width; x++) {
        vt_set_char(vt_device, ' ', vt_device->terminal_height - 1, x);
	}
}

ssize_t vt_write(void *dev, uint8_t *buffer, uint64_t offset, size_t length) {
    (void) offset; // Unused
	struct vt_device *vt_device = dev;
    size_t bytes_written = 0;
    for (size_t i = 0; i < length; i++) {
        if (vt_device->escape_state.type == VT_ESCAPE_ESC_PAREN) {
            if (buffer[i] == 'A') {
                // Cursor up
                if (vt_device->cursor_row != 0) {
                    vt_set_cursor(vt_device, vt_device->cursor_row - 1, vt_device->cursor_column);
                }
                vt_device->escape_state.type = VT_ESCAPE_NONE;
            } else if (buffer[i] == 'B') {
                // Cursor down
                if (vt_device->cursor_row != vt_device->terminal_height - 1) {
                    vt_set_cursor(vt_device, vt_device->cursor_row + 1, vt_device->cursor_column);
                }
                vt_device->escape_state.type = VT_ESCAPE_NONE;
            } else if (buffer[i] == 'C') {
                // Cursor right
                if (vt_device->cursor_column != vt_device->terminal_width - 1) {
                    vt_set_cursor(vt_device, vt_device->cursor_row, vt_device->cursor_column + 1);
                }
                vt_device->escape_state.type = VT_ESCAPE_NONE;
            } else if (buffer[i] == 'D') {
                // Cursor left
                if (vt_device->cursor_column != 0) {
                    vt_set_cursor(vt_device, vt_device->cursor_row, vt_device->cursor_column - 1);
                }
                vt_device->escape_state.type = VT_ESCAPE_NONE;
            } else if (buffer[i] == 'K') {
                // Clear to end of line
                for (uint16_t col = vt_device->cursor_column; col < vt_device->terminal_width; col++) {
                    vt_set_char(vt_device, ' ', vt_device->cursor_row, col);
                }
                vt_device->escape_state.type = VT_ESCAPE_NONE;
            } else if (buffer[i] == 'H') {
                // Cursor to (0, 0)
                vt_set_cursor(vt_device, 0, 0);
                vt_device->escape_state.type = VT_ESCAPE_NONE;
            } else if (buffer[i] == 'J') {
                // Clear screen from cursor down
                uint16_t row = vt_device->cursor_row;
                uint16_t col = vt_device->cursor_column;
                while (true) {
                    vt_set_char(vt_device, ' ', row, col);
                    col++;
                    if (col == vt_device->terminal_width) {
                        row++;
                        col = 0;
                    }
                    if (row == vt_device->terminal_height) {
                        break;
                    }
                }
                vt_device->escape_state.type = VT_ESCAPE_NONE;
            } else {
                vt_device->escape_state.type = VT_ESCAPE_NONE;
            }
        } else if (vt_device->escape_state.type == VT_ESCAPE_ESC) {
            if (buffer[i] == '[') {
                vt_device->escape_state.type = VT_ESCAPE_ESC_PAREN;
            } else {
                vt_device->escape_state.type = VT_ESCAPE_NONE;
            }
        } else if (vt_device->escape_state.type == VT_ESCAPE_NONE) {
            if (buffer[i] == 0x1B) {
                vt_device->escape_state.type = VT_ESCAPE_ESC;
            } else if (buffer[i] == '\n') {
                if (vt_device->cursor_row == vt_device->terminal_height - 1) {
                    vt_set_cursor(vt_device, vt_device->cursor_row, 0);
                    vt_shift_rows(vt_device);
                } else {
                    vt_set_cursor(vt_device, vt_device->cursor_row + 1, 0);
                }
            } else if (buffer[i] == '\r') {
                vt_set_cursor(vt_device, vt_device->cursor_row, 0);
            } else if (buffer[i] == '\b') {
                if (vt_device->cursor_column > 0) {
                    vt_set_cursor(vt_device, vt_device->cursor_row, vt_device->cursor_column - 1);
                }
            } else {
                vt_set_char(vt_device, buffer[i], vt_device->cursor_row, vt_device->cursor_column);
                if (vt_device->cursor_column == vt_device->terminal_width - 1) {
                    if (vt_device->cursor_row == vt_device->terminal_height - 1) {
                        vt_set_cursor(vt_device, vt_device->cursor_row, 0);
                        vt_shift_rows(vt_device);
                    } else {
                        vt_set_cursor(vt_device, vt_device->cursor_row + 1, 0);
                    }
                } else {
                    vt_set_cursor(vt_device, vt_device->cursor_row, vt_device->cursor_column + 1);                
                }
            }
        } else {
            // Unknown escape state
            halt_forever();
        }
        bytes_written++;
    }
    if (vt_device == active_vt_device) {
        vt_repaint_terminal(vt_device);
    }
    return bytes_written;
}

ssize_t vt_read(void *dev, uint8_t *buffer, uint64_t offset, size_t length) {
    (void) offset; // Unused
    struct vt_device *vt_device = dev;
    uint8_t *buf = buffer;
    while(true) {
        task_yield();
        while (vt_device->input_rb_head_idx != vt_device->input_rb_tail_idx) {
            int8_t c = vt_device->input_rb[vt_device->input_rb_head_idx];
            *buf++ = c;
            vt_device->input_rb_head_idx = (vt_device->input_rb_head_idx + 1) % VT_INPUT_BUFFER_LENGTH;
            if (buf == buffer + length) {
                return length;
            }
        }
    }
}

// Called in interrupt context
void vt_append_input_character(struct vt_device *vt_device, uint8_t character) {
    if (
        (VT_INPUT_BUFFER_LENGTH + vt_device->input_rb_head_idx - vt_device->input_rb_tail_idx)
        % VT_INPUT_BUFFER_LENGTH
        == 1
    ) {
        // If head is 1 ahead of tail, the ring buffer is full so we have to drop events
        return;
    }
    vt_device->input_rb[vt_device->input_rb_tail_idx] = character;
    vt_device->input_rb_tail_idx = (vt_device->input_rb_tail_idx + 1) % VT_INPUT_BUFFER_LENGTH;
}

// Called in interrupt context
void vt_update_input(struct vt_device *vt_device, struct keyboard_event keyboard_event) {
    if (keyboard_event.symbol_type == KBD_ASCII) {
        vt_append_input_character(vt_device, keyboard_event.symbol);
    } else if (keyboard_event.symbol_type == KBD_NONASCII) {
        switch (keyboard_event.symbol) {
            case KBD_ESC: {
                vt_append_input_character(vt_device, 0x1B);
                break;
            }
            case KBD_BACKSPACE: {
                vt_append_input_character(vt_device, 0xFF);
                break;
            }
            case KBD_ARROW_UP: {
                vt_append_input_character(vt_device, 0x1B);
                vt_append_input_character(vt_device, '[');
                vt_append_input_character(vt_device, 'A');
                break;
            }
            case KBD_ARROW_DOWN: {
                vt_append_input_character(vt_device, 0x1B);
                vt_append_input_character(vt_device, '[');
                vt_append_input_character(vt_device, 'B');
                break;
            }
            case KBD_ARROW_RIGHT: {
                vt_append_input_character(vt_device, 0x1B);
                vt_append_input_character(vt_device, '[');
                vt_append_input_character(vt_device, 'C');
                break;
            }
            case KBD_ARROW_LEFT: {
                vt_append_input_character(vt_device, 0x1B);
                vt_append_input_character(vt_device, '[');
                vt_append_input_character(vt_device, 'D');
                break;
            }
        }
    }
}

// TODO: go through a ring buffer to allow use in interrupt context
void printk(uint8_t* data) {
	vt_write(&tty1, data, 0 /* dummy */, strlen(data));
}

void panic(uint8_t *message) {
    printk(message);
    halt_forever();
}

void printk_uint8(uint8_t data) {
    uint8_t buffer[3];
    sprintf_uint8(data, buffer);
    vt_write(&tty1, buffer, 0 /* dummy */, 2);
}

void printk_uint16(uint16_t data) {
    uint8_t buffer[5];
    sprintf_uint16(data, buffer);
    vt_write(&tty1, buffer, 0 /* dummy */, 4);
}

void printk_uint32(uint32_t data) {
    uint8_t buffer[9];
    sprintf_uint32(data, buffer);
    vt_write(&tty1, buffer, 0 /* dummy */, 8);
}

void printk_uint64(uint64_t data) {
    uint8_t buffer[17];
    sprintf_uint64(data, buffer);
    vt_write(&tty1, buffer, 0 /* dummy */, 16);
}

struct file_operations tty_device_fops = {
    .read = vt_read,
    .write = vt_write
};
