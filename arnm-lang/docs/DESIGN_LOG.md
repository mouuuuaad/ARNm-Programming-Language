# ARNm Design Log

This document records architectural decisions, trade-offs, and rationale for the ARNm compiler and runtime.

---

## Phase I: Bootstrap Frontend

### Decision 1: Zero-Allocation Lexer

**Date**: 2026-01-09

**Context**: The lexer is the first stage of compilation, processing every byte of source code.

**Decision**: Tokens reference source memory directly via pointer + length.

**Rationale**:
- Eliminates heap allocation overhead during tokenization
- Cache-friendly: tokens are small (24 bytes) and sequential access into source buffer
- Source buffer lifetime is controlled by the caller

**Trade-offs**:
- Source buffer must remain valid for token lifetime
- Cannot modify source after tokenization begins
- Requires caller to manage source memory

---

### Decision 2: State-Machine Lexer Design

**Date**: 2026-01-09

**Context**: Lexer implementation approach—table-driven vs switch-based.

**Decision**: Switch-based state machine with character classification inline functions.

**Rationale**:
- Switch statements compile to efficient jump tables
- Inline classification functions enable compiler optimizations
- More readable and maintainable than large lookup tables
- Branch prediction works well for common ASCII characters

---

### Decision 3: Keyword Recognition via Binary Search

**Date**: 2026-01-09

**Context**: Keywords must be distinguished from identifiers efficiently.

**Decision**: Sorted keyword table with binary search (O(log n)).

**Alternatives Considered**:
1. Perfect hash (O(1) but complex to maintain)
2. Trie (O(k) but higher memory)
3. Linear scan (O(n) too slow)

**Rationale**:
- 26 keywords means ~5 comparisons worst case
- Simple to add new keywords
- Zero additional memory allocation
- Good cache behavior with sorted table

---

### Decision 4: AST Arena Allocation

**Date**: 2026-01-09

**Context**: AST nodes have varying lifetimes but are all freed together.

**Decision**: Single arena allocator for all AST nodes.

**Rationale**:
- Single allocation site eliminates fragmentation
- O(1) deallocation (free entire arena)
- No per-node free needed
- Memory locality improves cache performance

**Implementation**:
- `ast_arena_init()` allocates a contiguous buffer
- `ast_arena_alloc()` bumps pointer (8-byte aligned)
- `ast_arena_destroy()` frees entire buffer

---

### Decision 5: Tagged Union AST Nodes

**Date**: 2026-01-09

**Context**: AST needs to represent many node types with exhaustive matching.

**Decision**: Tagged union pattern—`kind` enum + union of structs.

**Rationale**:
- Single allocation per node
- Compiler warns on non-exhaustive switch
- Fixed-size nodes improve cache behavior
- Natural mapping to C struct layout

---

### Decision 6: Pratt Parser for Expressions

**Date**: 2026-01-09

**Context**: Expression parsing with correct precedence.

**Decision**: Pratt (precedence climbing) parser within recursive descent.

**Rationale**:
- Handles precedence elegantly without explicit grammar factoring
- Easy to add new operators with precedence table
- Natural left/right associativity control
- Integrates cleanly with recursive descent for statements

---

### Decision 7: Message Send as Infix Operator

**Date**: 2026-01-09

**Context**: ARNm uses `!` for message sending: `actor ! message`.

**Decision**: Parse `!` as binary infix operator with dedicated precedence.

**Rationale**:
- Erlang-inspired syntax is familiar to concurrent programmers
- Distinguished from logical NOT (prefix `!`)
- Clear visual separation between target and message
- Precedence below comparison, above logical operators

---

## Pending Decisions

### Runtime Scheduler Strategy

Options under consideration:
1. Work-stealing with per-core run queues
2. Global queue with local caching
3. Hybrid approach

Decision deferred until Phase III runtime implementation.

---

### Cycle Detection Algorithm

Options under consideration:
1. Trial deletion (simpler, may have pauses)
2. Concurrent mark-sweep (complex, lower latency)
3. Bacon-Rajan collector (proven for ARC supplements)

Decision deferred until Phase III memory subsystem.
