/*
 * ARNm Compiler - Semantic Analysis Implementation
 * 
 * Implements Hindley-Milner type inference (Algorithm W simplified).
 */

#include "../include/sema.h"
#include <string.h>
#include <stdlib.h>

static char* my_strdup(const char* s) {
    size_t len = strlen(s);
    char* copy = malloc(len + 1);
    memcpy(copy, s, len + 1);
    return copy;
}
#include <stdio.h>

/* ============================================================
 * Context Management
 * ============================================================ */

void sema_init(SemaContext* ctx) {
    type_arena_init(&ctx->type_arena, 1024 * 1024);  /* 1MB */
    symtab_init(&ctx->symbols, &ctx->type_arena);
    ctx->error_count = 0;
    ctx->had_error = false;
    ctx->current_fn_return = NULL;
    ctx->in_loop = false;
    ctx->in_actor = false;
}

void sema_destroy(SemaContext* ctx) {
    symtab_destroy(&ctx->symbols);
    type_arena_destroy(&ctx->type_arena);
}

void sema_error(SemaContext* ctx, Span span, const char* message) {
    ctx->had_error = true;
    if (ctx->error_count < MAX_SEMA_ERRORS) {
        ctx->errors[ctx->error_count].message = message;
        ctx->errors[ctx->error_count].span = span;
        ctx->error_count++;
    }
}

/* ============================================================
 * Expression Type Inference
 * ============================================================ */

static Type* infer_ident(SemaContext* ctx, AstIdentExpr* ident) {
    /* Check for bare actor field access (SCOPE rule: fields require self.prefix) */
    if (ctx->in_actor && ctx->cur_actor) {
        Type* actor = type_resolve(ctx->cur_actor);
        if (actor->kind == TYPE_ACTOR) {
            for (size_t i = 0; i < actor->as.actor.field_count; i++) {
                TypeField* f = &actor->as.actor.fields[i];
                if (f->name_len == ident->name_len &&
                    strncmp(f->name, ident->name, f->name_len) == 0) {
                    sema_error(ctx, ident->common.span, 
                        "actor field access requires 'self.' prefix");
                    return type_error(&ctx->type_arena);
                }
            }
        }
    }
    
    Symbol* sym = symbol_lookup(&ctx->symbols, ident->name, ident->name_len);
    if (!sym) {
        sema_error(ctx, ident->common.span, "undefined identifier");
        return type_error(&ctx->type_arena);
    }
    return sym->type ? sym->type : type_var(&ctx->type_arena);
}

/* Helper to extract span from any expression */
static Span get_expr_span(AstExpr* expr) {
    if (!expr) return (Span){0, 0, 0, 0};
    switch (expr->kind) {
        case AST_IDENT_EXPR:      return expr->as.ident.common.span;
        case AST_INT_LIT_EXPR:    return expr->as.int_lit.common.span;
        case AST_FLOAT_LIT_EXPR:  return expr->as.float_lit.common.span;
        case AST_STRING_LIT_EXPR: return expr->as.string_lit.common.span;
        case AST_CHAR_LIT_EXPR:   return expr->as.char_lit.common.span;
        case AST_BOOL_LIT_EXPR:   return expr->as.bool_lit.common.span;
        case AST_NIL_EXPR:        return expr->as.nil.span;
        case AST_SELF_EXPR:       return expr->as.self_expr.span;
        case AST_UNARY_EXPR:      return expr->as.unary.common.span;
        case AST_BINARY_EXPR:     return expr->as.binary.common.span;
        case AST_CALL_EXPR:       return expr->as.call.common.span;
        case AST_INDEX_EXPR:      return expr->as.index.common.span;
        case AST_FIELD_EXPR:      return expr->as.field.common.span;
        case AST_SEND_EXPR:       return expr->as.send.common.span;
        case AST_SPAWN_EXPR:      return expr->as.spawn_expr.common.span;
        case AST_GROUP_EXPR:      return expr->as.group.common.span;
        default:                  return (Span){0, 0, 0, 0};
    }
}

