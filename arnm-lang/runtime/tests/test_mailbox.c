/*
 * ARNm Runtime - Mailbox Test (Non-Blocking)
 * 
 * Tests message passing using polling instead of blocking receive.
 */

#include "../include/arnm.h"
#include <stdio.h>
#include <stdatomic.h>
#include <assert.h>

#define MSG_WORK 1
#define MSG_STOP 2

static atomic_int received_count = 0;

static void receiver(void* arg) {
    (void)arg;
    printf("  Receiver: started\n");
    
    /* Poll for messages until STOP */
    bool running = true;
    while (running) {
        ArnmMessage* msg = arnm_try_receive();
        if (msg) {
            if (arnm_message_tag(msg) == MSG_STOP) {
                printf("  Receiver: got STOP\n");
                running = false;
            } else {
                printf("  Receiver: got message %lu\n", arnm_message_tag(msg));
                atomic_fetch_add(&received_count, 1);
            }
            arnm_message_free(msg);
        } else {
             arnm_yield();
        }
    }
    
    printf("  Receiver: done\n");
}

static void sender(void* arg) {
    ArnmProcess* target = (ArnmProcess*)arg;
    printf("  Sender: started\n");
    
    /* Send a few messages */
    for (int i = 0; i < 5; i++) {
        printf("  Sender: sending message %d\n", i);
        int ret = arnm_send(target, MSG_WORK, NULL, 0);
        if (ret < 0) {
            printf("  Sender: send failed!\n");
        }
        arnm_yield();
    }
    
    /* Send STOP */
    printf("  Sender: sending STOP\n");
    arnm_send(target, MSG_STOP, NULL, 0);
    
    printf("  Sender: done\n");
}

int main(void) {
    printf("Testing non-blocking message passing...\n");
    
    int ret = arnm_init(2);
    assert(ret == 0);
    
    /* Spawn receiver first */
    ArnmProcess* recv_proc = arnm_spawn(receiver, NULL);
    assert(recv_proc != NULL);
    
    /* Spawn sender with receiver as argument */
    ArnmProcess* send_proc = arnm_spawn(sender, recv_proc);
    assert(send_proc != NULL);
    
    /* Run scheduler */
    arnm_run();
    
    /* Check results */
    int count = atomic_load(&received_count);
    printf("\nReceived count: %d\n", count);
    
    arnm_shutdown();
    
    if (count >= 1) {
        printf("Mailbox test passed!\n");
        return 0;
    } else {
        printf("Mailbox test FAILED!\n");
        return 1;
    }
}
