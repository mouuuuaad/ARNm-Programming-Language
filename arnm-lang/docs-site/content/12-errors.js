// Section 12: Error Handling
// Current error handling in ARNm

window.addDocSection({
    id: 'error-handling',
    section: 'Advanced',
    level: 1,
    title: 'Error Handling',
    content: `
# Error Handling

ARNm is in MVP stage with basic error handling. This guide covers current behavior and future plans.

---

## Compile-Time Errors

The compiler catches many errors before your program runs:

| Error Type | Example | Message |
|------------|---------|---------|
| Type Mismatch | \`let x: i32 = "hi"\` | "expected i32, got String" |
| Undefined Variable | \`print(xyz)\` | "undefined identifier: xyz" |
| Duplicate Definition | \`let x = 1; let x = 2;\` | "duplicate definition: x" |
| Invalid self | \`fn foo() { self.x }\` | "self used outside of actor" |
| Break Outside Loop | \`fn foo() { break; }\` | "break outside of loop" |

### Example Compiler Errors

\`\`\`arnm
fn main() {
    let x = 42;
    let x = 100;  // ERROR: duplicate definition
}
\`\`\`

\`\`\`arnm
fn main() {
    print(undefined_var);  // ERROR: undefined identifier
}
\`\`\`

\`\`\`arnm
fn calculate() -> i32 {
    return "oops";  // ERROR: type mismatch
}
\`\`\`

---

## Runtime Behavior (MVP)

Current runtime error handling is minimal:

| Situation | Behavior |
|-----------|----------|
| Normal process exit | Resources freed cleanly |
| Division by zero | Hardware exception (crash) |
| Stack overflow | SIGSEGV (guard page) |
| Send to dead process | Undefined behavior |
| Receive with no messages ever | Deadlock |
| Out of memory | System allocation fails |

### Process Termination

When a process terminates normally:

\`\`\`
1. Entry function returns
2. proc_exit() called → state = DEAD
3. Yield to scheduler
4. Scheduler calls proc_destroy()
5. Resources freed:
   - Actor state memory
   - Stack memory
   - Mailbox
   - Process struct
\`\`\`

---

## Defensive Programming

Without try/catch, use defensive patterns:

### Check Before Use

\`\`\`arnm
fn safe_divide(a: i32, b: i32) -> i32 {
    if (b == 0) {
        print(-1);  // Error indicator
        return 0;
    }
    return a / b;
}
\`\`\`

### Validate Input

\`\`\`arnm
fn process_value(x: i32) {
    if (x < 0 || x > 100) {
        print(-1);  // Out of range
        return;
    }
    // Safe to use x
    print(x);
}
\`\`\`

### Use Structs for Results

\`\`\`arnm
struct DivResult {
    value: i32,
    ok: bool
}

fn divide(a: i32, b: i32) -> DivResult {
    if (b == 0) {
        return DivResult { value: 0, ok: false };
    }
    return DivResult { value: a / b, ok: true };
}

fn main() {
    let result = divide(10, 0);
    if (result.ok) {
        print(result.value);
    } else {
        print(-1);  // Handle error
    }
}
\`\`\`

---

## Actor Error Isolation

One key benefit of actors: errors are isolated.

\`\`\`arnm
actor Worker {
    fn init() {
        // This worker crashes
        let x = 1 / 0;
    }
}

fn main() {
    spawn Worker();   // Worker crashes...
    spawn Worker();   // ...but this one still runs!
    print(42);        // Main continues
}
\`\`\`

Each actor is a separate process — one crashing doesn't kill others.

---

## Future Error Handling

### Supervision (Erlang-style)

\`\`\`arnm
// Future: Supervisors monitor workers
supervisor Main {
    children: [Worker, Worker, Worker],
    strategy: one_for_one  // Restart only failed child
}
\`\`\`

### Process Linking

\`\`\`arnm
// Future: Linked processes die together
spawn Worker() |> link();

// Future: Monitors detect death
let ref = spawn Worker() |> monitor();
receive {
    Down(ref, reason) => handle_death(reason)
}
\`\`\`

### Result Types

\`\`\`arnm
// Future: Result enum
enum Result<T, E> {
    Ok(T),
    Err(E)
}

fn parse_int(s: String) -> Result<i32, String> {
    // ...
}
\`\`\`

### Panic and Recover

\`\`\`arnm
// Future: explicit panics
fn must_be_positive(n: i32) {
    if (n <= 0) {
        panic("must be positive");
    }
}
\`\`\`
`
});
