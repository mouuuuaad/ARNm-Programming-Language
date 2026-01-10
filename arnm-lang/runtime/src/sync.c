/*
 * ARNm Runtime - Synchronization Primitives Implementation
 */

#include "../include/sync.h"
#include "../include/process.h"
#include "../include/scheduler.h"
#include "../include/arnm.h"
#include <stdlib.h>
#include <stdio.h>

/* ============================================================
 * Mutex Implementation
 * ============================================================ */

ArnmMutex* arnm_mutex_create(void) {
    ArnmMutex* mtx = (ArnmMutex*)malloc(sizeof(ArnmMutex));
    if (!mtx) return NULL;
    
    atomic_init(&mtx->locked, false);
    mtx->owner = NULL;
    pthread_spin_init(&mtx->spinlock, PTHREAD_PROCESS_PRIVATE);
    
    return mtx;
}

void arnm_mutex_destroy(ArnmMutex* mtx) {
    if (!mtx) return;
    
    if (atomic_load(&mtx->locked)) {
        fprintf(stderr, "[ARNM WARNING] Destroying locked mutex\n");
    }
    
    pthread_spin_destroy(&mtx->spinlock);
    free(mtx);
}

bool arnm_mutex_try_lock(ArnmMutex* mtx) {
    if (!mtx) return false;
    
    ArnmProcess* self = proc_current();
    
    bool expected = false;
    if (atomic_compare_exchange_strong(&mtx->locked, &expected, true)) {
        mtx->owner = self;
        return true;
    }
    
    return false;
}

void arnm_mutex_lock(ArnmMutex* mtx) {
    if (!mtx) return;
    
    ArnmProcess* self = proc_current();
    
    /* Deadlock detection: cannot acquire own lock */
    if (mtx->owner == self && self != NULL) {
        fprintf(stderr, "[ARNM PANIC] Mutex deadlock: process attempting to reacquire own lock\n");
        abort();
    }
    
    /* Spin with scheduler yield */
    while (!arnm_mutex_try_lock(mtx)) {
        arnm_yield();
    }
}

void arnm_mutex_unlock(ArnmMutex* mtx) {
    if (!mtx) return;
    
    ArnmProcess* self = proc_current();
    
    /* Only owner can unlock */
    if (self != NULL && mtx->owner != self) {
        fprintf(stderr, "[ARNM WARNING] Mutex unlock by non-owner\n");
        return;
    }
    
    mtx->owner = NULL;
    atomic_store(&mtx->locked, false);
}

/* ============================================================
 * Channel Implementation
 * ============================================================ */

ArnmChannel* arnm_channel_create(size_t capacity) {
    if (capacity == 0) capacity = 16;  /* Default capacity */
    
    ArnmChannel* chan = (ArnmChannel*)malloc(sizeof(ArnmChannel));
    if (!chan) return NULL;
    
    chan->buffer = (void**)calloc(capacity, sizeof(void*));
    if (!chan->buffer) {
        free(chan);
        return NULL;
    }
    
    chan->capacity = capacity;
    atomic_init(&chan->head, 0);
    atomic_init(&chan->tail, 0);
    atomic_init(&chan->count, 0);
    atomic_init(&chan->closed, false);
    pthread_spin_init(&chan->lock, PTHREAD_PROCESS_PRIVATE);
    
    return chan;
}

void arnm_channel_destroy(ArnmChannel* chan) {
    if (!chan) return;
    
    pthread_spin_destroy(&chan->lock);
    free(chan->buffer);
    free(chan);
}

bool arnm_channel_try_send(ArnmChannel* chan, void* data) {
    if (!chan || atomic_load(&chan->closed)) return false;
    
    pthread_spin_lock(&chan->lock);
    
    size_t count = atomic_load(&chan->count);
    if (count >= chan->capacity) {
        pthread_spin_unlock(&chan->lock);
        return false;
    }
    
    size_t tail = atomic_load(&chan->tail);
    chan->buffer[tail] = data;
    atomic_store(&chan->tail, (tail + 1) % chan->capacity);
    atomic_fetch_add(&chan->count, 1);
    
    pthread_spin_unlock(&chan->lock);
    return true;
}

bool arnm_channel_send(ArnmChannel* chan, void* data) {
    if (!chan) return false;
    
    while (!arnm_channel_try_send(chan, data)) {
        if (atomic_load(&chan->closed)) {
            return false;
        }
        arnm_yield();
    }
    
    return true;
}

void* arnm_channel_try_receive(ArnmChannel* chan) {
    if (!chan) return NULL;
    
    pthread_spin_lock(&chan->lock);
    
    size_t count = atomic_load(&chan->count);
    if (count == 0) {
        pthread_spin_unlock(&chan->lock);
        return NULL;
    }
    
    size_t head = atomic_load(&chan->head);
    void* data = chan->buffer[head];
    chan->buffer[head] = NULL;
    atomic_store(&chan->head, (head + 1) % chan->capacity);
    atomic_fetch_sub(&chan->count, 1);
    
    pthread_spin_unlock(&chan->lock);
    return data;
}

void* arnm_channel_receive(ArnmChannel* chan) {
    if (!chan) return NULL;
    
    void* data;
    while ((data = arnm_channel_try_receive(chan)) == NULL) {
        if (atomic_load(&chan->closed)) {
            return NULL;
        }
        arnm_yield();
    }
    
    return data;
}

void arnm_channel_close(ArnmChannel* chan) {
    if (chan) {
        atomic_store(&chan->closed, true);
    }
}

bool arnm_channel_is_closed(ArnmChannel* chan) {
    return chan ? atomic_load(&chan->closed) : true;
}

size_t arnm_channel_count(ArnmChannel* chan) {
    return chan ? atomic_load(&chan->count) : 0;
}

/* ============================================================
 * Barrier Implementation
 * ============================================================ */

ArnmBarrier* arnm_barrier_create(size_t count) {
    if (count == 0) return NULL;
    
    ArnmBarrier* bar = (ArnmBarrier*)malloc(sizeof(ArnmBarrier));
    if (!bar) return NULL;
    
    atomic_init(&bar->count, 0);
    bar->threshold = count;
    atomic_init(&bar->generation, 0);
    pthread_spin_init(&bar->lock, PTHREAD_PROCESS_PRIVATE);
    
    return bar;
}

void arnm_barrier_destroy(ArnmBarrier* bar) {
    if (!bar) return;
    pthread_spin_destroy(&bar->lock);
    free(bar);
}

void arnm_barrier_wait(ArnmBarrier* bar) {
    if (!bar) return;
    
    uint64_t my_gen = atomic_load(&bar->generation);
    
    pthread_spin_lock(&bar->lock);
    size_t arrived = atomic_fetch_add(&bar->count, 1) + 1;
    
    if (arrived == bar->threshold) {
        /* Last to arrive: reset and release */
        atomic_store(&bar->count, 0);
        atomic_fetch_add(&bar->generation, 1);
        pthread_spin_unlock(&bar->lock);
    } else {
        pthread_spin_unlock(&bar->lock);
        
        /* Wait for generation to change */
        while (atomic_load(&bar->generation) == my_gen) {
            arnm_yield();
        }
    }
}
