// Section 14: Compilation
// How the ARNm compiler works

window.addDocSection({
    id: 'compilation',
    section: 'Internals',
    level: 1,
    title: 'Compilation',
    content: `
# Compilation

The ARNm compiler (\`arnmc\`) transforms source code into machine code through several phases.

---

## Compilation Pipeline

\`\`\`
.arnm Source File
        │
        ▼
┌───────────────┐
│     Lexer     │  → Tokens
└───────────────┘
        │
        ▼
┌───────────────┐
│    Parser     │  → AST (Abstract Syntax Tree)
└───────────────┘
        │
        ▼
┌───────────────┐
│   Semantic    │  → Typed AST + Symbol Table
│   Analysis    │
└───────────────┘
        │
        ▼
┌───────────────┐
│  IR Generator │  → SSA IR (Intermediate Rep)
└───────────────┘
        │
        ▼
┌───────────────┐
│ Code Generator│  → x86_64 Assembly
└───────────────┘
        │
        ▼
    .s File
        │
        ▼ [GCC/AS]
    .o File
        │
        ▼ [Linker]
   Executable
\`\`\`

---

## Phase 1: Lexer

Converts source text into tokens:

\`\`\`arnm
fn main() { print(42); }
\`\`\`

Becomes:
\`\`\`
[FN] [IDENT:"main"] [LPAREN] [RPAREN] [LBRACE]
[IDENT:"print"] [LPAREN] [INT:42] [RPAREN] [SEMI]
[RBRACE]
\`\`\`

---

## Phase 2: Parser

Builds Abstract Syntax Tree:

\`\`\`
Program
└── FunctionDecl "main"
    └── Block
        └── ExprStmt
            └── Call "print"
                └── IntLit 42
\`\`\`

---

## Phase 3: Semantic Analysis

- Type checking
- Symbol resolution
- Scope validation
- Actor field analysis

---

## Phase 4: IR Generation

Generates SSA (Static Single Assignment) form:

\`\`\`
function @main:
    %0 = const i64 42
    call @arnm_print_int(%0)
    ret void
\`\`\`

---

## Phase 5: Code Generation

Produces x86_64 assembly:

\`\`\`asm
    .globl _arnm_main
_arnm_main:
    pushq %rbp
    movq %rsp, %rbp
    movq $42, %rdi
    call arnm_print_int
    movq %rbp, %rsp
    popq %rbp
    ret
\`\`\`

---

## Compiler Flags

| Flag | Description |
|------|-------------|
| \`--emit-asm\` | Output assembly |
| \`--emit-ir\` | Output IR |
| \`--dump-ast\` | Dump AST |
| \`--dump-tokens\` | Dump tokens |

\`\`\`bash
./build/arnmc --emit-asm hello.arnm > hello.s
./build/arnmc --dump-ast hello.arnm
\`\`\`

---

## Complete Build Process

### Step 1: Compile to Assembly

\`\`\`bash
./build/arnmc --emit-asm hello.arnm > hello.s
\`\`\`

### Step 2: Assemble

\`\`\`bash
gcc -c hello.s -o hello.o
\`\`\`

### Step 3: Link

\`\`\`bash
gcc -o hello \\
    runtime/build/crt0.o \\
    hello.o \\
    -Lruntime/build \\
    -larnm \\
    -lpthread
\`\`\`

### Step 4: Run

\`\`\`bash
./hello
\`\`\`

---

## Linking Components

| Component | Purpose |
|-----------|---------|
| \`crt0.o\` | Entry point, calls \`_arnm_main\` |
| \`hello.o\` | Your compiled code |
| \`libarnm.a\` | Runtime library |
| \`libpthread\` | Threading support |

---

## Generated Symbols

| Source | Assembly Symbol |
|--------|----------------|
| \`fn main()\` | \`_arnm_main\` |
| \`actor Counter\` | \`Counter_init\`, \`Counter_behavior\` |
| Actor field access | Offset from actor_state pointer |

---

## Quick Compile Script

\`\`\`bash
#!/bin/bash
NAME="\${1%.arnm}"
./build/arnmc --emit-asm "$1" > "$NAME.s" && \\
gcc -c "$NAME.s" -o "$NAME.o" && \\
gcc -o "$NAME" runtime/build/crt0.o "$NAME.o" \\
    -Lruntime/build -larnm -lpthread
\`\`\`
`
});
