#include <stdint.h>
#include "cstd.h"
#include <persistos.h>

void main(int argc, char* argv[]) {
    if (argc < 2) {
        fputs("Expected a filepath argument\n", stderr);
        exit(1);
    }

    uint64_t fd = open(argv[1], O_CREAT);
    if (is_error(fd)) {
        fputs("Error opening\n", stderr);
        exit(1);
    }

    uint64_t binary_buf = 7; // Dummy, should be overwritten
    if (read(fd, &binary_buf, 8) != 8) {
        fputs("Error reading\n", stderr);
        exit(1);
    }
    binary_buf++;
    if (is_error(lseek(fd, 0, 0))) {
        fputs("Error seeking\n", stderr);
        exit(1);
    }
    if (write(fd, &binary_buf, 8) != 8) {
        fputs("Error writing\n", stderr);
    }

    uint8_t text_buf[17];
    sprintf_uint64(binary_buf, &text_buf);
    puts(text_buf);
    puts("\n");
    exit(0);
}
