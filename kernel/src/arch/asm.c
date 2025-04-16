// Short assembly sequences and related helpers
#include <stdint.h>
#include <stdbool.h>
#include "arch/asm.h"

void outb(uint16_t port, uint8_t value) {
	//("a" puts value in eax, "dN" puts port in edx or uses 1-byte constant.)
	asm volatile ("outb %0, %1" :: "a" (value), "dN" (port));
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

void outl(uint16_t port, uint32_t value) {
	//("a" puts value in eax, "dN" puts port in edx or uses 1-byte constant.)
	asm volatile ("outl %0, %1" :: "a" (value), "dN" (port));
}

uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

void io_wait(void) {
    outb(0x80, 0);
}

void halt_forever(void) {
    asm volatile ("cli; hlt");
}

void halt_until_any_interrupt(void) {
    asm volatile ("sti; hlt");
}

uint64_t read_rflags() {
    uint64_t rflags;
    asm volatile (
        "pushfq\n"        // Push RFLAGS onto the stack
        "pop %0\n"        // Pop into the variable
        : "=r"(rflags)     // Output operand: store the result in 'flags'
    );
    return rflags;
}

uint64_t read_cr3() {
    uint64_t cr3;
    asm volatile ("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

bool are_interrupts_enabled() {
    return read_rflags() & EFLAGS_IF;
}
