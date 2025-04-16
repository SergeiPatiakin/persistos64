#include <stdint.h>
#include "cstd.h"
#include <persistos.h>

void main(int argc, char* argv[]) {
    if (argc < 2) {
        fputs("sleep: expected some arguments\n", stderr);
        exit(1);
    }
    uint64_t millis = 0;
    uint8_t parse_result = parse_n_dec(argv[1], 100, &millis);
    if (parse_result != strlen(argv[1])) {
        fputs("sleep: parse error\n", stderr);
        exit(1);
    }

    ssize_t sleep_result = sleep(millis);
    if (is_error(sleep_result)) {
        fputs("sleep: error\n", stderr);
        exit(1);
    }
    exit(0);
}
