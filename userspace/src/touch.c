#include <stdint.h>
#include "cstd.h"
#include <persistos.h>

void main(int argc, char* argv[]) {
    if (argc < 2) {
        fputs(u8p("Expected a filepath argument\n"), stderr);
        exit(1);
    }
    uint64_t x = open(u8p(argv[1]), 1);
    if ((int64_t)x < 0) {
        fputs(u8p("Error opening file\n"), stderr);
        exit(1);
    }
    exit(0);
}
