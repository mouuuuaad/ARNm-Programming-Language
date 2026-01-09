/*
 * ARNm Compiler - Type System Implementation
 */

#include "../include/types.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================
 * Type Arena
 * ============================================================ */

void type_arena_init(TypeArena* arena, size_t capacity) {
    arena->buffer = (char*)malloc(capacity);
    arena->capacity = capacity;
    arena->used = 0;
    arena->next_var_id = 0;
}

void* type_arena_alloc(TypeArena* arena, size_t size) {
    size = (size + 7) & ~7;  /* Align to 8 bytes */
    
    if (arena->used + size > arena->capacity) {
        return NULL;
    }
    
    void* ptr = arena->buffer + arena->used;
    arena->used += size;
    memset(ptr, 0, size);
    return ptr;
}

void type_arena_destroy(TypeArena* arena) {
    free(arena->buffer);
    arena->buffer = NULL;
    arena->capacity = 0;
    arena->used = 0;
}

/* ============================================================
 * Primitive Type Singletons (cached)
 * ============================================================ */

/* Static storage for primitive types - these are never freed */
static Type primitive_storage[16];
static Type* primitive_cache[16] = {0};

static Type* get_or_create_primitive(TypeArena* arena, TypeKind kind) {
    (void)arena;  /* Primitives don't use arena - they're eternal singletons */
    
    if (primitive_cache[kind]) {
        return primitive_cache[kind];
    }
    
    /* Use static storage instead of arena allocation */
    Type* type = &primitive_storage[kind];
    type->kind = kind;
    type->perm = PERM_UNKNOWN;
    primitive_cache[kind] = type;
    return type;
}

Type* type_unit(TypeArena* arena) { return get_or_create_primitive(arena, TYPE_UNIT); }
Type* type_bool(TypeArena* arena) { return get_or_create_primitive(arena, TYPE_BOOL); }
Type* type_i32(TypeArena* arena) { return get_or_create_primitive(arena, TYPE_I32); }
Type* type_i64(TypeArena* arena) { return get_or_create_primitive(arena, TYPE_I64); }
Type* type_f32(TypeArena* arena) { return get_or_create_primitive(arena, TYPE_F32); }
Type* type_f64(TypeArena* arena) { return get_or_create_primitive(arena, TYPE_F64); }
Type* type_string(TypeArena* arena) { return get_or_create_primitive(arena, TYPE_STRING); }
Type* type_char(TypeArena* arena) { return get_or_create_primitive(arena, TYPE_CHAR); }
Type* type_error(TypeArena* arena) { return get_or_create_primitive(arena, TYPE_ERROR); }

/* ============================================================
 * Type Variable
 * ============================================================ */

Type* type_var(TypeArena* arena) {
    Type* type = type_arena_alloc(arena, sizeof(Type));
    if (!type) return NULL;
    
    type->kind = TYPE_VAR;
    type->perm = PERM_UNKNOWN;
    type->as.var.id = arena->next_var_id++;
    type->as.var.instance = NULL;
    return type;
}

/* ============================================================
 * Compound Types
 * ============================================================ */

Type* type_fn(TypeArena* arena, Type** params, size_t param_count, Type* ret) {
    Type* type = type_arena_alloc(arena, sizeof(Type));
    if (!type) return NULL;
    
    type->kind = TYPE_FN;
    type->perm = PERM_IMMUTABLE;  /* Functions are immutable */
    type->as.fn.param_count = param_count;
    type->as.fn.return_type = ret;
    
    if (param_count > 0) {
        type->as.fn.param_types = type_arena_alloc(arena, sizeof(Type*) * param_count);
        if (type->as.fn.param_types) {
            memcpy(type->as.fn.param_types, params, sizeof(Type*) * param_count);
        }
    }
    return type;
}

Type* type_array(TypeArena* arena, Type* elem) {
    Type* type = type_arena_alloc(arena, sizeof(Type));
    if (!type) return NULL;
    
    type->kind = TYPE_ARRAY;
    type->perm = PERM_UNKNOWN;
    type->as.array.element_type = elem;
    return type;
}

Type* type_optional(TypeArena* arena, Type* inner) {
    Type* type = type_arena_alloc(arena, sizeof(Type));
    if (!type) return NULL;
    
    type->kind = TYPE_OPTIONAL;
    type->perm = inner ? inner->perm : PERM_UNKNOWN;
    type->as.optional.inner_type = inner;
    return type;
}

Type* type_process(TypeArena* arena, Type* actor_type) {
    Type* type = type_arena_alloc(arena, sizeof(Type));
    if (!type) return NULL;
    
    type->kind = TYPE_PROCESS;
    type->perm = PERM_UNIQUE;  /* Process handles are unique */
    return type;
}

