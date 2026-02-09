// Section 05: Operators
// Complete operator reference

window.addDocSection({
    id: 'operators',
    section: 'Basics',
    level: 1,
    title: 'Operators',
    content: `
# Operators in ARNm

Operators are symbols that perform operations on values. ARNm provides arithmetic, comparison, logical, and special operators.

---

## Arithmetic Operators

| Operator | Name | Example | Result |
|----------|------|---------|--------|
| + | Addition | 5 + 3 | 8 |
| - | Subtraction | 5 - 3 | 2 |
| * | Multiplication | 5 * 3 | 15 |
| / | Division | 15 / 4 | 3 |
| % | Modulo | 17 % 5 | 2 |

\`\`\`arnm
let a = 10;
let b = 3;

let sum = a + b;         // 13
let diff = a - b;        // 7
let product = a * b;     // 30
let quotient = a / b;    // 3 (integer division)
let remainder = a % b;   // 1

// Complex expressions
let result = (10 * 10) + (5 - 2);  // 103
\`\`\`

### Integer Division

Division always produces an integer result (truncated toward zero):

\`\`\`arnm
print(7 / 2);    // 3 (not 3.5)
print(7 / 3);    // 2
print(-7 / 2);   // -3 (toward zero)
\`\`\`

### Modulo Operator

The modulo operator returns the remainder after division:

\`\`\`arnm
print(10 % 3);   // 1 (10 = 3*3 + 1)
print(15 % 5);   // 0 (exactly divisible)
print(17 % 4);   // 1

// Common use: check if even/odd
let n = 42;
if (n % 2 == 0) {
    print(1);  // even
} else {
    print(0);  // odd
}
\`\`\`

---

## Comparison Operators

All comparison operators return a boolean (\`true\` or \`false\`).

| Operator | Meaning | Example | Result |
|----------|---------|---------|--------|
| == | Equal | 5 == 5 | true |
| != | Not equal | 5 != 3 | true |
| < | Less than | 3 < 5 | true |
| <= | Less or equal | 5 <= 5 | true |
| > | Greater than | 5 > 3 | true |
| >= | Greater or equal | 3 >= 5 | false |

\`\`\`arnm
let x = 10;
let y = 20;

print(x == y);   // false
print(x != y);   // true
print(x < y);    // true
print(x <= y);   // true
print(x > y);    // false
print(x >= y);   // false

// Chained comparisons (use &&)
let age = 25;
if (age >= 18 && age < 65) {
    print(1);  // working age
}
\`\`\`

---

## Logical Operators

| Operator | Name | Description |
|----------|------|-------------|
| && | AND | True if both operands are true |
| \\|\\| | OR | True if at least one operand is true |
| ! | NOT | Inverts the boolean value |

\`\`\`arnm
let a = true;
let b = false;

// AND: both must be true
print(a && b);    // false
print(a && a);    // true
print(b && b);    // false

// OR: at least one true
print(a || b);    // true
print(a || a);    // true
print(b || b);    // false

// NOT: invert
print(!a);        // false
print(!b);        // true
print(!!a);       // true (double negation)
\`\`\`

### Short-Circuit Evaluation

Logical operators stop evaluating as soon as the result is determined:

\`\`\`arnm
// With &&, if left is false, right is skipped
false && print(1);   // print never executes

// With ||, if left is true, right is skipped
true || print(1);    // print never executes

// Useful for safe access patterns
if (x != nil && x.value > 0) {
    // x.value only accessed if x is not nil
}
\`\`\`

---

## Message Send Operator

The \`!\` operator sends a message to a process:

\`\`\`arnm
target ! 42;           // Send 42 to target
target ! some_value;   // Send variable value

// Multiple sends
let p = spawn Worker();
p ! 1;
p ! 2;
p ! 3;
\`\`\`

**How it works:**
1. Left side is evaluated (target process)
2. Right side is evaluated (message value)
3. Message is copied and enqueued in target's mailbox
4. If target is waiting, it's woken up

---

## Unary Operators

| Operator | Name | Example |
|----------|------|---------|
| - | Negation | -42 |
| ! | Logical NOT | !true |
| ~ | Bitwise NOT | ~0xFF |

\`\`\`arnm
let x = 42;
let neg = -x;      // -42
let pos = -(-x);   // 42

let flag = true;
let inv = !flag;   // false
\`\`\`

---

## Operator Precedence

From **lowest** to **highest** precedence:

| Precedence | Operator | Associativity |
|------------|----------|---------------|
| 1 (lowest) | = | Right |
| 2 | \\|\\| | Left |
| 3 | && | Left |
| 4 | == != | Left |
| 5 | < <= > >= | Left |
| 6 | ! (send) | Left |
| 7 | + - | Left |
| 8 | * / % | Left |
| 9 | - ! ~ (unary) | Right |
| 10 (highest) | () [] . | Left |

**Examples:**

\`\`\`arnm
// Multiplication before addition
2 + 3 * 4     // = 2 + 12 = 14

// Comparison before logical
a < b && c > d  // = (a < b) && (c > d)

// Use parentheses for clarity
let result = (a + b) * (c - d);
\`\`\`

---

## Compound Expressions

\`\`\`arnm
// Complex arithmetic
let area = width * height;
let perimeter = 2 * (width + height);
let average = (a + b + c) / 3;

// Complex conditions
if ((age >= 18 && hasLicense) || isEmergency) {
    print(1);  // can drive
}

// Combining with message send
(spawn Worker()) ! 42;  // spawn and send
\`\`\`

---

## Best Practices

### 1. Use Parentheses for Clarity

\`\`\`arnm
// Good: clear precedence
let result = (a * b) + (c * d);

// Confusing: relies on precedence rules
let result = a * b + c * d;  // Same result, less clear
\`\`\`

### 2. Break Complex Expressions

\`\`\`arnm
// Good: readable steps
let base = width * height;
let bonus = extra * multiplier;
let total = base + bonus;

// Hard to read
let total = width * height + extra * multiplier;
\`\`\`

### 3. Use Short-Circuit for Guards

\`\`\`arnm
// Safe pattern: check before access
if (list != nil && list.length > 0) {
    process(list);
}
\`\`\`
`
});
