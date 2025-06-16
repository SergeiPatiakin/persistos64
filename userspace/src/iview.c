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
	int32_t width;
	int32_t height;
	uint16_t colour_plain;
	uint16_t bbp;
	uint32_t compression_method;
	uint32_t img_size;
	int32_t horizontal_res;
	int32_t vertical_res;
	uint32_t num_colours;
	uint32_t important_colours;
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
    ssize_t fb_fd = open(u8p("/dev/fb0"), 0);
    if (is_error(fb_fd)) {
        fputs(u8p("iview: error opening framebuffer\n"), stderr);
        exit(1);
    }
    struct fb_info fb_info;
    if (is_error(ioctl(fb_fd, 1, (uint64_t)&fb_info))) {
        fputs(u8p("iview: error in ioctl\n"), stderr);
        exit(1);
    }

    ssize_t img_fd = open(argv[1], 0);
    if (is_error(img_fd)) {
        fputs(u8p("iview: error opening file\n"), stderr);
        exit(1);
    }
    struct bmp_file_header file_header_buffer;
    read(img_fd, (void*)&file_header_buffer, sizeof(struct bmp_file_header));
    if (file_header_buffer.ident[0] != 'B' || file_header_buffer.ident[1] != 'M') {
        fputs(u8p("iview: not a BMP file\n"), stderr);
        exit(1);
    }
    struct bmp_bitmap_info bitmap_info_buffer;
    read(img_fd, (void*)&bitmap_info_buffer, sizeof(struct bmp_bitmap_info));
    if (bitmap_info_buffer.bbp != 24) {
        fputs(u8p("iview: only 24-bit bitmaps supported\n"), stderr);
        exit(1);
    }

    if (bitmap_info_buffer.compression_method != 0) {
        fputs(u8p("iview: compression not supported\n"), stderr);
        exit(1);
    }

    int fb_width = fb_info.fb_width;
    int fb_height = fb_info.fb_height;
    int fb_pitch = fb_info.fb_pitch;
    
    bool top_to_bottom;
    int bitmap_height;
    if (bitmap_info_buffer.height > 0) {
        bitmap_height = bitmap_info_buffer.height;
        top_to_bottom = false;
    } else {
        bitmap_height = -bitmap_info_buffer.height;
        top_to_bottom = true;
    }
    int bitmap_width = bitmap_info_buffer.width;
    int width_to_display = fb_width < bitmap_info_buffer.width ? fb_width : bitmap_width;
    int height_to_display = fb_height < bitmap_height ? fb_height : bitmap_height;
    int left_margin = (fb_width - width_to_display) / 2;
    int top_margin = (fb_height - height_to_display) / 2;

    uint8_t *img_pixels_buffer = malloc(width_to_display * 3);
    uint8_t *fb_pixels_buffer = malloc(fb_width * 4);
    for (int i = 0; i < fb_height; i++) {
        if ((i < top_margin) || (i > top_margin + height_to_display - 1)) {
            memset(fb_pixels_buffer, 0, 4 * fb_width);
        } else {
            int bitmap_top = i - top_margin;
            int bitmap_line = top_to_bottom ? bitmap_top : bitmap_height - 1 - bitmap_top;
            lseek(
                img_fd,
                file_header_buffer.pix_array_offset + bitmap_line * bitmap_width * 3,
                SEEK_SET
            );
            read(img_fd, img_pixels_buffer, width_to_display * 3);
            for (int j = 0; j < fb_width; j++) {
                uint8_t red, green, blue;
                if ((j < left_margin) || (j > left_margin + width_to_display - 1)) {
                    red = green = blue = 0;
                } else {
                    int bitmap_left = j - left_margin;
                    blue = img_pixels_buffer[3 * bitmap_left]; // B
                    green = img_pixels_buffer[3 * bitmap_left + 1]; // G
                    red = img_pixels_buffer[3 * bitmap_left + 2]; // R
                }
                fb_pixels_buffer[4 * j] = blue;
                fb_pixels_buffer[4 * j + 1] = green;
                fb_pixels_buffer[4 * j + 2] = red;
                fb_pixels_buffer[4 * j + 3] = 0; // Alpha
            }
        }
        write(fb_fd, fb_pixels_buffer, fb_width * 4);
        lseek(fb_fd, fb_pitch * (i + 1), SEEK_SET);
    }
    close(img_fd);
    
    uint8_t buf[1];
    read(0, buf, 1);
    exit(0);
}