/* Check if an expression is a valid assignment target and is mutable */
static bool check_assignment_target(SemaContext* ctx, AstExpr* target) {
    if (!target) return false;
    
    switch (target->kind) {
        case AST_IDENT_EXPR: {
            AstIdentExpr* ident = &target->as.ident;
            Symbol* sym = symbol_lookup(&ctx->symbols, ident->name, ident->name_len);
            if (!sym) {
                /* Already reported by infer */
                return false;
            }
            if (!sym->is_mutable) {
                sema_error(ctx, ident->common.span, 
                          "cannot assign to immutable variable");
                return false;
            }
            return true;
        }
        
        case AST_FIELD_EXPR: {
            /* self.field is assignable in actor context */
            AstFieldExpr* field = &target->as.field;
            if (field->object->kind == AST_SELF_EXPR) {
                if (!ctx->in_actor) {
                    sema_error(ctx, field->common.span, 
                              "'self.field' assignment outside actor");
                    return false;
                }
                return true;  /* Actor fields are mutable via self */
            }
            /* Other field access - check object mutability */
            return check_assignment_target(ctx, field->object);
        }
        
        case AST_INDEX_EXPR: {
            /* Array indexing - check array mutability */
            return check_assignment_target(ctx, target->as.index.object);
        }
        
        default:
            sema_error(ctx, get_expr_span(target), 
                      "invalid assignment target");
            return false;
    }
}

static Type* infer_binary(SemaContext* ctx, AstBinaryExpr* bin) {
    Type* left = sema_infer_expr(ctx, bin->left);
    Type* right = sema_infer_expr(ctx, bin->right);
    
    switch (bin->op) {
        case BINARY_ADD:
        case BINARY_SUB:
        case BINARY_MUL:
        case BINARY_DIV:
        case BINARY_MOD:
            /* Numeric operations */
            if (!type_unify(left, right)) {
                sema_error(ctx, bin->common.span, "type mismatch in binary operation");
            }
            return left;
            
        case BINARY_EQ:
        case BINARY_NE:
        case BINARY_LT:
        case BINARY_LE:
        case BINARY_GT:
        case BINARY_GE:
            /* Comparison returns bool */
            if (!type_unify(left, right)) {
                sema_error(ctx, bin->common.span, "type mismatch in comparison");
            }
            return type_bool(&ctx->type_arena);
            
        case BINARY_AND:
        case BINARY_OR:
            /* Logical operations */
            if (!type_unify(left, type_bool(&ctx->type_arena)) ||
                !type_unify(right, type_bool(&ctx->type_arena))) {
                sema_error(ctx, bin->common.span, "logical operators require bool");
            }
            return type_bool(&ctx->type_arena);
            
        case BINARY_ASSIGN:
            /* Check target is mutable */
            check_assignment_target(ctx, bin->left);
            if (!type_unify(left, right)) {
                sema_error(ctx, bin->common.span, "type mismatch in assignment");
            }
            return type_unit(&ctx->type_arena);
            
        case BINARY_SEND:
            /* left ! right: left must be Process, right is message */
            /* Validate target is Process type */
            left = type_resolve(left);
            if (left->kind != TYPE_PROCESS && left->kind != TYPE_VAR &&
                left->kind != TYPE_ERROR) {
                sema_error(ctx, bin->common.span, 
                          "send target must be a process");
            }
            return type_unit(&ctx->type_arena);
            
        default:
            return type_var(&ctx->type_arena);
    }
}

static Type* infer_unary(SemaContext* ctx, AstUnaryExpr* un) {
    Type* operand = sema_infer_expr(ctx, un->operand);
    
    switch (un->op) {
        case UNARY_NEG:
            return operand;  /* Same numeric type */
        case UNARY_NOT:
            if (!type_unify(operand, type_bool(&ctx->type_arena))) {
                sema_error(ctx, un->common.span, "! requires bool operand");
            }
            return type_bool(&ctx->type_arena);
        case UNARY_BITNOT:
            return operand;  /* Same integral type */
        default:
            return operand;
    }
}

