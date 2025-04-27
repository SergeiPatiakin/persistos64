#include "arch/asm.h"
#include "spinlock.h"

void spinlock_acquire(irq_state *state) {
    irq_disable(*state);
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
}

void spinlock_release(irq_state state) {
    __atomic_thread_fence(__ATOMIC_RELEASE);
    irq_restore(state);
}
