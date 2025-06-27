// Create an empty file
#include <stdint.h>
#include "cstd.h"
#include <persistos.h>

void main(int argc, uint8_t* argv[]) {
    if (argc < 2) {
        fputs(u8p("touch: Expected a filepath argument\n"), stderr);
        exit(1);
    }
    uint64_t x = open(argv[1], 1);
    if ((int64_t)x < 0) {
        fputs(u8p("touch: Error opening file\n"), stderr);
        exit(1);
    }
    exit(0);
}
