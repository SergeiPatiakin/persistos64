#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// idt.s
void handle_exception_0(void);
void handle_exception_8(void);
void handle_exception_13(void);
void handle_exception_14(void);

void handle_interrupt_32(void);
void handle_interrupt_33(void);
void handle_interrupt_41(void);
void handle_interrupt_42(void);
void handle_interrupt_43(void);

void handle_interrupt_128(void);

#define IDT_SYSCALL_NUM_SAVED_REGISTERS 14

void idt_init();
void zero_rax_and_iret();

// idt.c

extern uint64_t timer_ticks;
#define TIMER_TICKS_PER_SECOND 100

#endif
