/*
 * ARNm Runtime - M:N Scheduler
 */

#ifndef ARNM_SCHEDULER_H
#define ARNM_SCHEDULER_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "process.h"
#include <pthread.h>
#include <stdatomic.h>
#include <pthread.h>
#include <stdatomic.h>

/* ============================================================
 * Run Queue (Lock-free MPSC)
 * ============================================================ */

typedef struct {
    ArnmProcess*        head;
    ArnmProcess*        tail;
    atomic_size_t       count;
    pthread_spinlock_t  lock;       /* For simplicity; can optimize later */
} RunQueue;

/* ============================================================
 * Wait Queue (for parked processes)
 * ============================================================ */

typedef struct {
    ArnmProcess*        head;
    ArnmProcess*        tail;
    atomic_size_t       count;
    pthread_spinlock_t  lock;
} WaitQueue;

/* ============================================================
 * Worker Thread
 * ============================================================ */

typedef struct ArnmWorker {
    pthread_t           thread;         /* OS thread */
    uint32_t            id;             /* Worker ID */
    ArnmProcess*        current;        /* Running process */
    RunQueue            local_queue;    /* Local run queue */
    ArnmContext         scheduler_ctx;  /* Scheduler context */
    atomic_bool         running;        /* Worker active */
    uint64_t            steal_count;    /* Work stolen */
    uint64_t            run_count;      /* Processes run */
} ArnmWorker;

/* ============================================================
 * Global Scheduler State
 * ============================================================ */

typedef struct {
    ArnmWorker*         workers;        /* Worker array */
    uint32_t            num_workers;    /* Number of workers */
    RunQueue            global_queue;   /* Global run queue */
    WaitQueue           wait_queue;     /* Parked processes waiting for messages */
    atomic_bool         shutdown;       /* Shutdown flag */
    atomic_size_t       active_procs;   /* Active process count */
    atomic_size_t       waiting_procs;  /* Waiting (parked) process count */
} Scheduler;

/* ============================================================
 * Scheduler API
 * ============================================================ */

/* Initialize scheduler with N workers */
int sched_init(uint32_t num_workers);

/* Shutdown scheduler */
void sched_shutdown(void);

/* Run scheduler main loop (called by arnm_run) */
void sched_run(void);

/* Enqueue process to run queue */
void sched_enqueue(ArnmProcess* proc);

/* Enqueue to specific worker's local queue */
void sched_enqueue_local(ArnmProcess* proc, uint32_t worker_id);

/* Get next process to run (may steal from other workers) */
ArnmProcess* sched_next(ArnmWorker* worker);

/* Yield current process */
void arnm_sched_yield(void);

/* Get current worker */
ArnmWorker* sched_current_worker(void);

/* Get global scheduler */
Scheduler* sched_global(void);

/* Park a process in wait queue (called when waiting for message) */
void sched_park(ArnmProcess* proc);

/* Wake a parked process (called when message sent) */
void sched_wake(ArnmProcess* proc);

/* Check for deadlock condition */
bool sched_check_deadlock(void);

#endif /* ARNM_SCHEDULER_H */
