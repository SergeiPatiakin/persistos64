// Driver for the framebuffer console device
#include <stdint.h>
#include "drivers/font.h"
#include "drivers/tty.h"
#include "kernel/limine-requests.h"
#include "fbcon.h"

uint32_t fbcon_background_color(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t device_number) {
    if (device_number == 4) {
        return 0x0000ff;
    }
    uint32_t red = (device_number == 1 ? 60 : 0) + (uint32_t)x * 128 / width;
    uint32_t green = (device_number == 2 ? 60 : 0);
    uint32_t blue = (device_number == 3 ? 60 : 0) + (uint32_t)y * 128 / height;
    uint32_t color = (red << 16) + (green << 8) + blue;
    return color;
}

uint32_t fbcon_foreground_color(uint16_t device_number) {
    if (device_number == 4) {
        return 0xffffff;
    }
    return 0x00ff00;
}

uint32_t fbcon_desktop_color(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    uint32_t blue = (uint32_t)x * 255 / width;
    uint32_t red = (uint32_t)y * 255 / height;
    uint32_t green = (red + blue) / 2;
    uint32_t color = (red << 16) + (green << 8) + blue;
    return color;
}

void fbcon_repaint_desktop(struct vt_device *vt_device) {
    struct limine_framebuffer *framebuffer = vt_device->framebuffer;
    volatile uint32_t *fb_ptr = framebuffer->address;
    volatile uint64_t fb_dword_pitch = framebuffer->pitch / 4; // Framebuffer pitch in dwords
    for (uint16_t y = 0; y < framebuffer->height; y++) {
        for (uint16_t x = 0; x < framebuffer->width; x++) {
            fb_ptr[y * fb_dword_pitch + x] = fbcon_desktop_color(x, y, framebuffer->width, framebuffer->height);
        }
    }
}

void fbcon_repaint_char(
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
                ? fbcon_foreground_color(vt_device->device_number)
                : fbcon_background_color(pixel_x, pixel_y, vt_device->framebuffer->width, vt_device->framebuffer->height, vt_device->device_number);
            // Override with cursor pixel if applicable
            if (
                row == vt_device->cursor_row &&
                column == vt_device->cursor_column &&
                (i == 0 || i == 15 || j == 0)
            ) {
                color = fbcon_foreground_color(vt_device->device_number);
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
                ? fbcon_foreground_color(vt_device->device_number)
                : fbcon_background_color(pixel_x, pixel_y, vt_device->framebuffer->width, vt_device->framebuffer->height, vt_device->device_number);
            // Override with cursor pixel if applicable
            if (
                row == vt_device->cursor_row &&
                column == vt_device->cursor_column &&
                (i == 0 || i == 15 || j == 7)
            ) {
                color = fbcon_foreground_color(vt_device->device_number);
            }
            // Write pixel
            fb_ptr[pixel_y * fb_dword_pitch + pixel_x] = color;
        }
    }
}

void fbcon_repaint_terminal(struct vt_device *vt_device) {
    for (uint16_t i = 0; i < vt_device->terminal_height; i++) {
        for (uint16_t j = 0; j < vt_device->terminal_width; j++) {
            uint32_t terminal_buffer_index = i * VT_MAX_WIDTH + j;
            if (vt_device->repaint_flags[terminal_buffer_index >> 3] & (1 << (terminal_buffer_index % 8))) {
                fbcon_repaint_char(vt_device, i, j);
                vt_device->repaint_flags[terminal_buffer_index >> 3] ^= (1 << (terminal_buffer_index % 8));
            }
        }
    }
}

