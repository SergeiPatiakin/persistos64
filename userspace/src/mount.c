// Mount a filesystem
#include <stdint.h>
#include "cstd.h"
#include <persistos.h>

void main(int argc, uint8_t* argv[]) {
    if (argc < 4) {
        fputs(u8p("mount: expected three arguments\n"), stderr);
        exit(1);
    }

    uint8_t *device_path = (strcmp(argv[1], "none") == 0) ? NULL : argv[1];
    if (is_error(mount(device_path, argv[2], argv[3]))) {
        fputs(u8p("mount: error in mount syscall\n"), stderr);
        exit(1);
    };
    exit(0);
}
