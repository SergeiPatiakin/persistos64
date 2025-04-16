#include "arch/asm.h"

// On a uniprocessor system, no spinning is needed in a spinlock
#define spin_lock_irqsave(_spinlock, flags) \
if (are_interrupts_enabled()) { flags = 1; asm volatile ("cli"); } else { flags = 0; }

#define spin_lock_irqrestore(_spinlock, flags) \
if (flags == 1) { asm volatile ("sti"); }
