/*
 * ARNm Runtime - Process Model
 */

#ifndef ARNM_PROCESS_H
#define ARNM_PROCESS_H

#include "context.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

/* Forward declarations */
typedef struct ArnmMailbox ArnmMailbox;

/* ============================================================
 * Process Structure
 * ============================================================ */

typedef enum {
    PROC_STATE_READY,       /* Ready to run */
    PROC_STATE_RUNNING,     /* Currently executing */
    PROC_STATE_WAITING,     /* Blocked on receive */
    PROC_STATE_DEAD,        /* Terminated */
} ProcState;

typedef struct ArnmProcess {
    void*               actor_state;    /* Pointer to actor state (must be first) */
    uint64_t            pid;            /* Unique process ID */
    ProcState           state;          /* Current state */
    ArnmContext         context;        /* CPU context */
    
    /* Stack */
    void*               stack_base;     /* Allocated stack memory */
    size_t              stack_size;     /* Stack size in bytes */
    
    /* Mailbox */
    ArnmMailbox*        mailbox;        /* Incoming message queue */
    
    /* Scheduling */
    struct ArnmProcess* next;           /* Run queue link */
    uint32_t            worker_id;      /* Assigned worker */
    
    /* Statistics */
    uint64_t            spawn_time;     /* When process was created */
    uint64_t            run_count;      /* Number of times scheduled */
} ArnmProcess;

/* ============================================================
 * Process Lifecycle
 * ============================================================ */

/* Create a new process (does not start it) */
ArnmProcess* proc_create(void (*entry)(void*), void* arg, size_t stack_size, size_t state_size);

/* Destroy a process and free resources */
void proc_destroy(ArnmProcess* proc);

/* Mark process as ready and enqueue */
void proc_ready(ArnmProcess* proc);

/* Mark process as waiting (blocked on receive) */
void proc_wait(ArnmProcess* proc);

/* Mark process as dead */
void proc_exit(ArnmProcess* proc);

/* ============================================================
 * Process ID Generation
 * ============================================================ */

/* Get next unique process ID */
uint64_t proc_next_pid(void);

/* ============================================================
 * Current Process
 * ============================================================ */

/* Get currently running process on this worker */
ArnmProcess* proc_current(void);

/* Set currently running process */
void proc_set_current(ArnmProcess* proc);

#endif /* ARNM_PROCESS_H */
