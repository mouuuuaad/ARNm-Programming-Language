/*
 * ARNm Runtime - Stress Test: Message Flood
 * 
 * Floods actors with thousands of messages to test mailbox and message passing.
 */

#include "../include/arnm.h"
#include <stdio.h>
#include <stdatomic.h>
#include <assert.h>
#include <time.h>

#define NUM_RECEIVERS 5
#define MESSAGES_PER_RECEIVER 100

#define MSG_WORK 1
#define MSG_STOP 2

static atomic_int total_received = 0;
static ArnmProcess* receivers[NUM_RECEIVERS];

static void receiver(void* arg) {
    int id = (int)(intptr_t)arg;
    int my_count = 0;
    
    while (1) {
        ArnmMessage* msg = arnm_try_receive();
        if (msg) {
            uint64_t tag = arnm_message_tag(msg);
            arnm_message_free(msg);
            
            if (tag == MSG_STOP) {
                break;
            }
            my_count++;
        } else {
            arnm_yield();
        }
    }
    
    atomic_fetch_add(&total_received, my_count);
    printf("  Receiver %d: done, received %d messages\n", id, my_count);
    fflush(stdout);
}

static void sender(void* arg) {
    (void)arg;
    int total_sent = 0;
    
    /* Send messages to all receivers */
    for (int r = 0; r < NUM_RECEIVERS; r++) {
        for (int m = 0; m < MESSAGES_PER_RECEIVER; m++) {
            arnm_send(receivers[r], MSG_WORK, NULL, 0);
            total_sent++;
            
            /* Yield periodically to let receivers process */
            if (total_sent % 100 == 0) {
                arnm_yield();
            }
        }
    }
    
    /* Send stop to all */
    for (int r = 0; r < NUM_RECEIVERS; r++) {
        arnm_send(receivers[r], MSG_STOP, NULL, 0);
    }
}

int main(void) {
    printf("=== Stress Test: Message Flood ===\n");
    printf("Sending %d messages to %d receivers...\n", 
           NUM_RECEIVERS * MESSAGES_PER_RECEIVER, NUM_RECEIVERS);
    
    clock_t start = clock();
    
    int ret = arnm_init(0);
    assert(ret == 0);
    
    /* Spawn receivers first */
    for (int i = 0; i < NUM_RECEIVERS; i++) {
        receivers[i] = arnm_spawn(receiver, (void*)(intptr_t)i, 0);
        assert(receivers[i] != NULL);
    }
    
    /* Spawn sender */
    ArnmProcess* send_proc = arnm_spawn(sender, NULL, 0);
    assert(send_proc != NULL);
    
    /* Run scheduler */
    arnm_run();
    
    clock_t all_done = clock();
    
    /* Verify */
    int final = atomic_load(&total_received);
    int expected = NUM_RECEIVERS * MESSAGES_PER_RECEIVER;
    
    printf("\nTotal messages received: %d (expected %d)\n", final, expected);
    printf("Total time: %.3f ms\n", ((double)(all_done - start) / CLOCKS_PER_SEC) * 1000.0);
    printf("Throughput: %.0f msg/sec\n", 
           (double)expected / (((double)(all_done - start) / CLOCKS_PER_SEC)));
    
    arnm_shutdown();
    
    if (final == expected) {
        printf("\n[PASS] Stress test: message flood\n");
        return 0;
    } else {
        printf("\n[FAIL] Message count mismatch!\n");
        return 1;
    }
}
