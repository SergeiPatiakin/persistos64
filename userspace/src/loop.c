#include <stdint.h>
#include <stdbool.h>
#include "cstd.h"
#include <persistos.h>

void main(int argc, char* argv[]) {
    if (argc < 2) {
        fputs(u8p("loop: Expected a loop type argument\n"), stderr);
        exit(1);
    }
    if (strcmp(u8p(argv[1]), u8p("busy")) == 0) {
        while (true);
    } else if (strcmp(u8p(argv[1]), u8p("yield")) == 0) {
        while (true) sched_yield();
    } else if (strcmp(u8p(argv[1]), u8p("sleep")) == 0) {
        while (true) pause();
    } else {
        fputs(u8p("loop: Unknown loop type\n"), stderr);
        exit(1);
    }
}
