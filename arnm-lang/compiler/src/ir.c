/*
 * ARNm Compiler - IR Implementation
 */

#include "../include/ir.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * Initialization / Destroy
 * ============================================================ */

void ir_module_init(IrModule* mod) {
    mod->funcs = NULL;
}

void ir_module_destroy(IrModule* mod) {
    /* Rudimentary cleanup */
    IrFunction* fn = mod->funcs;
    while (fn) {
        IrFunction* next = fn->next;
        IrBlock* blk = fn->entry;
        while (blk) {
            IrBlock* next_blk = blk->next;
            IrInstr* inst = blk->head;
            while (inst) {
                IrInstr* next_inst = inst->next;
                if (inst->args) free(inst->args);
                free(inst);
                inst = next_inst;
            }
            free(blk);
            blk = next_blk;
        }
        if (fn->param_types) free(fn->param_types);
        free(fn);
        fn = next;
    }
}

/* ============================================================
 * Creation
 * ============================================================ */

IrFunction* ir_function_create(IrModule* mod, const char* name, IrType ret, IrType* params, size_t n_params) {
    IrFunction* fn = malloc(sizeof(IrFunction));
    fn->name = name; /* Assume persistent string or strdup if needed */
    fn->ret_type = ret;
    fn->param_count = n_params;
    fn->param_types = NULL;
    if (n_params > 0) {
        fn->param_types = malloc(sizeof(IrType) * n_params);
        memcpy(fn->param_types, params, sizeof(IrType) * n_params);
    }
    
    fn->entry = NULL;
    fn->blocks_tail = NULL;
    fn->vreg_counter = n_params; /* First n_params IDs are arguments */
    fn->block_counter = 0;
    fn->next = NULL;
    
    /* Append to module */
    if (!mod->funcs) {
        mod->funcs = fn;
    } else {
        IrFunction* last = mod->funcs;
        while (last->next) last = last->next;
        last->next = fn;
    }
    
    return fn;
}

IrBlock* ir_block_create(IrFunction* fn, const char* label) {
    IrBlock* blk = malloc(sizeof(IrBlock));
    blk->id = fn->block_counter++;
    blk->label = label;
    blk->head = NULL;
    blk->tail = NULL;
    blk->next = NULL;
    
    if (!fn->entry) {
        fn->entry = blk;
        fn->blocks_tail = blk;
    } else {
        fn->blocks_tail->next = blk;
        fn->blocks_tail = blk;
    }
    
    return blk;
}

static void block_append(IrBlock* blk, IrInstr* inst) {
    inst->next = NULL;
    inst->prev = blk->tail;
    if (blk->tail) {
        blk->tail->next = inst;
    } else {
        blk->head = inst;
    }
    blk->tail = inst;
}

/* ============================================================
 * Builders
 * ============================================================ */

static IrInstr* new_instr(IrOpcode op) {
    IrInstr* i = malloc(sizeof(IrInstr));
    memset(i, 0, sizeof(IrInstr));
    i->op = op;
    i->result.kind = VAL_UNDEF;
    i->op1.kind = VAL_UNDEF;
    i->op2.kind = VAL_UNDEF;
    return i;
}

IrInstr* ir_build_ret(IrBlock* block, IrValue val) {
    IrInstr* i = new_instr(IR_RET);
    i->op1 = val;
    block_append(block, i);
    return i;
}

IrInstr* ir_build_ret_void(IrBlock* block) {
    IrInstr* i = new_instr(IR_RET);
    i->op1.kind = VAL_UNDEF; /* void return */
    block_append(block, i);
    return i;
}

IrInstr* ir_build_add(IrFunction* fn, IrBlock* block, IrValue lhs, IrValue rhs) {
    IrInstr* i = new_instr(IR_ADD);
    i->type = lhs.type; /* Assume same type */
    i->result = ir_val_var(fn->vreg_counter++, lhs.type);
    i->op1 = lhs;
    i->op2 = rhs;
    block_append(block, i);
    return i;
}

IrInstr* ir_build_alloca(IrFunction* fn, IrBlock* block, IrType type) {
    IrInstr* i = new_instr(IR_ALLOCA);
    /* Result is a pointer to type */
    IrType ptr_type = { IR_PTR };
    i->type = ptr_type; 
    i->result = ir_val_var(fn->vreg_counter++, ptr_type);
    /* Store the allocated type in op1.type for codegen */
    i->op1.type = type;
    i->op1.kind = VAL_UNDEF;
    block_append(block, i);
    return i;
}

IrInstr* ir_build_store(IrBlock* block, IrValue val, IrValue ptr) {
    IrInstr* i = new_instr(IR_STORE);
    i->op1 = val;
    i->op2 = ptr;
    block_append(block, i);
    return i;
}

IrInstr* ir_build_load(IrFunction* fn, IrBlock* block, IrType type, IrValue ptr) {
    IrInstr* i = new_instr(IR_LOAD);
    i->type = type;
    i->result = ir_val_var(fn->vreg_counter++, type);
    i->op1 = ptr;
    block_append(block, i);
    return i;
}

IrInstr* ir_build_field_ptr(IrFunction* fn, IrBlock* block, IrValue ptr, int index) {
    IrInstr* i = new_instr(IR_FIELD_PTR);
    i->type = ir_type_ptr();
    i->result = ir_val_var(fn->vreg_counter++, i->type);
    i->op1 = ptr;
    i->op2 = ir_val_const_i32(index); /* Use op2 for index */
    block_append(block, i);
    return i;
}

