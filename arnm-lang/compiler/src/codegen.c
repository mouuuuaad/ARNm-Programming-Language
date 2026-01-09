/*
 * ARNm Compiler - LLVM Code Generation Implementation
 */

#include "../include/codegen.h"
#include <stdlib.h>
#include <string.h>

static void emit_type(FILE* out, IrType type) {
    switch (type.kind) {
        case IR_VOID: fprintf(out, "void"); break;
        case IR_BOOL: fprintf(out, "i1"); break;
        case IR_I8:   fprintf(out, "i8"); break;
        case IR_I32:  fprintf(out, "i32"); break;
        case IR_I64:  fprintf(out, "i64"); break;
        case IR_F64:  fprintf(out, "double"); break;
        case IR_PTR:  fprintf(out, "ptr"); break;
        default:      fprintf(out, "void"); break;
    }
}

static void emit_val(FILE* out, IrValue val) {
    switch (val.kind) {
        case VAL_VAR:   fprintf(out, "%%%u", val.storage.id); break;
        case VAL_CONST: fprintf(out, "%lu", val.storage.constant.as.i); break;
        case VAL_GLOBAL: fprintf(out, "@%s", val.storage.global.name); break;
        case VAL_UNDEF: fprintf(out, "undef"); break;
        default: break;
    }
}

static void emit_instr(FILE* out, IrInstr* inst) {
    switch (inst->op) {
        case IR_RET:
            fprintf(out, "  ret ");
            if (inst->op1.kind != VAL_UNDEF) {
                emit_type(out, inst->op1.type);
                fprintf(out, " ");
                emit_val(out, inst->op1);
            } else {
                fprintf(out, "void");
            }
            fprintf(out, "\n");
            break;
            
        case IR_ALLOCA:
            /* %result = alloca <type> */
            fprintf(out, "  ");
            emit_val(out, inst->result);
            fprintf(out, " = alloca ");
            /* Type is stored in op1.type */
            emit_type(out, inst->op1.type); /* The allocated type */
            fprintf(out, "\n");
            break;
            
        case IR_STORE:
            /* store <ty> <val>, ptr <ptr> */
            fprintf(out, "  store ");
            emit_type(out, inst->op1.type);
            fprintf(out, " ");
            emit_val(out, inst->op1);
            fprintf(out, ", ptr ");
            emit_val(out, inst->op2);
            fprintf(out, "\n");
            break;
            
        case IR_LOAD:
            /* %result = load <ty>, ptr <ptr> */
            fprintf(out, "  ");
            emit_val(out, inst->result);
            fprintf(out, " = load ");
            emit_type(out, inst->type);
            fprintf(out, ", ptr ");
            emit_val(out, inst->op1);
            fprintf(out, "\n");
            break;
            
        case IR_ADD:
            /* %res = add <ty> <op1>, <op2> */
            fprintf(out, "  ");
            emit_val(out, inst->result);
            fprintf(out, " = add ");
            emit_type(out, inst->type);
            fprintf(out, " ");
            emit_val(out, inst->op1);
            fprintf(out, ", ");
            emit_val(out, inst->op2);
            fprintf(out, "\n");
            break;
            
        case IR_EQ: case IR_NE: case IR_LT: case IR_LE: case IR_GT: case IR_GE:
            /* %r = icmp eq i32 %op1, %op2 */
            fprintf(out, "  ");
            if (inst->result.kind != VAL_UNDEF) {
                emit_val(out, inst->result);
                fprintf(out, " = ");
            }
            fprintf(out, "icmp ");
            switch(inst->op) {
                case IR_EQ: fprintf(out, "eq "); break;
                case IR_NE: fprintf(out, "ne "); break;
                case IR_LT: fprintf(out, "slt "); break; /* Signed comparison for now */
                case IR_LE: fprintf(out, "sle "); break;
                case IR_GT: fprintf(out, "sgt "); break;
                case IR_GE: fprintf(out, "sge "); break;
                default: break;
            }
            emit_type(out, inst->op1.type); /* Operand types */
            fprintf(out, " ");
            emit_val(out, inst->op1);
            fprintf(out, ", ");
            emit_val(out, inst->op2);
            fprintf(out, "\n");
            break;

        case IR_CALL:
            /* call <ret_ty> @name(<args>) */
            fprintf(out, "  ");
            if (inst->type.kind != IR_VOID) {
                emit_val(out, inst->result);
                fprintf(out, " = ");
            }
            fprintf(out, "call ");
            emit_type(out, inst->type);
            fprintf(out, " @%s(", inst->op1.storage.global.name);
            for (size_t i = 0; i < inst->arg_count; i++) {
                if (i > 0) fprintf(out, ", ");
                emit_type(out, inst->args[i].type);
                fprintf(out, " ");
                emit_val(out, inst->args[i]);
            }
            fprintf(out, ")\n");
            break;
            
        case IR_BR:
            /* br i1 %cond, label %then, label %else */
            fprintf(out, "  br ");
            emit_type(out, inst->op1.type);
            fprintf(out, " ");
            emit_val(out, inst->op1);
            fprintf(out, ", label ");
            if (inst->target1->label) fprintf(out, "%%%s", inst->target1->label);
            else fprintf(out, "%%b%d", inst->target1->id);
            fprintf(out, ", label ");
            if (inst->target2->label) fprintf(out, "%%%s", inst->target2->label);
            else fprintf(out, "%%b%d", inst->target2->id);
            fprintf(out, "\n");
            break;
            
        case IR_JMP:
            /* br label %dest */
            fprintf(out, "  br label ");
            if (inst->target1->label) fprintf(out, "%%%s", inst->target1->label);
            else fprintf(out, "%%b%d", inst->target1->id);
            fprintf(out, "\n");
            break;
            
        default: break;
    }
}

