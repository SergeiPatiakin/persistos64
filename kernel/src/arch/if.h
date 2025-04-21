#include "arch/asm.h"

// On a uniprocessor system, no spinning is needed in a spinlock
#define irq_disable(state) \
if (are_interrupts_enabled()) { state = 1; disable_interrupts(); } else { state = 0; }

#define irq_enable(state) \
if (are_interrupts_enabled()) { state = 1 } else { state = 0; enable_interrupts(); }

#define irq_restore(state) \
if (state == 1) { enable_interrupts(); } else { disable_interrupts(); }
