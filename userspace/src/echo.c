#include <stdint.h>
#include <stdbool.h>
#include "cstd.h"
#include <persistos.h>

void main(int argc, char* argv[]) {
    bool trailing_newline = true;
    for (int i = 1; i < argc; i++) {
        if (i == 1 && strcmp(argv[i], "-n") == 0) {
            // Handle -n argument
            trailing_newline = false;
            continue;
        }
        puts(argv[i]);
        if (i < argc - 1) {
            puts(" ");
        }
    }
    if (trailing_newline) {
        puts("\n");
    }
    exit(0);
}
