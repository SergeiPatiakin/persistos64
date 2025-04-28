#include <stdint.h>
#include <stdbool.h>
#include "cstd.h"
#include <persistos.h>

void main(int argc, char* argv[]) {
    if (argc < 2) {
        fputs("Expected a loop type argument\n", stderr);
        exit(1);
    }
    if (strcmp(argv[1], "busy") == 0) {
        while (true);
    } else if (strcmp(argv[1], "yield") == 0) {
        while (true) sched_yield();
    } else if (strcmp(argv[1], "sleep") == 0) {
        while (true) pause();
    } else {
        fputs("Unknown loop type\n", stderr);
        exit(1);
    }
}
