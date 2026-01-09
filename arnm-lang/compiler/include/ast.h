/*
 * ARNm Compiler - AST Definitions
 * 
 * DESIGN DECISION: Tagged union pattern for node types.
 * Rationale: Single allocation per node, cache-friendly traversal,
 * and exhaustive switch coverage enforced by compiler.
 * 
 * Every node contains a source span for LSP integration and error reporting.
 * Type slots are initially NULL and filled during semantic analysis.
 */

#ifndef ARNM_AST_H
#define ARNM_AST_H

#include "token.h"
#include <stdbool.h>

/* Forward declarations */
typedef struct AstNode AstNode;
typedef struct AstExpr AstExpr;
typedef struct AstStmt AstStmt;
typedef struct AstType AstType;

/* ============================================================
 * Node Kinds
 * ============================================================ */

typedef enum {
    /* Top-level declarations */
    AST_PROGRAM,
    AST_FN_DECL,
    AST_ACTOR_DECL,
    AST_STRUCT_DECL,
    AST_ENUM_DECL,
    
    /* Statements */
    AST_BLOCK,
    AST_LET_STMT,
    AST_EXPR_STMT,
    AST_RETURN_STMT,
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_FOR_STMT,
    AST_LOOP_STMT,
    AST_BREAK_STMT,
    AST_CONTINUE_STMT,
    AST_SPAWN_STMT,
    AST_RECEIVE_STMT,
    
    /* Expressions */
    AST_IDENT_EXPR,
    AST_INT_LIT_EXPR,
    AST_FLOAT_LIT_EXPR,
    AST_STRING_LIT_EXPR,
    AST_CHAR_LIT_EXPR,
    AST_BOOL_LIT_EXPR,
    AST_NIL_EXPR,
    AST_UNARY_EXPR,
    AST_BINARY_EXPR,
    AST_CALL_EXPR,
    AST_INDEX_EXPR,
    AST_FIELD_EXPR,
    AST_SEND_EXPR,        /* message send: expr ! expr */
    AST_SPAWN_EXPR,       /* spawn expression */
    AST_SELF_EXPR,
    AST_GROUP_EXPR,       /* parenthesized expression */
    
    /* Type nodes */
    AST_TYPE_IDENT,
    AST_TYPE_ARRAY,
    AST_TYPE_FN,
    AST_TYPE_OPTIONAL,
} AstNodeKind;

/* ============================================================
 * Common Fields (embedded in all nodes)
 * ============================================================ */
typedef struct {
    Span        span;           /* Source location */
    AstType*    resolved_type;  /* Filled during type inference (NULL initially) */
} AstCommon;

/* ============================================================
 * Expression Nodes
 * ============================================================ */

typedef enum {
    UNARY_NEG,      /* - */
    UNARY_NOT,      /* ! */
    UNARY_BITNOT,   /* ~ */
    UNARY_REF,      /* & */
    UNARY_DEREF,    /* * */
} UnaryOp;

typedef enum {
    BINARY_ADD,     /* + */
    BINARY_SUB,     /* - */
    BINARY_MUL,     /* * */
    BINARY_DIV,     /* / */
    BINARY_MOD,     /* % */
    BINARY_EQ,      /* == */
    BINARY_NE,      /* != */
    BINARY_LT,      /* < */
    BINARY_LE,      /* <= */
    BINARY_GT,      /* > */
    BINARY_GE,      /* >= */
    BINARY_AND,     /* && */
    BINARY_OR,      /* || */
    BINARY_BITAND,  /* & */
    BINARY_BITOR,   /* | */
    BINARY_BITXOR,  /* ^ */
    BINARY_ASSIGN,  /* = */
    BINARY_SEND,    /* ! (message send) */
} BinaryOp;

typedef struct {
    AstCommon   common;
    const char* name;       /* Pointer into source */
    uint32_t    name_len;
} AstIdentExpr;

typedef struct {
    AstCommon   common;
    int64_t     value;
} AstIntLitExpr;

