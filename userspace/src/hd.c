#include <stdint.h>
#include "cstd.h"
#include <persistos.h>

void main(int argc, uint8_t* argv[]) {
    uint64_t length_limit = 0;
    uint8_t *path = NULL;
    int processed_argc = 1;
    while (true) {
        if (processed_argc >= argc) {
            if (path == NULL) {
                fputs(u8p("hd: Expected a filepath argument\n"), stderr);
                exit(1);
            }
            break;
        } else if (strcmp(argv[processed_argc], u8p("-n")) == 0) {
            processed_argc++;
            if (processed_argc >= argc) {
                fputs(u8p("hd: Expected a length limit\n"), stderr);
                exit(1);
            }
            uint8_t parse_result = parse_n_dec(argv[processed_argc], 100, &length_limit);
            if (parse_result != strlen(argv[processed_argc])) {
                fputs(u8p("hd: parse error\n"), stderr);
                exit(1);
            }
            processed_argc++;
        } else {
            if (path != NULL) {
                fputs(u8p("hd: Only one path can be specified\n"), stderr);
                exit(1);
            }
            path = argv[processed_argc];
            processed_argc++;
        }
    }

    uint64_t fd = open(path, 0);
    if (is_error(fd)) {
        fputs(u8p("hd: Error opening\n"), stderr);
        exit(1);
    }

    uint8_t binary_buffer[16];
    uint8_t text_buffer[38];
    uint32_t file_offset = 0;
    
    while (true) {
        ssize_t bytes_to_read = (length_limit > 0 && file_offset + 16 > length_limit)
            ? length_limit - file_offset
            : 16;
        ssize_t bytes_read = read(fd, binary_buffer, bytes_to_read);
        if (is_error(bytes_read)) {
            fputs(u8p("hd: Error reading\n"), stderr);
            exit(1);
        }
        if (bytes_read == 0) {
            break;
        }

        memset(&text_buffer, 0, 38);
        sprintf_uint32(file_offset, text_buffer);
        puts(text_buffer);
        puts(u8p(":"));

        memset(&text_buffer, 0, 38);
        for (int i = 0; i < 4; i++) {
            if (bytes_read >= 4*i + 1) {
                text_buffer[9*i] = ' ';
                sprintf_uint8(binary_buffer[4*i], &text_buffer[9*i + 1]);
            }
            if (bytes_read >= 4*i + 2) sprintf_uint8(binary_buffer[4*i + 1], &text_buffer[9*i + 3]);
            if (bytes_read >= 4*i + 3) sprintf_uint8(binary_buffer[4*i + 2], &text_buffer[9*i + 5]);
            if (bytes_read >= 4*i + 4) sprintf_uint8(binary_buffer[4*i + 3], &text_buffer[9*i + 7]);
        }
        puts(text_buffer);
        puts(u8p("\n"));
        file_offset += bytes_read;
        if (length_limit != 0 && file_offset > length_limit) {
            break;
        }
    }

    // Print final line if file is not empty
    if (file_offset > 0) {
        memset(&text_buffer, 0, 38);
        sprintf_uint32(file_offset, text_buffer);
        puts(text_buffer);
        puts(u8p(":\n"));
    }

    exit(0);
}
