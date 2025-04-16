#include <stdint.h>
#include <stdbool.h>
#include "cstd.h"
#include <persistos.h>

void exec_shell() {
    uint32_t shell_pid = fork();
    if (shell_pid == 0) {
        uint8_t *shell_argv[] = { u8p("bin/shell"), NULL };
        if (is_error(exec(shell_argv[0], shell_argv))) {
            fputs(u8p("init: error starting shell\n"), stderr);
            return;
        }
    }
}

void main() {
    open("dev/tty1", 0);
    open("dev/tty1", 0);
    open("dev/tty1", 0);
    puts(u8p("Persistos64 on tty1\n"));
    exec_shell();

    uint64_t fd_tty2 = open("dev/tty2", 0);
    dup2(fd_tty2, 0);
    dup2(fd_tty2, 1);
    dup2(fd_tty2, 2);
    close(fd_tty2);

    puts(u8p("Persistos64 on tty2\n"));
    exec_shell();

    uint64_t fd_tty3 = open("dev/tty3", 0);
    dup2(fd_tty3, 0);
    dup2(fd_tty3, 1);
    dup2(fd_tty3, 2);
    close(fd_tty3);

    puts(u8p("Persistos64 on tty3\n"));
    exec_shell();

    // TODO: wait for all shells
    while (true) {
        sched_yield();
    }
}
