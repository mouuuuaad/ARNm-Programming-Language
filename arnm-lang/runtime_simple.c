
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

/* Runtime Globals */
void* arnm_spawn(void* fn, void* arg) {
    /* Dummy spawn for now, just runs it */
    printf("[Runtime] Spawning actor...\n");
    void (*entry)(void*) = fn;
    entry(arg);
    return NULL;
}

void arnm_receive(void* arg) {
    printf("[Runtime] Receive called (noop for now)\n");
}

void arnm_print_int(int32_t val) {
    printf("%d\n", val);
}

/* Entry point */
extern void _arnm_main();

int main() {
    printf("[Runtime] Starting...\n");
    _arnm_main();
    printf("[Runtime] Done.\n");
    return 0;
}
