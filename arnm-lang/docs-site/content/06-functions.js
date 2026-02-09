// Section 06: Functions
// Complete guide to functions in ARNm

window.addDocSection({
    id: 'functions',
    section: 'Functions',
    level: 1,
    title: 'Functions',
    content: `
# Functions in ARNm

Functions are reusable blocks of code that perform specific tasks. They are the building blocks of ARNm programs.

---

## Defining Functions

Use the \`fn\` keyword to declare a function:

\`\`\`arnm
fn greet() {
    print(100);
}
\`\`\`

### Function Components

\`\`\`arnm
fn function_name(param1: Type, param2: Type) -> ReturnType {
    // function body
    return value;
}
\`\`\`

| Component | Description |
|-----------|-------------|
| \`fn\` | Function declaration keyword |
| \`function_name\` | Identifier for the function |
| \`parameters\` | Input values (optional) |
| \`-> ReturnType\` | Return type (optional, omit for void) |
| \`{ body }\` | The code to execute |

---

## Parameters

### Basic Parameters

\`\`\`arnm
fn add(a: i32, b: i32) {
    print(a + b);
}

fn main() {
    add(5, 3);   // Prints: 8
}
\`\`\`

### Multiple Parameters

\`\`\`arnm
fn calculate(x: i32, y: i32, z: i32) {
    let result = x * y + z;
    print(result);
}

fn main() {
    calculate(2, 3, 4);   // Prints: 10
}
\`\`\`

### Mutable Parameters

Parameters are immutable by default. Use \`mut\` for mutable parameters:

\`\`\`arnm
fn increment(mut x: i32) {
    x = x + 1;
    print(x);
}

fn main() {
    increment(5);   // Prints: 6
}
\`\`\`

---

## Return Values

### Returning a Value

\`\`\`arnm
fn square(n: i32) -> i32 {
    return n * n;
}

fn main() {
    let result = square(5);
    print(result);   // Prints: 25
}
\`\`\`

### Early Return

\`\`\`arnm
fn absolute(n: i32) -> i32 {
    if (n < 0) {
        return -n;   // Early return for negative
    }
    return n;        // Return for non-negative
}
\`\`\`

### Void Functions

Functions without a return type return nothing:

\`\`\`arnm
fn log_message(code: i32) {
    print(code);
    // No return needed
}
\`\`\`

---

## The main Function

Every ARNm program must have a \`main\` function — it's the entry point:

\`\`\`arnm
fn main() {
    print(1);   // Program starts here
}
\`\`\`

**What happens at startup:**
1. Runtime initializes scheduler
2. \`main\` is spawned as the first process
3. \`main\` executes
4. When \`main\` returns, the process ends (other actors may continue)

---

## Function Calls

### Basic Calls

\`\`\`arnm
fn double(x: i32) -> i32 {
    return x * 2;
}

fn main() {
    let a = double(5);        // 10
    let b = double(double(3)); // 12
    print(a + b);              // 22
}
\`\`\`

### Evaluation Order

Arguments are evaluated **left-to-right**:

\`\`\`arnm
fn side_effect() -> i32 {
    print(1);
    return 10;
}

fn main() {
    let x = side_effect() + side_effect();
    // Output: 1, then 1
    // x = 20
}
\`\`\`

This is **guaranteed behavior** — you can rely on the order.

---

## Recursion

Functions can call themselves:

\`\`\`arnm
fn factorial(n: i32) -> i32 {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

fn main() {
    print(factorial(5));   // 120
}
\`\`\`

**Warning**: Deep recursion can cause stack overflow. Use loops for iteration when possible.

---

## Common Patterns

### Helper Functions

\`\`\`arnm
fn max(a: i32, b: i32) -> i32 {
    if (a > b) {
        return a;
    }
    return b;
}

fn min(a: i32, b: i32) -> i32 {
    if (a < b) {
        return a;
    }
    return b;
}

fn clamp(value: i32, low: i32, high: i32) -> i32 {
    return max(low, min(value, high));
}
\`\`\`

### Predicate Functions

\`\`\`arnm
fn is_even(n: i32) -> bool {
    return n % 2 == 0;
}

fn is_positive(n: i32) -> bool {
    return n > 0;
}

fn main() {
    if (is_even(42) && is_positive(42)) {
        print(1);   // 42 is even and positive
    }
}
\`\`\`

---

## Grammar Reference

\`\`\`
function_decl = "fn" IDENT "(" [ param_list ] ")" [ "->" type ] block ;
param_list    = param { "," param } ;
param         = [ "mut" ] IDENT ":" type ;
\`\`\`
`
});