static Type* infer_call(SemaContext* ctx, AstCallExpr* call) {
    Type* callee_type = sema_infer_expr(ctx, call->callee);
    callee_type = type_resolve(callee_type);
    
    if (callee_type->kind == TYPE_ACTOR) {
        /* Actor constructor call */
        /* Look for init method to validate args */
        char mangled[256];
        snprintf(mangled, sizeof(mangled), "%.*s_init", 
                callee_type->as.actor.name_len, callee_type->as.actor.name);
        
        Symbol* init_sym = symbol_lookup(&ctx->symbols, mangled, strlen(mangled));
        Type* init_fn_type = NULL;
        
        if (init_sym && init_sym->kind == SYMBOL_FN) {
            init_fn_type = init_sym->type;
        }
        
        if (init_fn_type) {
            /* Check arguments against init */
             if (call->arg_count != init_fn_type->as.fn.param_count) {
                sema_error(ctx, call->common.span, "wrong number of arguments for actor init");
                return type_error(&ctx->type_arena);
            }
            for (size_t i = 0; i < call->arg_count; i++) {
                Type* arg_type = sema_infer_expr(ctx, call->args[i]);
                if (!type_unify(arg_type, init_fn_type->as.fn.param_types[i])) {
                    sema_error(ctx, call->common.span, "argument type mismatch in actor init");
                    return type_error(&ctx->type_arena);
                }
            }
        } else {
            /* No init method - expect 0 args */
            if (call->arg_count > 0) {
                sema_error(ctx, call->common.span, "actor has no init method, expected 0 arguments");
                return type_error(&ctx->type_arena);
            }
        }
        return callee_type;
    }

    if (callee_type->kind != TYPE_FN && callee_type->kind != TYPE_VAR) {
        sema_error(ctx, call->common.span, "calling non-function");
        return type_error(&ctx->type_arena);
    }
    
    if (callee_type->kind == TYPE_VAR) {
        /* Create function type from call */
        Type** param_types = NULL;
        if (call->arg_count > 0) {
            param_types = type_arena_alloc(&ctx->type_arena, 
                sizeof(Type*) * call->arg_count);
            for (size_t i = 0; i < call->arg_count; i++) {
                param_types[i] = sema_infer_expr(ctx, call->args[i]);
            }
        }
        Type* ret_type = type_var(&ctx->type_arena);
        Type* fn_type = type_fn(&ctx->type_arena, param_types, call->arg_count, ret_type);
        type_unify(callee_type, fn_type);
        return ret_type;
    }
    
    /* Check argument count */
    if (call->arg_count != callee_type->as.fn.param_count) {
        sema_error(ctx, call->common.span, "wrong number of arguments");
        return callee_type->as.fn.return_type;
    }
    
    /* Unify arguments with parameters */
    for (size_t i = 0; i < call->arg_count; i++) {
        Type* arg_type = sema_infer_expr(ctx, call->args[i]);
        if (!type_unify(arg_type, callee_type->as.fn.param_types[i])) {
            sema_error(ctx, call->common.span, "argument type mismatch");
        }
    }
    
    return callee_type->as.fn.return_type;
}

static Type* infer_send(SemaContext* ctx, AstSendExpr* send) {
    Type* target = sema_infer_expr(ctx, send->target);
    Type* message = sema_infer_expr(ctx, send->message);
    (void)target;
    (void)message;
    /* Send returns unit */
    return type_unit(&ctx->type_arena);
}

static Type* infer_spawn(SemaContext* ctx, AstSpawnExpr* spawn) {
    Type* inner = sema_infer_expr(ctx, spawn->expr);
    
    /* Validate spawn target is callable */
    Type* resolved = type_resolve(inner);
    if (resolved->kind != TYPE_FN && resolved->kind != TYPE_VAR &&
        resolved->kind != TYPE_ACTOR && resolved->kind != TYPE_ERROR) {
        sema_error(ctx, spawn->common.span, 
                  "spawn requires function or actor method");
    }
    
    /* spawn returns Process handle */
    return type_process(&ctx->type_arena, inner);
}

