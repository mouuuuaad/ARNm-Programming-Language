/*
 * ARNm Runtime - M:N Scheduler Implementation
 */

#include "../include/scheduler.h"
#include "../include/process.h"
#include "../include/memory.h"
#include "../include/arnm.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

/* ============================================================
 * Runtime Assertions (Day 8 Hardening)
 * ============================================================ */

#define RUNTIME_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "[RUNTIME INVARIANT VIOLATION] %s\n", msg); \
        assert(cond); \
    } \
} while(0)

#define ASSERT_VALID_STATE(proc) \
    RUNTIME_ASSERT((proc)->state >= PROC_STATE_READY && (proc)->state <= PROC_STATE_DEAD, \
                   "process has invalid state")

/* ============================================================
 * Global Scheduler State
 * ============================================================ */

static Scheduler g_scheduler = {0};

Scheduler* sched_global(void) {
    return &g_scheduler;
}

/* ============================================================
 * Thread-Local Worker
 * ============================================================ */

static _Thread_local ArnmWorker* tls_worker = NULL;

ArnmWorker* sched_current_worker(void) {
    return tls_worker;
}

/* ============================================================
 * Run Queue Operations
 * ============================================================ */

static void runqueue_init(RunQueue* rq) {
    rq->head = NULL;
    rq->tail = NULL;
    atomic_init(&rq->count, 0);
    pthread_spin_init(&rq->lock, PTHREAD_PROCESS_PRIVATE);
}

static void runqueue_destroy(RunQueue* rq) {
    pthread_spin_destroy(&rq->lock);
}

static void runqueue_push(RunQueue* rq, ArnmProcess* proc) {
    pthread_spin_lock(&rq->lock);
    
    proc->next = NULL;
    if (rq->tail) {
        rq->tail->next = proc;
    } else {
        rq->head = proc;
    }
    rq->tail = proc;
    atomic_fetch_add(&rq->count, 1);
    
    pthread_spin_unlock(&rq->lock);
}

static ArnmProcess* runqueue_pop(RunQueue* rq) {
    pthread_spin_lock(&rq->lock);
    
    ArnmProcess* proc = rq->head;
    if (proc) {
        rq->head = proc->next;
        if (!rq->head) {
            rq->tail = NULL;
        }
        proc->next = NULL;
        atomic_fetch_sub(&rq->count, 1);
    }
    
    pthread_spin_unlock(&rq->lock);
    return proc;
}

static size_t runqueue_count(RunQueue* rq) {
    return atomic_load(&rq->count);
}

/* ============================================================
 * Wait Queue Operations (for parked processes)
 * ============================================================ */

static void waitqueue_init(WaitQueue* wq) {
    wq->head = NULL;
    wq->tail = NULL;
    atomic_init(&wq->count, 0);
    pthread_spin_init(&wq->lock, PTHREAD_PROCESS_PRIVATE);
}

static void waitqueue_destroy(WaitQueue* wq) {
    pthread_spin_destroy(&wq->lock);
}

static void waitqueue_push(WaitQueue* wq, ArnmProcess* proc) {
    pthread_spin_lock(&wq->lock);
    
    proc->next = NULL;
    if (wq->tail) {
        wq->tail->next = proc;
    } else {
        wq->head = proc;
    }
    wq->tail = proc;
    atomic_fetch_add(&wq->count, 1);
    
    pthread_spin_unlock(&wq->lock);
}

static ArnmProcess* waitqueue_pop(WaitQueue* wq) {
    pthread_spin_lock(&wq->lock);
    
    ArnmProcess* proc = wq->head;
    if (proc) {
        wq->head = proc->next;
        if (!wq->head) {
            wq->tail = NULL;
        }
        proc->next = NULL;
        atomic_fetch_sub(&wq->count, 1);
    }
    
    pthread_spin_unlock(&wq->lock);
    return proc;
}

static ArnmProcess* waitqueue_remove(WaitQueue* wq, ArnmProcess* target) {
    pthread_spin_lock(&wq->lock);
    
    ArnmProcess* prev = NULL;
    ArnmProcess* proc = wq->head;
    
    while (proc) {
        if (proc == target) {
            if (prev) {
                prev->next = proc->next;
            } else {
                wq->head = proc->next;
            }
            if (wq->tail == proc) {
                wq->tail = prev;
            }
            proc->next = NULL;
            atomic_fetch_sub(&wq->count, 1);
            pthread_spin_unlock(&wq->lock);
            return proc;
        }
        prev = proc;
        proc = proc->next;
    }
    
    pthread_spin_unlock(&wq->lock);
    return NULL;
}

