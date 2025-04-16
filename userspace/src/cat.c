#include <stdint.h>
#include "cstd.h"
#include <persistos.h>

void main(int argc, char* argv[]) {
    uint8_t buf[4096];
    if (argc < 2) {
        fputs("cat: expected a filepath argument\n", stderr);
        exit(1);
    }
    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        uint64_t fd = open(argv[i], 0);
        if (is_error(fd)) {
            fputs("cat: error opening ", stderr);
            fputs(arg, stderr);
            fputs("\n", stderr);
            exit(1);
        }
        while (true) {
            uint64_t bytes_read = read(fd, buf, 4096);
            if (is_error(bytes_read)) {
                fputs("cat: error reading file\n", stderr);
                exit(1);
            }
            if (bytes_read == 0) {
                break;
            }
            write(1, buf, bytes_read);
        }
        close(fd);
    }
    exit(0);
}
