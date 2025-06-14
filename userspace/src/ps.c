#include <stdint.h>
#include "cstd.h"
#include <persistos.h>
#include <persistos-headers.h>

int main(int argc, uint8_t* argv[]) {
    (void) argc;
    (void) argv; 
    uint8_t buf[4096];
    uint8_t pid_buf[12];
    ssize_t bytes_read = gettents(buf, 4096);
    if (is_error(bytes_read)) {
        fputs(u8p("ps: Error in gettents\n"), stderr);
        exit(1);
    }
    uint8_t *buf_cursor = buf;
    while (buf_cursor < buf + bytes_read) {
        struct tent_header *tent = (struct tent_header*)buf_cursor;
        uint8_t *name = buf_cursor + sizeof(struct tent_header);
        buf_cursor += sizeof(struct tent_header) + tent->len + 1;
        
        sprintf_dec(tent->pid, pid_buf, ' ', 5);
        puts(pid_buf);
        puts(u8p(" "));
        puts(
            tent->state == TS_RUNNING ? u8p("r") :
            tent->state == TS_WAITING ? u8p("w") :
            tent->state == TS_ZOMBIE ? u8p("z") :
            u8p(" ")
        );
        puts(u8p(" "));
        puts(name);
        puts(u8p("\n"));
    }
    return 0;
}
