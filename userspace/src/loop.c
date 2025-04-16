#include <stdint.h>
#include <stdbool.h>
#include "cstd.h"
#include <persistos.h>

void main() {
    while (true) {
        sched_yield();
    }
}
