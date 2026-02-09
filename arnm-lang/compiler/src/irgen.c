/*
 * ARNm Compiler - IR Generation Implementation
 */

#include "../include/irgen.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

/* ============================================================
 * Contract Assertions (Day 8 Hardening)
 * These validate that sema has done its job before irgen runs.
 * ============================================================ */

#define IRGEN_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "[IRGEN CONTRACT VIOLATION] %s\n", msg); \
        assert(cond); \
    } \
} while(0)

#define IRGEN_REQUIRE_NOT_NULL(ptr, name) \
    IRGEN_ASSERT((ptr) != NULL, name " must not be NULL")

#define IRGEN_REQUIRE_BLOCK(ctx) \
    IRGEN_ASSERT((ctx)->cur_block != NULL, "current block must be set")

#define IRGEN_REQUIRE_FN(ctx) \
    IRGEN_ASSERT((ctx)->cur_fn != NULL, "current function must be set")

typedef struct {
    SemaContext* sema;
    IrModule*    mod;
    IrFunction*  cur_fn;
    IrBlock*     cur_block;
    Type*        cur_actor_type; /* Current actor type if inside actor */
    
    /* Loop context for break/continue */
    IrBlock*     break_bb;    /* Target for break statements */
    IrBlock*     continue_bb; /* Target for continue statements */
    
    struct {
        char*   name; /* Owns the copy */
        IrValue val; 
        IrType  type; /* Content type */
    } locals[256];
    int local_count;
} GenContext;

static char* copy_name(const char* name, uint32_t len) {
    char* copy = malloc(len + 1);
    memcpy(copy, name, len);
    copy[len] = '\0';
    return copy;
}

static char* my_strdup(const char* s) {
    size_t len = strlen(s);
    char* copy = malloc(len + 1);
    memcpy(copy, s, len + 1);
    return copy;
}

static IrValue lookup_local(GenContext* ctx, const char* name, uint32_t len, IrType* out_type) {
    for (int i = 0; i < ctx->local_count; i++) {
        if (strlen(ctx->locals[i].name) == len &&
            strncmp(ctx->locals[i].name, name, len) == 0) {
            if (out_type) *out_type = ctx->locals[i].type;
            return ctx->locals[i].val;
        }
    }
    return (IrValue){ .kind = VAL_UNDEF };
}

static void add_local(GenContext* ctx, const char* name, uint32_t len, IrValue val, IrType type) {
    if (ctx->local_count < 256) {
        ctx->locals[ctx->local_count].name = copy_name(name, len);
        ctx->locals[ctx->local_count].val = val;
        ctx->locals[ctx->local_count].type = type;
        ctx->local_count++;
    }
}

static void free_locals(GenContext* ctx) {
    for (int i = 0; i < ctx->local_count; i++) {
        free(ctx->locals[i].name);
    }
    ctx->local_count = 0;
}

/* ============================================================
 * Expression Generation
 * ============================================================ */

static IrValue gen_expr(GenContext* ctx, AstExpr* expr);
static IrValue gen_spawn_call(GenContext* ctx, AstCallExpr* call); /* Forward declare */

