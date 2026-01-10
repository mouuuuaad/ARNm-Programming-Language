/*
 * ARNm Runtime - Synchronization Primitives
 */

#ifndef ARNM_SYNC_H
#define ARNM_SYNC_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>

/* Forward declarations */
typedef struct ArnmProcess ArnmProcess;

/* ============================================================
 * Mutex (Process-Level Mutual Exclusion)
 * ============================================================
 * Unlike pthread mutexes, these integrate with the scheduler:
 * - Blocking acquires yield to the scheduler instead of OS blocking
 * - Ownership is tracked at the process level
 */

typedef struct ArnmMutex {
    atomic_bool         locked;         /* Lock state */
    ArnmProcess*        owner;          /* Current owner (for deadlock detection) */
    pthread_spinlock_t  spinlock;       /* Internal protection */
} ArnmMutex;

/* Create a new mutex */
ArnmMutex* arnm_mutex_create(void);

/* Destroy a mutex */
void arnm_mutex_destroy(ArnmMutex* mtx);

/* Acquire mutex (scheduler-aware blocking) */
void arnm_mutex_lock(ArnmMutex* mtx);

/* Try to acquire mutex (non-blocking) */
bool arnm_mutex_try_lock(ArnmMutex* mtx);

/* Release mutex */
void arnm_mutex_unlock(ArnmMutex* mtx);

/* ============================================================
 * Channel (Bounded Buffer for Communication)
 * ============================================================
 * Type-safe communication between processes.
 * Bounded capacity with blocking send/receive.
 */

typedef struct ArnmChannel {
    void**              buffer;         /* Circular buffer */
    size_t              capacity;       /* Maximum elements */
    atomic_size_t       head;           /* Read position */
    atomic_size_t       tail;           /* Write position */
    atomic_size_t       count;          /* Current count */
    pthread_spinlock_t  lock;           /* Internal protection */
    atomic_bool         closed;         /* Channel closed flag */
} ArnmChannel;

/* Create a new channel with specified capacity */
ArnmChannel* arnm_channel_create(size_t capacity);

/* Destroy a channel */
void arnm_channel_destroy(ArnmChannel* chan);

/* Send data to channel (blocks if full) */
bool arnm_channel_send(ArnmChannel* chan, void* data);

/* Try to send data (non-blocking) */
bool arnm_channel_try_send(ArnmChannel* chan, void* data);

/* Receive data from channel (blocks if empty) */
void* arnm_channel_receive(ArnmChannel* chan);

/* Try to receive data (non-blocking) */
void* arnm_channel_try_receive(ArnmChannel* chan);

/* Close channel (unblocks all waiters) */
void arnm_channel_close(ArnmChannel* chan);

/* Check if channel is closed */
bool arnm_channel_is_closed(ArnmChannel* chan);

/* Get current message count */
size_t arnm_channel_count(ArnmChannel* chan);

/* ============================================================
 * Barrier (Process Synchronization Point)
 * ============================================================
 * All processes must reach barrier before any can proceed.
 */

typedef struct ArnmBarrier {
    atomic_size_t       count;          /* Number of arrived processes */
    size_t              threshold;      /* Required count to release */
    atomic_uint_fast64_t generation;    /* Barrier generation */
    pthread_spinlock_t  lock;
} ArnmBarrier;

/* Create a barrier for N processes */
ArnmBarrier* arnm_barrier_create(size_t count);

/* Destroy a barrier */
void arnm_barrier_destroy(ArnmBarrier* bar);

/* Wait at barrier (blocks until all arrive) */
void arnm_barrier_wait(ArnmBarrier* bar);

#endif /* ARNM_SYNC_H */
