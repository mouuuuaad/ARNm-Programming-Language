/*
 * ARNm Compiler - Type System Definitions
 * 
 * DESIGN DECISION: Structural typing with type variables.
 * Rationale: Enables Hindley-Milner inference while keeping
 * implementation simple. Types are interned for fast equality.
 */

#ifndef ARNM_TYPES_H
#define ARNM_TYPES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Forward declarations */
typedef struct Type Type;
typedef struct TypeVar TypeVar;

/* ============================================================
 * Type Kinds
 * ============================================================ */

typedef enum {
    TYPE_UNKNOWN,       /* Not yet determined */
    TYPE_VAR,           /* Type variable (for inference) */
    TYPE_UNIT,          /* () / void / nil */
    TYPE_BOOL,          /* bool */
    TYPE_I32,           /* i32 */
    TYPE_I64,           /* i64 */
    TYPE_F32,           /* f32 */
    TYPE_F64,           /* f64 */
    TYPE_STRING,        /* string */
    TYPE_CHAR,          /* char */
    TYPE_FN,            /* fn(T...) -> R */
    TYPE_ACTOR,         /* actor type */
    TYPE_ARRAY,         /* [T] */
    TYPE_OPTIONAL,      /* T? */
    TYPE_PROCESS,       /* Process handle (spawn result) */
    TYPE_ERROR,         /* Type error placeholder */
} TypeKind;

/* ============================================================
 * Permission Model
 * ============================================================
 * Compile-time tracking for data race prevention.
 */

typedef enum {
    PERM_UNIQUE,        /* Owned, movable, not shareable */
    PERM_SHARED,        /* Reference counted, readonly */
    PERM_IMMUTABLE,     /* Deeply immutable, freely copyable */
    PERM_UNKNOWN,       /* Not yet determined */
} Permission;

/* ============================================================
 * Type Variable (for inference)
 * ============================================================ */

struct TypeVar {
    uint32_t    id;         /* Unique identifier */
    Type*       instance;   /* Bound type (NULL if unbound) */
};

/* ============================================================
 * Type Structure
 * ============================================================
 * Tagged union for all type kinds.
 */

/* Function type */
typedef struct {
    Type**  param_types;
    size_t  param_count;
    Type*   return_type;
} TypeFn;

typedef struct {
    const char* name;
    uint32_t    name_len;
    Type*       type;
} TypeField;

/* Actor type */
typedef struct {
    const char* name;
    uint32_t    name_len;
    Type**      method_types;   /* Method signatures */
    size_t      method_count;
    TypeField*  fields;
    size_t      field_count;
} TypeActor;

/* Array type */
typedef struct {
    Type*   element_type;
} TypeArray;

/* Optional type */
typedef struct {
    Type*   inner_type;
} TypeOptional;

struct Type {
    TypeKind    kind;
    Permission  perm;       /* Permission annotation */
    union {
        TypeVar     var;        /* TYPE_VAR */
        TypeFn      fn;         /* TYPE_FN */
        TypeActor   actor;      /* TYPE_ACTOR */
        TypeArray   array;      /* TYPE_ARRAY */
        TypeOptional optional;  /* TYPE_OPTIONAL */
    } as;
};

/* ============================================================
 * Type Arena
 * ============================================================
 * Arena allocator for types. All types live until analysis completes.
 */

typedef struct {
    char*   buffer;
    size_t  capacity;
    size_t  used;
    uint32_t next_var_id;   /* For generating fresh type variables */
} TypeArena;

void type_arena_init(TypeArena* arena, size_t capacity);
void* type_arena_alloc(TypeArena* arena, size_t size);
void type_arena_destroy(TypeArena* arena);

/* ============================================================
 * Type Constructors
 * ============================================================ */

/* Primitive types (cached singletons) */
Type* type_unit(TypeArena* arena);
Type* type_bool(TypeArena* arena);
Type* type_i32(TypeArena* arena);
Type* type_i64(TypeArena* arena);
Type* type_f32(TypeArena* arena);
Type* type_f64(TypeArena* arena);
Type* type_string(TypeArena* arena);
Type* type_char(TypeArena* arena);
Type* type_error(TypeArena* arena);

/* Fresh type variable */
Type* type_var(TypeArena* arena);

/* Compound types */
Type* type_fn(TypeArena* arena, Type** params, size_t param_count, Type* ret);
Type* type_array(TypeArena* arena, Type* elem);
Type* type_optional(TypeArena* arena, Type* inner);
Type* type_process(TypeArena* arena, Type* actor_type);
Type* type_actor(TypeArena* arena, const char* name, uint32_t name_len);

/* ============================================================
 * Type Operations
 * ============================================================ */

/* Resolve type variables to their bound types */
Type* type_resolve(Type* type);

/* Check structural equality */
bool type_equals(Type* a, Type* b);

/* Unify two types, returns true if successful */
bool type_unify(Type* a, Type* b);

/* Check if type contains unbound variables */
bool type_has_free_vars(Type* type);

/* Apply permission to type */
Type* type_with_perm(TypeArena* arena, Type* type, Permission perm);

/* ============================================================
 * Type Printing
 * ============================================================ */

/* Print type to buffer (returns bytes written) */
int type_print(Type* type, char* buf, size_t buf_size);

/* Get human-readable type kind name */
const char* type_kind_name(TypeKind kind);

/* Get permission name */
const char* perm_name(Permission perm);

#endif /* ARNM_TYPES_H */