Type* type_actor(TypeArena* arena, const char* name, uint32_t name_len) {
    Type* type = type_arena_alloc(arena, sizeof(Type));
    if (!type) return NULL;
    
    type->kind = TYPE_ACTOR;
    type->perm = PERM_UNKNOWN;
    type->as.actor.name = name;
    type->as.actor.name_len = name_len;
    type->as.actor.method_types = NULL;
    type->as.actor.method_count = 0;
    type->as.actor.fields = NULL;
    type->as.actor.field_count = 0;
    return type;
}

Type* type_struct(TypeArena* arena, const char* name, uint32_t name_len) {
    Type* type = type_arena_alloc(arena, sizeof(Type));
    if (!type) return NULL;
    
    type->kind = TYPE_STRUCT;
    type->perm = PERM_UNKNOWN;
    type->as.struct_type.name = name;
    type->as.struct_type.name_len = name_len;
    type->as.struct_type.fields = NULL;
    type->as.struct_type.field_count = 0;
    return type;
}

/* ============================================================
 * Type Resolution
 * ============================================================ */

Type* type_resolve(Type* type) {
    if (!type) return NULL;
    
    /* Depth limit to prevent infinite loop on cyclic type variables */
    int depth = 0;
    const int MAX_DEPTH = 1000;
    
    while (type->kind == TYPE_VAR && type->as.var.instance) {
        type = type->as.var.instance;
        if (++depth > MAX_DEPTH) {
            /* Cyclic type detected - break the cycle */
            fprintf(stderr, "[ARNM] Warning: cyclic type detected in type_resolve\n");
            return type;
        }
    }
    return type;
}

/* ============================================================
 * Type Equality
 * ============================================================ */

bool type_equals(Type* a, Type* b) {
    a = type_resolve(a);
    b = type_resolve(b);
    
    if (!a || !b) return a == b;
    if (a == b) return true;
    if (a->kind != b->kind) return false;
    
    switch (a->kind) {
        case TYPE_VAR:
            return a->as.var.id == b->as.var.id;
            
        case TYPE_FN:
            if (a->as.fn.param_count != b->as.fn.param_count) return false;
            if (!type_equals(a->as.fn.return_type, b->as.fn.return_type)) return false;
            for (size_t i = 0; i < a->as.fn.param_count; i++) {
                if (!type_equals(a->as.fn.param_types[i], b->as.fn.param_types[i])) {
                    return false;
                }
            }
            return true;
            
        case TYPE_ARRAY:
            return type_equals(a->as.array.element_type, b->as.array.element_type);
            
        case TYPE_OPTIONAL:
            return type_equals(a->as.optional.inner_type, b->as.optional.inner_type);
            
        case TYPE_ACTOR:
            if (a->as.actor.name_len != b->as.actor.name_len) return false;
            return memcmp(a->as.actor.name, b->as.actor.name, a->as.actor.name_len) == 0;

        case TYPE_STRUCT:
            if (a->as.struct_type.name_len != b->as.struct_type.name_len) return false;
            return memcmp(a->as.struct_type.name, b->as.struct_type.name, a->as.struct_type.name_len) == 0;
            
        default:
            return true;  /* Primitive types match by kind */
    }
}

/* ============================================================
 * Occurs Check (for unification)
 * ============================================================ */

static bool occurs_in(TypeVar* var, Type* type) {
    type = type_resolve(type);
    if (!type) return false;
    
    if (type->kind == TYPE_VAR) {
        return type->as.var.id == var->id;
    }
    
    switch (type->kind) {
        case TYPE_FN:
            for (size_t i = 0; i < type->as.fn.param_count; i++) {
                if (occurs_in(var, type->as.fn.param_types[i])) return true;
            }
            return occurs_in(var, type->as.fn.return_type);
            
        case TYPE_ARRAY:
            return occurs_in(var, type->as.array.element_type);
            
        case TYPE_OPTIONAL:
            return occurs_in(var, type->as.optional.inner_type);
            
        default:
            return false;
    }
}

/* ============================================================
 * Unification
 * ============================================================ */

bool type_unify(Type* a, Type* b) {
    a = type_resolve(a);
    b = type_resolve(b);
    
    if (!a || !b) return false;
    if (a == b) return true;
    
    /* Error type unifies with anything */
    if (a->kind == TYPE_ERROR || b->kind == TYPE_ERROR) return true;
    
    /* Type variable: bind it (but not to itself!) */
    if (a->kind == TYPE_VAR) {
        if (occurs_in(&a->as.var, b)) return false;  /* Infinite type */
        if (a == b) return true;  /* Don't bind to self */
        a->as.var.instance = b;
        return true;
    }
    if (b->kind == TYPE_VAR) {
        if (occurs_in(&b->as.var, a)) return false;
        if (a == b) return true;  /* Don't bind to self */
        b->as.var.instance = a;
        return true;
    }
    
    /* Same kind required */
    if (a->kind != b->kind) return false;
    
    switch (a->kind) {
        case TYPE_FN:
            if (a->as.fn.param_count != b->as.fn.param_count) return false;
            for (size_t i = 0; i < a->as.fn.param_count; i++) {
                if (!type_unify(a->as.fn.param_types[i], b->as.fn.param_types[i])) {
                    return false;
                }
            }
            return type_unify(a->as.fn.return_type, b->as.fn.return_type);
            
        case TYPE_ARRAY:
            return type_unify(a->as.array.element_type, b->as.array.element_type);
            
        case TYPE_OPTIONAL:
            return type_unify(a->as.optional.inner_type, b->as.optional.inner_type);
            
        case TYPE_ACTOR:
            return type_equals(a, b);
            
        default:
            return true;  /* Primitives match */
    }
}