static IrValue gen_binary(GenContext* ctx, AstBinaryExpr* bin) {
    if (bin->op == BINARY_ASSIGN) {
        if (bin->left->kind == AST_IDENT_EXPR) {
            /* Assignment: lookup address, eval rhs, store */
            IrValue addr = lookup_local(ctx, bin->left->as.ident.name, bin->left->as.ident.name_len, NULL); // null type ok for store?
            /* Wait, lookup_local returns the address (alloca result). It is PTR. */
            
            if (addr.kind != VAL_UNDEF) {
                 IrValue rhs = gen_expr(ctx, bin->right);
                 ir_build_store(ctx->cur_block, rhs, addr);
                 return rhs;
            }
        } else if (bin->left->kind == AST_FIELD_EXPR) {
            AstFieldExpr* field = &bin->left->as.field;
            /* self.field = rhs */
            /* 1. Gen object (should be self) */
            IrValue obj = gen_expr(ctx, field->object);
            
            /* 2. Lookup field index */
            /* Same logic as gen_expr field read. Factor out? */
            if (ctx->cur_actor_type && field->object->kind == AST_SELF_EXPR) {
                 const char* name = field->field_name;
                 uint32_t len = field->field_name_len;
                 
                 int index = -1;
                 TypeActor* actor = &ctx->cur_actor_type->as.actor;
                 for (size_t i = 0; i < actor->field_count; i++) {
                     if (actor->fields[i].name_len == len &&
                         strncmp(actor->fields[i].name, name, len) == 0) {
                         index = (int)i;
                         break;
                     }
                 }
                 
                 if (index >= 0) {
                     /* Emit field ptr */
                     /* obj is Process*. Load actor_state from offset 0 (first field). */
                     IrInstr* state_load = ir_build_load(ctx->cur_fn, ctx->cur_block, ir_type_ptr(), obj);
                     
                     IrInstr* fptr = ir_build_field_ptr(ctx->cur_fn, ctx->cur_block, state_load->result, index);
                     
                     /* Emit store */
                     IrValue rhs = gen_expr(ctx, bin->right);
                     ir_build_store(ctx->cur_block, rhs, fptr->result);
                     return rhs;
                 }
            }
        }

        return (IrValue){ .kind = VAL_UNDEF };
    }

    IrValue lhs = gen_expr(ctx, bin->left);
    IrValue rhs = gen_expr(ctx, bin->right);
    
    IrInstr* inst = NULL;
    switch (bin->op) {
        case BINARY_ADD: inst = ir_build_add(ctx->cur_fn, ctx->cur_block, lhs, rhs); break;
        case BINARY_SUB: inst = ir_build_sub(ctx->cur_fn, ctx->cur_block, lhs, rhs); break;
        case BINARY_MUL: inst = ir_build_mul(ctx->cur_fn, ctx->cur_block, lhs, rhs); break;
        case BINARY_DIV: inst = ir_build_div(ctx->cur_fn, ctx->cur_block, lhs, rhs); break;
        case BINARY_MOD: inst = ir_build_mod(ctx->cur_fn, ctx->cur_block, lhs, rhs); break;
        case BINARY_EQ:  inst = ir_build_cmp(ctx->cur_fn, ctx->cur_block, IR_EQ, lhs, rhs); break;
        case BINARY_NE:  inst = ir_build_cmp(ctx->cur_fn, ctx->cur_block, IR_NE, lhs, rhs); break;
        case BINARY_LT:  inst = ir_build_cmp(ctx->cur_fn, ctx->cur_block, IR_LT, lhs, rhs); break;
        case BINARY_GT:  inst = ir_build_cmp(ctx->cur_fn, ctx->cur_block, IR_GT, lhs, rhs); break;
        case BINARY_LE:  inst = ir_build_cmp(ctx->cur_fn, ctx->cur_block, IR_LE, lhs, rhs); break;
        case BINARY_GE:  inst = ir_build_cmp(ctx->cur_fn, ctx->cur_block, IR_GE, lhs, rhs); break;
        case BINARY_AND: inst = ir_build_and(ctx->cur_fn, ctx->cur_block, lhs, rhs); break;
        case BINARY_OR:  inst = ir_build_or(ctx->cur_fn, ctx->cur_block, lhs, rhs); break;
        default: break;
    }
    
    return inst ? inst->result : (IrValue){ .kind = VAL_UNDEF };
}

static IrValue gen_identifier(GenContext* ctx, AstIdentExpr* ident) {
    IrType type = ir_type_i32(); /* fallback */
    IrValue ptr = lookup_local(ctx, ident->name, ident->name_len, &type);
    
    if (ptr.kind != VAL_UNDEF) {
        IrInstr* load = ir_build_load(ctx->cur_fn, ctx->cur_block, type, ptr);
        return load->result;
    }
    return (IrValue){ .kind = VAL_UNDEF };
}

static IrValue gen_send(GenContext* ctx, AstSendExpr* send) {
    IrValue target = gen_expr(ctx, send->target);
    IrValue msg = gen_expr(ctx, send->message);
    
    /* arnm_send(target, tag, data, size) */
    /* Prototype: assume msg is i32, so tag=msg, data=NULL, size=0 */
    /* Real implementation needs marshalling */
    
    IrValue args[4];
    args[0] = target; /* Target process */
    args[1] = msg;    /* Tag (i32) */
    
    /* Need NULL and 0 constants */
    /* hack: 0 cast to ptr is NULL */
    args[2] = ir_val_const_i32(0); args[2].type = ir_type_ptr(); 
    args[3] = ir_val_const_i32(0); args[3].type = ir_type_i64(); /* i64 0 */
    
    ir_build_call(ctx->cur_fn, ctx->cur_block, "arnm_send", args, 4, ir_type_void());
    
    return (IrValue){ .kind = VAL_UNDEF };
}



static IrValue gen_call(GenContext* ctx, AstCallExpr* call) {
    if (call->callee->kind == AST_IDENT_EXPR) {
        AstIdentExpr* id = &call->callee->as.ident;
        
        /* Resolve arguments */
        IrValue* args = NULL;
        if (call->arg_count > 0) {
            args = malloc(sizeof(IrValue) * call->arg_count);
            for (size_t i = 0; i < call->arg_count; i++) {
                args[i] = gen_expr(ctx, call->args[i]);
            }
        }
        
        /* Build call instruction */
        /* Note: For now, all calls are global function calls. 
           We assume the callee name IS the global symbol name.
           Indirect calls (function pointers) are not fully supported yet. */
           
        /* Determine return type (void for print, i32 otherwise for now) */
        IrType ret_type = ir_type_i32();
        if (id->name_len == 5 && strncmp(id->name, "print", 5) == 0) {
            ret_type = ir_type_void();
        }
           
        IrInstr* inst = ir_build_call(ctx->cur_fn, ctx->cur_block, 
                                      copy_name(id->name, id->name_len), 
                                      args, call->arg_count, ret_type);
                                      
        if (args) free(args);
        return inst->result;
    }
    return (IrValue){ .kind = VAL_UNDEF };
}

