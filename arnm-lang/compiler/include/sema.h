/*
 * ARNm Compiler - Semantic Analysis
 */

#ifndef ARNM_SEMA_H
#define ARNM_SEMA_H

#include "ast.h"
#include "types.h"
#include "symbol.h"

/* ============================================================
 * Semantic Context
 * ============================================================ */

#define MAX_SEMA_ERRORS 64

typedef struct {
    const char* message;
    Span        span;
} SemaError;

typedef struct {
    TypeArena   type_arena;
    SymbolTable symbols;
    SemaError   errors[MAX_SEMA_ERRORS];
    size_t      error_count;
    bool        had_error;
    
    /* Current context */
    Type*       current_fn_return;  /* Expected return type */
    bool        in_loop;            /* For break/continue */
    bool        in_actor;           /* For self access */
    Type*       cur_actor;          /* Current actor type */
} SemaContext;

/* ============================================================
 * Semantic Analysis
 * ============================================================ */

void sema_init(SemaContext* ctx);
void sema_destroy(SemaContext* ctx);

/* Main entry point: analyze entire program */
bool sema_analyze(SemaContext* ctx, AstProgram* program);

/* Individual analysis functions (for testing) */
Type* sema_infer_expr(SemaContext* ctx, AstExpr* expr);
void sema_check_stmt(SemaContext* ctx, AstStmt* stmt);
void sema_check_decl(SemaContext* ctx, AstDecl* decl);

/* Error reporting */
void sema_error(SemaContext* ctx, Span span, const char* message);

#endif /* ARNM_SEMA_H */