/* ============================================================
 * Free Variables Check
 * ============================================================ */

bool type_has_free_vars(Type* type) {
    type = type_resolve(type);
    if (!type) return false;
    
    if (type->kind == TYPE_VAR) return true;
    
    switch (type->kind) {
        case TYPE_FN:
            for (size_t i = 0; i < type->as.fn.param_count; i++) {
                if (type_has_free_vars(type->as.fn.param_types[i])) return true;
            }
            return type_has_free_vars(type->as.fn.return_type);
            
        case TYPE_ARRAY:
            return type_has_free_vars(type->as.array.element_type);
            
        case TYPE_OPTIONAL:
            return type_has_free_vars(type->as.optional.inner_type);
            
        default:
            return false;
    }
}

/* ============================================================
 * Type Printing
 * ============================================================ */

const char* type_kind_name(TypeKind kind) {
    switch (kind) {
        case TYPE_UNKNOWN:  return "unknown";
        case TYPE_VAR:      return "var";
        case TYPE_UNIT:     return "()";
        case TYPE_BOOL:     return "bool";
        case TYPE_I32:      return "i32";
        case TYPE_I64:      return "i64";
        case TYPE_F32:      return "f32";
        case TYPE_F64:      return "f64";
        case TYPE_STRING:   return "string";
        case TYPE_CHAR:     return "char";
        case TYPE_FN:       return "fn";
        case TYPE_ACTOR:    return "actor";
        case TYPE_STRUCT:   return "struct";
        case TYPE_ARRAY:    return "array";
        case TYPE_OPTIONAL: return "optional";
        case TYPE_PROCESS:  return "Process";
        case TYPE_ERROR:    return "<error>";
        default:            return "?";
    }
}

const char* perm_name(Permission perm) {
    switch (perm) {
        case PERM_UNIQUE:    return "unique";
        case PERM_SHARED:    return "shared";
        case PERM_IMMUTABLE: return "immut";
        case PERM_UNKNOWN:   return "";
        default:             return "?";
    }
}

int type_print(Type* type, char* buf, size_t buf_size) {
    type = type_resolve(type);
    if (!type) return snprintf(buf, buf_size, "null");
    
    switch (type->kind) {
        case TYPE_VAR:
            return snprintf(buf, buf_size, "t%u", type->as.var.id);
            
        case TYPE_FN: {
            int len = snprintf(buf, buf_size, "fn(");
            for (size_t i = 0; i < type->as.fn.param_count; i++) {
                if (i > 0) len += snprintf(buf + len, buf_size - len, ", ");
                len += type_print(type->as.fn.param_types[i], buf + len, buf_size - len);
            }
            len += snprintf(buf + len, buf_size - len, ") -> ");
            len += type_print(type->as.fn.return_type, buf + len, buf_size - len);
            return len;
        }
        
        case TYPE_ARRAY: {
            int len = snprintf(buf, buf_size, "[");
            len += type_print(type->as.array.element_type, buf + len, buf_size - len);
            len += snprintf(buf + len, buf_size - len, "]");
            return len;
        }
        
        case TYPE_OPTIONAL: {
            int len = type_print(type->as.optional.inner_type, buf, buf_size);
            len += snprintf(buf + len, buf_size - len, "?");
            return len;
        }
        
        case TYPE_ACTOR:
            return snprintf(buf, buf_size, "%.*s", 
                (int)type->as.actor.name_len, type->as.actor.name);

        case TYPE_STRUCT:
            return snprintf(buf, buf_size, "%.*s", 
                (int)type->as.struct_type.name_len, type->as.struct_type.name);
                
        default:
            return snprintf(buf, buf_size, "%s", type_kind_name(type->kind));
    }
}

Type* type_with_perm(TypeArena* arena, Type* type, Permission perm) {
    /* For primitives, just return the cached instance */
    type = type_resolve(type);
    if (!type) return NULL;
    
    /* If permission is already set, just return */
    if (type->perm == perm) return type;
    
    /* Clone type with new permission */
    Type* new_type = type_arena_alloc(arena, sizeof(Type));
    if (!new_type) return NULL;
    
    *new_type = *type;
    new_type->perm = perm;
    return new_type;
}
