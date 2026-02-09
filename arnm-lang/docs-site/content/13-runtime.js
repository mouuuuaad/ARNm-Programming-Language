// Section 13: Runtime Internals
// How the ARNm runtime works

window.addDocSection({
    id: 'runtime',
    section: 'Internals',
    level: 1,
    title: 'Runtime',
    content: `
# Runtime Internals

The ARNm runtime provides process management, scheduling, and messaging.

---

## Runtime Components

| Component | Role |
|-----------|------|
| **Scheduler** | Coordinates worker threads, manages run queues |
| **Workers** | OS threads that execute processes |
| **Processes** | Lightweight execution units (actors) |
| **Mailboxes** | Message queues for each process |
| **Context Switcher** | Saves/restores CPU state |

---

## Process Structure

Each process has:

\`\`\`
ArnmProcess
├── pid           // Unique process ID
├── state         // READY, RUNNING, WAITING, DEAD
├── actor_state   // Heap-allocated actor fields
├── stack_base    // Process stack (64KB)
├── mailbox       // Message queue
├── context       // Saved CPU registers
└── next          // Queue linkage
\`\`\`

---

## Process States

\`\`\`
       spawn()
          │
          ▼
      ┌───────┐   schedule   ┌─────────┐
      │ READY │ ───────────► │ RUNNING │
      └───────┘              └─────────┘
          ▲                      │  │
          │     yield()          │  │ receive()
          └──────────────────────┘  │ (no msg)
                                    ▼
                              ┌─────────┐
                              │ WAITING │
                              └─────────┘
                                    │
                                    │ message arrives
                                    ▼
                                 ┌────┐
          function returns  ───► │DEAD│
                                 └────┘
\`\`\`

---

## Scheduler Algorithm

\`\`\`
FOR EACH WORKER:
    WHILE running:
        1. Try local queue
        2. Try global queue
        3. Try stealing from other workers
        4. If no work, sleep briefly
        
        IF found process:
            - Set state = RUNNING
            - Context switch to process
            - Process executes
            - Process yields/returns
            - Handle state (re-queue or destroy)
\`\`\`

---

## Runtime API

Functions called from generated assembly:

| Function | Purpose |
|----------|---------|
| \`arnm_spawn()\` | Create new process |
| \`arnm_send()\` | Send message to process |
| \`arnm_receive()\` | Wait for message |
| \`arnm_self()\` | Get current process reference |
| \`arnm_sched_yield()\` | Yield to scheduler |
| \`arnm_print_int()\` | Debug output |

---

## Context Switching

The runtime saves/restores these registers:

| Register | Purpose |
|----------|---------|
| RSP | Stack pointer |
| RBP | Base pointer |
| RBX | Callee-saved |
| R12-R15 | Callee-saved |

When a process yields:
1. Save current registers to process context
2. Load scheduler's saved context
3. Scheduler picks next process
4. Load that process's context
5. Resume execution

---

## Memory Layout

\`\`\`
Per-Process Memory:

┌─────────────────────┐  High Address
│    Stack (64KB)     │  ← RSP starts here
│         ↓           │
│    (grows down)     │
├─────────────────────┤
│    Guard Page       │  ← Catches overflow
├─────────────────────┤  Low Address
│                     │

Heap Memory:

┌─────────────────────┐
│   Actor State       │  ← self.field access
├─────────────────────┤
│   Messages          │  ← Copied on send
├─────────────────────┤
│   Mailbox Nodes     │
└─────────────────────┘
\`\`\`

---

## Mailbox Implementation

Lock-free MPSC (Multiple Producer, Single Consumer) queue:

\`\`\`
Send (any thread):
    1. Allocate message node
    2. Copy data into node
    3. Atomic: swap tail pointer
    4. Link previous tail

Receive (owner only):
    1. Check head->next
    2. If nil: block (set WAITING)
    3. If message: dequeue atomically
    4. Return message to caller
\`\`\`

---

## Building the Runtime

\`\`\`bash
cd runtime
make clean
make
\`\`\`

Produces:
- \`build/libarnm.a\` — Static library
- \`build/crt0.o\` — Entry point
`
});
