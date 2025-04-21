#include "arch/asm.h"

// On a uniprocessor system, no spinning is needed in a spinlock
#define irq_disable(state) \
if (are_interrupts_enabled()) { state = 1; asm volatile ("cli"); } else { state = 0; }

#define irq_restore(state) \
if (state == 1) { asm volatile ("sti"); }
