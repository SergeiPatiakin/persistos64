#include <stdint.h>
#include "cstd.h"
#include <persistos.h>
#include <persistos-headers.h>

void main(int argc, uint8_t* argv[]) {
    if (argc < 2) {
        fputs(u8p("increment: Expected a filepath argument\n"), stderr);
        exit(1);
    }

    uint64_t fd = open(argv[1], O_CREAT);
    if (is_error(fd)) {
        fputs(u8p("increment: Error opening\n"), stderr);
        exit(1);
    }

    uint64_t binary_buf = 7; // Dummy, should be overwritten
    if (read(fd, (uint8_t*)&binary_buf, 8) != 8) {
        fputs(u8p("increment: Error reading\n"), stderr);
        exit(1);
    }
    binary_buf++;
    if (is_error(lseek(fd, 0, 0))) {
        fputs(u8p("increment: Error seeking\n"), stderr);
        exit(1);
    }
    if (write(fd, (uint8_t*)&binary_buf, 8) != 8) {
        fputs(u8p("increment: Error writing\n"), stderr);
    }

    uint8_t text_buf[17];
    sprintf_uint64(binary_buf, text_buf);
    puts(text_buf);
    puts(u8p("\n"));
    exit(0);
}
