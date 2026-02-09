// ARNm Documentation Content
// Comprehensive documentation for the ARNm programming language

window.documentationContent = [

    // ============================================================
    // GETTING STARTED
    // ============================================================

    {
        id: 'introduction',
        section: 'Getting Started',
        level: 1,
        title: 'Introduction',
        content: `
# Introduction

**ARNm** (pronounced "Arnum") is a modern, statically-typed compiled language designed from the ground up for scalable concurrency. Inspired by Erlang's actor model and C's raw performance, ARNm empowers developers to build fault-tolerant, distributed systems with ease.

## Key Features

- **Native Actor Model** — Lightweight processes with isolated memory and message-passing concurrency
- **Zero-Cost Abstractions** — Compiled directly to optimized x86_64 assembly
- **Safety & Speed** — Strong static typing with a focus on runtime performance
- **Massive Scalability** — Efficient user-space scheduling capable of handling millions of concurrent actors
- **Modern Syntax** — Clean, expressive syntax inspired by Rust and Swift

## Design Philosophy

ARNm follows these core principles:

| Principle | Description |
|-----------|-------------|
| Determinism | Given identical inputs, produce identical outputs |
| Totality | Every construct has defined behavior in every context |
| Composability | Features combine without hidden interactions |
| Locality | Changes to one part don't break unrelated parts |

## Hello World

\`\`\`arnm
fn main() {
    print(42);
}
\`\`\`
`
    },

    {
        id: 'getting-started',
        section: 'Getting Started',
        level: 1,
        title: 'Getting Started',
        content: `
# Getting Started

## Prerequisites

- Linux (x86_64)
- GCC / Make
- Git

## Installation

Clone the repository and build the compiler and runtime:

\`\`\`bash
git clone https://github.com/mouuuuaad/ARNm-Programming-Language.git
cd arnm-lang
make
make -C runtime
\`\`\`

This will produce the \`arnmc\` compiler binary in the \`build/\` directory.

## Writing Your First Program

Create a file named \`hello.arnm\`:

\`\`\`arnm
fn main() {
    print(42);
}
\`\`\`

## Compiling and Running

\`\`\`bash
# Compile to assembly
./build/arnmc --emit-asm hello.arnm > hello.s

# Assemble
gcc -c hello.s -o hello.o

# Link with runtime
gcc -o hello runtime/build/crt0.o hello.o -Lruntime/build -larnm -lpthread

# Run
./hello
\`\`\`

Output: \`42\`
`
    },

    // ============================================================
    // BASICS
    // ============================================================

    {
        id: 'variables',
        section: 'Basics',
        level: 1,
        title: 'Variables',
        content: `
# Variables

Variables are declared with the \`let\` keyword. By default, variables are immutable.

## Immutable Variables

\`\`\`arnm
let x = 42;
let name = "ARNm";
\`\`\`

Once bound, an immutable variable cannot be reassigned.

## Mutable Variables

Use \`mut\` to create mutable variables:

\`\`\`arnm
let mut counter = 0;
counter = counter + 1;
\`\`\`

## Type Annotations

You can optionally annotate types:

\`\`\`arnm
let x: i32 = 42;
let flag: bool = true;
\`\`\`

## Scope Rules

ARNm uses lexical (static) scoping:

\`\`\`arnm
fn main() {
    let x = 1;
    {
        let y = 2;    // y visible only in this block
        print(x + y); // x visible from outer scope
    }
    // y not accessible here
}
\`\`\`

**Shadowing**: Inner scopes can shadow outer variables:

\`\`\`arnm
let x = 1;
{
    let x = 2;  // shadows outer x
    print(x);   // prints 2
}
print(x);       // prints 1
\`\`\`
`
    },

    {
        id: 'data-types',
        section: 'Basics',
        level: 1,
        title: 'Data Types',
        content: `
# Data Types

ARNm has several primitive data types.

## Integer Types

| Type | Size | Range |
|------|------|-------|
| i32 | 32 bits | -2^31 to 2^31-1 |
| i64 | 64 bits | -2^63 to 2^63-1 |

\`\`\`arnm
let small: i32 = 100;
let large: i64 = 9223372036854775807;
\`\`\`

### Integer Literals

\`\`\`arnm
let decimal = 42;
let hex = 0xFF;
let binary = 0b1010;
let octal = 0o755;
\`\`\`

## Boolean Type

\`\`\`arnm
let flag: bool = true;
let done = false;
\`\`\`

## String Type

\`\`\`arnm
let greeting = "Hello, ARNm!";
\`\`\`

### Escape Sequences

| Escape | Meaning |
|--------|---------|
| \\n | Newline |
| \\r | Carriage return |
| \\t | Tab |
| \\\\ | Backslash |
| \\" | Double quote |

## Character Type

\`\`\`arnm
let ch: char = 'A';
\`\`\`

## Special Values

\`\`\`arnm
let nothing = nil;   // Null/undefined value
let me = self;       // Current process reference (in actors)
\`\`\`
`
    },

    {
        id: 'operators',
        section: 'Basics',
        level: 1,
        title: 'Operators',
        content: `
# Operators

## Arithmetic Operators

| Operator | Description | Example |
|----------|-------------|---------|
| + | Addition | 5 + 3 = 8 |
| - | Subtraction | 5 - 3 = 2 |
| * | Multiplication | 5 * 3 = 15 |
| / | Division | 15 / 3 = 5 |
| % | Modulo | 17 % 5 = 2 |

\`\`\`arnm
let result = (10 * 10) + (5 - 2);  // 103
\`\`\`

## Comparison Operators

| Operator | Description |
|----------|-------------|
| == | Equal |
| != | Not equal |
| < | Less than |
| <= | Less or equal |
| > | Greater than |
| >= | Greater or equal |

\`\`\`arnm
let check = 5 < 10;  // true
\`\`\`

## Logical Operators

| Operator | Description |
|----------|-------------|
| && | Logical AND (short-circuit) |
| \\|\\| | Logical OR (short-circuit) |
| ! | Logical NOT |

\`\`\`arnm
if (x > 0 && x < 100) {
    print(x);
}
\`\`\`

## Message Send Operator

The \`!\` operator sends a message to a process:

\`\`\`arnm
target ! 42;   // Send 42 to target process
\`\`\`

## Operator Precedence

From lowest to highest:

1. \`=\` (assignment)
2. \`||\` (logical or)
3. \`&&\` (logical and)
4. \`==\` \`!=\` (equality)
5. \`<\` \`<=\` \`>\` \`>=\` (comparison)
6. \`!\` (message send)
7. \`+\` \`-\` (additive)
8. \`*\` \`/\` \`%\` (multiplicative)
9. \`-\` \`!\` \`~\` (unary prefix)
10. \`()\` \`[]\` \`.\` (call, index, field)
`
    },

    // ============================================================
    // FUNCTIONS
    // ============================================================

    {
        id: 'functions',
        section: 'Functions',
        level: 1,
        title: 'Functions',
        content: `
# Functions

Functions are declared with the \`fn\` keyword.

## Basic Syntax

\`\`\`arnm
fn greet() {
    print(100);
}
\`\`\`

## Parameters

\`\`\`arnm
fn add(a: i32, b: i32) {
    print(a + b);
}
\`\`\`

### Mutable Parameters

\`\`\`arnm
fn increment(mut x: i32) {
    x = x + 1;
    print(x);
}
\`\`\`

## Return Values

\`\`\`arnm
fn square(n: i32) -> i32 {
    return n * n;
}

fn main() {
    let result = square(5);  // 25
}
\`\`\`

## Entry Point

Every ARNm program must have a \`main\` function:

\`\`\`arnm
fn main() {
    // Program starts here
}
\`\`\`

## Function Calls

\`\`\`arnm
fn side_effect() -> i32 {
    print(1);
    return 42;
}

fn main() {
    let x = side_effect() + side_effect();
    // Always prints: 1, then 1
    // x = 84
}
\`\`\`

**Evaluation Order**: Arguments are evaluated strictly left-to-right.
`
    },

    // ============================================================
    // CONTROL FLOW
    // ============================================================

    {
        id: 'if-else',
        section: 'Control Flow',
        level: 1,
        title: 'If / Else',
        content: `
# If / Else

## Basic If

\`\`\`arnm
if (x > 0) {
    print(1);
}
\`\`\`

## If-Else

\`\`\`arnm
if (x > 0) {
    print(1);
} else {
    print(0);
}
\`\`\`

## Else-If Chain

\`\`\`arnm
if (x < 0) {
    print(-1);
} else if (x == 0) {
    print(0);
} else {
    print(1);
}
\`\`\`

## Nested Conditions

\`\`\`arnm
if (x > 0) {
    if (x < 100) {
        print(x);
    }
}
\`\`\`

## Grammar

\`\`\`
if_stmt = "if" expression block [ "else" ( block | if_stmt ) ] ;
\`\`\`
`
    },

    {
        id: 'loops',
        section: 'Control Flow',
        level: 1,
        title: 'Loops',
        content: `
# Loops

ARNm supports \`while\`, \`loop\`, and \`for\` loops.

## While Loop

\`\`\`arnm
let mut i = 0;
while (i < 5) {
    print(i);
    i = i + 1;
}
\`\`\`

## Infinite Loop

\`\`\`arnm
loop {
    // runs forever until break
    if (condition) {
        break;
    }
}
\`\`\`

## For Loop

\`\`\`arnm
for i in range(0, 10) {
    print(i);
}
\`\`\`

## Break and Continue

\`\`\`arnm
let mut j = 0;
loop {
    j = j + 1;
    if (j == 2) {
        continue;  // skip to next iteration
    }
    if (j > 4) {
        break;     // exit loop
    }
    print(j);      // prints: 1, 3, 4
}
\`\`\`

## Grammar

\`\`\`
while_stmt    = "while" expression block ;
loop_stmt     = "loop" block ;
for_stmt      = "for" IDENT "in" expression block ;
break_stmt    = "break" ";" ;
continue_stmt = "continue" ";" ;
\`\`\`
`
    },

    // ============================================================
    // ACTORS & CONCURRENCY
    // ============================================================

    {
        id: 'actors',
        section: 'Actors & Concurrency',
        level: 1,
        title: 'Actors',
        content: `
# Actors

Actors are the fundamental unit of concurrency in ARNm. Each actor is a lightweight process with:

- **Isolated state** (fields accessible via \`self\`)
- **A mailbox** for receiving messages
- **An init function** and optional receive block

## Defining an Actor

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

## Actor State

Actor fields are accessed through \`self\`:

\`\`\`arnm
actor Worker {
    let id: i32;
    let processed: i32;
    
    fn init() {
        self.id = 1;
        self.processed = 0;
    }
}
\`\`\`

> **Note**: \`self.field\` is the only way to access actor state. Bare field names are not allowed.

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
}
\`\`\`

## Grammar

\`\`\`
actor_decl  = "actor" IDENT "{" { actor_item } "}" ;
actor_item  = function_decl | receive_block ;
\`\`\`
`
    },

    {
        id: 'spawn',
        section: 'Actors & Concurrency',
        level: 1,
        title: 'Spawn',
        content: `
# Spawn

The \`spawn\` keyword creates a new process.

## Spawning an Actor

\`\`\`arnm
fn main() {
    let worker = spawn Worker();
    // worker is now a process reference
}
\`\`\`

## Spawning with Init

\`\`\`arnm
let counter = spawn Counter.init();
\`\`\`

## How Spawn Works

When you call \`spawn\`:

1. A new \`ArnmProcess\` is allocated
2. Actor state memory is allocated
3. The init function is called
4. The process is added to the scheduler

\`\`\`arnm
fn main() {
    let p = spawn SomeActor.init();
    // p is valid here immediately
    // The spawned process may not run until main yields
}
\`\`\`

## Spawn Rules

| Rule | Description |
|------|-------------|
| SPAWN-001 | \`spawn\` always returns a valid Process reference immediately |
| SPAWN-002 | The spawned process may not run until the spawner yields |
| SPAWN-003 | The spawned process's \`self\` is its own \`ArnmProcess*\` |
| SPAWN-004 | Arguments to spawn are evaluated before the new process exists |

## Grammar

\`\`\`
spawn_stmt = "spawn" expression ";" ;
\`\`\`
`
    },

    {
        id: 'send-receive',
        section: 'Actors & Concurrency',
        level: 1,
        title: 'Send / Receive',
        content: `
# Send / Receive

Message passing is the only way actors communicate.

## Sending Messages

Use the \`!\` operator:

\`\`\`arnm
target ! 42;      // Send integer 42 to target
target ! value;   // Send variable value
\`\`\`

## Receiving Messages

The \`receive\` block waits for messages:

\`\`\`arnm
receive {
    x => {
        print(x);
    }
}
\`\`\`

## Actor Receive Block

Actors can have a permanent receive block:

\`\`\`arnm
actor Printer {
    receive {
        msg => print(msg);
    }
}

fn main() {
    let p = spawn Printer();
    p ! 1;
    p ! 2;
    p ! 3;
    // Output: 1, 2, 3 (FIFO order)
}
\`\`\`

## Self-Send

You can send messages to yourself:

\`\`\`arnm
actor Echo {
    fn init() {
        self ! 42;       // Message to self
        receive {
            x => print(x); // Prints 42
        }
    }
}
\`\`\`

## Message Properties

| Property | Guarantee |
|----------|-----------|
| Isolation | Processes share no mutable state except through messages |
| Copy semantics | Message data is deep-copied on send |
| FIFO | Messages from A to B arrive in order |
| Asynchronous | \`send\` returns immediately |
| Non-blocking send | \`send\` never blocks |
| Blocking receive | \`receive\` blocks until a message arrives |
`
    },

    {
        id: 'self',
        section: 'Actors & Concurrency',
        level: 1,
        title: 'Self',
        content: `
# Self

The \`self\` keyword refers to the current process.

## Accessing Actor State

\`\`\`arnm
actor Counter {
    let count: i32;
    
    fn init() {
        self.count = 0;  // Access field via self
    }
}
\`\`\`

## Storing Self Reference

\`\`\`arnm
fn worker() {
    let me = self;  // Store process reference
    me ! 42;        // Send to self
    receive {
        x => print(x);
    }
}
\`\`\`

## Self in Functions vs Actors

- In **actors**: \`self\` is both the process reference and access to state
- In **spawned functions**: \`self\` is the current process reference

\`\`\`arnm
fn greet() {
    print(self);  // Prints process ID or reference
}

fn main() {
    spawn greet();
}
\`\`\`

## Rules for Self

| Rule | Description |
|------|-------------|
| SCOPE-004 | \`self\` is only valid inside an actor or spawned process |
| ACTOR-001 | \`self\` in an actor refers to the process |
| ACTOR-002 | \`self.field\` accesses heap-allocated actor state |
`
    },

    {
        id: 'concurrency-model',
        section: 'Actors & Concurrency',
        level: 1,
        title: 'Concurrency Model',
        content: `
# Concurrency Model

ARNm uses an actor-based concurrency model with preemptive scheduling.

## Process States

\`\`\`
         spawn()
            │
            ▼
        ┌──────┐    sched_next()    ┌─────────┐
        │ READY │ ───────────────► │ RUNNING │
        └──────┘                    └─────────┘
           ▲                           │  │
           │    yield()                │  │  receive() (no message)
           └───────────────────────────┘  │
                                          ▼
                                    ┌─────────┐
                                    │ WAITING │
                                    └─────────┘
                                          │
                                          │ message arrives → READY
                                          ▼
                                     ┌──────┐
        fn returns ──────────────►  │ DEAD │
                                     └──────┘
\`\`\`

## Scheduler

The scheduler coordinates worker threads and process execution:

- **Work stealing**: Workers can steal tasks from each other
- **Local queues**: Each worker has a fast local queue
- **Global queue**: Overflow processes go to a shared queue

## Process Lifecycle

1. **spawn()** — Creates process, sets state to READY
2. **Running** — Scheduler picks process, executes
3. **Waiting** — Process calls receive with no messages
4. **Wake-up** — Message arrival moves to READY
5. **Dead** — Function returns, process is cleaned up

## Fairness

ARNm uses cooperative multitasking within a preemptive framework:

- Each process runs until it yields, receives, or returns
- The scheduler ensures all ready processes get time to run
`
    },

    // ============================================================
    // STRUCTS
    // ============================================================

    {
        id: 'structs',
        section: 'Data Structures',
        level: 1,
        title: 'Structs',
        content: `
# Structs

Structs group related data together.

## Defining a Struct

\`\`\`arnm
struct Config {
    mode: i32,
    limit: i32
}
\`\`\`

## Using Structs

\`\`\`arnm
fn main() {
    let config = Config {
        mode: 1,
        limit: 100
    };
    print(config.mode);
}
\`\`\`

## Field Access

Use dot notation to access fields:

\`\`\`arnm
let x = config.limit;
config.mode = 2;
\`\`\`

## Grammar

\`\`\`
struct_decl = "struct" IDENT "{" [ field_list ] "}" ;
field_list  = field { "," field } ;
field       = IDENT ":" type ;
\`\`\`

## Structs vs Actors

| Aspect | Struct | Actor |
|--------|--------|-------|
| State | Data only | Data + behavior |
| Access | Direct | Via self |
| Concurrency | None | Message-based |
| Allocation | Stack/heap | Always heap |
`
    },

    // ============================================================
    // PATTERNS
    // ============================================================

    {
        id: 'patterns',
        section: 'Advanced',
        level: 1,
        title: 'Pattern Matching',
        content: `
# Pattern Matching

Receive blocks use patterns to match messages.

## Simple Binding

\`\`\`arnm
receive {
    x => print(x);  // x binds to any message
}
\`\`\`

## Literal Patterns

Match specific values:

\`\`\`arnm
receive {
    1 => print(100);
    2 => print(200);
    3 => print(300);
}
\`\`\`

## Full Example

\`\`\`arnm
actor Worker {
    receive {
        1 => {
            // Handle command 1
            print(1);
        }
        2 => {
            // Handle command 2
            print(2);
        }
        x => {
            // Catch-all for other messages
            print(x);
        }
    }
}
\`\`\`

## Grammar

\`\`\`
receive_block = "receive" "{" { receive_arm } "}" ;
receive_arm   = pattern "=>" block ;
pattern       = IDENT | literal ;
\`\`\`

> **Note**: Advanced pattern matching (guards, destructuring) is planned for future versions.
`
    },

    // ============================================================
    // TYPE SYSTEM
    // ============================================================

    {
        id: 'type-system',
        section: 'Advanced',
        level: 1,
        title: 'Type System',
        content: `
# Type System

ARNm uses static typing with type inference.

## Type Annotations

\`\`\`arnm
let x: i32 = 42;
let flag: bool = true;
\`\`\`

## Type Inference

Types can often be inferred:

\`\`\`arnm
let x = 42;      // inferred as i64
let y = 3.14;    // inferred as f64
let s = "hello"; // inferred as String
\`\`\`

## Function Types

\`\`\`arnm
fn add(a: i32, b: i32) -> i32 {
    return a + b;
}
\`\`\`

## Type Grammar

\`\`\`
type         = type_primary [ type_suffix ] ;
type_primary = IDENT
             | "fn" "(" [ type_list ] ")" [ "->" type ]
             ;
type_suffix  = "?"                 // Optional
             | "[" [ expr ] "]"    // Array
             ;
\`\`\`

## Process Type

The \`Process\` type represents a process reference:

\`\`\`arnm
let p: Process = spawn Worker();
p ! 42;
\`\`\`
`
    },

    // ============================================================
    // ERROR HANDLING
    // ============================================================

    {
        id: 'error-handling',
        section: 'Advanced',
        level: 1,
        title: 'Error Handling',
        content: `
# Error Handling

ARNm is in MVP stage with basic error handling.

## Current Behavior

| Situation | Behavior |
|-----------|----------|
| Process returns normally | Resources freed |
| Division by zero | Hardware exception (undefined) |
| Stack overflow | SIGSEGV (guard page) |
| Send to dead process | Undefined |
| Receive with no senders | Deadlock |

## Process Termination

When a process terminates:

1. Entry function returns
2. State set to DEAD
3. Yield to scheduler
4. Scheduler calls proc_destroy
5. Resources freed

## Compile-Time Errors

| Error | Description |
|-------|-------------|
| Type mismatch | Incompatible types |
| Undefined identifier | Name not in scope |
| Duplicate definition | Name already defined |
| Self outside actor | \`self\` used incorrectly |

## Future Plans

- Process monitors and links (Erlang-style)
- Panic propagation
- \`Result\` type for error handling
- Timeouts on receive
`
    },

    // ============================================================
    // RUNTIME
    // ============================================================

    {
        id: 'runtime',
        section: 'Internals',
        level: 1,
        title: 'Runtime',
        content: `
# Runtime

The ARNm runtime provides process management, scheduling, and messaging.

## Components

| Component | Description |
|-----------|-------------|
| Scheduler | Coordinates worker threads |
| Workers | OS threads executing processes |
| Mailboxes | Message queues (MPSC) |
| Memory | Stack and heap management |

## Runtime API

Functions called from generated assembly:

\`\`\`c
// Process management
ArnmProcess* arnm_spawn(void (*entry)(void*), void* arg, size_t state_size);
void arnm_sched_yield(void);

// Messaging
void arnm_send(ArnmProcess* target, uint64_t tag, void* data, size_t size);
ArnmMessage* arnm_receive(int timeout_ms);

// Self reference
ArnmProcess* arnm_self(void);

// Debug output
void arnm_print_int(int64_t value);
\`\`\`

## Process Structure

\`\`\`c
typedef struct ArnmProcess {
    void*           actor_state;
    uint64_t        pid;
    ProcState       state;
    ArnmContext     context;
    void*           stack_base;
    size_t          stack_size;
    ArnmMailbox*    mailbox;
    ArnmProcess*    next;
    uint32_t        worker_id;
} ArnmProcess;
\`\`\`

## Context Switching

The runtime saves and restores CPU registers:

- rsp, rbp (stack)
- rbx, r12-r15 (callee-saved)
`
    },

    {
        id: 'compilation',
        section: 'Internals',
        level: 1,
        title: 'Compilation',
        content: `
# Compilation

The ARNm compiler (\`arnmc\`) transforms source code to machine code.

## Pipeline

\`\`\`
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
SSA IR (Internal Representation)
    │
    ▼ [Code Generation]
x86_64 Assembly (.s)
    │
    ▼ [Assembler: gcc/as]
Object File (.o)
    │
    ▼ [Linker]
Executable
\`\`\`

## Compiler Flags

\`\`\`bash
arnmc --emit-asm file.arnm    # Output assembly
arnmc --emit-ir file.arnm     # Output IR
arnmc --dump-ast file.arnm    # Dump AST
\`\`\`

## Linking

ARNm programs must be linked with the runtime:

\`\`\`bash
gcc -o program \\
    runtime/build/crt0.o \\
    program.o \\
    -Lruntime/build \\
    -larnm \\
    -lpthread
\`\`\`

- \`crt0.o\` — Entry point, calls \`_arnm_main\`
- \`libarnm.a\` — Runtime library
- \`pthread\` — Threading support
`
    },

    // ============================================================
    // REFERENCE
    // ============================================================

    {
        id: 'keywords',
        section: 'Reference',
        level: 1,
        title: 'Keywords',
        content: `
# Keywords

Reserved words in ARNm:

| Keyword | Description |
|---------|-------------|
| actor | Define an actor |
| break | Exit loop |
| const | Constant (future) |
| continue | Skip to next iteration |
| else | Alternative branch |
| enum | Enumeration (future) |
| false | Boolean false |
| fn | Function declaration |
| for | For loop |
| if | Conditional |
| immut | Immutable (future) |
| let | Variable binding |
| loop | Infinite loop |
| match | Pattern match (future) |
| mut | Mutable variable |
| nil | Null value |
| receive | Message receive |
| return | Return from function |
| self | Current process |
| shared | Shared reference (future) |
| spawn | Create process |
| struct | Structure definition |
| true | Boolean true |
| type | Type alias (future) |
| unique | Unique reference (future) |
| while | While loop |
`
    },

    {
        id: 'grammar',
        section: 'Reference',
        level: 1,
        title: 'Grammar Reference',
        content: `
# Grammar Reference

Complete EBNF grammar for ARNm.

## Program Structure

\`\`\`
program     = { declaration } ;
declaration = function_decl | actor_decl | struct_decl ;
\`\`\`

## Functions

\`\`\`
function_decl = "fn" IDENT "(" [ param_list ] ")" [ "->" type ] block ;
param_list    = param { "," param } ;
param         = [ "mut" ] IDENT ":" type ;
\`\`\`

## Actors

\`\`\`
actor_decl    = "actor" IDENT "{" { actor_item } "}" ;
actor_item    = function_decl | receive_block ;
receive_block = "receive" "{" { receive_arm } "}" ;
receive_arm   = pattern "=>" block ;
\`\`\`

## Statements

\`\`\`
block         = "{" { statement } "}" ;
statement     = let_stmt | return_stmt | if_stmt | while_stmt
              | for_stmt | loop_stmt | break_stmt | continue_stmt
              | spawn_stmt | receive_stmt | expr_stmt ;
let_stmt      = "let" [ "mut" ] IDENT [ ":" type ] [ "=" expression ] ";" ;
return_stmt   = "return" [ expression ] ";" ;
\`\`\`

## Expressions

\`\`\`
expression = assignment ;
assignment = logic_or [ "=" assignment ] ;
logic_or   = logic_and { "||" logic_and } ;
logic_and  = equality { "&&" equality } ;
equality   = comparison { ( "==" | "!=" ) comparison } ;
comparison = send { ( "<" | "<=" | ">" | ">=" ) send } ;
send       = term { "!" term } ;
term       = factor { ( "+" | "-" ) factor } ;
factor     = unary { ( "*" | "/" | "%" ) unary } ;
unary      = ( "-" | "!" | "~" ) unary | postfix ;
postfix    = primary { postfix_op } ;
\`\`\`
`
    },

    {
        id: 'examples',
        section: 'Reference',
        level: 1,
        title: 'Examples',
        content: `
# Examples

## Hello World

\`\`\`arnm
fn main() {
    print(42);
}
\`\`\`

## Simple Actor

\`\`\`arnm
actor Greeter {
    fn init() {
        print(100);
    }
}

fn main() {
    spawn Greeter();
}
\`\`\`

## Counter with Messages

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

fn main() {
    let c = spawn Counter.init();
    c ! 10;
    c ! 20;
    c ! 30;
    // Prints: 10, 30, 60
}
\`\`\`

## Ping Pong

\`\`\`arnm
actor Pinger {
    fn init() {
        let pong = spawn Ponger();
        pong ! self;
    }
    
    receive {
        x => print(x);
    }
}

actor Ponger {
    receive {
        sender => {
            sender ! 42;
        }
    }
}

fn main() {
    spawn Pinger.init();
}
\`\`\`

## Worker with Multiple Message Types

\`\`\`arnm
actor Worker {
    let processed: i32;
    
    fn init() {
        self.processed = 0;
    }
    
    receive {
        1 => {
            self.processed = self.processed + 1;
            print(self.processed);
        }
        2 => {
            let mut i = 0;
            while (i < 3) {
                print(200 + i);
                i = i + 1;
            }
        }
        3 => {
            let mut j = 0;
            loop {
                j = j + 1;
                if (j == 2) { continue; }
                if (j > 4) { break; }
                print(300 + j);
            }
        }
    }
}
\`\`\`
`
    }

];