static void emit_block(FILE* out, IrBlock* block) {
    if (block->label) {
        fprintf(out, "%s:\n", block->label);
    } else {
        fprintf(out, "b%d:\n", block->id);
    }
    
    IrInstr* inst = block->head;
    while (inst) {
        emit_instr(out, inst);
        inst = inst->next;
    }
}

static void emit_function(FILE* out, IrFunction* fn) {
    fprintf(out, "define ");
    emit_type(out, fn->ret_type);
    
    if (strcmp(fn->name, "main") == 0) {
         fprintf(out, " @_arnm_main(");
    } else {
         fprintf(out, " @%s(", fn->name);
    }
    
    for (size_t i = 0; i < fn->param_count; i++) {
        if (i > 0) fprintf(out, ", ");
        if (fn->param_types) {
            emit_type(out, fn->param_types[i]);
        } else {
             fprintf(out, "i32");
        }
        /* Optionally verify implicit %0, %1 matches vreg counter logic */
    }
    
    fprintf(out, ") {\n");
    
    IrBlock* blk = fn->entry;
    while (blk) {
        emit_block(out, blk);
        blk = blk->next;
    }
    
    fprintf(out, "}\n\n");
}

bool codegen_emit(IrModule* mod, FILE* out) {
    if (!mod || !out) return false;
    
    /* Header */
    fprintf(out, "; Generated by ARNm Compiler\n");
    fprintf(out, "target datalayout = \"e-m:e-i64:64-f80:128-n8:16:32:64-S128\"\n");
    fprintf(out, "target triple = \"x86_64-pc-linux-gnu\"\n\n");
    
    /* Runtime declarations */
    fprintf(out, "declare ptr @arnm_spawn(ptr, ptr)\n");
    fprintf(out, "declare void @arnm_send(ptr, i32, ptr, i64)\n");
    fprintf(out, "declare ptr @arnm_receive(ptr)\n");
    fprintf(out, "declare ptr @arnm_self()\n\n");

    IrFunction* fn = mod->funcs;
    while (fn) {
        emit_function(out, fn);
        fn = fn->next;
    }
    
    return true;
}
