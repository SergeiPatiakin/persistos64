// Unmount a filesystem
#include <stdint.h>
#include "cstd.h"
#include <persistos.h>

void main(int argc, uint8_t* argv[]) {
    if (argc < 2) {
        fputs(u8p("umount: expected one argument\n"), stderr);
        exit(1);
    }
    if (is_error(umount(argv[1]))) {
        fputs(u8p("umount: error in umount syscall\n"), stderr);
        exit(1);
    };
    exit(0);
}
