/*
 * ARNm Compiler - IR Generation Implementation
 */

#include "../include/irgen.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

typedef struct {
    SemaContext* sema;
    IrModule*    mod;
    IrFunction*  cur_fn;
    IrBlock*     cur_block;
    Type*        cur_actor_type; /* Current actor type if inside actor */
    
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
        case BINARY_EQ:  inst = ir_build_cmp(ctx->cur_fn, ctx->cur_block, IR_EQ, lhs, rhs); break;
        case BINARY_NE:  inst = ir_build_cmp(ctx->cur_fn, ctx->cur_block, IR_NE, lhs, rhs); break;
        case BINARY_LT:  inst = ir_build_cmp(ctx->cur_fn, ctx->cur_block, IR_LT, lhs, rhs); break;
        case BINARY_GT:  inst = ir_build_cmp(ctx->cur_fn, ctx->cur_block, IR_GT, lhs, rhs); break;
        case BINARY_LE:  inst = ir_build_cmp(ctx->cur_fn, ctx->cur_block, IR_LE, lhs, rhs); break;
        case BINARY_GE:  inst = ir_build_cmp(ctx->cur_fn, ctx->cur_block, IR_GE, lhs, rhs); break;
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
    switch (expr->kind) {
        case AST_BINARY_EXPR: return gen_binary(ctx, &expr->as.binary);
        case AST_INT_LIT_EXPR: return ir_val_const_i32(expr->as.int_lit.value);
        case AST_BOOL_LIT_EXPR: return ir_val_const_i32(expr->as.bool_lit.value ? 1 : 0);
        case AST_IDENT_EXPR: return gen_identifier(ctx, &expr->as.ident);
        case AST_CALL_EXPR: return gen_call(ctx, &expr->as.call);
        case AST_FIELD_EXPR: {
            /* self.field */
            /* 1. Gen object (self) */
            IrValue obj = gen_expr(ctx, expr->as.field.object);
            
            /* 2. Find field index */
            /* Need to look up field in type? 
               IR Gen usually relies on type info attached to expression or inferred.
               But our AST nodes don't store resolved Type* directly (Sema does verification but lost it?).
               Sema context has types. 
               But 'gen_expr' receives AstExpr.
               Typically compilers annotate AST with types during Sema. 
               We didn't add 'Type* type' to AstExpr common? 
               AstCommon has span. No type.
               
               We need to lookup type again? Or pass it?
               For `self`, we know it is `ctx->cur_fn`'s class? 
               Wait, `ctx->cur_fn` is IR function.
               We don't know which actor we are in easily unless we track it.
               
               Workaround: Assume `obj` is `IR_SELF` or `IR_PTR`.
               Resolve field name to index by looking at the Actor declaration?
               But we don't have the Actor decl here easily.
               
               Better: Annotate AST with type in Sema.
               Quick fix: re-lookup symbol or similar? 
               'self' type is semantically known. 
               
               Let's add `AstActorDecl* cur_actor` to `GenContext`.
            */
            
            /* Assume we are in an actor method if self is used. */
            /* We need to know which actor to look up field index. */
            
            /* Implementation Plan:
               1. Add `Type* self_type` (or `AstActorDecl*`) to GenContext.
               2. When generating actor methods, set it.
               3. Iterate `self_type` fields to find index.
            */
            
            if (ctx->cur_actor_type && expr->as.field.object->kind == AST_SELF_EXPR) {
                 const char* name = expr->as.field.field_name;
                 uint32_t len = expr->as.field.field_name_len;
                 
                 int index = -1;
                 /* TypeActor has fields now */
                 TypeActor* actor = &ctx->cur_actor_type->as.actor;
                 /* fprintf(stderr, "Debug: Looking for field '%.*s' in actor with %zu fields\n", len, name, actor->field_count); */
                 
                 for (size_t i = 0; i < actor->field_count; i++) {
                     /* fprintf(stderr, "Debug: Field %zu: '%.*s'\n", i, actor->fields[i].name_len, actor->fields[i].name); */
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
                     
                     /* Emit load */
                     /* For now assume i32. */
                     IrType ir_ftype = ir_type_i32(); // TODO: mapping
                     
                     IrInstr* load = ir_build_load(ctx->cur_fn, ctx->cur_block, ir_ftype, fptr->result);
                     return load->result;
                 } else {
                     fprintf(stderr, "Debug: Field '%.*s' NOT FOUND in actor. fields_count=%zu\n", len, name, actor->field_count);
                 }
            } else {
                 /* 
                 if (!ctx->cur_actor_type) fprintf(stderr, "Debug: cur_actor_type is NULL in field expr\n");
                 if (expr->as.field.object->kind != AST_SELF_EXPR) fprintf(stderr, "Debug: object not self in field expr\n");
                 */
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
        case AST_SPAWN_STMT: {
            AstSpawnStmt* spawn = &stmt->as.spawn_stmt;
            /* Use shared logic handles both foo() and Actor.method() */
            if (spawn->expr->kind == AST_CALL_EXPR) {
                gen_spawn_call(ctx, &spawn->expr->as.call);
            } else if (spawn->expr->kind == AST_GROUP_EXPR) {
                /* Unwrap group if needed or just use gen_expr if spawn stmt was an expression wrapper? 
                   AstSpawnStmt contains 'expr' which is the call. 
                   If the parser allows parens around call, we might need unwrapping. 
                   But checking AST_CALL_EXPR directly is fine for now. 
                */
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
            
            /* Entry -> Cond */
            ir_build_jmp(ctx->cur_block, cond_bb);
            
            /* Cond Block */
            ctx->cur_block = cond_bb;
            IrValue cond = gen_expr(ctx, w->condition);
            ir_build_br(ctx->cur_block, cond, body_bb, exit_bb);
            
            /* Body Block */
            ctx->cur_block = body_bb;
            if (w->body) gen_block(ctx, w->body);
            ir_build_jmp(ctx->cur_block, cond_bb);
            
            /* Exit Block */
            ctx->cur_block = exit_bb;
            break;
        }
        case AST_RECEIVE_STMT: {
            AstReceiveStmt* recv = &stmt->as.receive_stmt;
            
            /* Call runtime: %msg = arnm_receive(null) */
            IrValue args[1];
            args[0] = ir_val_const_i32(0); args[0].type = ir_type_ptr(); /* null */
            
            IrInstr* call = ir_build_call(ctx->cur_fn, ctx->cur_block, "arnm_receive", args, 1, ir_type_ptr());
            IrValue msg_val = call->result;
            
            /* For now, just take the first arm and bind it unconditionally */
            if (recv->arm_count > 0) {
                ReceiveArm* arm = &recv->arms[0];
                
                /* Create local scope block? AstBlock usually handles scope?
                   But if pattern binds variable, we need to register it.
                   We don't have block scoping in add_local (flat map). 
                   So just add it. It might shadow. */
                   
                /* Create alloca for local var 'val' */
                IrInstr* alloca = ir_build_alloca(ctx->cur_fn, ctx->cur_block, ir_type_i32());
                
                /* Load tag from message (Offset 0) */
                /* msg_val is ArnmMessage* */
                /* We want *msg_val (as i64 or i32) */
                /* Since tag is 64-bit, we load i64 then trunc? Or just load i64. */
                
                IrInstr* load_tag = ir_build_load(ctx->cur_fn, ctx->cur_block, ir_type_i64(), msg_val);
                
                /* Truncate to i32 (implicit or requires trunc instr?) */
                /* IR builder usually handles types strictly. */
                /* If we define val as i32, we need trunc. */
                /* For now, assume implicit casting or just store execution happens? */
                /* Actually ir_build_store takes value and ptr. */
                /* If types differ... */
                /* hack: store i64 into i32 slot? Stack slot size is usually 8 bytes anyway. */
                /* Better: use i32 in load if we know tag is small. Tag is u64. */
                /* Let's load i32 from offset 0. Little endian matches. */
                
                IrInstr* load_tag_32 = ir_build_load(ctx->cur_fn, ctx->cur_block, ir_type_i32(), msg_val);
                
                ir_build_store(ctx->cur_block, load_tag_32->result, alloca->result);
                
                /* Register pattern name */
                if (arm->pattern && arm->pattern_len > 0) {
                    add_local(ctx, arm->pattern, arm->pattern_len, alloca->result, ir_type_i32());
                }
                
                if (arm->body) {
                    gen_block(ctx, arm->body);
                }
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
         target_name = copy_name(id->name, id->name_len);
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
