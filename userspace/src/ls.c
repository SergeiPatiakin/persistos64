#include <stdint.h>
#include "cstd.h"
#include <persistos.h>

void main(int argc, uint8_t* argv[]) {
    uint8_t *path;
    if (argc < 2) {
        path = u8p("/");
    } else {
        path = argv[1];
    }
    uint64_t fd = open(path, 0);
    if (is_error(fd)) {
        fputs(u8p("ls: Error opening\n"), stderr);
        exit(1);        
    }
    uint8_t buf[4096];
    ssize_t bytes_read = getdents(fd, buf, 4096);
    if (is_error(bytes_read)) {
        fputs(u8p("ls: Error in getdents\n"), stderr);
        exit(1);
    }
    uint8_t *x = buf;
    while (x < buf + bytes_read) {
        uint16_t len = *((uint16_t*)x);
        puts(x + sizeof(uint16_t));
        puts(u8p("\n"));
        x += len;
    }
    exit(0);
}
