#include <stdint.h>
#include "cstd.h"
#include <persistos.h>

void main(int argc, char* argv[]) {
    if (argc < 2) {
        fputs("kill: expected some arguments\n", stderr);
        exit(1);
    }
    uint64_t pid = 0;
    uint8_t parse_result = parse_n_dec(argv[1], 100, &pid);
    if (parse_result != strlen(argv[1])) {
        fputs("kill: parse error\n", stderr);
        exit(1);
    }

    ssize_t kill_result = kill(pid, 0);
    if (is_error(kill_result)) {
        fputs("kill: error\n", stderr);
        exit(1);
    }
    exit(0);
}