typedef struct {
    AstCommon   common;
    double      value;
} AstFloatLitExpr;

typedef struct {
    AstCommon   common;
    const char* value;      /* Pointer into source (includes quotes) */
    uint32_t    value_len;
} AstStringLitExpr;

typedef struct {
    AstCommon   common;
    char        value;
} AstCharLitExpr;

typedef struct {
    AstCommon   common;
    bool        value;
} AstBoolLitExpr;

typedef struct {
    AstCommon   common;
    UnaryOp     op;
    AstExpr*    operand;
} AstUnaryExpr;

typedef struct {
    AstCommon   common;
    BinaryOp    op;
    AstExpr*    left;
    AstExpr*    right;
} AstBinaryExpr;

typedef struct {
    AstCommon   common;
    AstExpr*    callee;
    AstExpr**   args;
    size_t      arg_count;
} AstCallExpr;

typedef struct {
    AstCommon   common;
    AstExpr*    object;
    AstExpr*    index;
} AstIndexExpr;

typedef struct {
    AstCommon   common;
    AstExpr*    object;
    const char* field_name;
    uint32_t    field_name_len;
} AstFieldExpr;

typedef struct {
    AstCommon   common;
    AstExpr*    target;     /* Actor/process to send to */
    AstExpr*    message;    /* Message to send */
} AstSendExpr;

typedef struct {
    AstCommon   common;
    AstExpr*    inner;
} AstGroupExpr;

typedef struct {
    AstCommon   common;
    AstExpr*    expr;       /* Expression to spawn */
} AstSpawnExpr;

/* Unified expression type (tagged union) */
struct AstExpr {
    AstNodeKind kind;
    union {
        AstIdentExpr        ident;
        AstIntLitExpr       int_lit;
        AstFloatLitExpr     float_lit;
        AstStringLitExpr    string_lit;
        AstCharLitExpr      char_lit;
        AstBoolLitExpr      bool_lit;
        AstCommon           nil;        /* nil has no extra data */
        AstCommon           self_expr;  /* self has no extra data */
        AstUnaryExpr        unary;
        AstBinaryExpr       binary;
        AstCallExpr         call;
        AstIndexExpr        index;
        AstFieldExpr        field;
        AstSendExpr         send;
        AstSpawnExpr        spawn_expr;
        AstGroupExpr        group;
    } as;
};

/* ============================================================
 * Statement Nodes
 * ============================================================ */

typedef struct {
    AstCommon   common;
    AstStmt**   stmts;
    size_t      stmt_count;
} AstBlock;

typedef struct {
    AstCommon   common;
    const char* name;
    uint32_t    name_len;
    bool        is_mut;
    AstType*    type_ann;   /* Optional type annotation */
    AstExpr*    init;       /* Optional initializer */
} AstLetStmt;

typedef struct {
    AstCommon   common;
    AstExpr*    expr;
} AstExprStmt;

typedef struct {
    AstCommon   common;
    AstExpr*    value;      /* Optional return value */
} AstReturnStmt;

typedef struct {
    AstCommon   common;
    AstExpr*    condition;
    AstBlock*   then_block;
    AstStmt*    else_branch; /* Optional: AstBlock or AstIfStmt */
} AstIfStmt;

typedef struct {
    AstCommon   common;
    AstExpr*    condition;
    AstBlock*   body;
} AstWhileStmt;

typedef struct {
    AstCommon   common;
    const char* var_name;
    uint32_t    var_name_len;
    AstExpr*    iterable;
    AstBlock*   body;
} AstForStmt;

typedef struct {
    AstCommon   common;
    AstBlock*   body;
} AstLoopStmt;

typedef struct {
    AstCommon   common;
    AstExpr*    expr;       /* Expression to spawn */
} AstSpawnStmt;

/* Receive arm: pattern => block */
typedef struct {
    const char* pattern;    /* Pattern to match (simplified for now) */
    uint32_t    pattern_len;
    AstBlock*   body;
} ReceiveArm;

