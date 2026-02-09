// Section 03: Variables
// Complete guide to variables, bindings, and scope

window.addDocSection({
    id: 'variables',
    section: 'Basics',
    level: 1,
    title: 'Variables',
    content: `
# Variables in ARNm

Variables are the foundation of any program. In ARNm, variables are **bindings** that associate names with values. Understanding how variables work is essential for writing correct, efficient code.

---

## Declaring Variables

### The \`let\` Keyword

Variables are declared using the \`let\` keyword:

\`\`\`arnm
let x = 42;
let name = "ARNm";
let flag = true;
\`\`\`

**What happens internally:**
1. Stack space is allocated for the value
2. The value is computed and stored
3. The name is bound to that memory location

### The \`const\` Keyword

Use \`const\` for bindings that must never change and must be initialized:

\`\`\`arnm
const answer = 42;
const app_name: String = "ARNm";
\`\`\`

**Rules:**
- \`const\` is always immutable
- An initializer is required
- \`mut\` is not allowed with \`const\`

### Short Declarations (\`:=\`)

Go-style short declarations provide a compact syntax for immutable bindings:

\`\`\`arnm
total := 0;
name := "compiler";
\`\`\`

**Note:** \`x := expr\` is sugar for \`let x = expr\` (no type annotation).

### Type Inference

ARNm can usually infer the type from the initial value:

\`\`\`arnm
let x = 42;        // Inferred as i64
let pi = 3.14;     // Inferred as f64
let msg = "hello"; // Inferred as String
let ok = true;     // Inferred as bool
\`\`\`

### Explicit Type Annotations

You can specify types explicitly:

\`\`\`arnm
let x: i32 = 42;
let y: i64 = 1000000;
let flag: bool = true;
\`\`\`

**When to use explicit types:**
- When the inferred type isn't what you want
- For documentation/clarity
- When required by complex expressions

---

## Immutability by Default

### Immutable Variables

By default, all variables in ARNm are **immutable** — once assigned, their value cannot change:

\`\`\`arnm
let x = 42;
x = 100;    // ERROR: cannot assign to immutable variable
\`\`\`

**Why immutability?**
- Prevents accidental modification
- Enables compiler optimizations
- Makes concurrent code safer
- Reduces bugs

### Mutable Variables

Use the \`mut\` keyword for mutable variables:

\`\`\`arnm
let mut counter = 0;
counter = counter + 1;  // OK: counter is mutable
counter = counter + 1;
print(counter);         // Prints: 2
\`\`\`

**When to use \`mut\`:**
- Loop counters
- Accumulators
- State that genuinely needs to change

\`\`\`arnm
// Common pattern: loop counter
let mut i = 0;
while (i < 10) {
    print(i);
    i = i + 1;
}

// Common pattern: accumulator
let mut sum = 0;
let mut n = 1;
while (n <= 100) {
    sum = sum + n;
    n = n + 1;
}
print(sum);  // 5050
\`\`\`

---

## Scope and Lifetime

### Lexical Scoping

ARNm uses **lexical (static) scoping**. A variable is visible from its declaration to the end of its enclosing block:

\`\`\`arnm
fn main() {
    // x not visible here yet
    
    let x = 1;
    // x is visible from here...
    
    {
        let y = 2;
        // Both x and y visible here
        print(x + y);  // 3
    }
    // y no longer visible, but x still is
    
    print(x);  // 1
    // print(y);  // ERROR: y not in scope
}
\`\`\`

### Block Scope

Every \`{ }\` block creates a new scope:

\`\`\`arnm
fn main() {
    let a = 1;           // Scope level 1
    {
        let b = 2;       // Scope level 2
        {
            let c = 3;   // Scope level 3
            print(a + b + c);  // All visible: 6
        }
        // c gone
    }
    // b gone
    print(a);  // Still visible: 1
}
\`\`\`

### Variable Lifetime

A variable's **lifetime** is the portion of the program where the variable's memory is valid:

\`\`\`arnm
fn main() {
    let x = 42;        // x lifetime begins
    {
        let y = x * 2; // y lifetime begins, x still valid
        print(y);      // 84
    }                  // y lifetime ends, memory reclaimed
    print(x);          // x still valid
}                      // x lifetime ends
\`\`\`

---

## Shadowing

### What is Shadowing?

A new variable can **shadow** (hide) an existing one with the same name:

\`\`\`arnm
let x = 1;
print(x);    // 1

let x = 2;   // This shadows the previous x
print(x);    // 2

let x = "hello";  // Can even change types!
print(x);         // (prints string)
\`\`\`

### Shadowing vs Mutation

**Shadowing** creates a new variable:
\`\`\`arnm
let x = 1;
let x = x + 1;   // Creates NEW x, old x still exists (hidden)
let x = x * 2;   // Creates another NEW x
print(x);        // 4
\`\`\`

**Mutation** changes an existing variable:
\`\`\`arnm
let mut x = 1;
x = x + 1;       // Modifies SAME x
x = x * 2;       // Same x again
print(x);        // 4
\`\`\`

### When to Use Shadowing

Shadowing is useful for transformations:

\`\`\`arnm
let input = "  hello  ";
let input = trim(input);      // Shadow with trimmed version
let input = uppercase(input); // Shadow with uppercase
print(input);                 // "HELLO"
\`\`\`

---

## Scope Rules Summary

| Rule | Description |
|------|-------------|
| SCOPE-001 | A name must be defined before use |
| SCOPE-002 | A name cannot be redefined in the same scope |
| SCOPE-003 | Inner scopes can shadow outer names |
| SCOPE-004 | \`self\` is only valid inside actors/spawned processes |
| SCOPE-005 | \`break\`/\`continue\` only valid inside loops |

---

## Best Practices

### 1. Prefer Immutability

\`\`\`arnm
// Good: immutable where possible
let result = compute_value();

// Avoid: unnecessary mutation
let mut result = 0;
result = compute_value();
\`\`\`

### 2. Limit Mutable Variable Scope

\`\`\`arnm
// Good: mut limited to loop
let mut i = 0;
while (i < 10) {
    process(i);
    i = i + 1;
}
// i no longer needed

// Then use immutable for result
let total = calculate_total();
\`\`\`

### 3. Use Descriptive Names

\`\`\`arnm
// Good
let message_count = 42;
let is_connected = true;
let user_name = "Alice";

// Avoid
let x = 42;
let flag = true;
let s = "Alice";
\`\`\`

### 4. Keep Scopes Small

\`\`\`arnm
// Good: y exists only where needed
let x = 1;
{
    let y = expensive_compute();
    use_y(y);
}
// y memory freed

// Avoid: keeping y alive unnecessarily
let x = 1;
let y = expensive_compute();
use_y(y);
// y still taking memory even if not needed
\`\`\`
`
});
