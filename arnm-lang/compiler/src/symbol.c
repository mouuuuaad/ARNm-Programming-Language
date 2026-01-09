/*
 * ARNm Compiler - Symbol Table Implementation
 */

#include "../include/symbol.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * Hash Function (FNV-1a)
 * ============================================================ */

static uint32_t hash_name(const char* name, uint32_t len) {
    uint32_t hash = 2166136261u;
    for (uint32_t i = 0; i < len; i++) {
        hash ^= (uint8_t)name[i];
        hash *= 16777619u;
    }
    return hash;
}

/* ============================================================
 * Symbol Table Lifecycle
 * ============================================================ */

void symtab_init(SymbolTable* table, TypeArena* arena) {
    table->type_arena = arena;
    table->symbol_count = 0;
    table->current = NULL;
    table->global = NULL;
    
    /* Push global scope */
    scope_push(table);
    table->global = table->current;
}

void symtab_destroy(SymbolTable* table) {
    /* Scopes are arena-allocated, nothing to free */
    table->current = NULL;
    table->global = NULL;
    table->symbol_count = 0;
}

/* ============================================================
 * Scope Management
 * ============================================================ */

void scope_push(SymbolTable* table) {
    Scope* scope = type_arena_alloc(table->type_arena, sizeof(Scope));
    if (!scope) return;
    
    memset(scope->buckets, 0, sizeof(scope->buckets));
    scope->parent = table->current;
    scope->depth = table->current ? table->current->depth + 1 : 0;
    table->current = scope;
}

void scope_pop(SymbolTable* table) {
    if (table->current && table->current != table->global) {
        table->current = table->current->parent;
    }
}

size_t scope_depth(SymbolTable* table) {
    return table->current ? table->current->depth : 0;
}

/* ============================================================
 * Symbol Operations
 * ============================================================ */

Symbol* symbol_define(SymbolTable* table, const char* name, uint32_t name_len,
                      SymbolKind kind, Type* type, Span span) {
    if (!table->current) return NULL;
    
    /* Check for duplicate in current scope */
    if (symbol_lookup_current(table, name, name_len)) {
        return NULL;  /* Already defined */
    }
    
    /* Allocate symbol */
    Symbol* sym = type_arena_alloc(table->type_arena, sizeof(Symbol));
    if (!sym) return NULL;
    
    sym->name = name;
    sym->name_len = name_len;
    sym->kind = kind;
    sym->type = type;
    sym->perm = PERM_UNKNOWN;
    sym->def_span = span;
    sym->is_mutable = false;
    sym->is_defined = true;
    
    /* Insert into hash table */
    uint32_t bucket = hash_name(name, name_len) % SCOPE_BUCKET_COUNT;
    sym->next = table->current->buckets[bucket];
    table->current->buckets[bucket] = sym;
    
    table->symbol_count++;
    return sym;
}

static Symbol* lookup_in_scope(Scope* scope, const char* name, uint32_t name_len) {
    if (!scope) return NULL;
    
    uint32_t bucket = hash_name(name, name_len) % SCOPE_BUCKET_COUNT;
    Symbol* sym = scope->buckets[bucket];
    
    while (sym) {
        if (sym->name_len == name_len && memcmp(sym->name, name, name_len) == 0) {
            return sym;
        }
        sym = sym->next;
    }
    return NULL;
}

Symbol* symbol_lookup(SymbolTable* table, const char* name, uint32_t name_len) {
    Scope* scope = table->current;
    
    while (scope) {
        Symbol* sym = lookup_in_scope(scope, name, name_len);
        if (sym) return sym;
        scope = scope->parent;
    }
    return NULL;
}

Symbol* symbol_lookup_current(SymbolTable* table, const char* name, uint32_t name_len) {
    return lookup_in_scope(table->current, name, name_len);
}

void symbol_set_type(Symbol* sym, Type* type) {
    if (sym) {
        sym->type = type;
    }
}

/* ============================================================
 * Utility
 * ============================================================ */

const char* symbol_kind_name(SymbolKind kind) {
    switch (kind) {
        case SYMBOL_VAR:    return "variable";
        case SYMBOL_FN:     return "function";
        case SYMBOL_ACTOR:  return "actor";
        case SYMBOL_TYPE:   return "type";
        case SYMBOL_PARAM:  return "parameter";
        case SYMBOL_FIELD:  return "field";
        default:            return "symbol";
    }
}
