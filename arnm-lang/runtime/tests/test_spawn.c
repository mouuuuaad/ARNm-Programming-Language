/*
 * ARNm Runtime - Spawn Test
 * 
 * Tests process spawning and execution.
 */

#include "../include/arnm.h"
#include <stdio.h>
#include <stdatomic.h>
#include <assert.h>

static atomic_int counter = 0;

static void worker(void* arg) {
    int id = (int)(intptr_t)arg;
    printf("  Worker %d started (pid=%lu)\n", id, arnm_pid(arnm_self()));
    
    atomic_fetch_add(&counter, 1);
    
    printf("  Worker %d done\n", id);
}

int main(void) {
    printf("Testing process spawning...\n");
    
    int ret = arnm_init(2);
    assert(ret == 0);
    
    /* Spawn 5 processes */
    for (int i = 0; i < 5; i++) {
        ArnmProcess* proc = arnm_spawn(worker, (void*)(intptr_t)i, 0);
        assert(proc != NULL);
        printf("  Spawned process %d\n", i);
    }
    
    /* Run scheduler */
    arnm_run();
    
    /* Verify all workers ran */
    int final = atomic_load(&counter);
    printf("\nCounter: %d (expected 5)\n", final);
    assert(final == 5);
    
    arnm_shutdown();
    printf("Spawn test passed!\n");
    return 0;
}
