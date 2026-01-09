/*
 * ARNm Compiler - Intermediate Representation
 * 
 * SSA-based IR for analysis and code generation.
 */

#ifndef ARNM_IR_H
#define ARNM_IR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "types.h" /* Reuse frontend types where appropriate */

/* Forward declarations */
typedef struct IrFunction IrFunction;
typedef struct IrBlock IrBlock;
typedef struct IrInstr IrInstr;
typedef struct IrModule IrModule;

/* ============================================================
 * IR Types
 * ============================================================ */

typedef enum {
    IR_VOID,
    IR_BOOL,
    IR_I8,
    IR_I32,
    IR_I64,
    IR_F64,
    IR_PTR,
    IR_PROCESS,
    IR_BAD
} IrTypeKind;

typedef struct {
    IrTypeKind kind;
    /* For complex types, we might add more fields later */
} IrType;

/* ============================================================
 * IR Values
 * ============================================================ */

typedef enum {
    VAL_VAR,        /* SSA variable (%1, %2) */
    VAL_CONST,      /* Constant literal (42, true) */
    VAL_GLOBAL,     /* Global symbol (@main) */
    VAL_UNDEF
} IrValueKind;

typedef struct {
    IrValueKind kind;
    IrType      type;
    union {
        uint32_t    id;         /* For VAL_VAR */
        struct {
            union {
                uint64_t i;
                double   f;
                bool     b;
            } as;
        } constant;
        struct {
            const char* name;
        } global;
    } storage;
} IrValue;

/* ============================================================
 * IR Instructions
 * ============================================================ */

typedef enum {
    /* Memory */
    IR_ALLOCA,      /* %r = alloca T */
    IR_LOAD,        /* %r = load T, %ptr */
    IR_STORE,       /* store T %val, %ptr */
    IR_FIELD_PTR,   /* %r = field_ptr %ptr, field_index (offset calculated in backend or here?) */
    
    /* Arithmetic */
    IR_ADD, IR_SUB, IR_MUL, IR_DIV, IR_MOD,
    
    /* Comparison */
    IR_EQ, IR_NE, IR_LT, IR_LE, IR_GT, IR_GE,
    
    /* Logical */
    IR_AND, IR_OR,
    
    /* Control Flow */
    IR_RET,         /* ret %val */
    IR_BR,          /* br %cond, %then, %else */
    IR_JMP,         /* jmp %dest */
    IR_CALL,        /* %r = call @fn(%args...) */
    
    /* ActorOps */
    IR_SPAWN,       /* %r = spawn @fn(%args...) */
    IR_SEND,        /* send %target, %msg */
    IR_RECEIVE,     /* %r = receive */
    IR_SELF,        /* %r = self */
    
    /* Misc */
    IR_MOV          /* %r = %v (virtual move, mainly for phi resolution) */
} IrOpcode;

struct IrInstr {
    IrOpcode    op;
    IrType      type;       /* Result type */
    IrValue     result;     /* Result variable (if applicable) */
    
    /* Operands */
    IrValue     op1;
    IrValue     op2;
    IrValue*    args;       /* For call/spawn */
    size_t      arg_count;
    
    /* Branch targets */
    IrBlock*    target1;    /* For JMP/BR */
    IrBlock*    target2;    /* For BR */
    
    IrInstr*    next;
    IrInstr*    prev;
};

/* ============================================================
 * Basic Blocks
 * ============================================================ */

struct IrBlock {
    int         id;
    const char* label;      /* Optional debug label */
    IrInstr*    head;
    IrInstr*    tail;
    IrBlock*    next;       /* Next block in function layout */
};

/* ============================================================
 * Functions & Modules
 * ============================================================ */

struct IrFunction {
    const char* name;
    IrType      ret_type;
    IrType*     param_types;
    size_t      param_count;
    
    IrBlock*    entry; 
    IrBlock*    blocks_tail;
    
    uint32_t    vreg_counter; /* Counter for virtual registers */
    uint32_t    block_counter;
    
    IrFunction* next;
};

struct IrModule {
    IrFunction* funcs;
    /* Globals, strings, etc. go here */
};

/* ============================================================
 * API
 * ============================================================ */

void ir_module_init(IrModule* mod);
void ir_module_destroy(IrModule* mod);

IrFunction* ir_function_create(IrModule* mod, const char* name, IrType ret, IrType* params, size_t n_params);
IrBlock*    ir_block_create(IrFunction* fn, const char* label);

/* Instruction builders */
IrInstr* ir_build_ret(IrBlock* block, IrValue val);
IrInstr* ir_build_ret_void(IrBlock* block); 
IrInstr* ir_build_add(IrFunction* fn, IrBlock* block, IrValue lhs, IrValue rhs);
IrInstr* ir_build_sub(IrFunction* fn, IrBlock* block, IrValue lhs, IrValue rhs);
IrInstr* ir_build_mul(IrFunction* fn, IrBlock* block, IrValue lhs, IrValue rhs);
IrInstr* ir_build_div(IrFunction* fn, IrBlock* block, IrValue lhs, IrValue rhs);
IrInstr* ir_build_mod(IrFunction* fn, IrBlock* block, IrValue lhs, IrValue rhs);
IrInstr* ir_build_and(IrFunction* fn, IrBlock* block, IrValue lhs, IrValue rhs);
IrInstr* ir_build_or(IrFunction* fn, IrBlock* block, IrValue lhs, IrValue rhs);
IrInstr* ir_build_alloca(IrFunction* fn, IrBlock* block, IrType type);
IrInstr* ir_build_store(IrBlock* block, IrValue val, IrValue ptr);
IrInstr* ir_build_load(IrFunction* fn, IrBlock* block, IrType type, IrValue ptr);
IrInstr* ir_build_field_ptr(IrFunction* fn, IrBlock* block, IrValue ptr, int index);
IrInstr* ir_build_call(IrFunction* fn, IrBlock* block, const char* callee_name, IrValue* args, size_t arg_count, IrType ret_type);
IrInstr* ir_build_br(IrBlock* block, IrValue cond, IrBlock* then_bb, IrBlock* else_bb);
IrInstr* ir_build_jmp(IrBlock* block, IrBlock* dest);
IrInstr* ir_build_cmp(IrFunction* fn, IrBlock* block, IrOpcode op, IrValue lhs, IrValue rhs);
IrInstr* ir_build_constants_i32(int32_t val);
IrInstr* ir_build_cmp(IrFunction* fn, IrBlock* block, IrOpcode op, IrValue lhs, IrValue rhs);

/* Helpers */
IrValue ir_val_var(uint32_t id, IrType type);
IrValue ir_val_const_i32(int32_t i);
IrValue ir_val_const_bool(bool b);
IrType  ir_type_i32(void);
IrType  ir_type_i64(void);
IrType  ir_type_bool(void);
IrType  ir_type_void(void);
IrType  ir_type_ptr(void); /* New helper */

void ir_dump_module(IrModule* mod);

#endif /* ARNM_IR_H */
