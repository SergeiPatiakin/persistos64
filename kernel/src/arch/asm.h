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
void enable_interrupts(void);
void disable_interrupts(void);
uint64_t read_rflags();

uint64_t read_cr3();

#define EFLAGS_IF (1 << 9)

bool are_interrupts_enabled();

typedef uint8_t irq_state;

// On a uniprocessor system, no spinning is needed in a spinlock
#define irq_disable(state) \
if (are_interrupts_enabled()) { state = 1; disable_interrupts(); } else { state = 0; }

#define irq_enable(state) \
if (are_interrupts_enabled()) { state = 1; } else { state = 0; enable_interrupts(); }

#define irq_restore(state) \
if (state == 1) { enable_interrupts(); } else { disable_interrupts(); }

#endif
