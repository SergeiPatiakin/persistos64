#include <stdint.h>
#include "cstd.h"
#include <persistos.h>
#include <persistos-headers.h>

struct bmp_file_header {
	uint8_t ident[2];
	uint32_t file_size;
	uint8_t reserve_1[2];
	uint8_t reserve_2[2];
	uint32_t pix_array_offset;
} __attribute__((packed));

struct bmp_bitmap_info {
	uint32_t size;
	uint32_t width;
	uint32_t height;
	uint16_t colour_plain;
	uint16_t bbp;
	char compression_method[4];
	char img_size[4];
	char horizontal_res[4];
	char vertical_res[4];
	char num_colours[4];
	char important_colours[4];
} __attribute__((packed));

void main(int argc, uint8_t* argv[]) {
    if (argc < 2) {
        fputs(u8p("iview: expected a filepath argument\n"), stderr);
        exit(1);
    }
    if (argc > 2) {
        fputs(u8p("iview: unexpected argument\n"), stderr);
        exit(1);
    }
    ssize_t img_fd = open(argv[1], 0);
    if (is_error(img_fd)) {
        fputs(u8p("iview: error opening file\n"), stderr);
        exit(1);
    }
    ssize_t fb_fd = open("/dev/fb0", 0);
    if (is_error(fb_fd)) {
        fputs(u8p("iview: error opening framebuffer\n"), stderr);
        exit(1);
    }
    struct bmp_file_header file_header_buffer;
    read(img_fd, &file_header_buffer, sizeof(struct bmp_file_header));
    if (file_header_buffer.ident[0] != 'B' || file_header_buffer.ident[1] != 'M') {
        fputs(u8p("iview: not a BMP file\n"), stderr);
        exit(1);
    }
    struct bmp_bitmap_info bitmap_info_buffer;
    read(img_fd, &bitmap_info_buffer, sizeof(struct bmp_bitmap_info));
    if (bitmap_info_buffer.bbp != 24) {
        fputs(u8p("iview: only 24-bit bitmaps supported\n"), stderr);
        exit(1);
    }

    struct fb_info fb_info;
    if (is_error(ioctl(fb_fd, 1, &fb_info))) {
        fputs(u8p("iview: error in ioctl\n"), stderr);
        exit(1);
    }

    size_t fb_width = fb_info.fb_width;
    size_t fb_height = fb_info.fb_height;
    size_t fb_pitch = fb_info.fb_pitch;

    size_t width_to_display = fb_width < bitmap_info_buffer.width ? fb_width : bitmap_info_buffer.width;
    size_t height_to_display = fb_height < bitmap_info_buffer.height ? fb_height : bitmap_info_buffer.height;

    uint8_t *img_pixels_buffer = malloc(width_to_display * 3);
    uint8_t *fb_pixels_buffer = malloc(width_to_display * 4);
    for (int i = 0; i < height_to_display; i++) {
        lseek(
            img_fd,
            file_header_buffer.pix_array_offset + (bitmap_info_buffer.height - 1 - i) * (bitmap_info_buffer.width * 3),
            0
        );
        read(img_fd, img_pixels_buffer, width_to_display * 3);
        for (int j = 0; j < width_to_display; j++) {
            fb_pixels_buffer[4 * j] = img_pixels_buffer[3 * j]; // B
            fb_pixels_buffer[4 * j + 1] = img_pixels_buffer[3 * j + 1]; // G
            fb_pixels_buffer[4 * j + 2] = img_pixels_buffer[3 * j + 2]; // R
            fb_pixels_buffer[4 * j + 3] = 0; // Alpha
        }
        write(fb_fd, fb_pixels_buffer, width_to_display * 4);
        lseek(fb_fd, fb_pitch * (i + 1), 0);
    }
    exit(0);
}
