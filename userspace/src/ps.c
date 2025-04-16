#include <stdint.h>
#include "cstd.h"
#include <persistos.h>

void main(int argc, char* argv[]) {
    char buf[4096];
    char pid_buf[12];
    ssize_t bytes_read = gettasks(buf, 4096);
    if (is_error(bytes_read)) {
        fputs("Error in gettasks\n", stderr);
        exit(1);
    }
    uint8_t *x = buf;
    while (x < buf + bytes_read) {
        uint32_t pid = *((uint32_t*)x);
        x += sizeof(uint32_t);
        
        uint16_t len = *((uint16_t*)x);
        x += sizeof(uint16_t);

        uint8_t *name = x;
        x += len + 1;
        
        sprintf_dec(pid, &pid_buf, ' ', 5);
        puts(pid_buf);
        puts(" ");
        puts(name);
        puts("\n");
    }
}
