/*
 * ARNm Runtime - Process Implementation
 */

#include "../include/process.h"
#include "../include/mailbox.h"
#include "../include/memory.h"
#include "../include/scheduler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdatomic.h>

/* ============================================================
 * Process ID Counter
 * ============================================================ */

static atomic_uint_fast64_t next_pid = 1;

uint64_t proc_next_pid(void) {
    return atomic_fetch_add(&next_pid, 1);
}

/* ============================================================
 * Thread-Local Current Process
 * ============================================================ */

static _Thread_local ArnmProcess* current_process = NULL;

ArnmProcess* proc_current(void) {
    return current_process;
}

void proc_set_current(ArnmProcess* proc) {
    current_process = proc;
}

/* ============================================================
 * Process Lifecycle
 * ============================================================ */

ArnmProcess* proc_create(void (*entry)(void*), void* arg, size_t stack_size, size_t state_size) {
    ArnmProcess* proc = (ArnmProcess*)malloc(sizeof(ArnmProcess));
    if (!proc) return NULL;
    
    memset(proc, 0, sizeof(ArnmProcess));
    
    /* Allocate actor state if requested */
    if (state_size > 0) {
        proc->actor_state = malloc(state_size);
        if (!proc->actor_state) {
            free(proc);
            return NULL;
        }
        memset(proc->actor_state, 0, state_size);
    }
    
    /* Assign PID */
    proc->pid = proc_next_pid();
    proc->state = PROC_STATE_READY;
    
    /* Allocate stack */
    proc->stack_size = stack_size;
    proc->stack_base = stack_alloc(stack_size);
    if (!proc->stack_base) {
        free(proc);
        return NULL;
    }
    
    /* Stack grows down, so top is base + size */
    void* stack_top = (char*)proc->stack_base + stack_size;
    
    /* Initialize context */
    arnm_context_init(&proc->context, stack_top, entry, arg);
    
    /* Create mailbox */
    proc->mailbox = mailbox_create();
    if (!proc->mailbox) {
        stack_free(proc->stack_base, proc->stack_size);
        free(proc);
        return NULL;
    }
    mailbox_set_owner(proc->mailbox, proc);
    
    /* Scheduling state */
    proc->next = NULL;
    proc->worker_id = 0;
    proc->spawn_time = 0;  /* TODO: get current time */
    proc->run_count = 0;
    
    return proc;
}

void proc_destroy(ArnmProcess* proc) {
    if (!proc) return;
    
    if (proc->mailbox) {
        mailbox_destroy(proc->mailbox);
    }
    
    if (proc->actor_state) {
        free(proc->actor_state);
    }
    
    if (proc->stack_base) {
        stack_free(proc->stack_base, proc->stack_size);
    }
    
    free(proc);
}

void proc_ready(ArnmProcess* proc) {
    if (proc) {
        proc->state = PROC_STATE_READY;
    }
}

void proc_wait(ArnmProcess* proc) {
    if (proc) {
        proc->state = PROC_STATE_WAITING;
    }
}

void proc_exit(ArnmProcess* proc) {
    if (proc) {
        proc->state = PROC_STATE_DEAD;
    }
}

/* ============================================================
 * Exit Handler (called from assembly)
 * ============================================================ */

/* ============================================================
 * Exit Handler (called from assembly)
 * ============================================================ */

void arnm_process_exit_handler(void) {
    ArnmProcess* proc = proc_current();
    if (proc) {
        proc_exit(proc);
        
        /* Yield to scheduler - we won't return */
        arnm_sched_yield();
    }
    
    /* Should never reach here */
    while (1) { }
}

/* ============================================================ */

void arnm_print_int(int32_t val) {
    printf("%d\n", val);
}