static size_t waitqueue_count(WaitQueue* wq) {
    return atomic_load(&wq->count);
}

/* ============================================================
 * Work Stealing
 * ============================================================ */

static ArnmProcess* steal_from(ArnmWorker* victim) {
    return runqueue_pop(&victim->local_queue);
}

static ArnmProcess* try_steal(ArnmWorker* worker) {
    uint32_t num_workers = g_scheduler.num_workers;
    uint32_t start = worker->id;
    
    for (uint32_t i = 1; i < num_workers; i++) {
        uint32_t victim_id = (start + i) % num_workers;
        ArnmWorker* victim = &g_scheduler.workers[victim_id];
        
        if (runqueue_count(&victim->local_queue) > 1) {
            ArnmProcess* proc = steal_from(victim);
            if (proc) {
                worker->steal_count++;
                return proc;
            }
        }
    }
    
    return NULL;
}

/* ============================================================
 * Scheduler Core
 * ============================================================ */

ArnmProcess* sched_next(ArnmWorker* worker) {
    ArnmProcess* proc = NULL;
    
    /* Try local queue first */
    proc = runqueue_pop(&worker->local_queue);
    if (proc) return proc;
    
    /* Try global queue */
    proc = runqueue_pop(&g_scheduler.global_queue);
    if (proc) return proc;
    
    /* Try work stealing */
    proc = try_steal(worker);
    return proc;
}

void sched_enqueue(ArnmProcess* proc) {
    if (!proc) return;
    
    proc_ready(proc);
    atomic_fetch_add(&g_scheduler.active_procs, 1);
    
    /* Prefer local queue if we're on a worker */
    ArnmWorker* worker = sched_current_worker();
    if (worker) {
        runqueue_push(&worker->local_queue, proc);
    } else {
        runqueue_push(&g_scheduler.global_queue, proc);
    }
}

void sched_enqueue_local(ArnmProcess* proc, uint32_t worker_id) {
    if (!proc || worker_id >= g_scheduler.num_workers) return;
    
    proc_ready(proc);
    atomic_fetch_add(&g_scheduler.active_procs, 1);
    runqueue_push(&g_scheduler.workers[worker_id].local_queue, proc);
}

void arnm_sched_yield(void) {
    ArnmWorker* worker = sched_current_worker();
    if (!worker || !worker->current) return;
    
    ArnmProcess* current = worker->current;
    
    /* Re-enqueue if still runnable */
    if (current->state == PROC_STATE_READY || 
        current->state == PROC_STATE_RUNNING) {
        current->state = PROC_STATE_READY;
        runqueue_push(&worker->local_queue, current);
    } else if (current->state == PROC_STATE_DEAD) {
        /* Process exited, decrement active count */
        atomic_fetch_sub(&g_scheduler.active_procs, 1);
    }
    
    /* Switch back to scheduler context */
    arnm_context_switch(&current->context, &worker->scheduler_ctx);
}

/* ============================================================
 * Worker Thread
 * ============================================================ */

static void* worker_main(void* arg) {
    ArnmWorker* worker = (ArnmWorker*)arg;
    tls_worker = worker;
    
    while (!atomic_load(&g_scheduler.shutdown)) {
        ArnmProcess* proc = sched_next(worker);
        
        if (proc) {
            /* Day 8: Validate process state before running */
            RUNTIME_ASSERT(proc->state == PROC_STATE_READY || proc->state == PROC_STATE_WAITING,
                          "process popped from queue should be ready or waiting");
            
            worker->current = proc;
            proc->state = PROC_STATE_RUNNING;
            proc->run_count++;
            proc->worker_id = worker->id;
            proc->worker_id = worker->id;
            proc_set_current(proc);
            worker->run_count++;
            
            /* Switch to process */
            arnm_context_switch(&worker->scheduler_ctx, &proc->context);
            
            /* Returned from process */
            proc_set_current(NULL);
            worker->current = NULL;
            
            /* Handle dead processes */
            if (proc->state == PROC_STATE_DEAD) {
                proc_destroy(proc);
            }
        } else {
            /* No work - check if done */
            if (atomic_load(&g_scheduler.active_procs) == 0) {
                break;
            }
            
            /* Yield to other threads */
            usleep(100);  /* 100 microseconds */
        }
    }
    
    /* atomic_store(&worker->running, false); - moved inside if to be safe? No, just before return. */
    atomic_store(&worker->running, false);
    return NULL;
}

