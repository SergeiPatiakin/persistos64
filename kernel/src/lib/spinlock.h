#ifndef SPINLOCK_H
#define SPINLOCK_H
#include "arch/if.h"

void spinlock_acquire(irq_state *state);
void spinlock_release(irq_state state);

#endif
