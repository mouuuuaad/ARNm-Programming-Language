/*
 * ARNm Runtime - Basic Test
 * 
 * Tests runtime initialization and shutdown.
 */

#include "../include/arnm.h"
#include <stdio.h>
#include <assert.h>

int main(void) {
    printf("Testing runtime initialization...\n");
    
    /* Initialize with 2 workers */
    int ret = arnm_init(2);
    assert(ret == 0);
    printf("  arnm_init: OK\n");
    
    /* Shutdown */
    arnm_shutdown();
    printf("  arnm_shutdown: OK\n");
    
    printf("\nBasic test passed!\n");
    return 0;
}
