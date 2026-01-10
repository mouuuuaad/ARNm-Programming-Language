/*
 * ARNm Runtime - Stress Test: Contention
 * 
 * Multiple actors sending to a single target to test concurrency under contention.
 */

#include "../include/arnm.h"
#include "../include/sync.h"
#include <stdio.h>
#include <stdatomic.h>
#include <assert.h>
#include <time.h>

#define NUM_SENDERS 8
#define MESSAGES_PER_SENDER 500

#define MSG_INCREMENT 1
#define MSG_DONE 2

static ArnmProcess* target_proc = NULL;
static atomic_int senders_done = 0;

/* Target actor: increments a counter for each message */
static void target(void* arg) {
    (void)arg;
    int counter = 0;
    int done_count = 0;
    
    while (done_count < NUM_SENDERS) {
        ArnmMessage* msg = arnm_try_receive();
        if (msg) {
            uint64_t tag = arnm_message_tag(msg);
            arnm_message_free(msg);
            
            if (tag == MSG_INCREMENT) {
                counter++;
            } else if (tag == MSG_DONE) {
                done_count++;
            }
        } else {
            arnm_yield();
        }
    }
    
    int expected = NUM_SENDERS * MESSAGES_PER_SENDER;
    printf("  Target: final counter = %d (expected %d)\n", counter, expected);
    
    if (counter != expected) {
        printf("  [ERROR] Counter mismatch!\n");
    }
}

static void sender(void* arg) {
    int id = (int)(intptr_t)arg;
    
    for (int i = 0; i < MESSAGES_PER_SENDER; i++) {
        arnm_send(target_proc, MSG_INCREMENT, NULL, 0);
    }
    
    arnm_send(target_proc, MSG_DONE, NULL, 0);
    atomic_fetch_add(&senders_done, 1);
    
    printf("  Sender %d: sent %d messages\n", id, MESSAGES_PER_SENDER);
}

int main(void) {
    printf("=== Stress Test: Contention ===\n");
    printf("%d senders sending %d messages each to 1 target...\n", 
           NUM_SENDERS, MESSAGES_PER_SENDER);
    
    clock_t start = clock();
    
    int ret = arnm_init(0);
    assert(ret == 0);
    
    /* Spawn target first */
    target_proc = arnm_spawn(target, NULL, 0);
    assert(target_proc != NULL);
    
    /* Spawn all senders */
    for (int i = 0; i < NUM_SENDERS; i++) {
        ArnmProcess* proc = arnm_spawn(sender, (void*)(intptr_t)i, 0);
        assert(proc != NULL);
    }
    
    /* Run scheduler */
    arnm_run();
    
    clock_t all_done = clock();
    
    printf("\nTotal time: %.3f ms\n", ((double)(all_done - start) / CLOCKS_PER_SEC) * 1000.0);
    
    arnm_shutdown();
    
    printf("\n[PASS] Stress test: contention\n");
    return 0;
}
