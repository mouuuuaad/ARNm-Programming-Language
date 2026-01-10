/*
 * ARNm Runtime - Stress Test: Parallel Spawn
 * 
 * Spawns 1000 actors rapidly to stress-test process creation and scheduling.
 */

#include "../include/arnm.h"
#include <stdio.h>
#include <stdatomic.h>
#include <assert.h>
#include <time.h>

#define NUM_ACTORS 1000

static atomic_int completed_count = 0;

static void worker(void* arg) {
    int id = (int)(intptr_t)arg;
    (void)id;  /* Unused, just for identification */
    
    /* Do a small amount of work */
    volatile int sum = 0;
    for (int i = 0; i < 100; i++) {
        sum += i;
    }
    
    atomic_fetch_add(&completed_count, 1);
}

int main(void) {
    printf("=== Stress Test: Parallel Spawn ===\n");
    printf("Spawning %d actors...\n", NUM_ACTORS);
    
    clock_t start = clock();
    
    int ret = arnm_init(0);  /* Auto-detect workers */
    assert(ret == 0);
    
    /* Spawn all actors */
    for (int i = 0; i < NUM_ACTORS; i++) {
        ArnmProcess* proc = arnm_spawn(worker, (void*)(intptr_t)i, 0);
        if (!proc) {
            fprintf(stderr, "Failed to spawn actor %d\n", i);
            return 1;
        }
    }
    
    clock_t spawn_done = clock();
    printf("Spawned in %.3f ms\n", ((double)(spawn_done - start) / CLOCKS_PER_SEC) * 1000.0);
    
    /* Run scheduler */
    arnm_run();
    
    clock_t all_done = clock();
    
    /* Verify all completed */
    int final = atomic_load(&completed_count);
    printf("\nCompleted: %d / %d\n", final, NUM_ACTORS);
    printf("Total time: %.3f ms\n", ((double)(all_done - start) / CLOCKS_PER_SEC) * 1000.0);
    
    arnm_shutdown();
    
    if (final == NUM_ACTORS) {
        printf("\n[PASS] Stress test: parallel spawn\n");
        return 0;
    } else {
        printf("\n[FAIL] Not all actors completed!\n");
        return 1;
    }
}
