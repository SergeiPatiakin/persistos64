#include <stdint.h>
#include "cstd.h"
#include <persistos.h>
#include <persistos-headers.h>

#define INVALID_UINT64 0xffffffffffffffff

void main(int argc, uint8_t* argv[]) {
    uint8_t *input_file = NULL; // Mandatory
    uint8_t *output_file = NULL; // Mandatory
    uint64_t skip = 0; // Optional, defaults to 0
    uint64_t seek = 0; // Optional, defaults to 0
    uint64_t count = INVALID_UINT64; // Optional, defaults to unlimited
    uint64_t block_size = 512; // Optional, defaults to 512
    for (int argi = 1; argi < argc; argi++) {
        if (memcmp("if=", argv[argi], 3) == 0) {
            input_file = argv[argi] + 3;
        } else if (memcmp("of=", argv[argi], 3) == 0) {
            output_file = argv[argi] + 3;
        } else if (memcmp("count=", argv[argi], 6) == 0) {
            uint8_t parse_result = parse_n_dec(argv[argi] + 6, 100, &count);
            if (parse_result != strlen(argv[argi] + 6)) {
                fputs(u8p("dd: parse error in count"), stderr);
                exit(1);
            }
        } else if (memcmp("bs=", argv[argi], 3) == 0) {
            uint8_t parse_result = parse_n_dec(argv[argi] + 3, 100, &block_size);
            if (parse_result != strlen(argv[argi] + 3)) {
                fputs(u8p("dd: parse error in bs"), stderr);
                exit(1);
            }
        } else if (memcmp("skip=", argv[argi], 5) == 0) {
            uint8_t parse_result = parse_n_dec(argv[argi] + 5, 100, &skip);
            if (parse_result != strlen(argv[argi] + 5)) {
                fputs(u8p("dd: parse error in skip"), stderr);
                exit(1);
            }
        } else if (memcmp("seek=", argv[argi], 5) == 0) {
            uint8_t parse_result = parse_n_dec(argv[argi] + 5, 100, &seek);
            if (parse_result != strlen(argv[argi] + 5)) {
                fputs(u8p("dd: parse error in seek"), stderr);
                exit(1);
            }
        } else {
            fputs(u8p("dd: unknown argument\n"), stderr);
            exit(1);
        }
    }
    if (input_file == NULL) {
        fputs(u8p("dd: expected if argument\n"), stderr);
        exit(1);
    }
    if (output_file == NULL) {
        fputs(u8p("dd: expected of argument\n"), stderr);
        exit(1);
    }

    ssize_t in_fd = open(input_file, 0);
    if (is_error(in_fd)) {
        fputs(u8p("dd: error opening input file\n"), stderr);
        exit(1);
    }
    ssize_t out_fd = open(output_file, O_CREAT);
    if (is_error(out_fd)) {
        fputs(u8p("dd: error opening output file\n"), stderr);
        exit(1);
    }

    uint8_t *buffer = malloc(block_size);
    if (skip != 0) {
        if (is_error(lseek(in_fd, block_size * skip, SEEK_SET))) {
            fputs(u8p("dd: error skipping\n"), stderr);
            exit(1);
        }
    }
    if (seek != 0) {
        if (is_error(lseek(out_fd, block_size * seek, SEEK_SET))) {
            fputs(u8p("dd: error seeking\n"), stderr);
            exit(1);
        }
    }
    for (uint64_t i = 0; i < count || count == INVALID_UINT64; i++) {
        uint64_t block_bytes_read = 0;
        while (block_bytes_read < block_size) {
            ssize_t bytes_read = read(in_fd, buffer + block_bytes_read, block_size - block_bytes_read);
            if (is_error(bytes_read)) {
                fputs(u8p("dd: error reading file\n"), stderr);
                exit(1);
            }
            if (bytes_read == 0) {
                exit(0);
            }
            block_bytes_read += bytes_read;
        }
        if (is_error(write(out_fd, buffer, block_bytes_read))) {
            fputs(u8p("dd: error writing file\n"), stderr);
            exit(1);
        }
    }
    exit(0);
}
