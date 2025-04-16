#include <stdint.h>
#include <stdbool.h>
#include "cstd.h"
#include <persistos.h>

void main(int argc, char* argv[]) {
    if (argc < 2) {
        fputs("Expected a fault type argument\n", stderr);
        exit(1);
    }
    uint64_t x = 8;
    if (strcmp(argv[1], u8p("div-zero")) == 0) {
        x /= 0;
    } else if (strcmp(argv[1], u8p("priv-instr")) == 0) {
        asm("cli");
    } else if (strcmp(argv[1], u8p("bad-mem-read")) == 0) {
        uint64_t *bad_ptr = 0x900000;
        puts(*bad_ptr);
    } else if (strcmp(argv[1], u8p("bad-mem-write")) == 0) {
        uint64_t *bad_ptr = 0x900000;
        *bad_ptr = 7;
    } else {
        fputs("Unknown fault type. Not faulting\n", stderr);
        exit(1);
    }
}
