#include <stdint.h>
#include "cstd.h"
#include <persistos.h>

void main(int argc, char* argv[]) {
    if (argc < 2) {
        fputs(u8p("Expected a filepath argument\n"), stderr);
        exit(1);
    }
    uint64_t x = mkdir(u8p(argv[1]));
    if (is_error(x)) {
        fputs(u8p("Error\n"), stderr);
        exit(1);
    }
    exit(0);
}
