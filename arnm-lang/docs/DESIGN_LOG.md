# ARNm DESIGN LOG

> **This document is the authority.** Code must obey it, not the other way around.

## Philosophy

ARNm is a **systems-level organism**, not a syntax experiment. Every decision must be:
- **Reversible** — can remove without breaking the world
- **Observable** — can see what's happening
- **Measurable** — can prove correctness

## Core Metaphor (Biology)

| Concept     | ARNm Element   |
|-------------|----------------|
| Cell        | Runtime        |
| Organelle   | Process        |
| Protein     | Message        |
| Metabolism  | Scheduler      |
| Nutrients   | Memory         |

If a feature does not fit this metaphor → question it.

---

## Runtime Invariants (MUST NEVER BREAK)

### INV-001: Process Isolation
A process cannot directly access another process's stack or state.
Interaction happens ONLY through messages.

### INV-002: Message Ownership
A message is owned by exactly ONE process at any time.
Transfer is explicit. No shared mutable state.

### INV-003: Deterministic Scheduling  
Given the same inputs and random seed, execution order is reproducible.
FIFO scheduling within priority classes.

### INV-004: Graceful Shutdown
Every spawned process must be accounted for at shutdown.
No orphans. No leaks. Clean termination.

### INV-005: Observable State
At any point, the system can report:
- Number of live processes
- Mailbox depths  
- Scheduler queue state

---

## Execution Model (MVP)

```
┌─────────────────────────────────────────────┐
│              ARNm Runtime (OS Process)       │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐      │
│  │ Process │  │ Process │  │ Process │      │
│  │  P1     │  │  P2     │  │  P3     │      │
│  │ [stack] │  │ [stack] │  │ [stack] │      │
│  │ [mbox]  │  │ [mbox]  │  │ [mbox]  │      │
│  └─────────┘  └─────────┘  └─────────┘      │
│                    │                         │
│            ┌───────▼───────┐                 │
│            │   Scheduler   │                 │
│            │ (cooperative) │                 │
│            └───────────────┘                 │
│                    │                         │
│            ┌───────▼───────┐                 │
│            │  Shared Heap  │                 │
│            │   (ARC)       │                 │
│            └───────────────┘                 │
└─────────────────────────────────────────────┘
```

### Why Cooperative (Not Preemptive)?
- Simpler debugging
- Deterministic yielding
- Explicit control points (receive, await, sleep)
- Preemption comes later when correctness is proven

---

## Memory Model (MVP ARC)

Rules:
1. Every heap object has a refcount (uint32_t)
2. Increment on `send` (message enters another's domain)
3. Decrement on `receive` completion (message consumed)
4. Free when refcount reaches zero

NOT in MVP:
- Cycle detection
- Weak references
- Generational GC

But: **API assumes cycles will exist**. Design for future removal.

---

## Process Lifecycle

```
spawn() ──► READY ──► RUNNING ──► WAITING ──► READY ──► ...
                          │                      │
                          └──────► DEAD ◄────────┘
```

States:
- **READY**: In scheduler queue, waiting for CPU
- **RUNNING**: Currently executing
- **WAITING**: Blocked on receive/await
- **DEAD**: Terminated, awaiting cleanup

---

## Design Decisions Log

### 2026-01-09: MVP Scope Lock

**Decision**: Lock MVP scope to:
- actors, spawn, send, receive, print
- Linux x86_64 only
- Cooperative scheduling
- ARC memory

**Rationale**: Prove sovereignty first. Optimize later.

### 2026-01-09: Native x86_64 Backend

**Decision**: Use native x86_64 assembly output instead of LLVM IR for MVP.

**Rationale**:
- Zero external dependencies (no clang/llvm required)
- Complete control over generated code
- GCC available everywhere

### 2026-01-09: Day 4 Language Expansion

**Decision**: Add three features justified by execution needs:
1. **Full arithmetic operators** (`-`, `*`, `/`, `%`)
2. **Boolean literals and logical operators** (`true`, `false`, `&&`, `||`)
3. **Break/continue statements** for loop control

**Rationale**:
- Arithmetic: Only `+` was implemented; basic programs require subtraction and multiplication
- Booleans: `while (true)` patterns needed for infinite loops with break
- Break/continue: No way to exit loops early without workarounds

**Deferred**: Message tag matching in receive blocks (requires more design work)

**Impact on Runtime**: None. All features are compile-time only.

---

## Anti-Patterns (DO NOT DO)

1. **Over-generalization** — Don't build abstractions before you need them 3 times
2. **Rust-level safety** — Not yet. Correctness before provability.
3. **Micro-optimization** — Profile first. Guess never.
4. **Syntax sugar** — Ugly but working > pretty but broken

---

*Last updated: 2026-01-09*

