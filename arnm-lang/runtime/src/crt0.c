/*
 * ARNm Runtime - Startup Code (crt0)
 */

#include "../include/arnm.h"
#include <stdio.h>

/* The user's entry point, renamed by compiler */
extern void _arnm_main(void);

/* Helper wrapper to match ArnmEntry signature (void (*)(void*)) */
static void main_wrapper(void* arg) {
    (void)arg;
    _arnm_main();
}

int main(int argc, char** argv) {
    /* Initialize runtime with default workers (0 = auto) */
    if (arnm_init(0) != 0) {
        fprintf(stderr, "Failed to initialize ARNm runtime\n");
        return 1;
    }
    
    /* Spawn the main actor/process */
    if (!arnm_spawn(main_wrapper, NULL, 0)) {
        fprintf(stderr, "Failed to spawn main process\n");
        return 1;
    }
    
    /* Run the scheduler (blocks until all processes exit or deadlock) */
    arnm_run();
    
    /* Cleanup */
    arnm_shutdown();
    
    return 0;
}
