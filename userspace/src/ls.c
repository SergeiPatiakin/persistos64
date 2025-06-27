// List contents of a directory
#include <stdint.h>
#include "cstd.h"
#include <persistos.h>
#include <persistos-headers.h>

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
    uint8_t *buf_cursor = buf;
    while (buf_cursor < buf + bytes_read) {
        struct dent_header *header = (struct dent_header*)buf_cursor;
        puts(buf_cursor + sizeof(struct dent_header));
        puts(u8p("\n"));
        buf_cursor += header->len;
    }
    exit(0);
}
