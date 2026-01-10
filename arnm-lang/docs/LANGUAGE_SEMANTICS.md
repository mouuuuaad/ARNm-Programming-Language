# ARNm Language Semantics

> **Day 5 Deliverable**: This document formalizes how ARNm constructs interact, removes inconsistencies, and defines predictable behavior under all conditions.

---

## Table of Contents

1. [Design Goals](#1-design-goals)
2. [Scope and Binding Rules](#2-scope-and-binding-rules)
3. [Evaluation Order](#3-evaluation-order)
4. [Lifetime Rules](#4-lifetime-rules)
5. [Communication Semantics](#5-communication-semantics)
6. [Error Model](#6-error-model)
7. [Coherence Rules](#7-coherence-rules)
8. [Consistency Examples](#8-consistency-examples)

---

## 1. Design Goals

ARNm must be **coherent**: the same program always behaves the same way regardless of how it is written. This requires:

| Property | Meaning |
|----------|---------|
| **Determinism** | Given identical inputs, produce identical outputs |
| **Totality** | Every construct has defined behavior in every context |
| **Composability** | Features combine without hidden interactions |
| **Locality** | Changes to one part don't break unrelated parts |

### What This Document Defines

- How scopes nest and what names are visible where
- The exact order in which subexpressions are evaluated
- When values are created, when they die, and who owns them
- How processes communicate without data races
- What happens when things go wrong

---

## 2. Scope and Binding Rules

### 2.1 Lexical Scope

ARNm uses **lexical (static) scoping**. A name is resolved to the nearest enclosing definition.

```text
SCOPE HIERARCHY:

Global Scope
├── Function "main" → creates Function Scope
│   ├── Block → creates Block Scope
│   │   ├── let x = 1;  ← "x" visible in this block and nested blocks
│   │   └── { let y = 2; }  ← "y" visible only in inner block
│   └── "x" still visible here
└── Actor "Counter" → creates Actor Scope
    ├── Field "count" → visible via self.count in methods
    ├── Method "init" → Function Scope (self visible)
    └── receive block → Function Scope (self visible)
```

### 2.2 Scope Rules (Formalized)

| Rule ID | Rule | Violation |
|---------|------|-----------|
| **SCOPE-001** | A name must be defined before use | Compile error: "undefined identifier" |
| **SCOPE-002** | A name cannot be redefined in the same scope | Compile error: "duplicate definition" |
| **SCOPE-003** | An inner scope may shadow an outer scope's name | Legal (warning in strict mode) |
| **SCOPE-004** | `self` is only valid inside an actor | Compile error: "'self' used outside of actor" |
| **SCOPE-005** | `break`/`continue` are only valid inside loops | Compile error: "break/continue outside loop" |

### 2.3 Name Resolution Algorithm

```text
resolve(name, scope):
    if scope.defines(name):
        return scope.get(name)
    else if scope.parent != null:
        return resolve(name, scope.parent)
    else:
        error("undefined identifier: " + name)
```

### 2.4 Actor Field Access

Actor fields are accessed exclusively through `self`:

```arnm
actor Counter {
    let count: i32;  // Field
    
    fn init() {
        self.count = 0;  // ✓ Valid: field access via self
        count = 0;       // ✗ Error: "count" is not in local scope
    }
}
```

**Rationale**: Explicit `self` prevents confusion between local variables and fields, and makes mutation visible.

---

## 3. Evaluation Order

ARNm uses **strict left-to-right evaluation** for all expressions. This is critical for determinism.

### 3.1 Rules

| Rule ID | Rule |
|---------|------|
| **EVAL-001** | In `a OP b`, evaluate `a` fully before `b` |
| **EVAL-002** | In `f(a, b, c)`, evaluate `a`, then `b`, then `c`, then call `f` |
| **EVAL-003** | In `a.field`, evaluate `a` first, then access field |
| **EVAL-004** | In `a ! b`, evaluate `a` (target), then `b` (message), then send |
| **EVAL-005** | Short-circuit: `&&` and `||` evaluate right side only if needed |

### 3.2 Operator Precedence (Lowest to Highest)

```text
1.  =           (assignment, right-associative)
2.  ||          (logical or)
3.  &&          (logical and)
4.  == !=       (equality)
5.  < <= > >=   (comparison)
6.  !           (message send) ← NOTE: lower than arithmetic
7.  + -         (additive)
8.  * / %       (multiplicative)
9.  - ! ~       (unary prefix)
10. () [] .     (call, index, field)
```

### 3.3 Example: Order Matters

```arnm
fn side_effect() -> i32 {
    print(1);
    return 42;
}

fn main() {
    let x = side_effect() + side_effect();
    // Always prints: 1, then 1
    // x = 84
}
```

**Guarantee**: Side effects occur in a predictable, left-to-right order.

### 3.4 Statement Execution Order

Within a block, statements execute top-to-bottom, sequentially:

```arnm
{
    let a = 1;      // Step 1
    let b = a + 1;  // Step 2 (a is defined)
    print(b);       // Step 3
}
```

---

## 4. Lifetime Rules

### 4.1 Value Categories

| Category | Description | Example |
|----------|-------------|---------|
| **Stack value** | Lives on the process's stack | Local `let` bindings |
| **Heap value** | Lives on the shared heap | Actor state, messages |
| **Process-local** | Tied to a single process | Stack, context |
| **Transferable** | Can move between processes | Messages |

### 4.2 Lifetime Rules

| Rule ID | Rule |
|---------|------|
| **LIFE-001** | A local variable lives until the end of its scope |
| **LIFE-002** | Actor state lives until the owning process terminates |
| **LIFE-003** | A message's data is copied on send; the copy lives until `message_free` |
| **LIFE-004** | A process reference (`Process` type) is valid until the process is destroyed |
| **LIFE-005** | References to dead processes are undefined (no runtime check in MVP) |

### 4.3 Actor State Lifetime

```text
┌───────────────────────────────────────────────────────────────────┐
│ ACTOR LIFECYCLE                                                    │
├───────────────────────────────────────────────────────────────────┤
│                                                                    │
│  spawn Counter.init()                                              │
│       │                                                            │
│       ├── 1. Allocate ArnmProcess                                  │
│       ├── 2. Allocate actor_state (sizeof Counter fields)         │
│       ├── 3. Call Counter_init (initializes fields)               │
│       └── 4. Enqueue to scheduler                                  │
│                                                                    │
│  [process runs, handles messages...]                               │
│                                                                    │
│  init() or behavior() returns                                      │
│       │                                                            │
│       ├── 1. proc_exit → state = DEAD                              │
│       ├── 2. yield to scheduler                                    │
│       └── 3. proc_destroy:                                         │
│             ├── free(actor_state) ← FIELDS GONE                    │
│             ├── mailbox_destroy                                    │
│             ├── stack_free                                         │
│             └── free(process)                                      │
│                                                                    │
└───────────────────────────────────────────────────────────────────┘
```

### 4.4 Dangling Process Reference

```arnm
fn main() {
    let p = spawn SomeActor.init();
    // p is valid here
    
    // ... time passes, SomeActor terminates ...
    
    p ! 42;  // UNDEFINED: p points to dead process
}
```

**MVP Behavior**: Sending to a dead process may crash, silently fail, or corrupt memory. No runtime check.

**Future Work**: Process monitors detect termination and notify interested parties.

---

## 5. Communication Semantics

### 5.1 Message Passing Properties

| Property | Guarantee |
|----------|-----------|
| **Isolation** | Processes share no mutable state except through messages |
| **Copy semantics** | Message data is deep-copied on send |
| **FIFO** | Messages from A to B arrive in the order sent |
| **Asynchronous** | `send` returns immediately; does not wait for delivery |
| **Non-blocking send** | `send` never blocks the sender |
| **Blocking receive** | `receive` blocks until a message is available |

### 5.2 Send Semantics

```text
target ! value

1. Evaluate `target` → ArnmProcess* T
2. Evaluate `value` → i64 V (or struct, etc.)
3. Allocate new ArnmMessage M
4. M.tag = V (for simple values) or copy struct into M.data
5. Atomically enqueue M into T's mailbox
6. If T.state == WAITING:
      T.state = READY
      sched_enqueue(T)
7. Return () (unit type)
```

### 5.3 Receive Semantics

```text
receive { pattern => body }

1. Set current_process.state = RUNNING (already)
2. Try to dequeue from mailbox:
   - If empty:
       a. Set state = WAITING
       b. Yield to scheduler (context switch)
       c. When resumed, goto step 2
   - If message M available:
       a. Dequeue M
       b. Bind pattern variable to M.tag (or M.data)
       c. Execute body
       d. Free message (implicit or explicit)
3. Continue after receive block
```

### 5.4 Self-Send

Sending a message to `self` is **legal** and behaves correctly:

```arnm
actor Echo {
    fn init() {
        self ! 1;       // Message queued in own mailbox
        receive {       // Will receive the 1
            x => print(x);  // Prints: 1
        }
    }
}
```

The message is enqueued atomically. The receive will dequeue it on the next iteration.

---

## 6. Error Model

### 6.1 Error Categories

| Category | Example | Current Behavior | Future Behavior |
|----------|---------|-----------------|-----------------|
| **Compile-time** | Type mismatch | Rejected by compiler | Same |
| **Runtime panic** | Division by zero | Process terminates | Process terminates + optional notification |
| **Dead process** | Send to dead | Undefined | Return error or panic |
| **Mailbox full** | Capacity exceeded | Undefined (MVP: unbounded) | Block or return error |
| **Stack overflow** | Deep recursion | Guard page triggers SIGSEGV | Graceful termination |

### 6.2 Process Termination

When a process terminates (normally or due to panic):

```text
CURRENT (MVP):
1. Entry function returns or panic occurs
2. State set to DEAD
3. Yield to scheduler
4. Scheduler calls proc_destroy
5. Resources freed
6. No notification to any process

FUTURE (with supervision):
1. Entry function returns or panic occurs
2. Exit reason stored: {normal | {panic, reason}}
3. Linked processes receive:  {exit, pid, reason}
4. Monitors receive: {down, pid, reason}
5. Resources freed
```

### 6.3 Error Representation (Future)

```arnm
// Future syntax (not MVP)
type ExitReason = 
    | Normal
    | Panic(String)
    | Killed
    ;

// Link: if A links to B, A dies when B dies (unless trapped)
link(other_process);

// Monitor: if A monitors B, A receives a message when B dies
let ref = monitor(other_process);
receive {
    Down(ref, reason) => handle_death(reason)
}
```

### 6.4 MVP Error Contract

For MVP, we define minimal guarantees:

| Situation | Behavior |
|-----------|----------|
| Process returns normally | `active_procs--`, resources freed |
| Process panics (explicit) | Same as normal return (no panic mechanism yet) |
| Division by zero | Undefined (hardware exception) |
| Stack overflow | SIGSEGV (guard page) |
| Send to dead process | Undefined |
| Receive with no senders ever | Deadlock (scheduler spins) |

---

## 7. Coherence Rules

These rules ensure features compose predictably.

### 7.1 Spawn Coherence

| Rule ID | Rule |
|---------|------|
| **SPAWN-001** | `spawn` always returns a valid `Process` reference immediately |
| **SPAWN-002** | The spawned process may not run until the spawner yields |
| **SPAWN-003** | The spawned process's `self` is its own `ArnmProcess*` |
| **SPAWN-004** | Arguments to spawn are evaluated in the spawner, before the new process exists |

### 7.2 Message Coherence

| Rule ID | Rule |
|---------|------|
| **MSG-001** | A message is owned by exactly one process at any time |
| **MSG-002** | After send, the sender has no reference to the message data |
| **MSG-003** | The receiver owns the message until explicit or implicit free |
| **MSG-004** | Messages are self-contained; no pointers to external data |

### 7.3 Actor Coherence

| Rule ID | Rule |
|---------|------|
| **ACTOR-001** | `self` in an actor refers to the process, not the actor type |
| **ACTOR-002** | `self.field` accesses heap-allocated actor state |
| **ACTOR-003** | Actor methods are compiled to standalone functions with mangled names |
| **ACTOR-004** | Actor state is private; accessible only via `self` in that actor's code |

### 7.4 Control Flow Coherence

| Rule ID | Rule |
|---------|------|
| **FLOW-001** | `return` exits the current function, not the process |
| **FLOW-002** | Reaching end of `main` or actor entry causes process exit |
| **FLOW-003** | `break` exits the nearest enclosing loop |
| **FLOW-004** | A `receive` block is not a loop; after processing one message, control continues |
| **FLOW-005** | To loop on receive, wrap in explicit `loop { receive { ... } }` or use actor `receive` block |

---

## 8. Consistency Examples

These examples demonstrate that ARNm behaves predictably.

### 8.1 Deterministic Evaluation

```arnm
fn f() -> i32 { print(1); return 1; }
fn g() -> i32 { print(2); return 2; }
fn h() -> i32 { print(3); return 3; }

fn main() {
    let x = f() + g() * h();
    // Output: 1, 2, 3 (left-to-right)
    // x = 1 + (2 * 3) = 7 (precedence)
}
```

**Guarantee**: Regardless of optimization level, output is always `1 2 3`.

### 8.2 Scope Shadowing

```arnm
fn main() {
    let x = 1;
    {
        let x = 2;  // Shadows outer x
        print(x);   // Prints: 2
    }
    print(x);       // Prints: 1 (outer x unchanged)
}
```

**Guarantee**: Inner scope shadows but does not mutate outer scope.

### 8.3 Message Order

```arnm
actor Printer {
    receive {
        x => print(x);  // Infinite receive loop
    }
}

fn main() {
    let p = spawn Printer();
    p ! 1;
    p ! 2;
    p ! 3;
    // Output: 1, 2, 3 (FIFO guarantee)
}
```

**Guarantee**: Messages arrive in order.

### 8.4 Concurrent Counter (Stress Test)

```arnm
actor Counter {
    let count: i32;
    
    fn init() {
        self.count = 0;
    }
    
    receive {
        n => {
            self.count = self.count + n;
            print(self.count);
        }
    }
}

fn main() {
    let c = spawn Counter.init();
    
    // Spawn multiple senders
    spawn { c ! 10; };
    spawn { c ! 20; };
    spawn { c ! 30; };
    
    // Final count is 60 (order of prints may vary)
    // But each individual update is atomic
}
```

**Guarantees**:
- Each message is processed atomically (no interleaving)
- Final count is always 60
- Print order may vary (scheduling non-determinism)

### 8.5 Self-Reference

```arnm
fn worker() {
    let me = self;  // Valid: self is current process
    me ! 42;        // Send to self
    receive {
        x => print(x);  // Prints: 42
    }
}

fn main() {
    spawn worker();
}
```

**Clarification**: `self` in a plain function (spawned as a process) returns the current `ArnmProcess*`, which is valid.

---

## Summary: Coherence Checklist

| Area | Formalized? | Document Section |
|------|-------------|------------------|
| Lexical scoping | ✅ | §2 Scope and Binding |
| Name resolution | ✅ | §2.3 Algorithm |
| Evaluation order | ✅ | §3 Evaluation Order |
| Operator precedence | ✅ | §3.2 Precedence table |
| Stack lifetimes | ✅ | §4.1-4.2 |
| Actor state lifetime | ✅ | §4.3 Lifecycle |
| Message copy semantics | ✅ | §5.1-5.2 |
| FIFO guarantee | ✅ | §5.1 |
| Blocking receive | ✅ | §5.3 |
| Self-send | ✅ | §5.4 |
| Error categories | ✅ | §6.1 |
| MVP error contract | ✅ | §6.4 |
| Spawn behavior | ✅ | §7.1 |
| Actor invariants | ✅ | §7.3 |
| Deterministic examples | ✅ | §8 |

---

## Open Items (Future Work)

1. **Process monitors and links** - Erlang-style supervision
2. **Selective receive** - Match on message tag
3. **Timeouts** - `receive timeout(100) { ... }`
4. **Dangling reference detection** - Runtime check for dead processes
5. **Cycle detection** - For ARC memory management
6. **Panic propagation** - Exit reasons and linked process notification

---
