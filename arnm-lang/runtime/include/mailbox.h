/*
 * ARNm Runtime - Lock-Free Mailbox
 */

#ifndef ARNM_MAILBOX_H
#define ARNM_MAILBOX_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>

/* ============================================================
 * Message Structure
 * ============================================================ */

typedef struct ArnmMessage {
    uint64_t            tag;            /* Message type/tag */
    void*               data;           /* Payload pointer */
    size_t              size;           /* Payload size */
    struct ArnmMessage* next;           /* Queue link */
} ArnmMessage;

/* ============================================================
 * Mailbox (MPSC Queue)
 * ============================================================
 * Multiple producers (senders) can enqueue.
 * Single consumer (owner process) dequeues.
 */

typedef struct ArnmMailbox {
    _Atomic(ArnmMessage*)   head;       /* Dequeue from head */
    _Atomic(ArnmMessage*)   tail;       /* Enqueue at tail */
    atomic_size_t           count;      /* Message count */
} ArnmMailbox;

/* ============================================================
 * Mailbox API
 * ============================================================ */

/* Create a new mailbox */
ArnmMailbox* mailbox_create(void);

/* Destroy mailbox and free pending messages */
void mailbox_destroy(ArnmMailbox* mbox);

/* Enqueue message (thread-safe, lock-free) */
bool mailbox_send(ArnmMailbox* mbox, uint64_t tag, void* data, size_t size);

/* Dequeue message (blocking) */
ArnmMessage* mailbox_receive(ArnmMailbox* mbox);

/* Try to dequeue message (non-blocking) */
ArnmMessage* mailbox_try_receive(ArnmMailbox* mbox);

/* Check if mailbox is empty */
bool mailbox_empty(ArnmMailbox* mbox);

/* Get pending message count */
size_t mailbox_count(ArnmMailbox* mbox);

/* ============================================================
 * Message API
 * ============================================================ */

/* Create a new message */
ArnmMessage* message_create(uint64_t tag, void* data, size_t size);

/* Free a message */
void message_free(ArnmMessage* msg);

#endif /* ARNM_MAILBOX_H */
