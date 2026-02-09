// Section 02: Getting Started
// Complete setup guide for ARNm

window.addDocSection({
    id: 'getting-started',
    section: 'Getting Started',
    level: 1,
    title: 'Getting Started',
    content: `
# Getting Started with ARNm

This guide will walk you through installing ARNm, understanding the toolchain, and writing your first programs.

---

## System Requirements

### Supported Platforms

| Platform | Architecture | Status |
|----------|--------------|--------|
| Linux | x86_64 | <span class="status status-done"></span> Fully Supported |
| macOS | x86_64 | <span class="status status-wip"></span> Experimental |
| Windows | x86_64 | <span class="status status-todo"></span> Not Yet |

### Prerequisites

Before installing ARNm, ensure you have:

- **GCC** (GNU Compiler Collection) — for assembling and linking
- **Make** — for building the compiler and runtime
- **Git** — for cloning the repository

**Check your setup:**

\`\`\`bash
gcc --version    # Should show GCC 9.0+
make --version   # GNU Make 4.0+
git --version    # Any recent version
\`\`\`

---

## Installation

### Step 1: Clone the Repository

\`\`\`bash
git clone https://github.com/mouuuuaad/ARNm-Programming-Language.git
cd ARNm-Programming-Language/arnm-lang
\`\`\`

### Step 2: Build the Compiler

The ARNm compiler (\`arnmc\`) is written in C and compiles quickly:

\`\`\`bash
make clean    # Start fresh
make          # Build compiler
\`\`\`

**What gets built:**
- \`build/arnmc\` — The ARNm compiler executable
- Object files in \`build/\` for each compiler module

### Step 3: Build the Runtime

The runtime library provides process management, scheduling, and messaging:

\`\`\`bash
make -C runtime clean
make -C runtime
\`\`\`

**What gets built:**
- \`runtime/build/libarnm.a\` — Static runtime library
- \`runtime/build/crt0.o\` — C runtime startup code

### Step 4: Verify Installation

\`\`\`bash
./build/arnmc --help
\`\`\`

You should see the compiler's help message.

---

## Project Structure

After building, your directory looks like:

\`\`\`
arnm-lang/
├── build/
│   └── arnmc              # The compiler
├── compiler/
│   ├── src/               # Compiler source code
│   └── include/           # Compiler headers
├── runtime/
│   ├── build/
│   │   ├── libarnm.a      # Runtime library
│   │   └── crt0.o         # Entry point
│   ├── src/               # Runtime source
│   └── include/           # Runtime headers
├── examples/              # Example programs
└── docs/                  # Documentation
\`\`\`

---

## Your First Program

### Step 1: Create a Source File

Create \`hello.arnm\`:

\`\`\`arnm
fn main() {
    print(42);
}
\`\`\`

### Step 2: Compile to Assembly

\`\`\`bash
./build/arnmc --emit-asm hello.arnm > hello.s
\`\`\`

**What this does:**
1. Lexer tokenizes the source code
2. Parser builds an Abstract Syntax Tree
3. Semantic analysis checks types
4. IR generator creates intermediate representation
5. Code generator outputs x86_64 assembly

### Step 3: Assemble

Convert assembly to object code:

\`\`\`bash
gcc -c hello.s -o hello.o
\`\`\`

### Step 4: Link

Link with the runtime to create an executable:

\`\`\`bash
gcc -o hello \\
    runtime/build/crt0.o \\
    hello.o \\
    -Lruntime/build \\
    -larnm \\
    -lpthread
\`\`\`

**Understanding the linker command:**
- \`crt0.o\` — Entry point that initializes the runtime
- \`hello.o\` — Your compiled program
- \`-larnm\` — The ARNm runtime library
- \`-lpthread\` — POSIX threads (required by scheduler)

### Step 5: Run

\`\`\`bash
./hello
\`\`\`

**Output:**
\`\`\`
42
\`\`\`

---

## Quick Compile Script

For convenience, create a \`compile.sh\` script:

\`\`\`bash
#!/bin/bash
# compile.sh - Compile an ARNm program

if [ -z "$1" ]; then
    echo "Usage: ./compile.sh <file.arnm>"
    exit 1
fi

NAME="\${1%.arnm}"

./build/arnmc --emit-asm "$1" > "$NAME.s" && \\
gcc -c "$NAME.s" -o "$NAME.o" && \\
gcc -o "$NAME" runtime/build/crt0.o "$NAME.o" \\
    -Lruntime/build -larnm -lpthread && \\
echo "Built: $NAME"
\`\`\`

Usage:
\`\`\`bash
chmod +x compile.sh
./compile.sh hello.arnm
./hello
\`\`\`

---

## Running Tests

The ARNm project includes comprehensive tests:

\`\`\`bash
# Run all compiler tests
make test

# Run specific test suites
make test_lexer     # Tokenization tests
make test_parser    # Parsing tests
make test_sema      # Semantic analysis tests
make test_irgen     # IR generation tests
\`\`\`

---

## Troubleshooting

### "Command not found: arnmc"

You need to use the full path or add to PATH:

\`\`\`bash
./build/arnmc hello.arnm    # Use relative path
# OR
export PATH="$PWD/build:$PATH"
arnmc hello.arnm
\`\`\`

### "undefined reference to arnm_spawn"

Make sure you're linking with the runtime:

\`\`\`bash
gcc -o program ... -Lruntime/build -larnm -lpthread
\`\`\`

### Segmentation Fault on Run

Check that you:
1. Built the runtime (\`make -C runtime\`)
2. Linked \`crt0.o\` FIRST in the link command
3. Included \`-lpthread\`

---

## Next Steps

Now that you have ARNm running:

1. Learn about **[Variables](#variables)** and bindings
2. Explore **[Data Types](#data-types)**
3. Write **[Functions](#functions)**
4. Dive into **[Actors](#actors)** for concurrency
`
});