typedef struct {
    AstCommon   common;
    ReceiveArm* arms;
    size_t      arm_count;
} AstReceiveStmt;

/* Unified statement type (tagged union) */
struct AstStmt {
    AstNodeKind kind;
    union {
        AstBlock        block;
        AstLetStmt      let_stmt;
        AstExprStmt     expr_stmt;
        AstReturnStmt   return_stmt;
        AstIfStmt       if_stmt;
        AstWhileStmt    while_stmt;
        AstForStmt      for_stmt;
        AstLoopStmt     loop_stmt;
        AstSpawnStmt    spawn_stmt;
        AstReceiveStmt  receive_stmt;
        AstCommon       break_stmt;
        AstCommon       continue_stmt;
    } as;
};

/* ============================================================
 * Type Nodes
 * ============================================================ */

typedef struct {
    AstCommon   common;
    const char* name;
    uint32_t    name_len;
} AstTypeIdent;

typedef struct {
    AstCommon   common;
    AstType*    element_type;
    AstExpr*    size;       /* Optional size expression */
} AstTypeArray;

typedef struct {
    AstCommon   common;
    AstType**   param_types;
    size_t      param_count;
    AstType*    return_type;
} AstTypeFn;

typedef struct {
    AstCommon   common;
    AstType*    inner;
} AstTypeOptional;

struct AstType {
    AstNodeKind kind;
    union {
        AstTypeIdent    ident;
        AstTypeArray    array;
        AstTypeFn       fn;
        AstTypeOptional optional;
    } as;
};

/* ============================================================
 * Declaration Nodes
 * ============================================================ */

/* Function parameter */
typedef struct {
    const char* name;
    uint32_t    name_len;
    AstType*    type;
    bool        is_mut;
} FnParam;

typedef struct {
    AstCommon   common;
    const char* name;
    uint32_t    name_len;
    FnParam*    params;
    size_t      param_count;
    AstType*    return_type;    /* NULL for void */
    AstBlock*   body;
} AstFnDecl;

typedef struct {
    AstCommon       common;
    const char*     name;
    uint32_t        name_len;
    AstLetStmt**    fields;
    size_t          field_count;
    AstFnDecl**     methods;
    size_t          method_count;
    AstReceiveStmt* receive_block;  /* Optional receive handler */
} AstActorDecl;

typedef struct {
    AstCommon   common;
    const char* name;
    uint32_t    name_len;
    FnParam*    fields;     /* Reuse FnParam for struct fields */
    size_t      field_count;
} AstStructDecl;

/* Top-level declaration (tagged union) */
typedef struct AstDecl {
    AstNodeKind kind;
    union {
        AstFnDecl       fn_decl;
        AstActorDecl    actor_decl;
        AstStructDecl   struct_decl;
    } as;
} AstDecl;

/* ============================================================
 * Program (Root Node)
 * ============================================================ */

typedef struct {
    AstCommon   common;
    AstDecl**   decls;
    size_t      decl_count;
} AstProgram;

/* ============================================================
 * AST Allocation
 * ============================================================
 * Arena-based allocation for AST nodes.
 * All nodes are freed together when the arena is destroyed.
 */

typedef struct AstArena {
    char*   buffer;
    size_t  capacity;
    size_t  used;
} AstArena;

/* Initialize arena with given capacity */
void ast_arena_init(AstArena* arena, size_t capacity);

/* Allocate memory from arena (returns NULL if exhausted) */
void* ast_arena_alloc(AstArena* arena, size_t size);

/* Free all arena memory */
void ast_arena_destroy(AstArena* arena);

/* Convenience macros for allocation */
#define AST_NEW(arena, type) ((type*)ast_arena_alloc(arena, sizeof(type)))
#define AST_NEW_ARRAY(arena, type, count) ((type*)ast_arena_alloc(arena, sizeof(type) * (count)))

#endif /* ARNM_AST_H */
