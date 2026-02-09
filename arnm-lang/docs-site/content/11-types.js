// Section 11: Type System
// ARNm's static type system

window.addDocSection({
    id: 'type-system',
    section: 'Advanced',
    level: 1,
    title: 'Type System',
    content: `
# Type System

ARNm uses **static typing** — all types are known at compile time.

---

## Why Static Typing?

| Benefit | Description |
|---------|-------------|
| Early Error Detection | Type errors caught before running |
| Better Performance | No runtime type checks needed |
| Self-Documentation | Types describe what code expects |
| IDE Support | Better autocompletion and refactoring |

---

## Primitive Types

| Type | Description | Example |
|------|-------------|---------|
| i32 | 32-bit signed integer | \`42\` |
| i64 | 64-bit signed integer | \`1000000\` |
| bool | Boolean | \`true\`, \`false\` |
| char | Single character | \`'A'\` |
| String | Text string | \`"hello"\` |

---

## Type Inference

ARNm can often infer types from context:

\`\`\`arnm
let x = 42;          // i64 (default integer)
let y = 3.14;        // f64 (default float)
let s = "hello";     // String
let b = true;        // bool
let c = 'A';         // char
\`\`\`

---

## Explicit Type Annotations

Specify types explicitly when needed:

\`\`\`arnm
let count: i32 = 0;
let big: i64 = 9999999999;
let flag: bool = true;
let initial: char = 'X';
\`\`\`

### When to Use Explicit Types

1. **Different from default inference:**
\`\`\`arnm
let small: i32 = 42;   // i32 instead of i64
\`\`\`

2. **Documentation:**
\`\`\`arnm
let max_connections: i32 = 100;  // Clear intent
\`\`\`

3. **Required by compiler:**
\`\`\`arnm
fn add(a: i32, b: i32) -> i32 {  // Parameters need types
    return a + b;
}
\`\`\`

---

## Function Types

Functions have types based on parameters and return:

\`\`\`arnm
fn add(a: i32, b: i32) -> i32 {
    return a + b;
}
// Type: fn(i32, i32) -> i32

fn greet() {
    print(100);
}
// Type: fn() -> void
\`\`\`

---

## Process Type

References to actors/processes:

\`\`\`arnm
fn main() {
    let worker: Process = spawn Worker();
    worker ! 42;
}
\`\`\`

---

## Struct Types

User-defined compound types:

\`\`\`arnm
struct Point {
    x: i32,
    y: i32
}

let p: Point = Point { x: 10, y: 20 };
\`\`\`

---

## Type Checking Rules

### Assignment Compatibility

\`\`\`arnm
let x: i32 = 42;    // OK
let y: i32 = true;  // ERROR: type mismatch
\`\`\`

### Function Argument Types

\`\`\`arnm
fn double(n: i32) -> i32 {
    return n * 2;
}

double(10);     // OK
double("hi");   // ERROR: expected i32, got String
\`\`\`

### Return Type Checking

\`\`\`arnm
fn get_value() -> i32 {
    return "hello";  // ERROR: expected i32, got String
}
\`\`\`

---

## Type Grammar

\`\`\`
type         = type_primary [ type_suffix ] ;
type_primary = IDENT                           // Named type
             | "fn" "(" [ type_list ] ")" [ "->" type ]
             ;
type_suffix  = "?"                             // Optional (future)
             | "[" [ expr ] "]"                // Array (future)
             ;
\`\`\`

---

## Future Type Features

### Generics (Planned)
\`\`\`arnm
// Future syntax
fn map<T, U>(list: List<T>, f: fn(T) -> U) -> List<U>
\`\`\`

### Optional Types (Planned)
\`\`\`arnm
// Future syntax
let maybe_value: i32? = nil;
\`\`\`

### Sum Types / Enums (Planned)
\`\`\`arnm
// Future syntax
enum Result {
    Ok(i32),
    Error(String)
}
\`\`\`
`
});