static IrValue gen_expr(GenContext* ctx, AstExpr* expr) {
    /* Contract: gen_expr requires valid context and expression */
    IRGEN_REQUIRE_NOT_NULL(ctx, "context");
    if (!expr) {
        return (IrValue){ .kind = VAL_UNDEF };
    }
    IRGEN_REQUIRE_FN(ctx);
    IRGEN_REQUIRE_BLOCK(ctx);
    
    switch (expr->kind) {
        case AST_BINARY_EXPR: {
            if (expr->as.binary.op == BINARY_ASSIGN) {
                AstExpr* lhs = expr->as.binary.left;
                IrValue rhs_val = gen_expr(ctx, expr->as.binary.right);
                
                if (lhs->kind == AST_IDENT_EXPR) {
                    IrType type = ir_type_i32(); 
                    IrValue ptr = lookup_local(ctx, lhs->as.ident.name, lhs->as.ident.name_len, &type);
                    if (ptr.kind != VAL_UNDEF) {
                        ir_build_store(ctx->cur_block, rhs_val, ptr);
                    }
                } else if (lhs->kind == AST_FIELD_EXPR) {
                    /* Handle self.field = val OR struct.field = val */
                    IrValue obj_val = gen_expr(ctx, lhs->as.field.object);
                    Type* obj_type = NULL;
                    /* Get sema_type from the object expression based on its kind */
                    if (lhs->as.field.object->kind == AST_SELF_EXPR) {
                        obj_type = ctx->cur_actor_type;
                    } else if (lhs->as.field.object->kind == AST_IDENT_EXPR) {
                        obj_type = lhs->as.field.object->as.ident.common.sema_type;
                    } else if (lhs->as.field.object->kind == AST_FIELD_EXPR) {
                        obj_type = lhs->as.field.object->as.field.common.sema_type;
                    }
                    /* Fallback for self if sema_type missing? */
                    if (!obj_type && lhs->as.field.object->kind == AST_SELF_EXPR) obj_type = ctx->cur_actor_type;
                    
                    if (obj_type) {
                        obj_type = type_resolve(obj_type);
                        const char* name = lhs->as.field.field_name;
                        uint32_t len = lhs->as.field.field_name_len;
                        int index = -1;
                        
                        if (obj_type->kind == TYPE_STRUCT) {
                            TypeStruct* st = &obj_type->as.struct_type;
                            for (size_t i = 0; i < st->field_count; i++) {
                                if (st->fields[i].name_len == len && strncmp(st->fields[i].name, name, len) == 0) {
                                    index = (int)i;
                                    break;
                                }
                            }
                        } else if (obj_type->kind == TYPE_ACTOR) {
                            TypeActor* actor = &obj_type->as.actor;
                            for (size_t i = 0; i < actor->field_count; i++) {
                                if (actor->fields[i].name_len == len && strncmp(actor->fields[i].name, name, len) == 0) {
                                    index = (int)i;
                                    break;
                                }
                            }
                        }
                        
                        if (index >= 0) {
                            IrValue base = obj_val;
                            /* If actor (self), verify indirection. Assuming self is Process*, state is at *self (first field) or similar? 
                               Previous logic used ir_build_load(ptr, obj_val). Assuming obj_val is address of pointer?
                               If obj_val is IR_VAR (the parameter 'self'), it holds Process*.
                               If state is embedded?
                               If runtime stores state pointer as first generic field.
                               We load it. 
                            */
                            if (obj_type->kind == TYPE_ACTOR) {
                                IrInstr* state_load = ir_build_load(ctx->cur_fn, ctx->cur_block, ir_type_ptr(), base);
                                base = state_load->result;
                            }
                            
                            IrInstr* fptr = ir_build_field_ptr(ctx->cur_fn, ctx->cur_block, base, index);
                            ir_build_store(ctx->cur_block, rhs_val, fptr->result);
                        }
                    }
                }
                return rhs_val;
            }
            return gen_binary(ctx, &expr->as.binary);
        }
        case AST_INT_LIT_EXPR: return ir_val_const_i32(expr->as.int_lit.value);
        case AST_BOOL_LIT_EXPR: return ir_val_const_bool(expr->as.bool_lit.value);
        case AST_STRING_LIT_EXPR: {
            /* Create a constant value with string pointer stored in constant.as.i */
            IrValue v;
            v.kind = VAL_CONST;
            v.type.kind = IR_PTR;  /* Mark as pointer (string) */
            /* Store string pointer as uint64_t - interpreter will extract it */
            v.storage.constant.as.i = (uint64_t)(uintptr_t)expr->as.string_lit.value;
            return v;
        }
        case AST_IDENT_EXPR: return gen_identifier(ctx, &expr->as.ident);
        case AST_CALL_EXPR: return gen_call(ctx, &expr->as.call);
        case AST_FIELD_EXPR: {
            IrValue obj = gen_expr(ctx, expr->as.field.object);
            Type* obj_type = NULL;
            /* Get sema_type from the object expression based on its kind */
            if (expr->as.field.object->kind == AST_SELF_EXPR) {
                obj_type = ctx->cur_actor_type;
            } else if (expr->as.field.object->kind == AST_IDENT_EXPR) {
                obj_type = expr->as.field.object->as.ident.common.sema_type;
            } else if (expr->as.field.object->kind == AST_FIELD_EXPR) {
                obj_type = expr->as.field.object->as.field.common.sema_type;
            }
            
            if (obj_type) {
                obj_type = type_resolve(obj_type);
                const char* name = expr->as.field.field_name;
                uint32_t len = expr->as.field.field_name_len;
                int index = -1;
                
                if (obj_type->kind == TYPE_STRUCT) {
                    TypeStruct* st = &obj_type->as.struct_type;
                    for (size_t i = 0; i < st->field_count; i++) {
                        if (st->fields[i].name_len == len && strncmp(st->fields[i].name, name, len) == 0) {
                            index = (int)i;
                            break;
                        }
                    }
                } else if (obj_type->kind == TYPE_ACTOR) {
                    TypeActor* actor = &obj_type->as.actor;
                    for (size_t i = 0; i < actor->field_count; i++) {
                        if (actor->fields[i].name_len == len && strncmp(actor->fields[i].name, name, len) == 0) {
                            index = (int)i;
                            break;
                        }
                    }
                }
                
                if (index != -1) {
                    IrValue base = obj;
                    if (obj_type->kind == TYPE_ACTOR) {
                        IrInstr* state_load = ir_build_load(ctx->cur_fn, ctx->cur_block, ir_type_ptr(), base);
                        base = state_load->result;
                    }
                    
                    IrInstr* fptr = ir_build_field_ptr(ctx->cur_fn, ctx->cur_block, base, index);
                    
                    /* Determine result type for load */
                    IrType load_ty = ir_type_i32(); /* Default */
                    /* Ideally convert expr->common.sema_type to IrType. 
                       But current IRGen just uses i32/ptr. 
                       If field type is pointer (actor, struct)? 
                       We need type mapping. 
                       For now, use i32 unless we know better. */
                    
                    IrInstr* load = ir_build_load(ctx->cur_fn, ctx->cur_block, load_ty, fptr->result);
                    return load->result;
                }
            }
            return (IrValue){ .kind = VAL_UNDEF };
        }

        case AST_SEND_EXPR: return gen_send(ctx, &expr->as.send);
        case AST_SPAWN_EXPR: {
            AstExpr* target = expr->as.spawn_expr.expr;
            while (target->kind == AST_GROUP_EXPR) {
                target = target->as.group.inner;
            }
            
            if (target->kind == AST_CALL_EXPR) {
                 return gen_spawn_call(ctx, &target->as.call);
            }
            return (IrValue){ .kind = VAL_UNDEF };
        }
        case AST_SELF_EXPR: {
            /* call arnm_self() */
            IrInstr* call = ir_build_call(ctx->cur_fn, ctx->cur_block, "arnm_self", NULL, 0, ir_type_ptr());
            return call->result;
        }
        case AST_GROUP_EXPR: return gen_expr(ctx, expr->as.group.inner);
        default: return (IrValue){ .kind = VAL_UNDEF };
    }
}

