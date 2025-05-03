// Synchronization helpers
#include "arch/asm.h"
#include "lib/sync.h"
#include "kernel/scheduler.h"

void spinlock_acquire(irq_state *state) {
    irq_disable(*state);
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
}

void spinlock_release(irq_state state) {
    __atomic_thread_fence(__ATOMIC_RELEASE);
    irq_restore(state);
}

// Initialize waitqueue
void init_wq(struct wait_queue *wq) {
    init_list(&wq->waiters_lh);
}

// Awake waitqueue
void awake_wq(struct wait_queue *wq) {
    struct list_head *wp;
    struct list_head w;
    // Awake all waiters
    for (
        wp = wq->waiters_lh.next, w = *wp;
        wp != &wq->waiters_lh;
        wp = w.next, w = *wp
    ) {
        list_del(wp);
        wp->next = wp;
        wp->prev = wp;
        struct task_struct *waiting_process = container_of(wp, struct task_struct, wait_queue_le);
        waiting_process->task_state = TS_RUNNING;
    }
}

// Prepare to wait on a wait queue. Caller will need to call task_yield after this function returns
void prep_await_wq(struct wait_queue *wq) {
    list_add_tail(&current_task_ts->wait_queue_le, &wq->waiters_lh);
    current_task_ts->task_state = TS_WAITING;
}
