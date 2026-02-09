// Section 07: Control Flow
// Complete guide to control flow in ARNm

window.addDocSections([
    {
        id: 'if-else',
        section: 'Control Flow',
        level: 1,
        title: 'If / Else',
        content: `
# If / Else Statements

Conditional statements let your program make decisions based on boolean expressions.

---

## Basic If Statement

\`\`\`arnm
if (condition) {
    // executes if condition is true
}
\`\`\`

**Example:**

\`\`\`arnm
let x = 10;

if (x > 5) {
    print(1);   // This prints: 1
}
\`\`\`

---

## If-Else Statement

\`\`\`arnm
if (condition) {
    // executes if true
} else {
    // executes if false
}
\`\`\`

**Example:**

\`\`\`arnm
let age = 15;

if (age >= 18) {
    print(1);   // adult
} else {
    print(0);   // minor - This prints
}
\`\`\`

---

## Else-If Chains

\`\`\`arnm
if (condition1) {
    // first choice
} else if (condition2) {
    // second choice
} else if (condition3) {
    // third choice
} else {
    // default
}
\`\`\`

**Example:**

\`\`\`arnm
let score = 85;

if (score >= 90) {
    print(1);   // A
} else if (score >= 80) {
    print(2);   // B - This prints
} else if (score >= 70) {
    print(3);   // C
} else {
    print(4);   // F
}
\`\`\`

---

## Nested Conditions

\`\`\`arnm
let x = 10;
let y = 20;

if (x > 0) {
    if (y > 0) {
        print(1);   // Both positive
    } else {
        print(2);   // x positive, y not
    }
} else {
    print(3);       // x not positive
}
\`\`\`

---

## Combining Conditions

Use logical operators for complex conditions:

\`\`\`arnm
let age = 25;
let has_license = true;

// AND: both must be true
if (age >= 18 && has_license) {
    print(1);   // Can drive
}

// OR: at least one true
if (age < 12 || age > 65) {
    print(2);   // Special considerations
}

// NOT: invert condition
if (!has_license) {
    print(3);   // Cannot drive
}
\`\`\`

---

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
# Loops in ARNm

Loops allow you to execute code repeatedly.

---

## While Loop

Repeats while a condition is true:

\`\`\`arnm
while (condition) {
    // body
}
\`\`\`

**Example: Count to 5**

\`\`\`arnm
let mut i = 1;
while (i <= 5) {
    print(i);
    i = i + 1;
}
// Output: 1 2 3 4 5
\`\`\`

**Example: Sum numbers**

\`\`\`arnm
let mut sum = 0;
let mut n = 1;
while (n <= 100) {
    sum = sum + n;
    n = n + 1;
}
print(sum);   // 5050
\`\`\`

---

## Loop (Infinite Loop)

Runs forever until \`break\`:

\`\`\`arnm
loop {
    // runs forever
    if (should_stop) {
        break;
    }
}
\`\`\`

**Example: Find first match**

\`\`\`arnm
let mut n = 1;
loop {
    if (n * n > 100) {
        print(n);   // First n where n² > 100
        break;
    }
    n = n + 1;
}
// Output: 11
\`\`\`

---

## For Loop

Iterate over a range:

\`\`\`arnm
for i in range(0, 10) {
    print(i);
}
// Output: 0 1 2 3 4 5 6 7 8 9
\`\`\`

---

## Break Statement

Exit the loop immediately:

\`\`\`arnm
let mut i = 0;
while (i < 100) {
    if (i == 5) {
        break;   // Exit when i reaches 5
    }
    print(i);
    i = i + 1;
}
// Output: 0 1 2 3 4
\`\`\`

---

## Continue Statement

Skip to the next iteration:

\`\`\`arnm
let mut i = 0;
while (i < 5) {
    i = i + 1;
    if (i == 3) {
        continue;   // Skip printing 3
    }
    print(i);
}
// Output: 1 2 4 5
\`\`\`

---

## Complex Loop Example

\`\`\`arnm
let mut j = 0;
loop {
    j = j + 1;
    
    if (j == 2) {
        continue;   // Skip 2
    }
    
    if (j > 4) {
        break;      // Stop after 4
    }
    
    print(j);
}
// Output: 1 3 4
\`\`\`

---

## Nested Loops

\`\`\`arnm
let mut row = 0;
while (row < 3) {
    let mut col = 0;
    while (col < 3) {
        print(row * 10 + col);
        col = col + 1;
    }
    row = row + 1;
}
// Output: 0 1 2 10 11 12 20 21 22
\`\`\`

---

## Grammar

\`\`\`
while_stmt    = "while" expression block ;
loop_stmt     = "loop" block ;
for_stmt      = "for" IDENT "in" expression block ;
break_stmt    = "break" ";" ;
continue_stmt = "continue" ";" ;
\`\`\`
`
    }
]);
