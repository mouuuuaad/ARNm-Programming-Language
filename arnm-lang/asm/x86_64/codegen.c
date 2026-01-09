/*
 * ARNm Compiler - Native x86_64 Code Generator
 *
 * Implements a simple "spill-everywhere" backend.
 * Every IrValue is mapped to a stack slot [rbp - offset].
 */

#include "codegen_x86.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * Internal State
 * ============================================================ */

typedef struct {
    IrModule* mod;
    FILE* out;
    IrFunction* cur_fn;
} X86Context;

/* ============================================================
 * Helpers
 * ============================================================ */

static void get_operand(IrValue val, char* buffer) {
    if (val.kind == VAL_VAR) {
        int offset = (val.storage.id + 1) * 8;
        sprintf(buffer, "-%d(%%rbp)", offset);
    } else if (val.kind == VAL_CONST) {
        sprintf(buffer, "$%ld", (long)val.storage.constant.as.i);
    } else if (val.kind == VAL_GLOBAL) {
        sprintf(buffer, "$%s", val.storage.global.name);
    } else {
        sprintf(buffer, "$0");
    }
}

static void get_label(char* buffer, const char* fn_name, int block_id) {
    /* Mangle label with function name for uniqueness */
    /* Remove _ if present? No, just prefix. */
    /* Use local label .L */
    sprintf(buffer, ".L%s_BB_%d", fn_name, block_id);
}

/* ============================================================
 * Emitters
 * ============================================================ */

