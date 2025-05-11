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
        // TODO: investigate why this fails on real hardware:
        // fputs(u8p("mkdir: Error in mkdir\n"), stderr);
        write(2, u8p("mkdir: Error in mkdir\n"), 22);
        exit(2);
    }
    exit(0);
}
