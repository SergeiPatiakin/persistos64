#ifndef SERIAL_H
#define SERIAL_H

#include "fs/vfs.h"

#define SERIAL_IO_PORT 0x3f8          // COM1

struct serial_device {
    uint32_t device_number;
};
extern struct serial_device serial0;

int serial_init();
void write_serial(char a);
char read_serial();
extern struct device_operations serial_device_ops;

#endif
