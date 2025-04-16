#include <stdint.h>
#include "cstd.h"
#include <persistos.h>

void main(int argc, char* argv[]) {
    if (argc < 4) {
        fputs("mount: expected three arguments\n", stderr);
        exit(1);
    }
    mount(argv[1], argv[2], argv[3]);
    exit(0);
}
