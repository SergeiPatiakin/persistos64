#ifndef OUTB_H
#define OUTB_H
#include <stdint.h>
#include <stdbool.h>

void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);

void outl(uint16_t port, uint32_t value);
uint32_t inl(uint16_t port);

void io_wait(void);

void halt_forever(void);
void halt_until_any_interrupt(void);
uint64_t read_rflags();

uint64_t read_cr3();

#define EFLAGS_IF (1 << 9)

bool are_interrupts_enabled();

#endif
