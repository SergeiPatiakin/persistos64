#ifndef SYNC_H
#define SYNC_H
#include "arch/asm.h"
#include "lib/list.h"

void spinlock_acquire(irq_state *state);
void spinlock_release(irq_state state);

struct wait_queue {
    struct list_head waiters_lh;
};

void init_wq(struct wait_queue *wq);
void awake_wq(struct wait_queue *wq);
void prep_await_wq(struct wait_queue *wq);

#endif