/* ============================================================
 * Scheduler Lifecycle
 * ============================================================ */

int sched_init(uint32_t num_workers) {
    if (num_workers == 0) {
        num_workers = sysconf(_SC_NPROCESSORS_ONLN);
        if (num_workers < 1) num_workers = 1;
    }
    if (num_workers > ARNM_MAX_WORKERS) {
        num_workers = ARNM_MAX_WORKERS;
    }
    
    g_scheduler.workers = (ArnmWorker*)calloc(num_workers, sizeof(ArnmWorker));
    if (!g_scheduler.workers) return -1;
    
    g_scheduler.num_workers = num_workers;
    atomic_init(&g_scheduler.shutdown, false);
    atomic_init(&g_scheduler.active_procs, 0);
    atomic_init(&g_scheduler.waiting_procs, 0);
    
    runqueue_init(&g_scheduler.global_queue);
    waitqueue_init(&g_scheduler.wait_queue);
    
    for (uint32_t i = 0; i < num_workers; i++) {
        g_scheduler.workers[i].id = i;
        g_scheduler.workers[i].current = NULL;
        atomic_init(&g_scheduler.workers[i].running, false);
        runqueue_init(&g_scheduler.workers[i].local_queue);
    }
    
    return 0;
}

void sched_run(void) {
    /* Start worker threads (except worker 0 which runs on main) */
    for (uint32_t i = 1; i < g_scheduler.num_workers; i++) {
        atomic_store(&g_scheduler.workers[i].running, true);
        pthread_create(&g_scheduler.workers[i].thread, NULL, 
                       worker_main, &g_scheduler.workers[i]);
    }
    
    /* Run worker 0 on main thread */
    atomic_store(&g_scheduler.workers[0].running, true);
    worker_main(&g_scheduler.workers[0]);
    
    /* Wait for other workers to finish */
    for (uint32_t i = 1; i < g_scheduler.num_workers; i++) {
        pthread_join(g_scheduler.workers[i].thread, NULL);
    }
}

void sched_shutdown(void) {
    atomic_store(&g_scheduler.shutdown, true);
    
    /* Wait for workers to stop */
    for (uint32_t i = 1; i < g_scheduler.num_workers; i++) {
        if (atomic_load(&g_scheduler.workers[i].running)) {
            pthread_join(g_scheduler.workers[i].thread, NULL);
        }
    }
    
    /* Cleanup */
    runqueue_destroy(&g_scheduler.global_queue);
    waitqueue_destroy(&g_scheduler.wait_queue);
    for (uint32_t i = 0; i < g_scheduler.num_workers; i++) {
        runqueue_destroy(&g_scheduler.workers[i].local_queue);
    }
    
    free(g_scheduler.workers);
    memset(&g_scheduler, 0, sizeof(g_scheduler));
}

/* ============================================================
 * Process Parking/Waking
 * ============================================================ */

void sched_park(ArnmProcess* proc) {
    if (!proc) return;
    
    /* Mark process as waiting */
    proc->state = PROC_STATE_WAITING;
    
    /* Add to wait queue */
    waitqueue_push(&g_scheduler.wait_queue, proc);
    atomic_fetch_add(&g_scheduler.waiting_procs, 1);
}

void sched_wake(ArnmProcess* proc) {
    if (!proc) return;
    
    /* Try to remove from wait queue */
    ArnmProcess* found = waitqueue_remove(&g_scheduler.wait_queue, proc);
    if (found) {
        atomic_fetch_sub(&g_scheduler.waiting_procs, 1);
        
        /* Re-enqueue to run queue */
        found->state = PROC_STATE_READY;
        runqueue_push(&g_scheduler.global_queue, found);
    }
}

bool sched_check_deadlock(void) {
    size_t active = atomic_load(&g_scheduler.active_procs);
    size_t waiting = atomic_load(&g_scheduler.waiting_procs);
    
    /* Deadlock: processes exist but all are waiting */
    if (active > 0 && waiting == active) {
        /* Check if all waiting processes have empty mailboxes */
        /* For now, just return true - a full check requires mailbox inspection */
        fprintf(stderr, "[ARNM WARNING] Potential deadlock detected: %zu processes all waiting\n", 
                waiting);
        return true;
    }
    
    return false;
}