Type* sema_infer_expr(SemaContext* ctx, AstExpr* expr) {
    if (!expr) return type_error(&ctx->type_arena);
    
    /* Return cached type if already inferred? No, re-inference might be needed for generics? 
       For now, simple caching or overwrite. */
    if (expr->common.sema_type) return expr->common.sema_type;
    
    Type* result = NULL;
    
    switch (expr->kind) {
        case AST_INT_LIT_EXPR:
            result = type_i32(&ctx->type_arena);
            break;
            
        case AST_FLOAT_LIT_EXPR:
            result = type_f64(&ctx->type_arena);
            break;
            
        case AST_STRING_LIT_EXPR:
            result = type_string(&ctx->type_arena);
            break;
            
        case AST_CHAR_LIT_EXPR:
            result = type_char(&ctx->type_arena);
            break;
            
        case AST_BOOL_LIT_EXPR:
            result = type_bool(&ctx->type_arena);
            break;
            
        case AST_NIL_EXPR:
            result = type_unit(&ctx->type_arena);
            break;
            
        case AST_SELF_EXPR:
            if (ctx->cur_actor) {
                result = ctx->cur_actor;
            } else {
                sema_error(ctx, expr->as.self_expr.span, "'self' used outside of actor");
                result = type_error(&ctx->type_arena);
            }
            break;
            
        case AST_IDENT_EXPR:
            result = infer_ident(ctx, &expr->as.ident);
            break;
            
        case AST_BINARY_EXPR:
            result = infer_binary(ctx, &expr->as.binary);
            break;
            
        case AST_UNARY_EXPR:
            result = infer_unary(ctx, &expr->as.unary);
            break;
            
        case AST_CALL_EXPR:
            result = infer_call(ctx, &expr->as.call);
            break;
            
        case AST_SEND_EXPR:
            result = infer_send(ctx, &expr->as.send);
            break;
            
        case AST_SPAWN_EXPR:
            result = infer_spawn(ctx, &expr->as.spawn_expr);
            break;
            
        case AST_GROUP_EXPR:
            result = sema_infer_expr(ctx, expr->as.group.inner);
            break;
            
        case AST_FIELD_EXPR:
            /* Field access on struct/actor */
            {
                Type* obj = sema_infer_expr(ctx, expr->as.field.object);
                obj = type_resolve(obj);
                
                if (obj->kind == TYPE_ACTOR) {
                    const char* name = expr->as.field.field_name;
                    uint32_t len = expr->as.field.field_name_len;
                    
                    bool found = false;
                    for (size_t i = 0; i < obj->as.actor.field_count; i++) {
                        TypeField* f = &obj->as.actor.fields[i];
                        if (f->name_len == len &&
                            strncmp(f->name, name, len) == 0) {
                            result = f->type;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        /* Check for method (static lookup) */
                        char mangled[256];
                        snprintf(mangled, sizeof(mangled), "%.*s_%.*s", 
                                obj->as.actor.name_len, obj->as.actor.name,
                                len, name);
                                
                        Symbol* method = symbol_lookup(&ctx->symbols, mangled, strlen(mangled));
                        
                        if (method && method->kind == SYMBOL_FN) {
                            result = method->type;
                            found = true;
                        } else {
                             sema_error(ctx, expr->as.field.common.span, "field or method not found in actor");
                             result = type_error(&ctx->type_arena);
                        }
                    }
                } else if (obj->kind == TYPE_STRUCT) {
                     const char* name = expr->as.field.field_name;
                     uint32_t len = expr->as.field.field_name_len;
                     
                     bool found = false;
                     for (size_t i = 0; i < obj->as.struct_type.field_count; i++) {
                         TypeField* f = &obj->as.struct_type.fields[i];
                         if (f->name_len == len &&
                             strncmp(f->name, name, len) == 0) {
                             result = f->type;
                             found = true;
                             break;
                         }
                     }
                     if (!found) {
                         sema_error(ctx, expr->as.field.common.span, "field not found in struct");
                         result = type_error(&ctx->type_arena);
                     }
                } else {
                    sema_error(ctx, expr->as.field.common.span, "field access on non-actor/struct");
                     result = type_error(&ctx->type_arena);
                }
            }
            break;
            
        case AST_INDEX_EXPR: {
            Type* obj = sema_infer_expr(ctx, expr->as.index.object);
            Type* idx = sema_infer_expr(ctx, expr->as.index.index);
            (void)idx;
            obj = type_resolve(obj);
            if (obj->kind == TYPE_ARRAY) {
                result = obj->as.array.element_type;
            } else {
                result = type_var(&ctx->type_arena);
            }
            break;
        }
            
        default:
            result = type_var(&ctx->type_arena);
            break;
    }
    
    /* Store resolved type in AST */
    /* (Would need to add type field to expressions) */
    
    if (result) expr->common.sema_type = result;
    return result;
}

/* ============================================================
 * Statement Checking
 * ============================================================ */

static void check_block(SemaContext* ctx, AstBlock* block) {
    if (!block) return;
    scope_push(&ctx->symbols);
    for (size_t i = 0; i < block->stmt_count; i++) {
        sema_check_stmt(ctx, block->stmts[i]);
    }
    scope_pop(&ctx->symbols);
}

void sema_check_stmt(SemaContext* ctx, AstStmt* stmt) {
    if (!stmt) return;
    
    switch (stmt->kind) {
        case AST_LET_STMT: {
            AstLetStmt* let = &stmt->as.let_stmt;
            Type* init_type = let->init ? sema_infer_expr(ctx, let->init) : type_var(&ctx->type_arena);
            
            Symbol* sym = symbol_define(&ctx->symbols, let->name, let->name_len,
                                        SYMBOL_VAR, init_type, let->common.span);
            if (!sym) {
                sema_error(ctx, let->common.span, "duplicate variable definition");
            } else {
                sym->is_mutable = let->is_mut;
            }
            break;
        }
        
        case AST_EXPR_STMT:
            sema_infer_expr(ctx, stmt->as.expr_stmt.expr);
            break;
            
        case AST_RETURN_STMT: {
            Type* ret_type = stmt->as.return_stmt.value 
                ? sema_infer_expr(ctx, stmt->as.return_stmt.value)
                : type_unit(&ctx->type_arena);
            if (ctx->current_fn_return) {
                if (!type_unify(ret_type, ctx->current_fn_return)) {
                    sema_error(ctx, stmt->as.return_stmt.common.span, 
                              "return type mismatch");
                }
            }
            break;
        }
        
        case AST_IF_STMT: {
            AstIfStmt* if_stmt = &stmt->as.if_stmt;
            Type* cond = sema_infer_expr(ctx, if_stmt->condition);
            if (!type_unify(cond, type_bool(&ctx->type_arena))) {
                sema_error(ctx, if_stmt->common.span, "if condition must be bool");
            }
            check_block(ctx, if_stmt->then_block);
            if (if_stmt->else_branch) {
                sema_check_stmt(ctx, if_stmt->else_branch);
            }
            break;
        }
        
        case AST_WHILE_STMT: {
            AstWhileStmt* while_stmt = &stmt->as.while_stmt;
            Type* cond = sema_infer_expr(ctx, while_stmt->condition);
            if (!type_unify(cond, type_bool(&ctx->type_arena))) {
                sema_error(ctx, while_stmt->common.span, "while condition must be bool");
            }
            bool was_in_loop = ctx->in_loop;
            ctx->in_loop = true;
            check_block(ctx, while_stmt->body);
            ctx->in_loop = was_in_loop;
            break;
        }

        case AST_FOR_STMT: {
            AstForStmt* for_stmt = &stmt->as.for_stmt;
            /* Infer iterable type */
            Type* iter_type = sema_infer_expr(ctx, for_stmt->iterable);
            
            /* Create constraint: iter_type must be array or range of 'elem_type' */
            Type* elem_type = type_var(&ctx->type_arena);
            Type* array_type = type_array(&ctx->type_arena, elem_type);
            
            /* Attempt unify with array (TODO: Trait-based Iterable) */
            if (!type_unify(iter_type, array_type)) {
                /* If failed, maybe it's something else? for now just warn/error or allow loose typing via var */
            }
            
            /* Scope for loop variable */
            scope_push(&ctx->symbols);
            symbol_define(&ctx->symbols, for_stmt->var_name, for_stmt->var_name_len,
                          SYMBOL_VAR, elem_type, for_stmt->common.span);
            
            bool was_in_loop = ctx->in_loop;
            ctx->in_loop = true;
            check_block(ctx, for_stmt->body);
            ctx->in_loop = was_in_loop;
            scope_pop(&ctx->symbols);
            break;
        }

        case AST_LOOP_STMT: {
            bool was_in_loop = ctx->in_loop;
            ctx->in_loop = true;
            check_block(ctx, stmt->as.loop_stmt.body);
            ctx->in_loop = was_in_loop;
            break;
        }
        
        case AST_SPAWN_STMT:
            sema_infer_expr(ctx, stmt->as.spawn_stmt.expr);
            break;
            
        case AST_RECEIVE_STMT:
            /* Check receive arms */
            for (size_t i = 0; i < stmt->as.receive_stmt.arm_count; i++) {
                ReceiveArm* arm = &stmt->as.receive_stmt.arms[i];
                if (arm->body) {
                    /* Manually manage scope to include pattern */
                    scope_push(&ctx->symbols);
                    
                    if (arm->pattern && arm->pattern_len > 0) {
                        /* Define pattern var */
                        symbol_define(&ctx->symbols, arm->pattern, arm->pattern_len, 
                                      SYMBOL_VAR, type_var(&ctx->type_arena), (Span){0});
                    }
                    
                    for (size_t j = 0; j < arm->body->stmt_count; j++) {
                        sema_check_stmt(ctx, arm->body->stmts[j]);
                    }
                    scope_pop(&ctx->symbols);
                }
            }
            break;
            
        case AST_BREAK_STMT:
        case AST_CONTINUE_STMT:
            if (!ctx->in_loop) {
                sema_error(ctx, stmt->as.break_stmt.span, 
                          "break/continue outside loop");
            }
            break;
            
        case AST_BLOCK:
            check_block(ctx, &stmt->as.block);
            break;
            
        default:
            break;
    }
}

/* ============================================================
 * Declaration Checking
 * ============================================================ */

static void check_function(SemaContext* ctx, AstFnDecl* fn, const char* override_name) {
    /* Create function type */
    Type** param_types = NULL;
    if (fn->param_count > 0) {
        param_types = type_arena_alloc(&ctx->type_arena, 
            sizeof(Type*) * fn->param_count);
    }
    
    /* Push function scope */
    scope_push(&ctx->symbols);
    
    /* Define parameters */
    for (size_t i = 0; i < fn->param_count; i++) {
        param_types[i] = type_var(&ctx->type_arena);  /* Will resolve from usage */
        Symbol* sym = symbol_define(&ctx->symbols, 
            fn->params[i].name, fn->params[i].name_len,
            SYMBOL_PARAM, param_types[i], fn->common.span);
        if (sym) {
            sym->is_mutable = fn->params[i].is_mut;
        }
    }
    
    /* Set return type context */
    Type* ret_type = type_var(&ctx->type_arena);
    Type* saved_return = ctx->current_fn_return;
    ctx->current_fn_return = ret_type;
    
    /* Check body */
    if (fn->body) {
        for (size_t i = 0; i < fn->body->stmt_count; i++) {
            sema_check_stmt(ctx, fn->body->stmts[i]);
        }
    }
    
    ctx->current_fn_return = saved_return;
    scope_pop(&ctx->symbols);
    
    /* Build function type */
    Type* fn_type = type_fn(&ctx->type_arena, param_types, fn->param_count, ret_type);
    
    /* Define function in enclosing scope */
    if (override_name) {
        char* permanent_name = my_strdup(override_name);
        symbol_define(&ctx->symbols, permanent_name, strlen(permanent_name), SYMBOL_FN, fn_type, fn->common.span);
    } else {
        symbol_define(&ctx->symbols, fn->name, fn->name_len, SYMBOL_FN, fn_type, fn->common.span);
    }
}

static void check_actor(SemaContext* ctx, AstActorDecl* actor) {
    /* Define actor type */
    /* Lookup existing actor type from Pass 1 */
    Symbol* sym = symbol_lookup(&ctx->symbols, actor->name, actor->name_len);
    Type* actor_type;
    
    if (sym && sym->kind == SYMBOL_ACTOR) {
        if (sym->type->kind != TYPE_ACTOR) {
            sym->type = type_actor(&ctx->type_arena, actor->name, actor->name_len);
        }
        actor_type = sym->type;
    } else {
        /* Should create new if not found (e.g. not top level? though check_actor usually top level) */
        actor_type = type_actor(&ctx->type_arena, actor->name, actor->name_len);
        symbol_define(&ctx->symbols, actor->name, actor->name_len, 
                      SYMBOL_ACTOR, actor_type, actor->common.span);
    }
    
    /* Populate fields */
    if (actor->field_count > 0) {
        actor_type->as.actor.fields = type_arena_alloc(&ctx->type_arena, sizeof(TypeField) * actor->field_count);
        actor_type->as.actor.field_count = actor->field_count;
        
        for (size_t i = 0; i < actor->field_count; i++) {
            AstLetStmt* field = actor->fields[i];
            Type* field_type = type_var(&ctx->type_arena);
            if (field->type_ann) {
                /* TODO: parse type annotation to Type*? 
                   We don't have sema_resolve_type yet? 
                   Wait, types.c has type constructors but parser returns AstType. 
                   We need to lower AstType to Type*.
                   We don't have that helper visible here? 
                   Let's assume generic type_var for now if annotation resolver missing.
                   Actually we really need `sema_resolve_type` for explicit fields.
                   For now, let's just use `type_i32` if we can't resolve, 
                   or leave as var and rely on init?
                   But fields usually require explicit types in structs.
                */
                 /* Hack: if init exists, infer from it. If not, default to i32? */
                 if (field->init) {
                     field_type = sema_infer_expr(ctx, field->init);
                 } else {
                     /* Fallback to i32 for demo if no annotation resolver */
                     field_type = type_i32(&ctx->type_arena); 
                 }
            } else if (field->init) {
                field_type = sema_infer_expr(ctx, field->init);
            }
            
            actor_type->as.actor.fields[i].name = field->name;
            actor_type->as.actor.fields[i].name_len = field->name_len;
            actor_type->as.actor.fields[i].type = field_type;
        }
    }
    
    /* Check methods - define them in global scope with mangled names */
    bool was_in_actor = ctx->in_actor;
    Type* was_cur_actor = ctx->cur_actor;
    
    ctx->in_actor = true;
    ctx->cur_actor = actor_type;
    
    /* We do NOT push a scope here, so methods are defined in the enclosing (global) scope */
    
    char buffer[256];
    char actor_name[128];
    int len = actor->name_len < 127 ? actor->name_len : 127;
    memcpy(actor_name, actor->name, len);
    actor_name[len] = '\0';
    
    for (size_t i = 0; i < actor->method_count; i++) {
        AstFnDecl* method = actor->methods[i];
        
        /* Construct mangled name */
        snprintf(buffer, sizeof(buffer), "%s_%.*s", actor_name, method->name_len, method->name);
        
        check_function(ctx, method, buffer);
    }
    
    /* Check receive block if present */
    if (actor->receive_block) {
        /* check receive stmt arms */
        /* We need a dummy context or just check the statement directly? */
        /* AST_RECEIVE_STMT checking expects to be in a function usually? */
        /* Just check it directly. */
        AstStmt stmt;
        stmt.kind = AST_RECEIVE_STMT;
        stmt.as.receive_stmt = *actor->receive_block;
        sema_check_stmt(ctx, &stmt);
    }
    
    ctx->in_actor = was_in_actor;
    ctx->cur_actor = was_cur_actor;
}

static void check_struct_decl(SemaContext* ctx, AstStructDecl* decl) {
    Symbol* sym = symbol_lookup(&ctx->symbols, decl->name, decl->name_len);
    Type* struct_type;
    
    if (sym) { // && sym->kind == SYMBOL_STRUCT (assume) (Forward declared)
        if (sym->type->kind != TYPE_STRUCT) {
             sym->type = type_struct(&ctx->type_arena, decl->name, decl->name_len);
        }
        struct_type = sym->type;
    } else {
        struct_type = type_struct(&ctx->type_arena, decl->name, decl->name_len);
        symbol_define(&ctx->symbols, decl->name, decl->name_len, 
                      SYMBOL_VAR, struct_type, decl->common.span); // Using SYMBOL_VAR for type name? Or need SYMBOL_TYPE? 
                      // AstActor uses SYMBOL_ACTOR. Lexer has TOK_STRUCT. 
                      // Need SYMBOL_STRUCT in symbol.h? Or reuse.
    }
    
    if (decl->field_count > 0) {
        struct_type->as.struct_type.fields = type_arena_alloc(&ctx->type_arena, sizeof(TypeField) * decl->field_count);
        struct_type->as.struct_type.field_count = decl->field_count;
        
        for (size_t i = 0; i < decl->field_count; i++) {
            /* Fields are FnParam */
            struct_type->as.struct_type.fields[i].name = decl->fields[i].name;
            struct_type->as.struct_type.fields[i].name_len = decl->fields[i].name_len;
            struct_type->as.struct_type.fields[i].type = type_var(&ctx->type_arena); 
            /* If we had resolver, use decl->fields[i].type (AstType*) */
        }
    }
}

void sema_check_decl(SemaContext* ctx, AstDecl* decl) {
    if (!decl) return;
    
    switch (decl->kind) {
        case AST_FN_DECL:
            check_function(ctx, &decl->as.fn_decl, NULL);
            break;
            
        case AST_ACTOR_DECL:
            check_actor(ctx, &decl->as.actor_decl);
            break;

        case AST_STRUCT_DECL:
            check_struct_decl(ctx, &decl->as.struct_decl);
            break;
            
        default:
            break;
    }
}

/* ============================================================
 * Main Entry Point
 * ============================================================ */

bool sema_analyze(SemaContext* ctx, AstProgram* program) {
    if (!program) return false;
    
    /* First pass: collect top-level declarations */
    for (size_t i = 0; i < program->decl_count; i++) {
        AstDecl* decl = program->decls[i];
        const char* name = NULL;
        uint32_t name_len = 0;
        SymbolKind kind = SYMBOL_VAR;
        
        switch (decl->kind) {
            case AST_FN_DECL:
                name = decl->as.fn_decl.name;
                name_len = decl->as.fn_decl.name_len;
                kind = SYMBOL_FN;
                break;
            case AST_ACTOR_DECL:
                name = decl->as.actor_decl.name;
                name_len = decl->as.actor_decl.name_len;
                kind = SYMBOL_ACTOR;
                break;
            case AST_STRUCT_DECL:
                name = decl->as.struct_decl.name;
                name_len = decl->as.struct_decl.name_len;
                kind = SYMBOL_VAR; /* SYMBOL_STRUCT if checking kind strictly, but standard name */
                break;
            default:
                continue;
        }
        
        /* Forward declare */
        Symbol* sym = symbol_define(&ctx->symbols, name, name_len, kind, 
                                    type_var(&ctx->type_arena), 
                                    (Span){0});
        if (sym) {
            sym->is_defined = false;
        }
    }

    /* Inject intrinsics */
    {
        /* fn print(val: i32) -> void */
        Type** params = type_arena_alloc(&ctx->type_arena, sizeof(Type*));
        params[0] = type_i32(&ctx->type_arena);
        Type* print_type = type_fn(&ctx->type_arena, params, 1, type_unit(&ctx->type_arena));
        symbol_define(&ctx->symbols, "print", 5, SYMBOL_FN, print_type, (Span){0});
    }
    
    /* Second pass: check declarations */
    for (size_t i = 0; i < program->decl_count; i++) {
        sema_check_decl(ctx, program->decls[i]);
    }
    
    return !ctx->had_error;
}
