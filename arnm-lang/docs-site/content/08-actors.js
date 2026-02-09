// Section 08: Actors
// Complete guide to actors - the core concurrency primitive

window.addDocSections([
    {
        id: 'actors',
        section: 'Actors & Concurrency',
        level: 1,
        title: 'Actors',
        content: `
# Actors in ARNm

Actors are the fundamental unit of concurrency in ARNm. They are lightweight processes with isolated state that communicate through message passing.

---

## What is an Actor?

An actor is:
- A **lightweight process** (not an OS thread)
- Has its own **isolated state** (no shared memory)
- Has a **mailbox** for receiving messages
- Can **spawn** other actors
- Can **send/receive** messages

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

---

## Defining an Actor

\`\`\`arnm
actor ActorName {
    // Fields (state)
    let field1: Type;
    let field2: Type;
    
    // Init function (called when spawned)
    fn init() {
        self.field1 = initial_value;
    }
    
    // Other methods
    fn some_method() {
        // ...
    }
    
    // Receive block (message handler)
    receive {
        pattern => { /* handle */ }
    }
}
\`\`\`

---

## Actor State

Fields hold the actor's private state:

\`\`\`arnm
actor Worker {
    let id: i32;
    let processed: i32;
    let is_busy: bool;
    
    fn init() {
        self.id = 1;
        self.processed = 0;
        self.is_busy = false;
    }
}
\`\`\`

**Key rules:**
- Fields are accessed via \`self.field_name\`
- Fields are private to the actor
- Other actors cannot access your fields

---

## The init Function

Called when the actor is spawned:

\`\`\`arnm
actor Server {
    let connections: i32;
    
    fn init() {
        self.connections = 0;
        print(100);  // "Server started"
    }
}

fn main() {
    spawn Server();  // Calls init automatically
}
\`\`\`

---

## Actor Methods

Actors can have multiple functions:

\`\`\`arnm
actor Calculator {
    let result: i32;
    
    fn init() {
        self.result = 0;
    }
    
    fn add(x: i32) {
        self.result = self.result + x;
    }
    
    fn multiply(x: i32) {
        self.result = self.result * x;
    }
    
    fn get_result() -> i32 {
        return self.result;
    }
}
\`\`\`

---

## The Receive Block

Handles incoming messages:

\`\`\`arnm
actor Printer {
    receive {
        msg => {
            print(msg);
        }
    }
}
\`\`\`

The receive block runs in a loop, processing each message as it arrives.

---

## Complete Actor Example

\`\`\`arnm
actor Counter {
    let count: i32;
    
    fn init() {
        self.count = 0;
        print(999);  // "Counter initialized"
    }
    
    receive {
        1 => {
            // Increment
            self.count = self.count + 1;
            print(self.count);
        }
        2 => {
            // Decrement
            self.count = self.count - 1;
            print(self.count);
        }
        3 => {
            // Reset
            self.count = 0;
            print(0);
        }
    }
}

fn main() {
    let c = spawn Counter();
    c ! 1;  // Increment -> prints 1
    c ! 1;  // Increment -> prints 2
    c ! 2;  // Decrement -> prints 1
    c ! 3;  // Reset -> prints 0
}
\`\`\`

---

## Grammar

\`\`\`
actor_decl   = "actor" IDENT "{" { actor_item } "}" ;
actor_item   = field_decl | function_decl | receive_block ;
field_decl   = "let" IDENT ":" type ";" ;
receive_block = "receive" "{" { receive_arm } "}" ;
receive_arm   = pattern "=>" block ;
\`\`\`
`
    },
    {
        id: 'spawn',
        section: 'Actors & Concurrency',
        level: 1,
        title: 'Spawn',
        content: `
# Spawning Processes

The \`spawn\` keyword creates a new concurrent process.

---

## Basic Spawn

\`\`\`arnm
fn main() {
    let worker = spawn Worker();
    // worker now holds a reference to the new process
}
\`\`\`

---

## Spawning with init

\`\`\`arnm
let counter = spawn Counter.init();
\`\`\`

---

## How Spawn Works

\`\`\`
spawn Actor()
    │
    ├── 1. Allocate ArnmProcess struct
    ├── 2. Allocate stack (64KB default)
    ├── 3. Allocate actor state (fields)
    ├── 4. Create mailbox
    ├── 5. Set state = READY
    ├── 6. Add to scheduler queue
    └── 7. Return Process reference
\`\`\`

---

## Multiple Spawns

\`\`\`arnm
fn main() {
    let w1 = spawn Worker();
    let w2 = spawn Worker();
    let w3 = spawn Worker();
    
    // All three workers run concurrently
    w1 ! 1;
    w2 ! 2;
    w3 ! 3;
}
\`\`\`

---

## Spawn in Loops

\`\`\`arnm
fn main() {
    let mut i = 0;
    while (i < 10) {
        spawn Worker();
        i = i + 1;
    }
    // 10 workers now running
}
\`\`\`

---

## Spawn Rules

| Rule | Description |
|------|-------------|
| Returns immediately | \`spawn\` doesn't wait for the process to start |
| Valid reference | The returned Process reference is immediately valid |
| Scheduling | The spawned process runs when the scheduler picks it |
| Isolation | The new process has completely separate memory |
`
    },
    {
        id: 'send-receive',
        section: 'Actors & Concurrency',
        level: 1,
        title: 'Send / Receive',
        content: `
# Message Passing

Actors communicate through messages. This is the ONLY way to share data between actors.

---

## Sending Messages

Use the \`!\` operator:

\`\`\`arnm
process ! message;
\`\`\`

**Examples:**

\`\`\`arnm
let worker = spawn Worker();

worker ! 42;        // Send integer
worker ! 100;       // Send another
worker ! value;     // Send variable

// Chained spawning and sending
(spawn Printer()) ! 999;
\`\`\`

---

## Receiving Messages

The \`receive\` block waits for and handles messages:

\`\`\`arnm
receive {
    x => {
        print(x);
    }
}
\`\`\`

---

## Pattern Matching in Receive

Match on specific values:

\`\`\`arnm
receive {
    1 => { print(100); }
    2 => { print(200); }
    3 => { print(300); }
    x => { print(x); }     // Catch-all
}
\`\`\`

---

## Message Guarantees

| Property | Guarantee |
|----------|-----------|
| **Isolation** | No shared mutable state |
| **Copy** | Messages are deep-copied |
| **FIFO** | Messages from A→B arrive in order |
| **Async** | Send returns immediately |
| **Non-blocking send** | Sender never waits |
| **Blocking receive** | Receiver waits if mailbox empty |

---

## Self-Send

You can send messages to yourself:

\`\`\`arnm
actor Echo {
    fn init() {
        self ! 42;
        receive {
            x => print(x);
        }
    }
}
// Prints: 42
\`\`\`

---

## Ping-Pong Example

\`\`\`arnm
actor Ping {
    fn init() {
        let pong = spawn Pong();
        pong ! self;   // Send our reference
    }
    
    receive {
        x => print(x);
    }
}

actor Pong {
    receive {
        sender => {
            sender ! 42;  // Reply to sender
        }
    }
}

fn main() {
    spawn Ping();
}
// Prints: 42
\`\`\`
`
    },
    {
        id: 'self',
        section: 'Actors & Concurrency',
        level: 1,
        title: 'Self',
        content: `
# The self Keyword

\`self\` refers to the current process/actor.

---

## In Actors

Access actor fields:

\`\`\`arnm
actor Counter {
    let count: i32;
    
    fn init() {
        self.count = 0;   // Access field
    }
    
    fn increment() {
        self.count = self.count + 1;
    }
}
\`\`\`

---

## As Process Reference

Store and pass your own reference:

\`\`\`arnm
actor Sender {
    fn init() {
        let receiver = spawn Receiver();
        receiver ! self;  // Send our reference
    }
    
    receive {
        reply => print(reply);
    }
}

actor Receiver {
    receive {
        sender => {
            sender ! 100;  // Reply to sender
        }
    }
}
\`\`\`

---

## In Spawned Functions

\`self\` also works in plain functions when spawned:

\`\`\`arnm
fn worker() {
    let me = self;
    me ! 42;
    receive {
        x => print(x);
    }
}

fn main() {
    spawn worker();
}
\`\`\`

---

## Rules for self

| Rule | Description |
|------|-------------|
| Required for fields | \`self.field_name\` not just \`field_name\` |
| Valid in actors | Access state and process reference |
| Valid in spawned functions | Process reference only |
| Invalid in main | Main is also a process, but check context |
`
    }
]);
