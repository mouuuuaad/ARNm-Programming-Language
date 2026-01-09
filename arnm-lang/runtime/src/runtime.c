/*
 * ARNm Runtime - Main Runtime Implementation
 */

#include "../include/arnm.h"
#include "../include/process.h"
#include "../include/scheduler.h"
#include "../include/mailbox.h"
#include "../include/memory.h"
#include <stdlib.h>
#include <stdio.h>

/* ============================================================
 * Runtime Lifecycle
 * ============================================================ */

int arnm_init(int num_workers) {
    return sched_init(num_workers > 0 ? num_workers : 0);
}

void arnm_shutdown(void) {
    sched_shutdown();
}

void arnm_run(void) {
    sched_run();
}

/* ============================================================
 * Process API
 * ============================================================ */

ArnmProcess* arnm_spawn(void (*entry)(void*), void* arg, size_t state_size) {
    ArnmProcess* proc = proc_create(entry, arg, ARNM_DEFAULT_STACK_SIZE, state_size);
    if (proc) {
        sched_enqueue(proc);
    }
    return proc;
}

ArnmProcess* arnm_self(void) {
    return proc_current();
}

uint64_t arnm_pid(ArnmProcess* proc) {
    return proc ? proc->pid : 0;
}

void arnm_yield(void) {
    arnm_sched_yield();
}

void arnm_exit(void) {
    ArnmProcess* proc = proc_current();
    if (proc) {
        proc_exit(proc);
        arnm_sched_yield();
    }
}

/* ============================================================
 * Message Passing API
 * ============================================================ */

int arnm_send(ArnmProcess* target, uint64_t tag, void* data, size_t size) {
    if (!target || !target->mailbox) return -1;
    return mailbox_send(target->mailbox, tag, data, size) ? 0 : -1;
}

ArnmMessage* arnm_receive(void) {
    ArnmProcess* proc = proc_current();
    if (!proc) return NULL;
    if (!proc->mailbox) return NULL;
    return mailbox_receive(proc->mailbox);
}

ArnmMessage* arnm_try_receive(void) {
    ArnmProcess* proc = proc_current();
    if (!proc || !proc->mailbox) return NULL;
    return mailbox_try_receive(proc->mailbox);
}

void arnm_message_free(ArnmMessage* msg) {
    message_free(msg);
}

uint64_t arnm_message_tag(ArnmMessage* msg) {
    return msg ? msg->tag : 0;
}

void* arnm_message_data(ArnmMessage* msg) {
    return msg ? msg->data : NULL;
}

size_t arnm_message_size(ArnmMessage* msg) {
    return msg ? msg->size : 0;
}

void arnm_panic_nomatch(void) {
    fprintf(stderr, "[ARNM PANIC] Unmatched message in receive block\n");
    abort();
}

/* ============================================================
 * Memory API
 * ============================================================ */

void* arnm_alloc(size_t size, ArnmDestructor dtor) {
    return arnm_arc_alloc(size, dtor);
}

void arnm_retain(void* obj) {
    arnm_arc_retain(obj);
}

void arnm_release(void* obj) {
    arnm_arc_release(obj);
}

uint32_t arnm_refcount(void* obj) {
    return arnm_arc_refcount(obj);
}
