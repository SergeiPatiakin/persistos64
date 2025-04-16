#include <stdint.h>
#include "cstd.h"
#include <persistos.h>

void main(int argc, char* argv[]) {
    if (argc < 2) {
        fputs("Expected a filepath argument\n", stderr);
        exit(1);
    }
    uint64_t x = mkdir(argv[1]);
    if (is_error(x)) {
        fputs("Error\n", stderr);
        exit(1);
    }
    exit(0);
}