/* ============================================================
 * Statement Generation
 * ============================================================ */

static void gen_block(GenContext* ctx, AstBlock* block);

static void gen_stmt(GenContext* ctx, AstStmt* stmt) {
    /* Contract: gen_stmt requires valid context and statement */
    IRGEN_REQUIRE_NOT_NULL(ctx, "context");
    if (!stmt) return;  /* Silently skip null statements */
    IRGEN_REQUIRE_FN(ctx);
    IRGEN_REQUIRE_BLOCK(ctx);
    
    switch (stmt->kind) {
        case AST_LET_STMT: {
            AstLetStmt* let = &stmt->as.let_stmt;
            IrValue init_val;
            if (let->init) {
                init_val = gen_expr(ctx, let->init);
            } else {
                init_val = ir_val_const_i32(0);
            }
            
            IrType var_type = init_val.type.kind != IR_BAD ? init_val.type : ir_type_i32();
            
            IrInstr* alloca = ir_build_alloca(ctx->cur_fn, ctx->cur_block, var_type);
            ir_build_store(ctx->cur_block, init_val, alloca->result);
            add_local(ctx, let->name, let->name_len, alloca->result, var_type);
            break;
        }
        case AST_RETURN_STMT: {
            AstReturnStmt* ret = &stmt->as.return_stmt;
            if (ret->value) {
                IrValue val = gen_expr(ctx, ret->value);
                ir_build_ret(ctx->cur_block, val);
            } else {
                ir_build_ret_void(ctx->cur_block);
            }
            break;
        }
        case AST_EXPR_STMT: {
            gen_expr(ctx, stmt->as.expr_stmt.expr);
            break;
        }
        case AST_LOOP_STMT: {
            IrBlock* body_bb = ir_block_create(ctx->cur_fn, "loop.body");
            IrBlock* end_bb  = ir_block_create(ctx->cur_fn, "loop.end");
            
            /* Jump to body */
            ir_build_jmp(ctx->cur_block, body_bb);
            
            /* Save previous loop context */
            IrBlock* prev_break = ctx->break_bb;
            IrBlock* prev_continue = ctx->continue_bb;
            ctx->break_bb = end_bb;
            ctx->continue_bb = body_bb;
            
            /* Body Block */
            ctx->cur_block = body_bb;
            gen_block(ctx, stmt->as.loop_stmt.body);
            
            /* Loop back unconditionally */
            ir_build_jmp(ctx->cur_block, body_bb);
            
            /* Restore context */
            ctx->break_bb = prev_break;
            ctx->continue_bb = prev_continue;
            
            /* Continue compilation at end_bb */
            ctx->cur_block = end_bb;
            break;
        }
        case AST_FOR_STMT: {
            fprintf(stderr, "Code generation for 'for' loops not yet implemented\n");
            break;
        }
        case AST_SPAWN_STMT: {
            AstSpawnStmt* spawn = &stmt->as.spawn_stmt;
            if (spawn->expr->kind == AST_CALL_EXPR) {
                gen_spawn_call(ctx, &spawn->expr->as.call);
            } else if (spawn->expr->kind == AST_GROUP_EXPR) {
                AstExpr* target = spawn->expr;
                while (target->kind == AST_GROUP_EXPR) target = target->as.group.inner;
                if (target->kind == AST_CALL_EXPR) {
                    gen_spawn_call(ctx, &target->as.call);
                }
            }
            break;
        }
        case AST_IF_STMT: {
            AstIfStmt* if_stmt = &stmt->as.if_stmt;
            
            /* Gen condition */
            IrValue cond = gen_expr(ctx, if_stmt->condition);
            /* Ensure cond is i1? Arnm likely has no strict bool requirement yet, but LLVM needs i1 for br */
            /* If cond is i32, compare ne 0? */
            if (cond.type.kind == IR_I32) {
                /* Helper: %cmp = icmp ne i32 %cond, 0 */
                /* For now assume it's i1 or compatible, or emit trunc/cmp */
                /* Assume gen_expr returns i1 for bools. and int for i32. */
                /* If i32, we should emit `icmp ne`. */
                /* Hack: Just assume bool used properly for prototype. */
            }
            
            IrBlock* then_bb = ir_block_create(ctx->cur_fn, "then");
            IrBlock* else_bb = ir_block_create(ctx->cur_fn, "else");
            IrBlock* merge_bb = ir_block_create(ctx->cur_fn, "merge");
            
            /* Branch */
            if (if_stmt->else_branch) {
                ir_build_br(ctx->cur_block, cond, then_bb, else_bb);
            } else {
                /* no else -> branch to merge if false */
                /* Actually we can optimize out else_bb, but for structure consistency: */
                ir_build_br(ctx->cur_block, cond, then_bb, merge_bb);
                /* If no else, else_bb is unused/orphaned unless we repurpose logic.
                   If !else_branch, false goes to merge. True goes to then.
                */
            }
            
            /* Then block */
            ctx->cur_block = then_bb;
            gen_block(ctx, if_stmt->then_block);
            if (!ctx->cur_block->tail || (ctx->cur_block->tail->op != IR_RET && ctx->cur_block->tail->op != IR_BR && ctx->cur_block->tail->op != IR_JMP)) {
                ir_build_jmp(ctx->cur_block, merge_bb);
            }
            
            /* Else block */
            if (if_stmt->else_branch) {
                ctx->cur_block = else_bb;
                gen_stmt(ctx, if_stmt->else_branch); /* It's a stmt, possibly block or if */
                if (!ctx->cur_block->tail || (ctx->cur_block->tail->op != IR_RET && ctx->cur_block->tail->op != IR_BR && ctx->cur_block->tail->op != IR_JMP)) {
                    ir_build_jmp(ctx->cur_block, merge_bb);
                }
            }
            
            ctx->cur_block = merge_bb;
            break;
        }
        case AST_WHILE_STMT: {
            AstWhileStmt* w = &stmt->as.while_stmt;
            
            IrBlock* cond_bb = ir_block_create(ctx->cur_fn, "while.cond");
            IrBlock* body_bb = ir_block_create(ctx->cur_fn, "while.body");
            IrBlock* exit_bb = ir_block_create(ctx->cur_fn, "while.exit");
            
            /* Save previous loop context */
            IrBlock* prev_break = ctx->break_bb;
            IrBlock* prev_continue = ctx->continue_bb;
            
            /* Set loop context for break/continue */
            ctx->break_bb = exit_bb;
            ctx->continue_bb = cond_bb;
            
            /* Entry -> Cond */
            ir_build_jmp(ctx->cur_block, cond_bb);
            
            /* Cond Block */
            ctx->cur_block = cond_bb;
            IrValue cond = gen_expr(ctx, w->condition);
            ir_build_br(ctx->cur_block, cond, body_bb, exit_bb);
            
            /* Body Block */
            ctx->cur_block = body_bb;
            if (w->body) gen_block(ctx, w->body);
            /* Only jump back if not already terminated */
            if (!ctx->cur_block->tail || (ctx->cur_block->tail->op != IR_JMP && ctx->cur_block->tail->op != IR_BR && ctx->cur_block->tail->op != IR_RET)) {
                ir_build_jmp(ctx->cur_block, cond_bb);
            }
            
            /* Restore previous loop context */
            ctx->break_bb = prev_break;
            ctx->continue_bb = prev_continue;
            
            /* Exit Block */
            ctx->cur_block = exit_bb;
            break;
        }
        case AST_RECEIVE_STMT: {
            AstReceiveStmt* recv = &stmt->as.receive_stmt;
            
            /* Call runtime: %msg = arnm_receive(null) */
            /* Call runtime: %msg = arnm_receive(null) */
            IrValue args[1];
            args[0] = ir_val_const_i32(0); args[0].type = ir_type_ptr(); /* null */
            
            IrInstr* call = ir_build_call(ctx->cur_fn, ctx->cur_block, "arnm_receive", args, 1, ir_type_ptr());
            IrValue msg_val = call->result;
            
            /* Tag is at offset 0 in ArnmMessage */
            IrInstr* field = ir_build_field_ptr(ctx->cur_fn, ctx->cur_block, msg_val, 0);
            IrInstr* load_tag = ir_build_load(ctx->cur_fn, ctx->cur_block, ir_type_i64(), field->result);
            IrValue tag_val = load_tag->result;
            
            if (recv->arm_count == 0) {
                /* No arms - nothing to match */
                break;
            }
            
            /* Handle all arms uniformly with matching logic */
            
            /* Create blocks for each arm and a merge block */
            IrBlock** arm_blocks = malloc(sizeof(IrBlock*) * recv->arm_count);
            IrBlock* merge_bb = ir_block_create(ctx->cur_fn, "recv.merge");
            IrBlock* nomatch_bb = ir_block_create(ctx->cur_fn, "recv.nomatch");
            
            for (size_t i = 0; i < recv->arm_count; i++) {
                char label[32];
                snprintf(label, sizeof(label), "recv.arm%zu", i);
                arm_blocks[i] = ir_block_create(ctx->cur_fn, my_strdup(label));
            }
            
            /* Generate comparison chain */
            for (size_t i = 0; i < recv->arm_count; i++) {
                ReceiveArm* arm = &recv->arms[i];
                
                uint32_t expected_tag = 0;
                
                /* Check if pattern is integer literal */
                if (arm->pattern && arm->pattern_len > 0 && isdigit(arm->pattern[0])) {
                    expected_tag = (uint32_t)atoi(arm->pattern);
                } else {
                    /* Hash pattern name to get expected tag */
                    expected_tag = 5381;
                    for (uint32_t j = 0; j < arm->pattern_len; j++) {
                        expected_tag = ((expected_tag << 5) + expected_tag) + (uint8_t)arm->pattern[j];
                    }
                }
                
                /* Compare: tag_val == expected_tag */
                IrValue expected = ir_val_const_i32((int32_t)expected_tag);
                IrInstr* cmp = ir_build_cmp(ctx->cur_fn, ctx->cur_block, IR_EQ, tag_val, expected);
                
                /* Branch: if match goto arm_block, else continue to next check or nomatch */
                IrBlock* next_check = (i + 1 < recv->arm_count) ? 
                                      ir_block_create(ctx->cur_fn, "recv.check") : nomatch_bb;
                ir_build_br(ctx->cur_block, cmp->result, arm_blocks[i], next_check);
                
                ctx->cur_block = next_check;
            }
                
                /* Generate nomatch block - call panic */
                ctx->cur_block = nomatch_bb;
                ir_build_call(ctx->cur_fn, ctx->cur_block, "arnm_panic_nomatch", NULL, 0, ir_type_void());
                ir_build_jmp(ctx->cur_block, merge_bb);
                
                /* Generate each arm body */
                for (size_t i = 0; i < recv->arm_count; i++) {
                    ReceiveArm* arm = &recv->arms[i];
                    ctx->cur_block = arm_blocks[i];
                    
                    /* Bind pattern variable to the message tag value */
                    IrInstr* alloca = ir_build_alloca(ctx->cur_fn, ctx->cur_block, ir_type_i32());
                    ir_build_store(ctx->cur_block, tag_val, alloca->result);
                    
                    if (arm->pattern && arm->pattern_len > 0) {
                        add_local(ctx, arm->pattern, arm->pattern_len, alloca->result, ir_type_i32());
                    }
                    
                    if (arm->body) {
                        gen_block(ctx, arm->body);
                    }
                    
                    /* Jump to merge if not already terminated */
                    if (!ctx->cur_block->tail || 
                        (ctx->cur_block->tail->op != IR_JMP && 
                         ctx->cur_block->tail->op != IR_BR && 
                         ctx->cur_block->tail->op != IR_RET)) {
                        ir_build_jmp(ctx->cur_block, merge_bb);
                    }
                }
                
                free(arm_blocks);
                ctx->cur_block = merge_bb;
            
            break;
        }
        case AST_BREAK_STMT: {
            /* Jump to break target (loop exit) */
            if (ctx->break_bb) {
                ir_build_jmp(ctx->cur_block, ctx->break_bb);
            }
            break;
        }
        case AST_CONTINUE_STMT: {
            /* Jump to continue target (loop condition) */
            if (ctx->continue_bb) {
                ir_build_jmp(ctx->cur_block, ctx->continue_bb);
            }
            break;
        }
        default: break;
    }
}

