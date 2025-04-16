#include <stdint.h>
#include "cstd.h"
#include <persistos.h>

void main(int argc, char* argv[]) {
    size_t processed_argc = 1;
    uint64_t truncate_size = 0;
    if (argc <= processed_argc) {
        fputs("truncate: expected some arguments\n", stderr);
        exit(1);
    }
    if (strcmp(argv[processed_argc], "-s") == 0) {
        processed_argc++;
        if (argc <= processed_argc) {
            fputs("truncate: expected size argument\n", stderr);
            exit(1);
        }
        uint8_t parse_result = parse_n_dec(argv[processed_argc], 100, &truncate_size);
        if (parse_result != strlen(argv[processed_argc])) {
            fputs("truncate: parse error", stderr);
            exit(1);
        }
        processed_argc++;
    }
    if (argc <= processed_argc) {
        fputs("truncate: expected filepath argument\n", stderr);
        exit(1);
    }
    uint8_t *path = argv[processed_argc];

    ssize_t fd = open(path, 1);
    if (is_error(fd)) {
        fputs("Error opening file\n", stderr);
        exit(1);
    }
    if (is_error(ftruncate(fd, truncate_size))) {
        fputs("Error truncating file\n", stderr);
        exit(1);
    }
    exit(0);
}
