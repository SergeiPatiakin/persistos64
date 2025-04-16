#include <stdint.h>
#include "cstd.h"
#include <persistos.h>

void main(int argc, char* argv[]) {
    uint8_t a_buf[4096];
    uint8_t b_buf[4096];
    if (argc != 3) {
        fputs("diff: expected two arguments\n", stderr);
        exit(1);
    }

    char *a_path = argv[1];
    uint64_t a_fd = open(a_path, 0);
    if (is_error(a_fd)) {
        fputs("diff: error opening ", stderr);
        fputs(a_path, stderr);
        fputs("\n", stderr);
        exit(1);
    }

    char *b_path = argv[2];
    uint64_t b_fd = open(b_path, 0);
    if (is_error(b_fd)) {
        fputs("diff: error opening ", stderr);
        fputs(b_path, stderr);
        fputs("\n", stderr);
        exit(1);
    }

    while (true) {
        uint64_t a_bytes_read = read(a_fd, a_buf, 4096);
        if (is_error(a_bytes_read)) {
            fputs("diff: error reading file a\n", stderr);
            exit(1);
        }
        uint64_t b_bytes_read = read(b_fd, b_buf, 4096);
        if (is_error(b_bytes_read)) {
            fputs("diff: error reading file b\n", stderr);
            exit(1);
        }
        if (a_bytes_read != b_bytes_read) {
            puts("Files differ, they have different lengths\n");
            exit(2);
        }
        if (memcmp(&a_buf, &b_buf, a_bytes_read) != 0) {
            puts("Files differ\n");
            exit(2);
        }
        exit(0);
    }
}
