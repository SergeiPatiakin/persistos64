#include <stdint.h>
#include "cstd.h"
#include <persistos.h>

void main(int argc, char* argv[]) {
    if (argc < 2) {
        fputs("hd: Expected a filepath argument\n", stderr);
        exit(1);
    }
    
    uint64_t length_limit = 0;
    if (argc > 2) {
        if (strcmp(argv[2], "-n") == 0) {
            if (argc < 4) {
                fputs("hd: Expected a length limit\n", stderr);
                exit(1);
            }
            uint8_t parse_result = parse_n_dec(argv[3], 100, &length_limit);
            if (parse_result != strlen(argv[3])) {
                fputs("hd: parse error\n", stderr);
                exit(1);
            }
        } else {
            fputs("hd: Unexpected argument\n", stderr);
            exit(1);
        }
    }

    uint64_t fd = open(argv[1], 0);
    if (is_error(fd)) {
        fputs("Error opening\n", stderr);
        exit(1);
    }

    uint8_t binary_buffer[16];
    uint8_t text_buffer[38];
    uint32_t file_offset = 0;
    
    while (true) {
        ssize_t bytes_read = read(fd, &binary_buffer, 16);
        if (is_error(bytes_read)) {
            fputs("Error reading\n", stderr);
            exit(1);
        }
        if (bytes_read == 0) {
            break;
        }

        memset(&text_buffer, 0, 38);
        sprintf_uint32(file_offset, text_buffer);
        puts(text_buffer);
        puts(":");

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
        puts("\n");
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
        puts(":\n");
    }

    exit(0);
}