static void gen_block(GenContext* ctx, AstBlock* block) {
    for (size_t i = 0; i < block->stmt_count; i++) {
        gen_stmt(ctx, block->stmts[i]);
    }
}

/* ============================================================
 * Declaration Generation
 * ============================================================ */

static void gen_func(GenContext* ctx, AstFnDecl* func, const char* override_name, const char* chain_call) {
    IrType ret_type = func->return_type ? ir_type_i32() : ir_type_void();
    
    char* fn_name;
    if (override_name) {
        fn_name = my_strdup(override_name);
    } else {
        fn_name = copy_name(func->name, func->name_len);
    }
    
    /* Build param types array */
    IrType* param_types = NULL;
    if (func->param_count > 0) {
        param_types = malloc(sizeof(IrType) * func->param_count);
        for (size_t i = 0; i < func->param_count; i++) {
             param_types[i] = ir_type_i32(); /* Default to i32 for now */
        }
    }
    
    IrFunction* ir_fn = ir_function_create(ctx->mod, fn_name, ret_type, param_types, func->param_count);
    if (param_types) free(param_types);
    
    ctx->cur_fn = ir_fn;
    ctx->cur_block = ir_block_create(ir_fn, "entry");
    ctx->local_count = 0; 
    
    /* Process parameters: alloca and store arg values */
    for (size_t i = 0; i < func->param_count; i++) {
        FnParam* p = &func->params[i];
        IrType ty = ir_type_i32(); /* Default */
        
        /* 1. Create argument value */
        IrValue arg_val = ir_val_var(i, ty);
        
        /* 2. Alloca for the local variable */
        IrInstr* alloca = ir_build_alloca(ctx->cur_fn, ctx->cur_block, ty);
        
        /* 3. Store arg to local */
        ir_build_store(ctx->cur_block, arg_val, alloca->result);
        
        /* 4. Register local */
        add_local(ctx, p->name, p->name_len, alloca->result, ty);
    }
    
    if (func->body) {
        gen_block(ctx, func->body);
    }
    
    if (ret_type.kind == IR_VOID) {
       if (!ctx->cur_block->tail || ctx->cur_block->tail->op != IR_RET) {
           if (chain_call) {
               ir_build_call(ctx->cur_fn, ctx->cur_block, chain_call, NULL, 0, ir_type_void());
           }
           ir_build_ret_void(ctx->cur_block);
       }
    }
    
    /* Clean up locals strings (just the array names, not IR values) */
    free_locals(ctx);
}


