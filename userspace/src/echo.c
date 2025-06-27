// Write text to stdout
#include <stdint.h>
#include <stdbool.h>
#include "cstd.h"
#include <persistos.h>

void main(int argc, uint8_t* argv[]) {
    bool trailing_newline = true;
    for (int i = 1; i < argc; i++) {
        if (i == 1 && strcmp(argv[i], u8p("-n")) == 0) {
            // Handle -n argument
            trailing_newline = false;
            continue;
        }
        puts(argv[i]);
        if (i < argc - 1) {
            puts(u8p(" "));
        }
    }
    if (trailing_newline) {
        puts(u8p("\n"));
    }
    exit(0);
}
