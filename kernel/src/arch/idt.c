#include "lib/cstd.h"
#include "arch/asm.h"
#include "arch/idt.h"
#include "arch/pic.h"
#include "drivers/keyboard.h"
#include "drivers/pit.h"
#include "drivers/nvme.h"
#include "drivers/tty.h"
#include "kernel/scheduler.h"
#include "kernel/syscall.h"

#define KERNEL_CODE_GDT_ENTRY_IDX 5 // Based on Limine boot protocol

typedef struct {
	uint16_t    isr_low;      // The lower 16 bits of the ISR's address
	uint16_t    kernel_cs;    // The GDT segment selector that the CPU will load into CS before calling the ISR
	uint8_t	    ist;          // The IST in the TSS that the CPU will load into RSP; set to zero for now
	uint8_t     attributes;   // Type and attributes; see the IDT page
	uint16_t    isr_mid;      // The higher 16 bits of the lower 32 bits of the ISR's address
    uint32_t    isr_high;
    uint32_t    reserved;
} __attribute__((packed)) idt_entry_t;

__attribute__((aligned(0x20)))
static idt_entry_t idt[256]; // Create an array of IDT entries; aligned for performance

typedef struct {
	uint16_t	limit;
	uint64_t	base;
} __attribute__((packed)) idtr_t;
static idtr_t idtr;

void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags) {
    idt_entry_t* descriptor = &idt[vector];
    descriptor->isr_low        = (uint64_t)isr & 0xFFFF;
    descriptor->kernel_cs      = 8 * KERNEL_CODE_GDT_ENTRY_IDX;
    descriptor->attributes     = flags;
    descriptor->isr_mid        = (uint64_t)isr >> 16;
    descriptor->isr_high       = (uint64_t)isr >> 32;
    descriptor->reserved       = 0;
}

void cpu_exception_handler(
    uint64_t interrupt_number,
    uint64_t error_code,
    uint64_t interrupt_rsp,
    uint64_t arg1
) {
    if (interrupt_number == 0) {
        printk(u8p("Division by zero\n"));
        current_task_ts->task_state = TS_ZOMBIE;
        current_task_ts->exit_code = 1; // TODO
        task_yield();
    } else if (interrupt_number == 8) {
        panic(u8p("Double fault\n"));
    } else if (interrupt_number == 13) {
        printk(u8p("General protection fault\n"));
        current_task_ts->task_state = TS_ZOMBIE;
        current_task_ts->exit_code = 128 + 11; // 11 is SIGSEGV
        task_yield();
    } else if (interrupt_number == 14) {
        uint8_t access_address_buf[17];
        sprintf_uint64(arg1, access_address_buf);
        
        uint8_t instruction_address_buf[17];
        uint64_t fault_rip = *((uint64_t*)interrupt_rsp + 10);
        sprintf_uint64(fault_rip, instruction_address_buf);

        printk(u8p("The instruction at 0x"));
        printk(instruction_address_buf);
        printk(u8p(" referenced memory at 0x"));
        printk(access_address_buf);
        printk(u8p(". The memory could not be "));
        printk(error_code & 0x2 ? u8p("written.") : u8p("read."));
        printk(u8p("\n"));
        current_task_ts->task_state = TS_ZOMBIE;
        current_task_ts->exit_code = 1; // TODO
        task_yield();
    } else {
        panic(u8p("Unknown CPU exception"));
    }
}

uint64_t timer_ticks = 0;
uint64_t hw_interrupt_handler(
    uint32_t interrupt_number,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5
) {
    uint8_t interrupt_line = interrupt_number - 0x20;
    if (interrupt_line == 0x7) {
        // Spurious interrupt. Return without sending EOI
    } else if (interrupt_line == 0x0) {
        timer_ticks++;
        pic_send_eoi(interrupt_line);
    } else if (interrupt_line == 0x1) {
        // Keyboard interrupt
        keyboard_rb_fill();
        pic_send_eoi(interrupt_line);
    } else if (interrupt_line == 0x9 || interrupt_line == 0xA || interrupt_line == 0xB) {
        // Hardware interrupt
        // Check NVME devices
        nvme_handle_interrupt(interrupt_line);
        pic_send_eoi(interrupt_line);
    } else if (interrupt_line == 0x60) { // software int 0x80
        return handle_syscall(arg1, arg2, arg3, arg4, arg5);
    } else {
        panic(u8p("Unknown interrupt"));
    }
    return 0; // Unknown interrupt
}

void idt_init() {
    idtr.base = (uintptr_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(idt_entry_t) * 256 - 1;

    // TODO: more exceptions and interrupts
    idt_set_descriptor(0, handle_exception_0, 0x8F); // Division by zero
    idt_set_descriptor(8, handle_exception_8, 0x8F); // Double Fault
    idt_set_descriptor(13, handle_exception_13, 0x8F); // General Protection Fault
    idt_set_descriptor(14, handle_exception_14, 0x8F); // Page Fault

    idt_set_descriptor(32, handle_interrupt_32, 0x8E); // Timer
    idt_set_descriptor(33, handle_interrupt_33, 0x8E); // Keyboard
    idt_set_descriptor(41, handle_interrupt_41, 0x8E); // PCI
    idt_set_descriptor(42, handle_interrupt_42, 0x8E); // PCI
    idt_set_descriptor(43, handle_interrupt_43, 0x8E); // PCI


    idt_set_descriptor(128, handle_interrupt_128, 0xEE); // Software interrupt

    asm volatile ("lidt %0" : : "m"(idtr)); // load the new IDT
    pic_remap(); // Remap PIC
    set_pit_channel_0(TIMER_TICKS_PER_SECOND); // Initialize PIT channel 0 to tick at 100Hz

    asm volatile ("sti"); // set the interrupt flag
}