static IrValue gen_spawn_call(GenContext* ctx, AstCallExpr* call) {
    char* target_name = NULL;
    size_t state_size = 0;
    
    if (call->callee->kind == AST_IDENT_EXPR) {
         AstIdentExpr* id = &call->callee->as.ident;
         
         /* Check if it's an actor constructor */
         Symbol* sym = symbol_lookup(&ctx->sema->symbols, id->name, id->name_len);
         if (sym && sym->kind == SYMBOL_ACTOR) {
             /* Target is Name_init */
             char buffer[256];
             snprintf(buffer, sizeof(buffer), "%.*s_init", (int)id->name_len, id->name);
             target_name = my_strdup(buffer);
             
             /* Calculate state size */
             if (sym->type) {
                 state_size = sym->type->as.actor.field_count * 8;
             }
         } else {
             /* Normal function */
             target_name = copy_name(id->name, id->name_len);
         }
    } else if (call->callee->kind == AST_FIELD_EXPR) {
         /* Actor.init -> Actor_init */
         AstFieldExpr* field = &call->callee->as.field;
         if (field->object->kind == AST_IDENT_EXPR) {
             /* ActorName.method */
             AstIdentExpr* obj_id = &field->object->as.ident;
             
             char buffer[256];
             snprintf(buffer, sizeof(buffer), "%.*s_%.*s", 
                      (int)obj_id->name_len, obj_id->name,
                      (int)field->field_name_len, field->field_name);
             target_name = my_strdup(buffer);
             
             /* Calculate Actor State Size */
             /* We need the TypeActor to count fields. */
             Symbol* sym = symbol_lookup(&ctx->sema->symbols, obj_id->name, obj_id->name_len);
             if (sym && sym->kind == SYMBOL_ACTOR && sym->type) {
                 /* 8 bytes per field for now */
                 state_size = sym->type->as.actor.field_count * 8; 
             }
         }
    }
    
    if (!target_name) {
        return (IrValue){ .kind = VAL_UNDEF };
    }

    IrValue args[3];
    /* 1. Address of function: @name */
    args[0].kind = VAL_GLOBAL;
    args[0].storage.global.name = target_name; 
    args[0].type = ir_type_ptr();
    
    /* 2. Argument */
    if (call->arg_count > 0) {
        IrValue val = gen_expr(ctx, call->args[0]);
        val.type = ir_type_ptr(); /* Hack cast */
        args[1] = val;
    } else {
        args[1] = ir_val_const_i32(0); args[1].type = ir_type_ptr();
    }
    
    /* 3. State Size */
    args[2] = ir_val_const_i32((int)state_size); /* i32 is fine? Runtime uses size_t (i64) */
    args[2].type = ir_type_i64(); 
    
    /* Call arnm_spawn(func, arg, size) */
    IrInstr* inst = ir_build_call(ctx->cur_fn, ctx->cur_block, "arnm_spawn", args, 3, ir_type_ptr());
    return inst->result;
}