IrInstr* ir_build_call(IrFunction* fn, IrBlock* block, const char* callee_name, IrValue* args, size_t arg_count, IrType ret_type) {
    IrInstr* i = new_instr(IR_CALL);
    
    /* op1 is the callee symbol */
    i->op1.kind = VAL_GLOBAL;
    i->op1.storage.global.name = callee_name;
    i->op1.type = ret_type;
    
    i->type = ret_type;
    if (ret_type.kind != IR_VOID) {
        i->result = ir_val_var(fn->vreg_counter++, ret_type);
    }
    
    /* Copy args (shallow copy array) */
    if (arg_count > 0) {
        i->args = malloc(sizeof(IrValue) * arg_count);
        memcpy(i->args, args, sizeof(IrValue) * arg_count);
        i->arg_count = arg_count;
    }
    
    block_append(block, i);
    return i;
}

IrInstr* ir_build_br(IrBlock* block, IrValue cond, IrBlock* then_bb, IrBlock* else_bb) {
    IrInstr* i = new_instr(IR_BR);
    i->op1 = cond;
    i->target1 = then_bb;
    i->target2 = else_bb;
    block_append(block, i);
    return i;
}

IrInstr* ir_build_jmp(IrBlock* block, IrBlock* dest) {
    IrInstr* i = new_instr(IR_JMP);
    i->target1 = dest;
    block_append(block, i);
    return i;
}

IrInstr* ir_build_cmp(IrFunction* fn, IrBlock* block, IrOpcode op, IrValue lhs, IrValue rhs) {
    IrInstr* i = new_instr(op);
    i->type = ir_type_bool(); /* Result is bool (i1) */
    i->result = ir_val_var(fn->vreg_counter++, i->type);
    i->op1 = lhs;
    i->op2 = rhs;
    block_append(block, i);
    return i;
} 

/* ... helpers ... */

IrType ir_type_bool(void) {
    IrType t = { IR_BOOL };
    return t;
}

/* ============================================================
 * Helpers
 * ============================================================ */

IrValue ir_val_var(uint32_t id, IrType type) {
    IrValue v;
    v.kind = VAL_VAR;
    v.type = type;
    v.storage.id = id;
    return v;
}

IrValue ir_val_const_i32(int32_t i) {
    IrValue v;
    v.kind = VAL_CONST;
    v.type = ir_type_i32();
    v.storage.constant.as.i = (uint64_t)i;
    return v;
}

IrType ir_type_i32(void) {
    IrType t = { IR_I32 };
    return t;
}

IrType ir_type_i64(void) {
    IrType t = { IR_I64 };
    return t;
}

IrType ir_type_void(void) {
    IrType t = { IR_VOID };
    return t;
}

IrType ir_type_ptr(void) {
    IrType t = { IR_PTR };
    return t;
}

/* ============================================================
 * Dumping / Debug
 * ============================================================ */

static void dump_type(IrType t) {
    switch(t.kind) {
        case IR_VOID: printf("void"); break;
        case IR_BOOL: printf("bool"); break;
        case IR_I32:  printf("i32"); break;
        case IR_I64:  printf("i64"); break;
        case IR_PTR:  printf("ptr"); break;
        default:      printf("?"); break;
    }
}

static void dump_val(IrValue v) {
    switch(v.kind) {
        case VAL_VAR:   printf("%%%u", v.storage.id); break;
        case VAL_CONST: printf("%lu", v.storage.constant.as.i); break; /* simplify */
        case VAL_UNDEF: printf("undef"); break;
        default:        printf("?"); break;
    }
}

void ir_dump_module(IrModule* mod) {
    IrFunction* fn = mod->funcs;
    while (fn) {
        printf("define ");
        dump_type(fn->ret_type);
        printf(" @%s(", fn->name);
        /* params would go here */
        printf(") {\n");
        
        IrBlock* b = fn->entry;
        while (b) {
            printf("%s:\n", b->label ? b->label : "block");
            IrInstr* i = b->head;
            while (i) {
                printf("  ");
                if (i->result.kind == VAL_VAR) {
                    dump_val(i->result);
                    printf(" = ");
                }
                
                switch(i->op) {
                    case IR_RET:    printf("ret "); break;
                    case IR_ADD:    printf("add "); break;
                    case IR_EQ:     printf("eq "); break;
                    case IR_NE:     printf("ne "); break;
                    case IR_LT:     printf("lt "); break;
                    case IR_LE:     printf("le "); break;
                    case IR_GT:     printf("gt "); break;
                    case IR_GE:     printf("ge "); break;
                    case IR_ALLOCA: printf("alloca "); break;
                    case IR_LOAD:   printf("load "); break;
                    case IR_STORE:  printf("store "); break;
                    case IR_FIELD_PTR: printf("field_ptr "); break;
                    case IR_CALL:   printf("call "); break;
                    case IR_SPAWN:  printf("spawn "); break;
                    case IR_SEND:   printf("send "); break;
                    case IR_BR:     printf("br "); break;
                    case IR_JMP:    printf("jmp "); break;
                    default:        printf("op%d ", i->op); break;
                }
                
                if (i->op == IR_BR) {
                    dump_val(i->op1);
                    printf(", label %s, label %s", 
                        i->target1->label ? i->target1->label : "?",
                        i->target2->label ? i->target2->label : "?");
                } else if (i->op == IR_JMP) {
                    printf("label %s", i->target1->label ? i->target1->label : "?");
                } else if (i->op != IR_ALLOCA) { /* alloca doesn't use op1/2 usually the same way */
                   if (i->op1.kind != VAL_UNDEF) dump_val(i->op1);
                   if (i->op2.kind != VAL_UNDEF) { printf(", "); dump_val(i->op2); }
                }
                
                printf("\n");
                i = i->next;
            }
            b = b->next;
        }
        
        printf("}\n\n");
        fn = fn->next;
    }
}
