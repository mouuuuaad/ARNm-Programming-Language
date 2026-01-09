# ARNm Minimal Executable Core Specification

> **Day 3 Deliverable**: This document defines the irreducible set of constructs, data structures, and runtime behaviors required for any ARNm program to execute. Nothing here is optional.

---

## Table of Contents

1. [Minimal Language Constructs](#1-minimal-language-constructs)
2. [Instruction Representation](#2-instruction-representation)
3. [Runtime Data Structures](#3-runtime-data-structures)
4. [Process Lifecycle](#4-process-lifecycle)
5. [Message Format and Flow](#5-message-format-and-flow)
6. [Scheduler Behavior](#6-scheduler-behavior)
7. [Compiler Output](#7-compiler-output)
8. [Instruction → State Change Traces](#8-instruction--state-change-traces)

---

## 1. Minimal Language Constructs

For **any** ARNm program to run, these constructs must exist. All others are syntactic sugar or extensions.

### 1.1 Required Syntax

| Construct | Syntax | Purpose |
|-----------|--------|---------|
| **Entry Point** | `fn main() { ... }` | Program start |
| **Variable Binding** | `let x = expr;` | Local storage |
| **Integer Literal** | `42`, `-7` | i64 constants |
| **Arithmetic** | `+`, `-`, `*`, `/`, `%` | Binary ops |
| **Comparison** | `==`, `!=`, `<`, `<=`, `>`, `>=` | Boolean result |
| **Function Call** | `fn_name(args)` | Control transfer |
| **Actor Definition** | `actor Name { let field: Type; ... }` | Stateful entity |
| **Spawn** | `spawn Actor.init()` or `spawn fn()` | Create process |
| **Send** | `target ! value` | Enqueue message |
| **Receive** | `receive { pat => body }` | Dequeue message |
| **Self** | `self` | Current process reference |
| **Print** | `print(expr)` | Debug output (temporary) |

### 1.2 What is NOT Required for MVP

The following are **excluded** from the minimal core (can be added later):

- Pattern matching beyond simple binding (`receive { x => ... }`)
- Selective receive / message filtering
- Timeouts on receive
- Actor supervision / linking
- Garbage collection / cycle detection
- Polymorphism / generics
- Module system / imports
- Closures / captured variables

---

## 2. Instruction Representation

The compiler produces an **Intermediate Representation (IR)** before generating assembly. This is the internal representation of execution.

### 2.1 IR Opcode Set (Minimal)

```text
MEMORY
├── IR_ALLOCA      %r = alloca T              # Stack allocation
├── IR_LOAD        %r = load T, %ptr          # Read from pointer
├── IR_STORE       store T %val, %ptr         # Write to pointer
└── IR_FIELD_PTR   %r = field_ptr %ptr, idx   # Actor field offset

ARITHMETIC
├── IR_ADD         %r = add %a, %b
├── IR_SUB         %r = sub %a, %b
├── IR_MUL         %r = mul %a, %b
├── IR_DIV         %r = div %a, %b
└── IR_MOD         %r = mod %a, %b

COMPARISON
├── IR_EQ          %r = eq %a, %b
├── IR_NE          %r = ne %a, %b
├── IR_LT          %r = lt %a, %b
├── IR_LE          %r = le %a, %b
├── IR_GT          %r = gt %a, %b
└── IR_GE          %r = ge %a, %b

CONTROL FLOW
├── IR_RET         ret %val                   # Return from function
├── IR_RET_VOID    ret void                   # Return nothing
├── IR_BR          br %cond, %then, %else     # Conditional branch
├── IR_JMP         jmp %block                 # Unconditional jump
└── IR_CALL        %r = call @fn(%args...)    # Function call

ACTOR OPERATIONS
├── IR_SPAWN       %r = spawn @fn, state_size # Create new process
├── IR_SEND        send %target, %tag, %data  # Enqueue message
├── IR_RECEIVE     %r = receive               # Block until message
└── IR_SELF        %r = self                  # Get current process
```

### 2.2 IR Value Types

```c
typedef enum {
    VAL_VAR,    // SSA variable: %1, %2, ...
    VAL_CONST,  // Constant: 42, true
    VAL_GLOBAL, // Symbol: @main, @Counter_init
    VAL_UNDEF   // Undefined (error state)
} IrValueKind;
```

### 2.3 IR Type System

```c
typedef enum {
    IR_VOID,     // No value
    IR_BOOL,     // true/false
    IR_I32,      // 32-bit signed integer
    IR_I64,      // 64-bit signed integer
    IR_PTR,      // Generic pointer
    IR_PROCESS   // Process reference (ArnmProcess*)
} IrTypeKind;
```

---

## 3. Runtime Data Structures

These structures **must exist** for execution to proceed.

### 3.1 Process (`ArnmProcess`)

The atomic unit of execution. Size: ~144 bytes (platform-dependent).

```c
typedef struct ArnmProcess {
    void*           actor_state;    // Pointer to heap-allocated actor state
    uint64_t        pid;            // Globally unique process ID
    ProcState       state;          // READY | RUNNING | WAITING | DEAD
    ArnmContext     context;        // Saved CPU registers (64 bytes)
    void*           stack_base;     // mmap'd stack memory
    size_t          stack_size;     // Default: 64KB
    ArnmMailbox*    mailbox;        // Incoming message queue
    ArnmProcess*    next;           // Run queue linkage
    uint32_t        worker_id;      // Assigned worker thread
    uint64_t        run_count;      // Scheduling statistics
} ArnmProcess;
```

### 3.2 Process States

```
┌───────────────────────────────────────────────────────────────────┐
│                        PROCESS STATE MACHINE                       │
├───────────────────────────────────────────────────────────────────┤
│                                                                    │
│    spawn()                                                         │
│      │                                                             │
│      ▼                                                             │
│   ┌──────┐    sched_next()    ┌─────────┐                         │
│   │ READY │ ───────────────► │ RUNNING │                          │
│   └──────┘                    └─────────┘                          │
│      ▲                           │  │                              │
│      │                           │  │                              │
│      │    yield()                │  │  receive() with no message  │
│      └───────────────────────────┘  │                              │
│                                     ▼                              │
│                               ┌─────────┐                          │
│                               │ WAITING │                          │
│                               └─────────┘                          │
│                                     │                              │
│                                     │ message arrives              │
│                                     │    (→ READY, re-enqueue)     │
│                                     ▼                              │
│                                ┌──────┐                            │
│       fn returns ──────────►  │ DEAD │                             │
│                                └──────┘                            │
│                                     │                              │
│                                     │ proc_destroy()               │
│                                     ▼                              │
│                                 [freed]                            │
│                                                                    │
└───────────────────────────────────────────────────────────────────┘
```

### 3.3 Context (`ArnmContext`)

Saved CPU state for context switching. x86_64 System V ABI.

```c
typedef struct ArnmContext {
    uint64_t rsp;   // Stack pointer
    uint64_t rbp;   // Base pointer
    uint64_t rbx;   // Callee-saved
    uint64_t r12;   // Stores: arg for entry
    uint64_t r13;   // Stores: entry function pointer
    uint64_t r14;   // Callee-saved
    uint64_t r15;   // Callee-saved
    uint64_t rip;   // Return address (unused; implicit via stack)
} ArnmContext;
```

### 3.4 Message (`ArnmMessage`)

The unit of inter-process communication. Size: 32 bytes.

```c
typedef struct ArnmMessage {
    uint64_t         tag;   // User-defined message type (currently i64)
    void*            data;  // Pointer to payload (can be NULL)
    size_t           size;  // Payload size in bytes
    ArnmMessage*     next;  // MPSC queue linkage
} ArnmMessage;
```

**Critical invariant**: Message data is **COPIED** on send. The sender's memory is not shared.

### 3.5 Mailbox (`ArnmMailbox`)

Lock-free MPSC (Multiple Producer, Single Consumer) queue.

```c
typedef struct ArnmMailbox {
    _Atomic(ArnmMessage*)  head;    // Dequeue point
    _Atomic(ArnmMessage*)  tail;    // Enqueue point
    atomic_size_t          count;   // Message count
} ArnmMailbox;
```

**Implementation**: Uses a dummy node sentinel for lock-free operation.

### 3.6 Scheduler (`Scheduler`)

Global coordinator of all workers and processes.

```c
typedef struct {
    ArnmWorker*     workers;       // Array of worker threads
    uint32_t        num_workers;   // Auto-detected or specified
    RunQueue        global_queue;  // Overflow queue
    atomic_bool     shutdown;      // Termination signal
    atomic_size_t   active_procs;  // Living process count
} Scheduler;
```

### 3.7 Worker (`ArnmWorker`)

OS thread executing processes.

```c
typedef struct ArnmWorker {
    pthread_t       thread;         // OS thread handle
    uint32_t        id;             // Worker ID (0..N-1)
    ArnmProcess*    current;        // Currently running process
    RunQueue        local_queue;    // Work-stealing queue
    ArnmContext     scheduler_ctx;  // Return point after yield
    atomic_bool     running;        // Active flag
} ArnmWorker;
```

---

## 4. Process Lifecycle

### 4.1 Process Creation (`spawn`)

```text
SOURCE:     let p = spawn Actor.init();

COMPILED TO:
    movq $Actor_init, %rdi    # Entry function
    movq $0, %rsi             # Argument (NULL)
    movq $8, %rdx             # Actor state size (bytes)
    call arnm_spawn
    movq %rax, -8(%rbp)       # Store ArnmProcess* in local

RUNTIME EFFECT:
    1. proc_create(entry=Actor_init, arg=NULL, stack=64KB, state_size=8)
       ├── malloc(sizeof(ArnmProcess)) → proc
       ├── proc->pid = atomic_fetch_add(&next_pid, 1)
       ├── proc->state = READY
       ├── stack_alloc(64KB) → proc->stack_base
       ├── proc->actor_state = malloc(8)    // if state_size > 0
       ├── arnm_context_init(proc->context, stack_top, entry, arg)
       │   └── Sets r12=arg, r13=entry, rsp=stack_top-8
       └── mailbox_create() → proc->mailbox
    2. sched_enqueue(proc)
       ├── active_procs++
       └── runqueue_push(&global_queue, proc)
    3. Return proc pointer
```

### 4.2 Process Blocking (`receive`)

```text
SOURCE:     receive { msg => body }

COMPILED TO:
    call arnm_receive
    movq %rax, -8(%rbp)       # ArnmMessage* stored

RUNTIME EFFECT (arnm_receive):
    loop:
        msg = mailbox_try_receive(proc->mailbox)
        if (msg != NULL):
            return msg
        proc->state = WAITING
        arnm_sched_yield()    # Context switch to scheduler
        goto loop             # Eventually woken up

WAKE-UP MECHANISM:
    When sender calls arnm_send(target, ...):
        mailbox_send(target->mailbox, ...)
        if (target->state == WAITING):
            target->state = READY
            sched_enqueue(target)    # Back in run queue
```

### 4.3 Process Termination

```text
When entry function returns:
    1. Control returns to context_entry_wrapper (assembly)
    2. Wrapper calls arnm_process_exit_handler()
    3. Handler calls proc_exit(proc)  → proc->state = DEAD
    4. Handler calls arnm_sched_yield()
    5. Scheduler sees DEAD state:
       ├── active_procs--
       └── proc_destroy(proc)
           ├── mailbox_destroy()
           ├── free(actor_state)
           ├── stack_free(munmap)
           └── free(proc)
```

---

## 5. Message Format and Flow

### 5.1 Message at Runtime

Every message is represented as:

```text
┌────────────────────────────────────────┐
│ ArnmMessage                            │
├────────────────────────────────────────┤
│ tag:  uint64_t = sender-chosen value   │
│ data: void*    → heap-allocated copy   │
│ size: size_t   = byte count of data    │
│ next: ArnmMessage* = queue link        │
└────────────────────────────────────────┘
         │
         ▼
    ┌─────────────────┐
    │ Payload (copy)  │
    │ [size bytes]    │
    └─────────────────┘
```

### 5.2 Send Operation

```text
SOURCE:     target ! 42

COMPILED TO:
    movq -8(%rbp), %rdi       # target: ArnmProcess*
    movq $42, %rsi            # tag: 42 (message value)
    movq $0, %rdx             # data: NULL (no payload)
    movq $0, %rcx             # size: 0
    call arnm_send

RUNTIME EFFECT (arnm_send):
    1. msg = message_create(tag=42, data=NULL, size=0)
       ├── malloc(sizeof(ArnmMessage))
       └── If data != NULL: memcpy(msg->data, data, size)
    2. mailbox_send(target->mailbox, msg)
       ├── ATOMIC: msg->next = NULL
       ├── ATOMIC: prev = xchg(&tail, msg)
       └── ATOMIC: prev->next = msg
    3. If target->state == WAITING:
       ├── target->state = READY
       └── sched_enqueue(target)
```

### 5.3 Receive Operation

```text
SOURCE:     receive { x => body }

RUNTIME EFFECT (arnm_receive):
    1. msg = mailbox_try_receive(mbox)
       ├── ATOMIC: if (head->next == NULL) return NULL
       ├── ATOMIC: old_head = head
       ├── ATOMIC: head = head->next
       └── free(old_head)  // Remove dummy
    2. If msg == NULL:
       ├── proc->state = WAITING
       ├── arnm_sched_yield()
       └── goto 1  // Retry after wake-up
    3. Return msg to compiled code
    4. Compiled code binds msg->tag to variable x
    5. Compiled code executes body
    6. message_free(msg)  // Caller responsibility
```

---

## 6. Scheduler Behavior

### 6.1 Minimal Scheduler Algorithm

```text
FOR EACH WORKER (worker_main):
    tls_worker = worker    # Thread-local storage

    WHILE NOT shutdown:
        proc = sched_next(worker)
            ├── TRY: runqueue_pop(&worker->local_queue)
            ├── TRY: runqueue_pop(&global_queue)
            └── TRY: steal from random worker
        
        IF proc != NULL:
            worker->current = proc
            proc->state = RUNNING
            proc_set_current(proc)
            
            arnm_context_switch(&worker->scheduler_ctx, &proc->context)
            # ─────────────────────────────────────────────────────
            # PROCESS NOW EXECUTING ON THIS THREAD
            # Control returns here when process yields/dies
            # ─────────────────────────────────────────────────────
            
            proc_set_current(NULL)
            worker->current = NULL
            
            IF proc->state == DEAD:
                proc_destroy(proc)
        ELSE:
            IF active_procs == 0:
                BREAK    # All processes done
            usleep(100)  # Backoff, no work
```

### 6.2 Yield Behavior

```text
arnm_sched_yield():
    worker = sched_current_worker()  # From TLS
    proc = worker->current
    
    IF proc->state == READY OR RUNNING:
        proc->state = READY
        runqueue_push(&worker->local_queue, proc)
    ELSE IF proc->state == DEAD:
        active_procs--
    # WAITING state: do NOT re-enqueue (waiting for message)
    
    arnm_context_switch(&proc->context, &worker->scheduler_ctx)
    # Control returns to scheduler loop
```

### 6.3 Termination Condition

```text
The scheduler exits when:
    active_procs == 0 AND global_queue empty AND all local_queues empty

This means:
    - All spawned processes have terminated
    - No messages are pending that could wake processes
    - The program is complete
```

---

## 7. Compiler Output

### 7.1 Compilation Pipeline

```text
.arnm source
    │
    ▼ [Lexer]
Tokens
    │
    ▼ [Parser]
AST (Abstract Syntax Tree)
    │
    ▼ [Semantic Analysis]
Typed AST + Symbol Table
    │
    ▼ [IR Generation]
IR Module (functions, blocks, instructions)
    │
    ▼ [x86_64 Code Generation]
.s file (assembly)
    │
    ▼ [Assembler: as]
.o file (object)
    │
    ▼ [Linker: gcc]
Executable (linked with runtime)
```

### 7.2 Generated Assembly Structure

Every compiled ARNm program produces:

```asm
    .text

    # User's main becomes _arnm_main
    .globl _arnm_main
    .type _arnm_main, @function
_arnm_main:
    pushq %rbp
    movq %rsp, %rbp
    subq $STACK_SIZE, %rsp
    # ... compiled instructions ...
    movq %rbp, %rsp
    popq %rbp
    ret

    # Actor init functions
    .globl ActorName_init
ActorName_init:
    # ... init body ...
    call ActorName_behavior    # Falls through to receive loop
    ret

    # Actor behavior (receive loop)
    .globl ActorName_behavior
ActorName_behavior:
    # Infinite loop calling arnm_receive
    jmp .receive_loop
.receive_loop:
    call arnm_receive
    # ... handle message ...
    jmp .receive_loop

    .section .note.GNU-stack,"",@progbits
```

### 7.3 Runtime Function Signatures

The generated assembly calls these runtime functions:

```c
// Process management
ArnmProcess* arnm_spawn(void (*entry)(void*), void* arg, size_t state_size);
void arnm_sched_yield(void);

// Messaging
void arnm_send(ArnmProcess* target, uint64_t tag, void* data, size_t size);
ArnmMessage* arnm_receive(int timeout_ms);  // timeout unused in MVP

// Self reference
ArnmProcess* arnm_self(void);

// Debug output
void arnm_print_int(int64_t value);
```

---

## 8. Instruction → State Change Traces

This section proves we can trace any instruction to its concrete runtime effect.

### 8.1 Trace: `let x = 42;`

```text
SOURCE:     let x = 42;

IR:
    %1 = alloca i64
    store i64 42, %1

ASM:
    subq $16, %rsp            # Stack allocation for x
    movq %rsp, %rax           # Address → rax
    movq %rax, -8(%rbp)       # Save address
    movq $42, %rax            # Value → rax
    movq -8(%rbp), %rbx       # Reload address
    movq %rax, (%rbx)         # Store 42

STATE CHANGE:
    BEFORE: Stack frame at rbp, rsp at some address
    AFTER:  rsp -= 16, memory at old_rsp contains 42
    No runtime functions called. Pure register/memory manipulation.
```

### 8.2 Trace: `spawn Counter.init()`

```text
SOURCE:     let c = spawn Counter.init();

IR:
    %1 = spawn @Counter_init, state_size=8
    %2 = alloca ptr
    store ptr %1, %2

ASM:
    movq $Counter_init, %rdi
    movq $0, %rsi
    movq $8, %rdx
    call arnm_spawn
    movq %rax, -8(%rbp)

STATE CHANGE:
    BEFORE:
        g_scheduler.active_procs = N
        Process table has M processes
    AFTER:
        g_scheduler.active_procs = N+1
        New ArnmProcess allocated:
            pid = M+1
            state = READY
            actor_state = malloc'd 8 bytes
            stack = mmap'd 64KB
            mailbox = initialized with dummy node
        Process pointer in global_queue
        Local variable c holds ArnmProcess*
```

### 8.3 Trace: `c ! 10`

```text
SOURCE:     c ! 10;

IR:
    %1 = load ptr, %c_addr
    send %1, tag=10, data=NULL, size=0

ASM:
    movq -8(%rbp), %rdi       # Load c (ArnmProcess*)
    movq $10, %rsi            # Tag = 10
    movq $0, %rdx             # Data = NULL
    movq $0, %rcx             # Size = 0
    call arnm_send

STATE CHANGE:
    BEFORE:
        c->mailbox contains M messages
        c->state could be READY, RUNNING, or WAITING
    AFTER:
        c->mailbox contains M+1 messages
        New ArnmMessage node:
            tag = 10
            data = NULL
            size = 0
            next = NULL
        If c->state was WAITING:
            c->state = READY
            c is now in some run queue
```

### 8.4 Trace: `receive { x => body }`

```text
SOURCE:     receive { x => print(x); }

IR:
    %1 = receive
    %2 = field_ptr %1, 0      # msg->tag
    %3 = load i64, %2
    call @arnm_print_int(%3)

ASM:
    call arnm_receive
    movq %rax, -8(%rbp)       # Save ArnmMessage*
    movq -8(%rbp), %rax
    movq (%rax), %rbx         # Load msg->tag
    movq %rbx, %rdi
    call arnm_print_int

STATE CHANGE IF MESSAGE AVAILABLE:
    BEFORE:
        proc->mailbox.count = N (N > 0)
        proc->state = RUNNING
    AFTER:
        proc->mailbox.count = N-1
        Local variable x = msg->tag
        Print output: "10\n"

STATE CHANGE IF NO MESSAGE:
    BEFORE:
        proc->mailbox.count = 0
        proc->state = RUNNING
    AFTER (before context switch):
        proc->state = WAITING
        proc is NOT in any run queue
    THEN:
        Context switches to scheduler
        Scheduler picks another process
        Eventually, when message arrives:
            proc->state = READY
            proc re-enters run queue
            proc resumes from receive
```

---

## Summary Checklist

| Component | Defined? | Location |
|-----------|----------|----------|
| Process struct | ✅ | `runtime/include/process.h` |
| Process states | ✅ | READY, RUNNING, WAITING, DEAD |
| Message struct | ✅ | `runtime/include/mailbox.h` |
| Message storage | ✅ | Heap-allocated, MPSC queue |
| Message consumption | ✅ | mailbox_receive, copied to stack |
| Process blocking | ✅ | state=WAITING, yield to scheduler |
| Process resuming | ✅ | sender sets READY, sched_enqueue |
| Wake-up event | ✅ | mailbox_send detects WAITING |
| Scheduler algorithm | ✅ | Local queue → global → steal |
| Compiler output | ✅ | `.s` → `.o` → linked executable |
| IR opcodes | ✅ | 22 opcodes defined |
| Runtime API | ✅ | spawn, send, receive, self, yield |

---

*Created: 2026-01-09 (Day 3)*
