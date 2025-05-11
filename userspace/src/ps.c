#include <stdint.h>
#include "cstd.h"
#include <persistos.h>
#include <persistos-headers.h>

void main(int argc, uint8_t* argv[]) {
    (void) argc;
    (void) argv; 
    uint8_t buf[4096];
    uint8_t pid_buf[12];
    ssize_t bytes_read = gettasks(buf, 4096);
    if (is_error(bytes_read)) {
        fputs(u8p("ps: Error in gettasks\n"), stderr);
        exit(1);
    }
    uint8_t *x = buf;
    while (x < buf + bytes_read) {
        uint32_t pid = *((uint32_t*)x);
        x += sizeof(uint32_t);
        
        uint8_t state = *((uint8_t*)x);
        x += sizeof(uint8_t);

        uint16_t len = *((uint16_t*)x);
        x += sizeof(uint16_t);

        uint8_t *name = x;
        x += len + 1;
        
        sprintf_dec(pid, pid_buf, ' ', 5);
        puts(pid_buf);
        puts(u8p(" "));
        puts(
            state == TS_RUNNING ? u8p("r") :
            state == TS_WAITING ? u8p("w") :
            state == TS_ZOMBIE ? u8p("z") :
            u8p(" ")
        );
        puts(u8p(" "));
        puts(name);
        puts(u8p("\n"));
    }
}
