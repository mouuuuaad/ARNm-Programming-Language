/*
 * ARNm Runtime - Main Header
 * 
 * libarnm provides the runtime environment for ARNm programs:
 * - Lightweight process model
 * - M:N scheduling with work-stealing
 * - Lock-free message passing
 * - Automatic reference counting
 */

#ifndef ARNM_RUNTIME_H
#define ARNM_RUNTIME_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* ============================================================
 * Configuration
 * ============================================================ */

#define ARNM_DEFAULT_STACK_SIZE     (64 * 1024)     /* 64KB per process */
#define ARNM_MAX_WORKERS            64
#define ARNM_MAILBOX_CAPACITY       1024

/* ============================================================
 * Forward Declarations
 * ============================================================ */

typedef struct ArnmProcess ArnmProcess;
typedef struct ArnmMessage ArnmMessage;
typedef struct ArnmMailbox ArnmMailbox;
typedef struct ArnmContext ArnmContext;

/* ============================================================
 * Process State
 * ============================================================ */

typedef enum {
    PROC_READY,         /* Ready to run */
    PROC_RUNNING,       /* Currently executing */
    PROC_WAITING,       /* Waiting for message */
    PROC_DEAD,          /* Terminated */
} ProcessState;

/* ============================================================
 * Runtime Lifecycle
 * ============================================================ */

/* Initialize runtime with specified number of worker threads */
int arnm_init(int num_workers);

/* Shutdown runtime and cleanup */
void arnm_shutdown(void);

/* Run the scheduler (blocks until all processes complete) */
void arnm_run(void);

/* ============================================================
 * Process API
 * ============================================================ */

/* Spawn a new process with given entry function */
ArnmProcess* arnm_spawn(void (*entry)(void*), void* arg, size_t state_size);

/* Get current process */
ArnmProcess* arnm_self(void);

/* Get process ID */
uint64_t arnm_pid(ArnmProcess* proc);

/* Yield to scheduler */
void arnm_yield(void);

/* Exit current process */
void arnm_exit(void);

/* ============================================================
 * Message Passing
 * ============================================================ */

/* Send message to target process */
int arnm_send(ArnmProcess* target, uint64_t tag, void* data, size_t size);

/* Receive message (blocks until available) */
ArnmMessage* arnm_receive(void);

/* Try to receive message (non-blocking, returns NULL if empty) */
ArnmMessage* arnm_try_receive(void);

/* Free received message */
void arnm_message_free(ArnmMessage* msg);

/* Access message contents */
uint64_t arnm_message_tag(ArnmMessage* msg);
void* arnm_message_data(ArnmMessage* msg);
size_t arnm_message_size(ArnmMessage* msg);

/* Panic on unmatched message (for receive blocks) */
void arnm_panic_nomatch(void);

/* ============================================================
 * Memory Management (ARC)
 * ============================================================ */

typedef void (*ArnmDestructor)(void*);

/* Allocate reference-counted object */
void* arnm_alloc(size_t size, ArnmDestructor dtor);

/* Increment reference count */
void arnm_retain(void* obj);

/* Decrement reference count (frees if zero) */
void arnm_release(void* obj);

/* Get current reference count */
uint32_t arnm_refcount(void* obj);

#endif /* ARNM_RUNTIME_H */
