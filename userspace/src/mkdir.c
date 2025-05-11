#include <stdint.h>
#include "cstd.h"
#include <persistos.h>

void main(int argc, uint8_t* argv[]) {
    if (argc < 2) {
        fputs(u8p("mkdir: Expected a filepath argument\n"), stderr);
        exit(1);
    }
    uint64_t x = mkdir(argv[1]);
    if (is_error(x)) {
        fputs(u8p("mkdir: Error in mkdir\n"), stderr);
        exit(2);
    }
    exit(0);
}
