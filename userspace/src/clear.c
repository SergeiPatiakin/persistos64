#include <stdint.h>
#include "cstd.h"
#include <persistos.h>

void main() {
    puts(u8p("\x1B[H\x1B[J"));
    exit(0);
}
