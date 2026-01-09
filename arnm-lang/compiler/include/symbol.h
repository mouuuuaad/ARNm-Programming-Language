/*
 * ARNm Compiler - Symbol Table
 * 
 * DESIGN: Chained hash tables per scope for O(1) lookup.
 * Scopes form a stack, enabling nested shadowing.
 */

#ifndef ARNM_SYMBOL_H
#define ARNM_SYMBOL_H

#include "types.h"
#include "token.h"
#include <stdbool.h>

/* ============================================================
 * Symbol Kinds
 * ============================================================ */

typedef enum {
    SYMBOL_VAR,         /* Variable binding */
    SYMBOL_FN,          /* Function definition */
    SYMBOL_ACTOR,       /* Actor definition */
    SYMBOL_TYPE,        /* Type alias */
    SYMBOL_PARAM,       /* Function parameter */
    SYMBOL_FIELD,       /* Struct field */
} SymbolKind;

/* ============================================================
 * Symbol
 * ============================================================ */

typedef struct Symbol {
    const char*     name;
    uint32_t        name_len;
    SymbolKind      kind;
    Type*           type;
    Permission      perm;
    Span            def_span;       /* Definition location */
    bool            is_mutable;
    bool            is_defined;     /* False for forward declarations */
    struct Symbol*  next;           /* Hash chain */
} Symbol;

/* ============================================================
 * Scope
 * ============================================================ */

#define SCOPE_BUCKET_COUNT 64

typedef struct Scope {
    Symbol*         buckets[SCOPE_BUCKET_COUNT];
    struct Scope*   parent;
    size_t          depth;
} Scope;

/* ============================================================
 * Symbol Table
 * ============================================================ */

typedef struct {
    Scope*      current;        /* Current scope */
    Scope*      global;         /* Global scope */
    TypeArena*  type_arena;     /* For allocating symbols */
    size_t      symbol_count;   /* Total symbols defined */
} SymbolTable;

/* ============================================================
 * Symbol Table Lifecycle
 * ============================================================ */

void symtab_init(SymbolTable* table, TypeArena* arena);
void symtab_destroy(SymbolTable* table);

/* ============================================================
 * Scope Management
 * ============================================================ */

/* Push a new scope onto the stack */
void scope_push(SymbolTable* table);

/* Pop the current scope */
void scope_pop(SymbolTable* table);

/* Get current scope depth */
size_t scope_depth(SymbolTable* table);

/* ============================================================
 * Symbol Operations
 * ============================================================ */

/*
 * Define a new symbol in the current scope.
 * Returns NULL if symbol already exists in current scope.
 */
Symbol* symbol_define(SymbolTable* table, const char* name, uint32_t name_len,
                      SymbolKind kind, Type* type, Span span);

/*
 * Lookup a symbol by name, searching from current scope up.
 * Returns NULL if not found.
 */
Symbol* symbol_lookup(SymbolTable* table, const char* name, uint32_t name_len);

/*
 * Lookup a symbol in current scope only (no parent search).
 */
Symbol* symbol_lookup_current(SymbolTable* table, const char* name, uint32_t name_len);

/*
 * Set symbol type (for type inference).
 */
void symbol_set_type(Symbol* sym, Type* type);

/* ============================================================
 * Utility
 * ============================================================ */

const char* symbol_kind_name(SymbolKind kind);

#endif /* ARNM_SYMBOL_H */
