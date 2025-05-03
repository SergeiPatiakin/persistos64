#include <stdint.h>
#include <stdbool.h>
#include "cstd.h"
#include <persistos.h>

void main(int argc, uint8_t* argv[]) {
    if (argc < 2) {
        fputs(u8p("fault: Expected a fault type argument\n"), stderr);
        exit(1);
    }
    uint64_t x = 8;
    if (strcmp(argv[1], u8p("div-zero")) == 0) {
        uint64_t y = 0;
        x /= y;
    } else if (strcmp(argv[1], u8p("priv-instr")) == 0) {
        asm("cli");
    } else if (strcmp(argv[1], u8p("bad-read")) == 0) {
        uint64_t *bad_ptr = (uint64_t*)0x900000;
        puts(u8p(*bad_ptr));
    } else if (strcmp(argv[1], u8p("bad-write")) == 0) {
        uint64_t *bad_ptr = (uint64_t*)0x900000;
        *bad_ptr = 7;
    } else {
        fputs(u8p("fault: Unknown fault type. Not faulting\n"), stderr);
        exit(1);
    }
}
