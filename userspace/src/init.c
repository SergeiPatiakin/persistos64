#include <stdint.h>
#include <stdbool.h>
#include "cstd.h"
#include <persistos.h>

void exec_loop() {
    uint32_t loop_pid = fork();
    if (loop_pid == 0) {
        uint8_t *loop_argv[] = { u8p("bin/loop"), u8p("yield"), NULL };
        if (is_error(exec(loop_argv[0], loop_argv))) {
            fputs(u8p("init: error starting loop\n"), stderr);
            return;
        }
    }
}

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
    open(u8p("dev/tty1"), 0);
    open(u8p("dev/tty1"), 0);
    open(u8p("dev/tty1"), 0);
    exec_loop();

    puts(u8p("Persistos64 on tty1\n"));
    exec_shell();

    uint64_t fd_tty2 = open(u8p("dev/tty2"), 0);
    dup2(fd_tty2, 0);
    dup2(fd_tty2, 1);
    dup2(fd_tty2, 2);
    close(fd_tty2);

    puts(u8p("Persistos64 on tty2\n"));
    exec_shell();

    uint64_t fd_tty3 = open(u8p("dev/tty3"), 0);
    dup2(fd_tty3, 0);
    dup2(fd_tty3, 1);
    dup2(fd_tty3, 2);
    close(fd_tty3);

    puts(u8p("Persistos64 on tty3\n"));
    exec_shell();

    // TODO: wait for all shells
    pause();
}
