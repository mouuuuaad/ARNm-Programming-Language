/*
 * ARNm Runtime - Context Switching
 */

#ifndef ARNM_CONTEXT_H
#define ARNM_CONTEXT_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================
 * Context Structure (x86_64)
 * ============================================================
 * Stores callee-saved registers per System V AMD64 ABI.
 */

#if defined(__x86_64__) || defined(_M_X64)

typedef struct ArnmContext {
    uint64_t rsp;       /* Stack pointer */
    uint64_t rbp;       /* Base pointer */
    uint64_t rbx;       /* Callee-saved */
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rip;       /* Return address / entry point */
} ArnmContext;

#elif defined(__aarch64__) || defined(_M_ARM64)

typedef struct ArnmContext {
    uint64_t sp;        /* Stack pointer */
    uint64_t x19;       /* Callee-saved x19-x29 */
    uint64_t x20;
    uint64_t x21;
    uint64_t x22;
    uint64_t x23;
    uint64_t x24;
    uint64_t x25;
    uint64_t x26;
    uint64_t x27;
    uint64_t x28;
    uint64_t x29;       /* Frame pointer */
    uint64_t x30;       /* Link register (return address) */
} ArnmContext;

#else
#error "Unsupported architecture for context switching"
#endif

/* ============================================================
 * Context Operations (implemented in assembly)
 * ============================================================ */

/*
 * Initialize a context for a new process.
 * 
 * @param ctx       Context to initialize
 * @param stack_top Top of the allocated stack
 * @param entry     Entry point function
 * @param arg       Argument to pass to entry
 */
void arnm_context_init(ArnmContext* ctx, void* stack_top, 
                       void (*entry)(void*), void* arg);

/*
 * Switch from current context to new context.
 * Saves current state to 'from', restores state from 'to'.
 * 
 * @param from  Context to save current state
 * @param to    Context to switch to
 */
void arnm_context_switch(ArnmContext* from, ArnmContext* to);

/*
 * Start executing a context (does not save current state).
 * Used for initial process startup.
 * 
 * @param ctx   Context to start
 */
void arnm_context_start(ArnmContext* ctx);

#endif /* ARNM_CONTEXT_H */