static void gen_actor(GenContext* ctx, AstActorDecl* actor) {
     /* Wait, type_actor creates a new type? We should lookup the existing one from Sema! */
    /* Accessing Sema symbols? */
    /* Symbol table look up: */
    Symbol* sym = symbol_lookup(&ctx->sema->symbols, actor->name, actor->name_len);
    if (sym && sym->kind == SYMBOL_ACTOR) {
        ctx->cur_actor_type = sym->type;
    } else {
        ctx->cur_actor_type = NULL;
    }

    /* Generate methods with mangled names: Actor_method */
    /* Generate methods with mangled names: Actor_method */
    char buffer[256];
    char behavior_name[256];
    
    /* Copy actor name to stack buffer for prefixing */
    char actor_name[128];
    int len = actor->name_len < 127 ? actor->name_len : 127;
    memcpy(actor_name, actor->name, len);
    actor_name[len] = '\0';
    
    /* Generate receive block FIRST so we have the name for init chaining */
    if (actor->receive_block) {
        snprintf(behavior_name, sizeof(behavior_name), "%s_behavior", actor_name);
        
        /* Create synthetic function for behavior */
        IrFunction* ir_fn = ir_function_create(ctx->mod, my_strdup(behavior_name), ir_type_void(), NULL, 0);
        ctx->cur_fn = ir_fn;
        ctx->cur_block = ir_block_create(ir_fn, "entry");
        ctx->local_count = 0;
    
        /* Loop header */
        IrBlock* loop_bb = ir_block_create(ir_fn, "loop");
        ir_build_jmp(ctx->cur_block, loop_bb);
        ctx->cur_block = loop_bb;
        
        /* Generate receive stmt */
        AstStmt stmt;
        stmt.kind = AST_RECEIVE_STMT;
        stmt.as.receive_stmt = *actor->receive_block;
        gen_stmt(ctx, &stmt);
        
        /* Jump back to loop start */
        ir_build_jmp(ctx->cur_block, loop_bb);
        
        /* No return needed since infinite loop, but for safety/structure validity: */
        /* ir_build_ret_void(ctx->cur_block); */ /* Unreachable */
        free_locals(ctx);
    } else {
        behavior_name[0] = '\0';
    }

    /* helper to check for init */
    #define IS_INIT(m) ((m)->name_len == 4 && strncmp((m)->name, "init", 4) == 0)

    for (size_t i = 0; i < actor->method_count; i++) {
        AstFnDecl* method = actor->methods[i];
        
        /* Construct mangled name */
        snprintf(buffer, sizeof(buffer), "%s_%.*s", actor_name, method->name_len, method->name);
        
        const char* chain = NULL;
        if (IS_INIT(method) && actor->receive_block) {
            chain = behavior_name;
        }
        
        gen_func(ctx, method, buffer, chain);
    }
}

bool ir_generate(SemaContext* ctx, AstProgram* program, IrModule* out_mod) {
    GenContext gen_ctx;
    gen_ctx.sema = ctx;
    gen_ctx.mod = out_mod;
    gen_ctx.cur_fn = NULL;
    gen_ctx.cur_block = NULL;
    gen_ctx.cur_actor_type = NULL;
    gen_ctx.break_bb = NULL;
    gen_ctx.continue_bb = NULL;
    gen_ctx.cur_block = NULL;
    gen_ctx.cur_actor_type = NULL;
    gen_ctx.local_count = 0;
    
    ir_module_init(out_mod);
    
    for (size_t i = 0; i < program->decl_count; i++) {
        if (program->decls[i]->kind == AST_FN_DECL) {
             gen_func(&gen_ctx, &program->decls[i]->as.fn_decl, NULL, NULL);
        } else if (program->decls[i]->kind == AST_ACTOR_DECL) {
             gen_actor(&gen_ctx, &program->decls[i]->as.actor_decl);
        }
    }
    
    return true;
}