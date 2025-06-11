#include "serial.h"
#include "arch/asm.h"
#include "drivers/tty.h"
#include "drivers/device-numbers.h"
#include "kernel/scheduler.h"

struct serial_device serial0 = {.device_number = 0};

int serial_init() {
    outb(SERIAL_IO_PORT + 1, 0x00);    // Disable all interrupts
    outb(SERIAL_IO_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(SERIAL_IO_PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(SERIAL_IO_PORT + 1, 0x00);    //                  (hi byte)
    outb(SERIAL_IO_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(SERIAL_IO_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(SERIAL_IO_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
    outb(SERIAL_IO_PORT + 4, 0x1E);    // Set in loopback mode, test the serial chip
    outb(SERIAL_IO_PORT + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)

    // Check if serial is faulty (i.e: not same byte as sent)
    if(inb(SERIAL_IO_PORT + 0) != 0xAE) {
        printk("Serial is faulty\n");
        return 1;
    }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    outb(SERIAL_IO_PORT + 4, 0x0F);
    printk("Serial works\n");
    serial0.device_number = 1;
    vfs_mknod(
        vfs_dev_dir_inode,
        u8p("serial0"),
        DEVICE_SERIAL,
        serial0.device_number,
        &serial_device_ops,
        &serial0
    );
    return 0;
}

int is_transmit_empty() {
    return inb(SERIAL_IO_PORT + 5) & 0x20;
}
 
void write_serial(char a) {
    while (is_transmit_empty() == 0);
 
    outb(SERIAL_IO_PORT, a);
}

int serial_received() {
    return inb(SERIAL_IO_PORT + 5) & 1;
}
 
char read_serial() {
    while (serial_received() == 0);

    return inb(SERIAL_IO_PORT);
}

ssize_t serial_write(void *dev, uint8_t *buffer, uint64_t offset, size_t length) {
    (void) offset; // Unused
    size_t bytes_written = 0;
    for (size_t i = 0; i < length; i++) {
        while (is_transmit_empty() == 0);
        outb(SERIAL_IO_PORT, buffer[i]);
        bytes_written++;
    }
    return bytes_written;
}

ssize_t serial_read(void *dev, uint8_t *buffer, uint64_t offset, size_t length) {
    (void) offset; // Unused
    while (serial_received() == 0) {
        task_yield();
    }
    uint8_t byte = inb(SERIAL_IO_PORT);
    
    // Newlines are sent as \r over a serial port
    if (byte == '\r') {
        buffer[0] = '\n';
    } else {
        buffer[0] = byte;
    }
    
    return 1;
}

struct device_operations serial_device_ops = {
    .read = serial_read,
    .write = serial_write
};
