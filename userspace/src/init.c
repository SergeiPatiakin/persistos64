// Init process
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

void maybe_remount_root(char *kernel_cmdline_buf) {
    char *root_device_start = NULL;
    char *root_device_end = NULL;
    char *root_fstype_start = NULL;
    char *root_fstype_end = NULL;
    for (char *ptr = kernel_cmdline_buf; *ptr; ptr++) {
        if (strncmp(ptr, "root=", 5) == 0) {
            // Set root_device_start
            ptr += 5;
            root_device_start = ptr;
            // Truncate the string
            while (*ptr && *ptr != ' ' && *ptr != '\n') {
                ptr++;
            }
            root_device_end = ptr;
            continue;
        }
        if (strncmp(ptr, "rootfstype=", 11) == 0) {
            // Set root_fstype_start
            ptr += 11;
            root_fstype_start = ptr;
            // Truncate the string
            while (*ptr && *ptr != ' ' && *ptr != '\n') {
                ptr++;
            }
            root_fstype_end = ptr;
            continue;
        }
    }
    if (root_device_end) {
        *root_device_end = 0;
    }
    if (root_fstype_end) {
        *root_fstype_end = 0;
    }
    if (!root_fstype_start) {
        root_fstype_start = "exfat";
    }

    if (root_device_start) {
        if (is_error(mkdir("newroot"))) {
            exit(4);
        };
        for (int i=0;; i++) {
            // Wait up to 1 second in 100ms increments for device probing
            sleep(100);
            if (!is_error(mount(root_device_start, "newroot", root_fstype_start))) {
                break;
            }
            if (i>=10) {
                exit(5);
            }
        }

        if (is_error(mount(NULL, "newroot/sys", "sysfs"))) {
            exit(6);
        }
        if (is_error(switch_root("newroot"))) {
            exit(7);
        }
    }
}

void main() {
    if (is_error(mkdir("sys"))) {
        exit(1);
    }
    if (is_error(mount(NULL, "sys", "sysfs"))) {
        exit(2);
    }

    exec_loop();

    ssize_t kernel_cmdline_fd = open("sys/cmdline", 0);
    if (is_error(kernel_cmdline_fd)) {
        exit(8);
    }
    char kernel_cmdline_buf[100];
    ssize_t kernel_cmdline_readres = read(kernel_cmdline_fd, &kernel_cmdline_buf, 99);
    kernel_cmdline_buf[kernel_cmdline_readres] = 0;
    if (is_error(close(kernel_cmdline_fd))) {
        exit(9);
    }

    maybe_remount_root(kernel_cmdline_buf);

    // TODO: use openat to guarantee fd 0,1,2
    if (
        is_error(open(u8p("sys/dev/tty1"), 0)) ||
        is_error(open(u8p("sys/dev/tty1"), 0)) ||
        is_error(open(u8p("sys/dev/tty1"), 0))
    ) {
        exit(3);
    }

    puts(u8p("Persistos64 on tty1\n"));
    exec_shell();

    uint64_t fd_tty2 = open(u8p("sys/dev/tty2"), 0);
    dup2(fd_tty2, 0);
    dup2(fd_tty2, 1);
    dup2(fd_tty2, 2);
    close(fd_tty2);

    puts(u8p("Persistos64 on tty2\n"));
    exec_shell();

    uint64_t fd_tty3 = open(u8p("sys/dev/tty3"), 0);
    dup2(fd_tty3, 0);
    dup2(fd_tty3, 1);
    dup2(fd_tty3, 2);
    close(fd_tty3);

    puts(u8p("Persistos64 on tty3\n"));
    exec_shell();

    uint64_t fd_serial0 = open(u8p("sys/dev/serial0"), 0);
    if (!is_error(fd_serial0)) {
        dup2(fd_serial0, 0);
        dup2(fd_serial0, 1);
        dup2(fd_serial0, 2);
        close(fd_serial0);

        puts(u8p("Persistos64 on serial0\n"));
        exec_shell();
    }

    // TODO: wait for all shells
    pause();
}
