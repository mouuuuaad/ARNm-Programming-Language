/*
 * ARNm Runtime - Lock-Free Mailbox Implementation
 */

#include "../include/mailbox.h"
#include "../include/process.h"
#include "../include/scheduler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================
 * Message Implementation
 * ============================================================ */

ArnmMessage* message_create(uint64_t tag, void* data, size_t size) {
    ArnmMessage* msg = (ArnmMessage*)malloc(sizeof(ArnmMessage));
    if (!msg) return NULL;
    
    msg->tag = tag;
    msg->size = size;
    msg->next = NULL;
    
    if (size > 0 && data) {
        msg->data = malloc(size);
        if (!msg->data) {
            free(msg);
            return NULL;
        }
        memcpy(msg->data, data, size);
    } else {
        msg->data = data;  /* For zero-copy when data is already heap-allocated */
    }
    
    return msg;
}

void message_free(ArnmMessage* msg) {
    // fprintf(stderr, "[DEBUG] free msg=%p\n", msg);
    if (!msg) return;
    
    if (msg->data && msg->size > 0) {
        free(msg->data);
    }
    free(msg);
}

/* ============================================================
 * Mailbox Implementation (Lock-Free MPSC)
 * ============================================================ */

ArnmMailbox* mailbox_create(void) {
    ArnmMailbox* mbox = (ArnmMailbox*)malloc(sizeof(ArnmMailbox));
    if (!mbox) return NULL;
    
    /* Create dummy node for easier queue management */
    ArnmMessage* dummy = message_create(0, NULL, 0);
    if (!dummy) {
        free(mbox);
        return NULL;
    }
    
    atomic_init(&mbox->head, dummy);
    atomic_init(&mbox->tail, dummy);
    atomic_init(&mbox->count, 0);
    
    return mbox;
}

void mailbox_destroy(ArnmMailbox* mbox) {
    if (!mbox) return;
    
    /* Free all pending messages */
    ArnmMessage* msg;
    while ((msg = mailbox_try_receive(mbox)) != NULL) {
        message_free(msg);
    }
    
    /* Free dummy node */
    ArnmMessage* dummy = atomic_load(&mbox->head);
    message_free(dummy);
    
    free(mbox);
}

bool mailbox_send(ArnmMailbox* mbox, uint64_t tag, void* data, size_t size) {
    if (!mbox) return false;
    
    ArnmMessage* msg = message_create(tag, data, size);
    if (!msg) return false;
    
    /* Lock-free enqueue at tail */
    ArnmMessage* prev = atomic_exchange(&mbox->tail, msg);
    atomic_store(&prev->next, msg);
    
    atomic_fetch_add(&mbox->count, 1);
    
    /* TODO: Wake up waiting process if any */
    
    return true;
}

ArnmMessage* mailbox_try_receive(ArnmMailbox* mbox) {
    if (!mbox) return NULL;
    // fprintf(stderr, "[DEBUG] try_receive mbox=%p\n", mbox);
    ArnmMessage* head = atomic_load(&mbox->head);
    if (!head) {
         fprintf(stderr, "[DEBUG] head is NULL!\n");
         return NULL;
    }
    ArnmMessage* next = atomic_load(&head->next);
    
    if (next == NULL) {
        return NULL;  /* Queue empty */
    }
    
    /* Dequeue: move head to next node */
    atomic_store(&mbox->head, next);
    // fprintf(stderr, "[DEBUG] Dequeue next=%p tag=%lu\n", next, next->tag);
    atomic_fetch_sub(&mbox->count, 1);
    
    /* Free the old dummy node */
    message_free(head);
    
    /* Return message data (next becomes the new dummy) */
    ArnmMessage* msg = message_create(next->tag, NULL, 0);
    if (msg) {
        msg->tag = next->tag;
        msg->data = next->data;
        msg->size = next->size;
        next->data = NULL;  /* Prevent double free */
        next->size = 0;
    }
    
    return msg;
}

ArnmMessage* mailbox_receive(ArnmMailbox* mbox) {
    if (!mbox) return NULL;
    
    ArnmMessage* msg;
    
    /* Spin/yield until message available */
    while ((msg = mailbox_try_receive(mbox)) == NULL) {
        /* Mark current process as waiting */
        ArnmProcess* proc = proc_current();
        if (proc) {
            proc_wait(proc);
            arnm_sched_yield();
        } else {
            /* Not in a process context, spin wait */
            for (volatile int i = 0; i < 1000; i++) { }
        }
    }
    
    return msg;
}

bool mailbox_empty(ArnmMailbox* mbox) {
    return mbox ? atomic_load(&mbox->count) == 0 : true;
}

size_t mailbox_count(ArnmMailbox* mbox) {
    return mbox ? atomic_load(&mbox->count) : 0;
}
