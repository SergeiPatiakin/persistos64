// Change the size of a file
#include <stdint.h>
#include "cstd.h"
#include <persistos.h>

void main(int argc, uint8_t* argv[]) {
    int processed_argc = 1;
    uint64_t truncate_size = 0;
    if (argc <= processed_argc) {
        fputs(u8p("truncate: expected some arguments\n"), stderr);
        exit(1);
    }
    if (strcmp(argv[processed_argc], u8p("-s")) == 0) {
        processed_argc++;
        if (argc <= processed_argc) {
            fputs(u8p("truncate: expected size argument\n"), stderr);
            exit(1);
        }
        uint8_t parse_result = parse_n_dec(argv[processed_argc], 100, &truncate_size);
        if (parse_result != strlen(argv[processed_argc])) {
            fputs(u8p("truncate: parse error"), stderr);
            exit(1);
        }
        processed_argc++;
    }
    if (argc <= processed_argc) {
        fputs(u8p("truncate: expected filepath argument\n"), stderr);
        exit(1);
    }
    uint8_t *path = argv[processed_argc];
    processed_argc++;
    if (argc > processed_argc) {
        fputs(u8p("truncate: unexpected argument\n"), stderr);
        exit(1);
    }

    ssize_t fd = open(path, 1);
    if (is_error(fd)) {
        fputs(u8p("truncate: Error opening file\n"), stderr);
        exit(1);
    }
    if (is_error(ftruncate(fd, truncate_size))) {
        fputs(u8p("truncate: Error truncating file\n"), stderr);
        exit(1);
    }
    exit(0);
}
