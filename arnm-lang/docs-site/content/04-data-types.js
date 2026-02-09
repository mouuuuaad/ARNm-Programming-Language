// Section 04: Data Types
// Complete reference for all ARNm data types

window.addDocSection({
    id: 'data-types',
    section: 'Basics',
    level: 1,
    title: 'Data Types',
    content: `
# Data Types in ARNm

ARNm is a **statically typed** language — every value has a known type at compile time. This guide covers all built-in types and how to use them.

---

## Integer Types

Integers are whole numbers without decimal points.

### Signed Integers

| Type | Size | Minimum | Maximum |
|------|------|---------|---------|
| i8 | 8 bits | -128 | 127 |
| i16 | 16 bits | -32,768 | 32,767 |
| i32 | 32 bits | -2,147,483,648 | 2,147,483,647 |
| i64 | 64 bits | -9.2×10¹⁸ | 9.2×10¹⁸ |

**Default integer type:** When you write \`42\`, ARNm infers \`i64\`.

\`\`\`arnm
let small: i32 = 100;
let large: i64 = 9223372036854775807;
let byte: i8 = -128;
\`\`\`

### Integer Literals

ARNm supports multiple number formats:

| Format | Prefix | Example | Decimal Value |
|--------|--------|---------|---------------|
| Decimal | (none) | 42 | 42 |
| Hexadecimal | 0x / 0X | 0xFF | 255 |
| Binary | 0b / 0B | 0b1010 | 10 |
| Octal | 0o / 0O | 0o755 | 493 |

\`\`\`arnm
let decimal = 42;
let hex = 0xFF;           // 255
let binary = 0b11110000;  // 240
let octal = 0o644;        // 420

// These are all the same:
let a = 255;
let b = 0xFF;
let c = 0b11111111;
let d = 0o377;
\`\`\`

### Integer Operations

\`\`\`arnm
let a = 10;
let b = 3;

print(a + b);   // 13  (addition)
print(a - b);   // 7   (subtraction)
print(a * b);   // 30  (multiplication)
print(a / b);   // 3   (integer division)
print(a % b);   // 1   (modulo/remainder)
\`\`\`

**Integer Division**: Division of integers always rounds toward zero:
\`\`\`arnm
print(7 / 2);    // 3 (not 3.5)
print(-7 / 2);   // -3 (toward zero)
\`\`\`

---

## Boolean Type

Booleans represent truth values: \`true\` or \`false\`.

\`\`\`arnm
let is_active = true;
let is_done = false;
\`\`\`

### Boolean Operations

\`\`\`arnm
let a = true;
let b = false;

// Logical AND (both must be true)
print(a && b);   // false
print(a && a);   // true

// Logical OR (at least one true)
print(a || b);   // true
print(b || b);   // false

// Logical NOT (invert)
print(!a);       // false
print(!b);       // true
\`\`\`

### Short-Circuit Evaluation

Logical operators are **short-circuit** — they stop early if the result is determined:

\`\`\`arnm
// With &&, if left is false, right is NOT evaluated
false && expensive_function();  // expensive_function never called

// With ||, if left is true, right is NOT evaluated
true || expensive_function();   // expensive_function never called
\`\`\`

### Comparison Results

All comparisons produce booleans:

\`\`\`arnm
let x = 5;
let y = 10;

print(x == y);   // false
print(x != y);   // true
print(x < y);    // true
print(x <= y);   // true
print(x > y);    // false
print(x >= y);   // false
\`\`\`

---

## String Type

Strings are sequences of characters enclosed in double quotes.

\`\`\`arnm
let greeting = "Hello, ARNm!";
let empty = "";
let quote = "She said \\"hello\\"";
\`\`\`

### Escape Sequences

| Escape | Character | Description |
|--------|-----------|-------------|
| \\n | Newline | Line break |
| \\r | Carriage Return | Return to line start |
| \\t | Tab | Horizontal tab |
| \\\\\\\\ | Backslash | Literal backslash |
| \\" | Double Quote | Literal quote in string |
| \\0 | Null | Null character |

\`\`\`arnm
let multiline = "Line 1\\nLine 2\\nLine 3";
let path = "C:\\\\Users\\\\Name";
let quoted = "He said \\"Hi\\"";
let tabbed = "Col1\\tCol2\\tCol3";
\`\`\`

---

## Character Type

A single character, enclosed in single quotes:

\`\`\`arnm
let letter: char = 'A';
let digit: char = '7';
let newline: char = '\\n';
\`\`\`

### Character vs String

\`\`\`arnm
let c: char = 'A';     // Single character
let s: String = "A";   // String with one character

// These are different types!
\`\`\`

---

## Special Values

### nil

The \`nil\` value represents the absence of a value:

\`\`\`arnm
let nothing = nil;
\`\`\`

**Use cases:**
- Uninitialized optional values
- Representing "no result"
- Sentinel values

### self

The \`self\` keyword returns a reference to the current process:

\`\`\`arnm
actor Counter {
    let count: i32;
    
    fn init() {
        self.count = 0;      // Access field
        let me = self;       // Store process reference
        me ! 42;             // Send message to self
    }
}
\`\`\`

---

## Process Type

The \`Process\` type represents a reference to an actor/process:

\`\`\`arnm
fn main() {
    let worker: Process = spawn Worker();
    worker ! 42;            // Send message
}
\`\`\`

**Process lifecycle:**
- Created by \`spawn\`
- Lives until the process terminates
- References to dead processes are invalid (MVP: undefined behavior)

---

## Type Annotations Summary

\`\`\`arnm
// Integers
let a: i32 = 42;
let b: i64 = 1000000;

// Boolean
let flag: bool = true;

// Character
let ch: char = 'A';

// Process reference
let p: Process = spawn Actor();

// With inference (type omitted)
let x = 42;        // i64
let s = "hello";   // String
let ok = true;     // bool
\`\`\`

---

## Type Conversion

Currently, ARNm has limited type conversion. Explicit casts are planned for future versions.

\`\`\`arnm
// Integer to different size (planned)
// let small: i32 = large as i32;
\`\`\`

---

## Best Practices

### 1. Use the Smallest Sufficient Type

\`\`\`arnm
// For small counts, i32 is often enough
let count: i32 = 0;

// For large values or when unsure, use i64
let big_number: i64 = 10000000000;
\`\`\`

### 2. Be Explicit When It Matters

\`\`\`arnm
// Good: clear intention
let max_size: i32 = 1024;
let is_valid: bool = true;

// OK: inference is fine for local temporaries
let temp = x + y;
\`\`\`

### 3. Use Meaningful Boolean Names

\`\`\`arnm
// Good: reads like English
let is_connected = true;
let has_permission = false;
let can_edit = true;

// Avoid: unclear meaning
let flag1 = true;
let ok = false;
\`\`\`
`
});
