# ARNm Execution Model: A Complete Mental Walkthrough

> **Purpose**: This document exists to remove vagueness. Every concept that felt fuzzy has been written down and questioned. Every responsibility has been assigned. Read this before touching any code.

---

## Table of Contents

1. [The Chain of Responsibility](#the-chain-of-responsibility)
2. [What Memory Exists at Startup](#what-memory-exists-at-startup)
3. [Data Structures Alive Before First Process](#data-structures-alive-before-first-process)
4. [Who Can Mutate What](#who-can-mutate-what)
5. [Complete Lifecycle of a Trivial Program](#complete-lifecycle-of-a-trivial-program)
6. [Open Questions and Fuzzy Areas](#open-questions-and-fuzzy-areas)

---

## The Chain of Responsibility

There are exactly **three** phases in an ARNm program's life. Each has a clear owner. When one stops thinking, the next starts acting.

### Phase 1: Compilation (Compiler's Responsibility)

**Owner**: `arnmc` (the ARNm compiler)

**Inputs**:
- Source file (`.arnm`)

**Outputs**:
- x86_64 assembly (`.s`) → assembled to object file (`.o`) → linked with runtime to executable

**What the Compiler Decides**:
1. **Syntax correctness** — Is this valid ARNm?
2. **Type correctness** — Do types match? Are actors defined before use?
3. **Name resolution** — What does `self` refer to? What is `x`?
4. **Actor state layout** — How many bytes does an actor's state require?
5. **Function entry points** — What is the symbol name for each function?
6. **`main` → `_arnm_main` renaming** — The user's `main` becomes `_arnm_main`

**What the Compiler Does NOT Decide**:
- Process scheduling order
- Memory allocation strategy
- How many workers to use
- When to yield

**Boundary**: The compiler's job ends when it produces a valid object file. The machine takes over from the linker forward.

---

### Phase 2: Runtime Initialization (Runtime's Responsibility)

**Owner**: `crt0.c` → `runtime.c` → `scheduler.c`

**Trigger**: The OS invokes `main()` in the linked executable.

**What Happens (in exact order)**:

```
OS invokes main(int argc, char** argv)
         │
         ▼
┌─────────────────────────────────────────────────────┐
│ 1. arnm_init(0) is called                           │
│    └─► sched_init(auto-detect workers)              │
│        ├─► Allocate Scheduler (g_scheduler)         │
│        ├─► Allocate ArnmWorker array                │
│        ├─► For each worker:                         │
│        │   ├─► id = i                               │
│        │   ├─► current = NULL                       │
│        │   ├─► running = false (atomic)             │
│        │   └─► runqueue_init (local_queue)          │
│        ├─► runqueue_init (global_queue)             │
│        ├─► active_procs = 0 (atomic)                │
│        └─► shutdown = false (atomic)                │
└─────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────┐
│ 2. arnm_spawn(main_wrapper, NULL, 0) is called      │
│    └─► proc_create(entry, arg, 64KB, 0)             │
│        ├─► malloc(sizeof(ArnmProcess))              │
│        ├─► pid = atomic_fetch_add(&next_pid, 1)     │
│        ├─► state = PROC_STATE_READY                 │
│        ├─► stack_alloc(64KB) via mmap               │
│        │   └─► Adds guard page (PROT_NONE)          │
│        ├─► arnm_context_init(context, stack_top,    │
│        │                     entry, arg)            │
│        │   └─► Sets up fake return to wrapper       │
│        │       Stores entry in r13, arg in r12      │
│        ├─► mailbox_create()                         │
│        │   └─► Creates dummy node for MPSC queue    │
│        └─► Returns ArnmProcess*                     │
│    └─► sched_enqueue(proc)                          │
│        ├─► proc_ready(proc)                         │
│        ├─► active_procs++ (atomic)                  │
│        └─► Push to global_queue (no TLS worker yet) │
└─────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────┐
│ 3. arnm_run() is called                             │
│    └─► sched_run()                                  │
│        ├─► For workers 1..N-1:                      │
│        │   ├─► running = true                       │
│        │   └─► pthread_create → worker_main         │
│        ├─► Worker 0 runs on main thread:            │
│        │   └─► worker_main(&workers[0])             │
│        └─► pthread_join all workers on return       │
└─────────────────────────────────────────────────────┘
```

**Boundary**: Runtime initialization ends when `worker_main` begins the scheduler loop. The scheduler now owns execution.

---

### Phase 3: Scheduling and Execution (Scheduler's Responsibility)

**Owner**: Worker threads executing `worker_main`

**The Scheduler Loop** (per worker):

```
while (!shutdown) {
    proc = sched_next(worker);  ─────────────────┐
                                                  │
    if (proc) {                                   │
        worker->current = proc;  ◄────────────────┘
        proc->state = PROC_STATE_RUNNING;
        proc->run_count++;
        proc_set_current(proc);  // TLS
        
        ┌─────────────────────────────────────────┐
        │ arnm_context_switch(                    │
        │     &worker->scheduler_ctx,             │
        │     &proc->context                      │
        │ )                                       │
        │                                         │
        │ CPU NOW EXECUTES THE PROCESS            │
        │ (Scheduler has yielded control)         │
        └─────────────────────────────────────────┘
        
        // Process returns here after:
        // - yield
        // - receive (blocking)
        // - exit
        
        proc_set_current(NULL);
        worker->current = NULL;
        
        if (proc->state == PROC_STATE_DEAD) {
            proc_destroy(proc);
        }
    } else {
        if (active_procs == 0) break;
        usleep(100);  // No work, backoff
    }
}
```

**How a Process Yields Control**:

When a process calls `arnm_sched_yield()`:

```
arnm_sched_yield() {
    worker = sched_current_worker();  // TLS lookup
    current = worker->current;
    
    if (current->state == READY || RUNNING) {
        current->state = READY;
        runqueue_push(&worker->local_queue, current);
    } else if (current->state == DEAD) {
        active_procs--;  // Atomic decrement
    }
    
    arnm_context_switch(
        &current->context,      // Save here
        &worker->scheduler_ctx  // Resume to scheduler
    );
}
```

**Boundary**: The scheduler gives up control when it context-switches into a process. It regains control when that process (or any process on this worker) yields.

---

## What Memory Exists at Startup

Before the first process executes a single instruction, the following memory is allocated:

### Heap Allocations

| Item | Owner | Size | Purpose |
|------|-------|------|---------|
| `g_scheduler` | Global static | ~64 bytes | Scheduler state (pointers, atomics) |
| `workers` array | Scheduler | `num_workers * sizeof(ArnmWorker)` | Per-worker state |
| `main` process | crt0 | `sizeof(ArnmProcess)` + 0 bytes state | The first ARNm process |
| `main` mailbox | Process | `sizeof(ArnmMailbox)` + dummy node | Message queue |

### mmap Allocations

| Item | Owner | Size | Purpose |
|------|-------|------|---------|
| `main` stack | Process | 64KB + guard page | Stack for _arnm_main |

### Static/Global Memory

| Item | Location | Purpose |
|------|----------|---------|
| `next_pid` | `process.c` | Atomic counter starting at 1 |
| `current_process` | `process.c` | Thread-local, one per worker |
| `tls_worker` | `scheduler.c` | Thread-local, one per worker |
| `g_scheduler` | `scheduler.c` | Global scheduler singleton |

### What is NOT Allocated at Startup

- Actor state (allocated only when `spawn` is called with state_size > 0)
- Messages (allocated only on `send`)
- Additional process stacks (allocated only on `spawn`)

---

## Data Structures Alive Before First Process

At the moment `worker_main` is about to call `sched_next` for the first time, these structures exist:

### 1. Scheduler (`g_scheduler`)

```
┌────────────────────────────────────────────┐
│ Scheduler                                  │
├────────────────────────────────────────────┤
│ workers:      ArnmWorker* ──────────────►  │
│ num_workers:  uint32_t (e.g., 8)           │
│ global_queue: RunQueue {head, tail, lock}  │
│ shutdown:     atomic_bool = false          │
│ active_procs: atomic_size_t = 1            │
└────────────────────────────────────────────┘
```

### 2. Workers Array (`g_scheduler.workers`)

```
┌─────────────────────────────────────────────────────┐
│ ArnmWorker[0] (runs on main thread)                 │
├─────────────────────────────────────────────────────┤
│ thread:       pthread_t (main thread)               │
│ id:           0                                     │
│ current:      NULL (no process running yet)         │
│ local_queue:  RunQueue (empty)                      │
│ scheduler_ctx: ArnmContext (uninitialized)          │
│ running:      atomic_bool = true                    │
│ steal_count:  0                                     │
│ run_count:    0                                     │
└─────────────────────────────────────────────────────┘
[Similar for workers 1..N-1, each on own pthread]
```

### 3. The Main Process (`ArnmProcess`)

```
┌─────────────────────────────────────────────────────┐
│ ArnmProcess (the main actor)                        │
├─────────────────────────────────────────────────────┤
│ actor_state:  NULL (main has no state)              │
│ pid:          1                                     │
│ state:        PROC_STATE_READY                      │
│ context:      ArnmContext {                         │
│                 rsp = stack_top - 8                 │
│                 rbp = 0                             │
│                 rbx = 0                             │
│                 r12 = arg (NULL for main)           │
│                 r13 = entry (main_wrapper)          │
│                 r14 = 0                             │
│                 r15 = 0                             │
│               }                                     │
│ stack_base:   mmap'd region (64KB)                  │
│ stack_size:   65536                                 │
│ mailbox:      ArnmMailbox* ──────────────────────►  │
│ next:         NULL                                  │
│ worker_id:    0                                     │
│ spawn_time:   0 (TODO)                              │
│ run_count:    0                                     │
└─────────────────────────────────────────────────────┘
```

### 4. The Main Mailbox (`ArnmMailbox`)

```
┌─────────────────────────────────────────────────────┐
│ ArnmMailbox                                         │
├─────────────────────────────────────────────────────┤
│ head:  atomic<ArnmMessage*> → dummy node            │
│ tail:  atomic<ArnmMessage*> → dummy node            │
│ count: atomic_size_t = 0                            │
└─────────────────────────────────────────────────────┘
          │
          ▼
    ┌───────────────┐
    │ Dummy Message │
    │ tag = 0       │
    │ data = NULL   │
    │ size = 0      │
    │ next = NULL   │
    └───────────────┘
```

### 5. Global Run Queue

The main process is in the **global queue**, not a local queue (because no worker thread was running when `sched_enqueue` was called):

```
global_queue.head ──► main_process
global_queue.tail ──► main_process
global_queue.count = 1
```

---

## Who Can Mutate What

### Mutation Rules (Invariants)

| Data | Who Can Mutate | When |
|------|----------------|------|
| `proc->state` | Scheduler (worker) | When switching process state |
| `proc->context` | Assembly | During `arnm_context_switch` only |
| `proc->mailbox` | Any process (sender) | Via `mailbox_send` (lock-free, atomic) |
| `proc->actor_state` | Only the owning process | During execution |
| `RunQueue` | Worker (with spinlock) | `runqueue_push`/`runqueue_pop` |
| `active_procs` | Scheduler | On spawn (++), on death (--) |
| `next_pid` | `proc_create` | Atomic increment |
| `shutdown` | Main thread | At termination |

### Isolation Guarantees

1. **A process cannot read/write another process's stack** — stacks are separate mmap regions with guard pages
2. **A process cannot read/write another process's actor_state** — no pointer sharing
3. **Messages are copied** — `mailbox_send` allocates new memory for message data
4. **Ownership transfers on send** — sender loses access (conceptually; no runtime enforcement yet)

### Shared Mutable State (Carefully Controlled)

These are the **only** shared mutable structures:

1. **Run queues** — protected by spinlocks
2. **Mailboxes** — lock-free MPSC (atomic operations)
3. **Global counters** — `active_procs`, `shutdown` (atomic)

---

## Complete Lifecycle of a Trivial Program

Consider this program:

```arnm
fn main() {
    print(42);
}
```

### Step-by-Step Execution (nothing skipped)

**T0: Executable invoked by OS**
```
OS: loads ELF binary
OS: sets up stack for main thread
OS: jumps to _start (glibc) → calls main()
```

**T1: main() in crt0.c**
```
crt0.c:main() called
  └─► arnm_init(0)
      └─► sched_init(auto-detect = 8 workers)
          ├─► calloc(8, sizeof(ArnmWorker)) = 8 workers allocated
          ├─► runqueue_init(&global_queue)
          ├─► for i in 0..7: runqueue_init(&workers[i].local_queue)
          └─► returns 0 (success)
```

**T2: Spawn main process**
```
crt0.c: arnm_spawn(main_wrapper, NULL, 0)
  └─► proc_create(main_wrapper, NULL, 64KB, 0)
      ├─► malloc(ArnmProcess) → pid=1, state=READY
      ├─► stack_alloc(64KB) → mmap 64KB + 4KB guard
      ├─► arnm_context_init:
      │   ├─► Align stack to 16 bytes
      │   ├─► Push context_entry_wrapper address on stack
      │   ├─► Set r12 = NULL (arg), r13 = main_wrapper (entry)
      │   └─► Set rsp to aligned stack
      └─► mailbox_create() → dummy node
  └─► sched_enqueue(proc)
      ├─► proc_ready(proc) → state = READY
      ├─► active_procs++ → now 1
      └─► runqueue_push(&global_queue, proc)
```

**T3: Run scheduler**
```
crt0.c: arnm_run()
  └─► sched_run()
      ├─► for i in 1..7: pthread_create(worker_main)
      └─► worker_main(&workers[0]) // Main thread becomes worker 0
```

**T4: Worker 0 picks up main process**
```
worker_main(worker=&workers[0]):
  tls_worker = worker  // Thread-local storage set
  
  while (!shutdown):
    proc = sched_next(worker)
      ├─► runqueue_pop(&worker->local_queue) → NULL
      ├─► runqueue_pop(&global_queue) → main process!
      └─► returns main process
    
    worker->current = proc
    proc->state = RUNNING
    proc->run_count = 1
    proc_set_current(proc)  // TLS
    
    arnm_context_switch(&worker->scheduler_ctx, &proc->context)
    // ════════════════════════════════════════════════════════
    // CONTROL TRANSFERS TO PROCESS
    // ════════════════════════════════════════════════════════
```

**T5: Process starts executing**
```
CPU now in context_entry_wrapper (assembly):
  movq %r12, %rdi  // arg = NULL
  callq *%r13      // call main_wrapper

main_wrapper(void* arg):
  (void)arg;
  _arnm_main();    // Call compiler-generated entry

_arnm_main (compiled from print(42)):
  pushq %rbp
  movq %rsp, %rbp
  subq $288, %rsp
  
  movq $42, %rdi
  call arnm_print_int  // Prints "42\n"
  
  movq %rbp, %rsp
  popq %rbp
  ret                  // Returns to main_wrapper
```

**T6: main_wrapper returns to context_entry_wrapper**
```
context_entry_wrapper:
  // Entry function returned
  callq arnm_process_exit_handler

arnm_process_exit_handler:
  proc = proc_current()  // Get this process via TLS
  proc_exit(proc)        // proc->state = DEAD
  arnm_sched_yield()     // Switch back to scheduler
    └─► if (proc->state == DEAD):
            active_procs--  // Now 0
        arnm_context_switch(&proc->context, &worker->scheduler_ctx)
        // ════════════════════════════════════════════
        // CONTROL RETURNS TO SCHEDULER
        // ════════════════════════════════════════════
```

**T7: Scheduler handles dead process**
```
worker_main (continues after context_switch):
  proc_set_current(NULL)
  worker->current = NULL
  
  if (proc->state == DEAD):
    proc_destroy(proc)
      ├─► mailbox_destroy(mailbox)
      ├─► free(actor_state)  // NULL, no-op
      ├─► stack_free(stack_base, stack_size)  // munmap
      └─► free(proc)
```

**T8: Worker checks for more work**
```
  proc = sched_next(worker)
    ├─► local_queue → empty
    ├─► global_queue → empty
    └─► try_steal → all workers empty
    └─► returns NULL
  
  if (active_procs == 0):  // true, it's 0
    break;  // Exit worker loop
```

**T9: Workers join**
```
sched_run():
  worker_main returns
  for i in 1..7:
    pthread_join(workers[i].thread, NULL)
  returns
```

**T10: Shutdown**
```
crt0.c: arnm_shutdown()
  └─► sched_shutdown()
      ├─► shutdown = true
      ├─► pthread_join for any still-running workers
      ├─► runqueue_destroy for all queues
      └─► free(workers)
      └─► memset(&g_scheduler, 0, sizeof)

crt0.c: return 0;
// Program exits
```

---

## Open Questions and Fuzzy Areas

These are concepts that remain unclear or need future work:

### 1. Waiting Process Wake-Up
**Current**: A waiting process spins in `mailbox_receive`, yielding repeatedly until a message arrives.

**Problem**: This is inefficient. The waiting process is re-enqueued even though it has no message.

**Question**: Should we have a "parked" state where waiting processes are NOT in any run queue? Who wakes them?

### 2. Message Ownership Enforcement
**Current**: Ownership is conceptual. After `send`, the sender still has a pointer to the data (if they kept it).

**Question**: Do we copy on send (current), or do we move and NULL the sender's pointer? How do we enforce this at compile time?

### 3. Cycle Detection
**Current**: ARC cannot detect cycles. Two actors referencing each other will leak.

**Question**: Is this acceptable for MVP? When do we add cycle detection or weak references?

### 4. Error Propagation
**Current**: If a process panics or crashes, it dies silently. No linked processes are notified.

**Question**: Do we want Erlang-style links/monitors? Who implements this?

### 5. Deadlock Detection
**Current**: If all processes are waiting and no messages exist, the scheduler will spin forever (usleep loop).

**Question**: Should we detect this and terminate? Should we call it an error?

### 6. Self Reference
**Current**: `arnm_self()` returns the process pointer.

**Question**: What happens if a process sends a message to itself? Is this valid? (Currently works, but is it intended?)

### 7. spawn_time Field
**Current**: `spawn_time` is set to 0 with a TODO comment.

**Question**: Should this use `clock_gettime`? Which clock? Is this for debugging only or observable by user code?

---

## Summary: The Three Owners

| Phase | Owner | When It Ends |
|-------|-------|--------------|
| **Compilation** | `arnmc` | Object file produced |
| **Initialization** | `crt0.c` + runtime | `worker_main` starts looping |
| **Execution** | Scheduler + Processes | `active_procs == 0` |

If you can narrate this lifecycle without looking at the code, you understand ARNm. If any part feels fuzzy, re-read it until it doesn't.

---

*Created: 2026-01-09 (Day 2)*
*Last updated: 2026-01-09*
