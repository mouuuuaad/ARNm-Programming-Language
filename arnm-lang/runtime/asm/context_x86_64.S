/*
 * ARNm Runtime - x86_64 Context Switching
 * 
 * System V AMD64 ABI:
 * - Callee-saved: rbx, rbp, r12-r15
 * - Caller-saved: rax, rcx, rdx, rsi, rdi, r8-r11
 * - Return value: rax
 * - Arguments: rdi, rsi, rdx, rcx, r8, r9
 */

.text
.global arnm_context_switch
.global arnm_context_init

/*
 * void arnm_context_switch(ArnmContext* from, ArnmContext* to)
 * 
 * Saves current context to 'from' and restores context from 'to'.
 * Arguments:
 *   rdi = from (ArnmContext*)
 *   rsi = to   (ArnmContext*)
 */
.type arnm_context_switch, @function
arnm_context_switch:
    /* Save callee-saved registers to 'from' context */
    movq %rbx, 16(%rdi)     /* rbx */
    movq %rbp, 8(%rdi)      /* rbp */
    movq %r12, 24(%rdi)     /* r12 */
    movq %r13, 32(%rdi)     /* r13 */
    movq %r14, 40(%rdi)     /* r14 */
    movq %r15, 48(%rdi)     /* r15 */
    
    /* Save stack pointer (after saving regs, before modifying) */
    movq %rsp, 0(%rdi)      /* rsp */
    
    /* Restore callee-saved registers from 'to' context */
    movq 48(%rsi), %r15     /* r15 */
    movq 40(%rsi), %r14     /* r14 */
    movq 32(%rsi), %r13     /* r13 */
    movq 24(%rsi), %r12     /* r12 */
    movq 8(%rsi), %rbp      /* rbp */
    movq 16(%rsi), %rbx     /* rbx */
    movq 0(%rsi), %rsp      /* rsp - restore last */
    
    /* Return to caller (ret pops return address from new stack) */
    ret
.size arnm_context_switch, .-arnm_context_switch

/*
 * void arnm_context_init(ArnmContext* ctx, void* stack_top, 
 *                        void (*entry)(void*), void* arg)
 * 
 * Initialize a context for a new process.
 * Arguments:
 *   rdi = ctx
 *   rsi = stack_top
 *   rdx = entry function
 *   rcx = arg
 */
.type arnm_context_init, @function
arnm_context_init:
    /* Align stack to 16 bytes (required by System V ABI) */
    movq %rsi, %rax
    andq $-16, %rax
    
    /* Reserve space: push fake return address for wrapper */
    subq $8, %rax
    
    /* Store wrapper address as return address on stack */
    leaq context_entry_wrapper(%rip), %r8
    movq %r8, (%rax)
    
    /* Set up context structure */
    movq %rax, 0(%rdi)      /* rsp = aligned stack with return addr */
    movq $0, 8(%rdi)        /* rbp = 0 */
    movq $0, 16(%rdi)       /* rbx = 0 */
    movq %rcx, 24(%rdi)     /* r12 = arg */
    movq %rdx, 32(%rdi)     /* r13 = entry function */
    movq $0, 40(%rdi)       /* r14 = 0 */
    movq $0, 48(%rdi)       /* r15 = 0 */
    
    ret
.size arnm_context_init, .-arnm_context_init

/*
 * Internal wrapper that sets up argument and calls entry function.
 * Called when a new process starts.
 * On entry (from restored r12/r13):
 *   r12 = arg
 *   r13 = entry function
 */
.type context_entry_wrapper, @function
context_entry_wrapper:
    /* Align stack for call (should already be aligned) */
    
    /* Set up argument in rdi */
    movq %r12, %rdi
    
    /* Call entry function */
    callq *%r13
    
    /* Entry returned - process is done, call exit handler */
    xorl %edi, %edi         /* No argument needed */
    callq arnm_process_exit_handler
    
    /* Should not return, but if it does, spin */
1:  pause
    jmp 1b
.size context_entry_wrapper, .-context_entry_wrapper

/* Ensure stack is not executable */
.section .note.GNU-stack,"",@progbits
