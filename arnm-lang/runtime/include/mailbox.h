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

/* Forward declaration */
typedef struct ArnmProcess ArnmProcess;

typedef struct ArnmMailbox {
    _Atomic(ArnmMessage*)   head;       /* Dequeue from head */
    _Atomic(ArnmMessage*)   tail;       /* Enqueue at tail */
    atomic_size_t           count;      /* Message count */
    size_t                  capacity;   /* Max messages (0 = unlimited) */
    ArnmProcess*            owner;      /* Owner process (for wake-on-send) */
} ArnmMailbox;

/* Overflow behavior when mailbox is full */
#define MAILBOX_OVERFLOW_BLOCK   0  /* Block sender until space available */
#define MAILBOX_OVERFLOW_DROP    1  /* Drop message silently */
#define MAILBOX_OVERFLOW_PANIC   2  /* Crash with error */

/* ============================================================
 * Mailbox API
 * ============================================================ */

/* Create a new mailbox with owner and capacity */
ArnmMailbox* mailbox_create_ex(ArnmProcess* owner, size_t capacity);

/* Create a new mailbox (unlimited capacity, no owner yet) */
ArnmMailbox* mailbox_create(void);

/* Destroy mailbox and free pending messages */
void mailbox_destroy(ArnmMailbox* mbox);

/* Enqueue message (thread-safe, lock-free) */
bool mailbox_send(ArnmMailbox* mbox, uint64_t tag, void* data, size_t size);

/* Enqueue message with overflow policy */
bool mailbox_send_ex(ArnmMailbox* mbox, uint64_t tag, void* data, size_t size, int overflow_policy);

/* Set owner process (for wake-on-send) */
void mailbox_set_owner(ArnmMailbox* mbox, ArnmProcess* owner);

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