static void emit_prologue(X86Context* ctx, IrFunction* fn) {
    const char* name = strcmp(fn->name, "main") == 0 ? "_arnm_main" : fn->name;
    
    fprintf(ctx->out, "\t.globl %s\n", name);
    fprintf(ctx->out, "\t.type %s, @function\n", name);
    fprintf(ctx->out, "%s:\n", name);
    fprintf(ctx->out, "\tpushq %%rbp\n");
    fprintf(ctx->out, "\tmovq %%rsp, %%rbp\n");
    
    int stack_size = (fn->vreg_counter + 32) * 8; 
    if (stack_size % 16 != 0) stack_size += 8;
    
    fprintf(ctx->out, "\t# Stack size: %d, Param count: %zu\n", stack_size, fn->param_count);
    fprintf(ctx->out, "\tsubq $%d, %%rsp\n", stack_size); 
    
    /* Move arguments from registers to stack slots (VAR_0, VAR_1...) */
    const char* arg_regs[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
    for (size_t i = 0; i < fn->param_count && i < 6; i++) {
        /* VAR_i maps to -(i+1)*8(%rbp) */
        int offset = (i + 1) * 8;
        fprintf(ctx->out, "\tmovq %s, -%d(%%rbp)\n", arg_regs[i], offset);
    } 
}

static void emit_epilogue(X86Context* ctx) {
    fprintf(ctx->out, "\tmovq %%rbp, %%rsp\n");
    fprintf(ctx->out, "\tpopq %%rbp\n");
    fprintf(ctx->out, "\tret\n");
}

static void emit_instr(X86Context* ctx, IrInstr* instr) {
    char op1[64], op2[64], dest[64];
    
    if (instr->op != IR_CALL && instr->op != IR_SPAWN) {
        get_operand(instr->op1, op1);
        get_operand(instr->op2, op2);
        get_operand(instr->result, dest);
    }

    switch (instr->op) {
        case IR_MOV:
            fprintf(ctx->out, "\tmovq %s, %%rax\n", op1);
            fprintf(ctx->out, "\tmovq %%rax, %s\n", dest);
            break;
            
        case IR_ALLOCA:
            fprintf(ctx->out, "\tsubq $16, %%rsp\n"); 
            fprintf(ctx->out, "\tmovq %%rsp, %%rax\n");
            fprintf(ctx->out, "\tmovq %%rax, %s\n", dest);
            break;
            
        case IR_LOAD:
            fprintf(ctx->out, "\tmovq %s, %%rax\n", op1);
            fprintf(ctx->out, "\tmovq (%%rax), %%rbx\n"); 
            fprintf(ctx->out, "\tmovq %%rbx, %s\n", dest);
            break;
            
        case IR_STORE:
            fprintf(ctx->out, "\tmovq %s, %%rax\n", op1);
            fprintf(ctx->out, "\tmovq %s, %%rbx\n", op2);
            fprintf(ctx->out, "\tmovq %%rax, (%%rbx)\n");
            break;

        case IR_ADD:
            fprintf(ctx->out, "\tmovq %s, %%rax\n", op1);
            fprintf(ctx->out, "\taddq %s, %%rax\n", op2); 
            fprintf(ctx->out, "\tmovq %%rax, %s\n", dest);
            break;
            
        case IR_SUB:
            fprintf(ctx->out, "\tmovq %s, %%rax\n", op1);
            fprintf(ctx->out, "\tsubq %s, %%rax\n", op2);
            fprintf(ctx->out, "\tmovq %%rax, %s\n", dest);
            break;
            
        case IR_MUL:
            fprintf(ctx->out, "\tmovq %s, %%rax\n", op1);
            fprintf(ctx->out, "\timulq %s, %%rax\n", op2);
            fprintf(ctx->out, "\tmovq %%rax, %s\n", dest);
            break;
            
        case IR_DIV:
            fprintf(ctx->out, "\tmovq %s, %%rax\n", op1);
            fprintf(ctx->out, "\tcqo\n"); 
            fprintf(ctx->out, "\tmovq %s, %%rbx\n", op2);
            fprintf(ctx->out, "\tidivq %%rbx\n");
            fprintf(ctx->out, "\tmovq %%rax, %s\n", dest);
            break;

        case IR_MOD:
            fprintf(ctx->out, "\tmovq %s, %%rax\n", op1);
            fprintf(ctx->out, "\tcqo\n"); 
            fprintf(ctx->out, "\tmovq %s, %%rbx\n", op2);
            fprintf(ctx->out, "\tidivq %%rbx\n");
            fprintf(ctx->out, "\tmovq %%rdx, %s\n", dest); /* Remainder in rdx */
            break;
            
        case IR_AND:
            fprintf(ctx->out, "\tmovq %s, %%rax\n", op1);
            fprintf(ctx->out, "\tandq %s, %%rax\n", op2);
            fprintf(ctx->out, "\tmovq %%rax, %s\n", dest);
            break;
            
        case IR_OR:
            fprintf(ctx->out, "\tmovq %s, %%rax\n", op1);
            fprintf(ctx->out, "\torq %s, %%rax\n", op2);
            fprintf(ctx->out, "\tmovq %%rax, %s\n", dest);
            break;
            
        case IR_EQ: case IR_NE: case IR_LT: case IR_GT: case IR_LE: case IR_GE:
            fprintf(ctx->out, "\tmovq %s, %%rax\n", op1);
            fprintf(ctx->out, "\tcmpq %s, %%rax\n", op2);
            switch (instr->op) {
                case IR_EQ: fprintf(ctx->out, "\tsete %%al\n"); break;
                case IR_NE: fprintf(ctx->out, "\tsetne %%al\n"); break;
                case IR_LT: fprintf(ctx->out, "\tsetl %%al\n"); break;
                case IR_LE: fprintf(ctx->out, "\tsetle %%al\n"); break;
                case IR_GT: fprintf(ctx->out, "\tsetg %%al\n"); break;
                case IR_GE: fprintf(ctx->out, "\tsetge %%al\n"); break;
                default: break;
            }
            fprintf(ctx->out, "\tmovzbl %%al, %%eax\n");
            fprintf(ctx->out, "\tmovq %%rax, %s\n", dest);
            break;
            
        case IR_JMP:
            {
                if (!instr->target1) {
                    fprintf(ctx->out, "\t# ERROR: JMP with null target\n");
                    break;
                }
                char label[64];
                get_label(label, ctx->cur_fn->name, instr->target1->id);
                fprintf(ctx->out, "\tjmp %s\n", label);
            }
            break;
            
        case IR_BR:
            {
                if (!instr->target1 || !instr->target2) {
                    fprintf(ctx->out, "\t# ERROR: BR with null target\n");
                    break;
                }
                fprintf(ctx->out, "\tmovq %s, %%rax\n", op1);
                fprintf(ctx->out, "\tcmpq $0, %%rax\n");
                
                char l_else[64], l_then[64];
                get_label(l_then, ctx->cur_fn->name, instr->target1->id);
                get_label(l_else, ctx->cur_fn->name, instr->target2->id);
                
                fprintf(ctx->out, "\tje %s\n", l_else); 
                fprintf(ctx->out, "\tjmp %s\n", l_then); 
            }
            break;
            
        case IR_RET:
            if (instr->op1.kind != VAL_UNDEF) {
                fprintf(ctx->out, "\tmovq %s, %%rax\n", op1);
            }
            emit_epilogue(ctx);
            break;
            
        case IR_CALL:
        case IR_SPAWN:
            {
                const char* regs[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
                for (size_t i = 0; i < instr->arg_count && i < 6; i++) {
                    char arg_op[64];
                    get_operand(instr->args[i], arg_op);
                    fprintf(ctx->out, "\tmovq %s, %s\n", arg_op, regs[i]);
                }
                
                if (instr->op1.kind == VAL_GLOBAL) {
                     const char* name = instr->op1.storage.global.name;
                     if (strcmp(name, "print") == 0) {
                         fprintf(ctx->out, "\tcall arnm_print_int\n");
                     } else if (strcmp(name, "arnm_panic_nomatch") == 0) {
                         fprintf(ctx->out, "\tcall arnm_panic_nomatch\n");
                     } else {
                         fprintf(ctx->out, "\tcall %s\n", name);
                     }
                } else if (instr->op == IR_SPAWN) {
                     fprintf(ctx->out, "\tcall arnm_spawn\n");
                } else {
                     get_operand(instr->op1, op1);
                     fprintf(ctx->out, "\tcall *%s\n", op1);
                }
                
                if (instr->result.kind != VAL_UNDEF) {
                    get_operand(instr->result, dest);
                    fprintf(ctx->out, "\tmovq %%rax, %s\n", dest);
                }
            }
            break;
            
        case IR_SEND:
            /* send target, tag, data, size */
             fprintf(ctx->out, "\t# Send not fully impl in simplified backend\n");
             break;

        case IR_RECEIVE:
             fprintf(ctx->out, "\tmovq $0, %%rdi\n");
             fprintf(ctx->out, "\tcall arnm_receive\n");
             get_operand(instr->result, dest);
             fprintf(ctx->out, "\tmovq %%rax, %s\n", dest);
             break;

        case IR_FIELD_PTR:
             {
                 /* op1 = base ptr, op2 = index (const) */
                 get_operand(instr->op1, op1); 
                 get_operand(instr->result, dest);
                 int idx = (int)instr->op2.storage.constant.as.i;
                 int offset = idx * 8; /* Assuming 8-byte fields for now */
                 
                 fprintf(ctx->out, "\tmovq %s, %%rax\n", op1);
                 if (offset > 0) {
                     fprintf(ctx->out, "\taddq $%d, %%rax\n", offset);
                 }
                 fprintf(ctx->out, "\tmovq %%rax, %s\n", dest);
             }
             break;

        default:
            fprintf(ctx->out, "\t# Unimplemented instr %d\n", instr->op);
            break;
    }
}

static void emit_block(X86Context* ctx, IrBlock* block) {
    char label[64];
    get_label(label, ctx->cur_fn->name, block->id);
    fprintf(ctx->out, "%s:\n", label);
    
    IrInstr* instr = block->head;
    while (instr) {
        emit_instr(ctx, instr);
        instr = instr->next;
    }
}

static void emit_function(X86Context* ctx, IrFunction* fn) {
    ctx->cur_fn = fn;
    
    emit_prologue(ctx, fn);
    
    IrBlock* block = fn->entry;
    while (block) {
        emit_block(ctx, block);
        block = block->next;
    }
    
    /* Prologue handles ret emission if control flow falls through?
       Usually IR_RET is last instruction.
    */
    /* emit_epilogue(ctx); -- handled by IR_RET */
}

/* ============================================================
 * Public API
 * ============================================================ */

void x86_emit(IrModule* mod, FILE* out) {
    X86Context ctx = { .mod = mod, .out = out };
    
    fprintf(out, "\t.text\n");
    
    IrFunction* fn = mod->funcs;
    while (fn) {
        emit_function(&ctx, fn);
        fn = fn->next;
    }
    
    fprintf(out, "\t.section .note.GNU-stack,\"\",@progbits\n");
}

