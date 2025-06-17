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

int fb_width;
int fb_height;
int fb_pitch;
ssize_t fb_fd;
uint8_t *img_pixels_buffer;
uint8_t *fb_pixels_buffer;


int view_image(ssize_t img_fd, bool error_ok) {
    if (is_error(img_fd)) {
        if (!error_ok) {
            fputs(u8p("iview: error opening file\n"), stderr);
        }
        return 1;
    }
    struct bmp_file_header file_header_buffer;
    read(img_fd, (void*)&file_header_buffer, sizeof(struct bmp_file_header));
    if (file_header_buffer.ident[0] != 'B' || file_header_buffer.ident[1] != 'M') {
        if (!error_ok) {
            fputs(u8p("iview: not a BMP file\n"), stderr);
        }
        return 2;
    }
    struct bmp_bitmap_info bitmap_info_buffer;
    read(img_fd, (void*)&bitmap_info_buffer, sizeof(struct bmp_bitmap_info));
    if (bitmap_info_buffer.bbp != 24) {
        if (!error_ok) {
            fputs(u8p("iview: only 24-bit bitmaps supported\n"), stderr);
        }
        return 3;
    }

    if (bitmap_info_buffer.compression_method != 0) {
        if (!is_error) {
            fputs(u8p("iview: compression not supported\n"), stderr);
        }
        return 4;
    }

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
        lseek(fb_fd, fb_pitch * i, SEEK_SET);
        write(fb_fd, fb_pixels_buffer, fb_width * 4);
    }
    close(img_fd);
    return 0;
}

int main(int argc, uint8_t* argv[]) {
    if (argc < 2) {
        fputs(u8p("iview: expected a filepath argument\n"), stderr);
        exit(1);
    }
    if (argc > 2) {
        fputs(u8p("iview: unexpected argument\n"), stderr);
        exit(1);
    }
    fb_fd = open(u8p("/dev/fb0"), 0);
    if (is_error(fb_fd)) {
        fputs(u8p("iview: error opening framebuffer\n"), stderr);
        exit(1);
    }
    struct fb_info fb_info;
    if (is_error(ioctl(fb_fd, 1, (uint64_t)&fb_info))) {
        fputs(u8p("iview: error in ioctl\n"), stderr);
        exit(1);
    }

    fb_width = fb_info.fb_width;
    fb_height = fb_info.fb_height;
    fb_pitch = fb_info.fb_pitch;

    img_pixels_buffer = malloc(fb_width * 3);
    fb_pixels_buffer = malloc(fb_width * 4);

    ssize_t path_fd = open(argv[1], 0); // Could be a directory or a file
    if (is_error(path_fd)) {
        fputs(u8p("iview: error opening file\n"), stderr);
        exit(1);
    }
    uint8_t dents_buf[4096];
    ssize_t bytes_read = getdents(path_fd, dents_buf, 4096);
    if (is_error(bytes_read)) {
        // It was a file. Display single image and exit
        int result = view_image(path_fd, false);
        if (result) {
            exit(result);
        }
        uint8_t buf[1];
        read(0, buf, 1);
        exit(0);
    } else {
        // It was a directory. Show a slideshow
        uint8_t *dents_buf_cursor = dents_buf;
        uint8_t path_buf[4096];
        strcpy(path_buf, argv[1]);
        uint8_t *path_buf_cursor = path_buf + strlen(argv[1]);
        *(path_buf_cursor++) = '/';
        
        while (dents_buf_cursor < dents_buf + bytes_read) {
            struct dent_header *header = (struct dent_header*)dents_buf_cursor;
            memcpy(path_buf_cursor, (void*)header + sizeof(struct dent_header), header->len);
            *(path_buf_cursor + sizeof(struct dent_header) + header->len) = 0;
            ssize_t img_fd = open(path_buf, 0);
            if (is_error(img_fd)) {
                fputs(u8p("iview: unexpected error opening image\n"), stderr);
                exit(1);
            }
            view_image(img_fd, true);
            close(img_fd);
            sleep(5000);
            dents_buf_cursor += header->len;
        }
        return 0;
    }
}
