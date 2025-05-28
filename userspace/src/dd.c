#include <stdint.h>
#include "cstd.h"
#include <persistos.h>
#include <persistos-headers.h>

void main(int argc, uint8_t* argv[]) {
    uint8_t *input_file = NULL;
    uint8_t *output_file = NULL;
    for (int argi = 1; argi < argc; argi++) {
        if (memcmp("if=", argv[argi], 3) == 0) {
            input_file = argv[argi] + 3;
        } else if (memcmp("of=", argv[argi], 3) == 0) {
            output_file = argv[argi] + 3;
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

    uint64_t block_size = 4096;
    uint8_t *buffer = malloc(block_size);
    while (true) {
        uint64_t bytes_read = read(in_fd, buffer, 4096);
        if (is_error(bytes_read)) {
            fputs(u8p("dd: error reading file\n"), stderr);
            exit(1);
        }
        if (bytes_read == 0) {
            break;
        }
        if(is_error(write(out_fd, buffer, bytes_read))) {
            fputs(u8p("dd: error writing file\n"), stderr);
            exit(1);
        }
    }
    exit(0);
}
