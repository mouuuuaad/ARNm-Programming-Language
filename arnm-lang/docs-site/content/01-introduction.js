// Section 01: Introduction
// Comprehensive overview of the ARNm programming language

window.addDocSection({
    id: 'introduction',
    section: 'Getting Started',
    level: 1,
    title: 'Introduction',
    content: `
# Introduction to ARNm

**ARNm** (pronounced "Arnum") is a modern, statically-typed compiled programming language designed from the ground up for **scalable concurrency**. Drawing inspiration from Erlang's battle-tested actor model and C's raw performance, ARNm empowers developers to build fault-tolerant, distributed systems with ease.

---

## What is ARNm?

ARNm is a **systems programming language** that treats concurrency as a first-class citizen. Unlike traditional languages where concurrency is bolted on as an afterthought, ARNm builds everything around the concept of **actors** — independent processes that communicate exclusively through message passing.

### The Name

The name "ARNm" is inspired by messenger RNA (mRNA) in biology — just as mRNA carries genetic messages between cells, ARNm processes carry messages between concurrent actors.

---

## Why ARNm?

### The Problem with Traditional Concurrency

Traditional concurrent programming faces several challenges:

| Challenge | Traditional Approach | ARNm Solution |
|-----------|---------------------|---------------|
| **Shared State** | Locks, mutexes, deadlocks | No shared mutable state — actors own their data |
| **Data Races** | Hard to detect, undefined behavior | Impossible by design — message passing only |
| **Complexity** | Thread pools, synchronization primitives | Simple spawn/send/receive model |
| **Scalability** | OS thread overhead limits scale | Lightweight processes, millions of actors |
| **Debugging** | Race conditions are non-deterministic | Deterministic message ordering |

### Key Differentiators

**1. Native Actor Model**

Every concurrent unit in ARNm is an **actor** — a lightweight process with its own isolated state. Actors cannot directly access each other's memory. The only way to interact is through **asynchronous message passing**.

\`\`\`arnm
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
\`\`\`

**2. Zero-Cost Abstractions**

ARNm compiles directly to optimized **x86_64 assembly**. There's no interpreter, no virtual machine, no garbage collector pauses. You get the expressiveness of a high-level language with the performance of hand-tuned assembly.

**3. Safety Without Sacrifice**

Strong static typing catches errors at compile time, not in production. The type system is expressive enough to model complex domains while remaining simple to understand.

**4. Massive Scalability**

ARNm's user-space scheduler can handle **millions of concurrent actors**. Each actor has a minimal memory footprint (~2KB stack by default), allowing you to model problems naturally without worrying about resource limits.

---

## Core Design Philosophy

ARNm follows four fundamental principles:

### 1. Determinism

> Given identical inputs, produce identical outputs.

Side effects are explicit and controlled. Message ordering within a sender-receiver pair is guaranteed FIFO. This makes testing and debugging dramatically easier.

### 2. Totality

> Every construct has defined behavior in every context.

There are no undefined behaviors in safe ARNm code. The compiler rejects ambiguous constructs rather than guessing what you meant.

### 3. Composability

> Features combine without hidden interactions.

You can confidently compose language features. Spawning an actor inside a loop, sending messages in a conditional — everything works as expected without surprises.

### 4. Locality

> Changes to one part don't break unrelated parts.

Actor isolation ensures that bugs are contained. A misbehaving actor can crash without bringing down the entire system (future: supervision trees).

---

## Your First ARNm Program

Here's the simplest possible ARNm program:

\`\`\`arnm
fn main() {
    print(42);
}
\`\`\`

**What happens when this runs:**

1. The runtime initializes the scheduler
2. \`main\` is spawned as the initial process
3. \`print(42)\` outputs "42" to stdout
4. \`main\` returns, the process dies
5. The scheduler detects zero active processes and exits

### A Concurrent Example

Here's a taste of ARNm's concurrency model:

\`\`\`arnm
actor Greeter {
    fn init() {
        print(100);  // "Hello from Greeter!"
    }
}

fn main() {
    spawn Greeter();       // Create a new actor
    spawn Greeter();       // Create another one
    spawn Greeter();       // And another
    print(42);             // Main says hello too
}
\`\`\`

**Output** (order may vary):
\`\`\`
42
100
100
100
\`\`\`

Notice that \`main\` doesn't wait for the spawned actors — they run concurrently!

---

## Language at a Glance

| Feature | Status | Description |
|---------|--------|-------------|
| **Static Typing** | <span class="status status-done"></span> | Strong types, compile-time checks |
| **Actor Model** | <span class="status status-done"></span> | spawn, send, receive |
| **Pattern Matching** | <span class="status status-wip"></span> | Basic patterns in receive |
| **Control Flow** | <span class="status status-done"></span> | if/else, while, loop, for |
| **Functions** | <span class="status status-done"></span> | First-class functions with parameters |
| **Structs** | <span class="status status-wip"></span> | User-defined data structures |
| **Modules** | <span class="status status-todo"></span> | Coming in v0.3 |
| **Generics** | <span class="status status-todo"></span> | Future work |

---

## Next Steps

Ready to dive in? Here's your learning path:

1. **[Getting Started](#getting-started)** — Install ARNm, write your first program
2. **[Variables](#variables)** — Learn about bindings and mutability
3. **[Data Types](#data-types)** — Explore ARNm's type system
4. **[Functions](#functions)** — Define reusable code
5. **[Control Flow](#if-else)** — Make decisions in your code
6. **[Actors](#actors)** — Master concurrent programming

Welcome to ARNm! 🧬
`
});
