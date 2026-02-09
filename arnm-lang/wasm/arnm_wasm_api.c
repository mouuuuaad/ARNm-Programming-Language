/*
 * ARNm WebAssembly API
 * 
 * This file provides the C API that Emscripten will export to JavaScript.
 * It wraps the ARNm compiler and provides a simple interface for the browser.
 * 
 * Features:
 * - Full compilation pipeline: Lexer → Parser → Semantic Analysis → IR Generation
 * - Variable ID-based interpreter for IR execution
 * - Proper memory model with address/value separation
 * - Built-in function support (print, println)
 * - String literal support
 * - Actor operation simulation (spawn, send, receive, self)
 * - Control flow support (if, while, loop, for)
 */

#include "../compiler/include/lexer.h"
#include "../compiler/include/parser.h"
#include "../compiler/include/sema.h"
#include "../compiler/include/ir.h"
#include "../compiler/include/irgen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define WASM_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define WASM_EXPORT
#endif

/* ============================================================
 * Output Buffer Management
 * ============================================================ */

#define OUTPUT_INITIAL_CAPACITY 8192

static char* output_buffer = NULL;
static size_t output_size = 0;
static size_t output_capacity = 0;

/* Clear output buffer */
static void clear_output(void) {
    if (output_buffer) {
        output_buffer[0] = '\0';
        output_size = 0;
    }
}

/* Append to output buffer */
static void append_output(const char* text) {
    if (!text) return;
    
    size_t len = strlen(text);
    if (len == 0) return;
    
    /* Grow buffer if needed */
    while (output_size + len + 1 > output_capacity) {
        size_t new_capacity = output_capacity == 0 ? OUTPUT_INITIAL_CAPACITY : output_capacity * 2;
        char* new_buffer = realloc(output_buffer, new_capacity);
        if (!new_buffer) return; /* Out of memory */
        output_buffer = new_buffer;
        output_capacity = new_capacity;
    }
    
    memcpy(output_buffer + output_size, text, len);
    output_size += len;
    output_buffer[output_size] = '\0';
}

/* Append formatted output */
static void append_output_fmt(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    append_output(buf);
}

/* Initialize output buffer */
static void init_output(void) {
    if (!output_buffer) {
        output_capacity = OUTPUT_INITIAL_CAPACITY;
        output_buffer = malloc(output_capacity);
        if (output_buffer) {
            output_buffer[0] = '\0';
            output_size = 0;
        }
    }
}

/* ============================================================
 * Variable ID-Based Interpreter for IR
 * 
 * Memory model:
 * - vars[0..511]     : IR variable values (from computations)
 * - vars[512..1023]  : Memory slots (for alloca'd variables)
 * ============================================================ */

#define MAX_VARS 2048
#define MEMORY_OFFSET 512

typedef struct {
    int64_t vars[MAX_VARS];    /* Variable ID -> Value mapping */
    char*   strings[256];      /* String table for literals */
    int     string_count;
    int     rand_seed;         /* For actor simulation */
} Interpreter;

static Interpreter interp;

/* Initialize interpreter */
static void interp_init(void) {
    memset(interp.vars, 0, sizeof(interp.vars));
    for (int i = 0; i < interp.string_count; i++) {
        if (interp.strings[i]) {
            free(interp.strings[i]);
            interp.strings[i] = NULL;
        }
    }
    interp.string_count = 0;
    interp.rand_seed = (int)time(NULL);
}

/* Simple random number generator */
static int interp_rand(void) {
    interp.rand_seed = interp.rand_seed * 1103515245 + 12345;
    return (interp.rand_seed >> 16) & 0x7fff;
}

/* Set value for variable ID */
static void interp_set(uint32_t id, int64_t val) {
    if (id < MAX_VARS) {
        interp.vars[id] = val;
    }
}

/* Get value for variable ID */
static int64_t interp_get(uint32_t id) {
    if (id < MAX_VARS) {
        return interp.vars[id];
    }
    return 0;
}

/* Add string to string table, return index */
static int interp_add_string(const char* str, size_t len) {
    if (interp.string_count >= 256) return -1;
    
    char* copy = malloc(len + 1);
    if (!copy) return -1;
    
    memcpy(copy, str, len);
    copy[len] = '\0';
    
    interp.strings[interp.string_count] = copy;
    return interp.string_count++;
}

/* Get string from string table */
static const char* interp_get_string(int index) {
    if (index >= 0 && index < interp.string_count) {
        return interp.strings[index];
    }
    return NULL;
}

/* Get value from IrValue */
static int64_t get_value(IrValue v) {
    switch (v.kind) {
        case VAL_CONST:
            return (int64_t)v.storage.constant.as.i;
        case VAL_VAR:
            return interp_get(v.storage.id);
        case VAL_GLOBAL:
            /* For global symbols (function names), return 0 */
            return 0;
        default:
            return 0;
    }
}

/* Check if IrValue is a string constant */
static bool is_string_value(IrValue v) {
    if (v.kind == VAL_CONST && v.type.kind == IR_PTR) {
        return true;
    }
    return false;
}

/* ============================================================
 * IR Execution Engine
 * ============================================================ */

/* Forward declaration for control flow */
static void execute_block(IrBlock* block, IrBlock** next_block);

/* Execute a single IR instruction */
static void execute_instr(IrInstr* instr, IrBlock** next_block) {
    if (!instr) return;
    
    char buf[1024];
    int64_t val1, val2, result;
    
    /* Get result variable ID if this instruction produces one */
    uint32_t result_id = 0;
    if (instr->result.kind == VAL_VAR) {
        result_id = instr->result.storage.id;
    }
    
    switch (instr->op) {
        /* Arithmetic Operations */
        case IR_ADD:
            val1 = get_value(instr->op1);
            val2 = get_value(instr->op2);
            interp_set(result_id, val1 + val2);
            break;
            
        case IR_SUB:
            val1 = get_value(instr->op1);
            val2 = get_value(instr->op2);
            interp_set(result_id, val1 - val2);
            break;
            
        case IR_MUL:
            val1 = get_value(instr->op1);
            val2 = get_value(instr->op2);
            interp_set(result_id, val1 * val2);
            break;
            
        case IR_DIV:
            val1 = get_value(instr->op1);
            val2 = get_value(instr->op2);
            if (val2 != 0) {
                interp_set(result_id, val1 / val2);
            } else {
                append_output("[Runtime Error] Division by zero\n");
                interp_set(result_id, 0);
            }
            break;
            
        case IR_MOD:
            val1 = get_value(instr->op1);
            val2 = get_value(instr->op2);
            if (val2 != 0) {
                interp_set(result_id, val1 % val2);
            } else {
                append_output("[Runtime Error] Modulo by zero\n");
                interp_set(result_id, 0);
            }
            break;
            
        /* Comparison Operations */
        case IR_EQ:
            val1 = get_value(instr->op1);
            val2 = get_value(instr->op2);
            interp_set(result_id, val1 == val2 ? 1 : 0);
            break;
            
        case IR_NE:
            val1 = get_value(instr->op1);
            val2 = get_value(instr->op2);
            interp_set(result_id, val1 != val2 ? 1 : 0);
            break;
            
        case IR_LT:
            val1 = get_value(instr->op1);
            val2 = get_value(instr->op2);
            interp_set(result_id, val1 < val2 ? 1 : 0);
            break;
            
        case IR_LE:
            val1 = get_value(instr->op1);
            val2 = get_value(instr->op2);
            interp_set(result_id, val1 <= val2 ? 1 : 0);
            break;
            
        case IR_GT:
            val1 = get_value(instr->op1);
            val2 = get_value(instr->op2);
            interp_set(result_id, val1 > val2 ? 1 : 0);
            break;
            
        case IR_GE:
            val1 = get_value(instr->op1);
            val2 = get_value(instr->op2);
            interp_set(result_id, val1 >= val2 ? 1 : 0);
            break;
            
        /* Logical Operations */
        case IR_AND:
            val1 = get_value(instr->op1);
            val2 = get_value(instr->op2);
            interp_set(result_id, (val1 && val2) ? 1 : 0);
            break;
            
        case IR_OR:
            val1 = get_value(instr->op1);
            val2 = get_value(instr->op2);
            interp_set(result_id, (val1 || val2) ? 1 : 0);
            break;
            
        /* Function Call */
        case IR_CALL:
            if (instr->op1.kind == VAL_GLOBAL && instr->op1.storage.global.name) {
                const char* callee = instr->op1.storage.global.name;
                
                /* Handle print function */
                if (strcmp(callee, "print") == 0 || strcmp(callee, "println") == 0) {
                    if (instr->arg_count > 0 && instr->args) {
                        IrValue arg = instr->args[0];
                        
                        /* Check if argument is string (IR_PTR type with constant) */
                        if (arg.type.kind == IR_PTR && arg.kind == VAL_CONST) {
                            /* String pointer is stored in constant.as.i */
                            const char* str = (const char*)(uintptr_t)arg.storage.constant.as.i;
                            if (str && str[0] == '"') {
                                /* Find closing quote to determine string length */
                                const char* end = str + 1;
                                while (*end && *end != '"') {
                                    if (*end == '\\' && *(end + 1)) end++; /* Skip escape sequences */
                                    end++;
                                }
                                int len = (int)(end - str - 1); /* Length without quotes */
                                snprintf(buf, sizeof(buf), "%.*s\n", len, str + 1);
                                append_output(buf);
                            } else if (str) {
                                append_output(str);
                                append_output("\n");
                            } else {
                                append_output("[null string]\n");
                            }
                        } else {
                            /* Integer/boolean value */
                            result = get_value(arg);
                            snprintf(buf, sizeof(buf), "%lld\n", (long long)result);
                            append_output(buf);
                        }
                    }
                }
                /* Handle other runtime functions */
                else if (strcmp(callee, "arnm_panic_nomatch") == 0) {
                    append_output("[Runtime Error] No matching receive pattern\n");
                }
            }
            break;
            
        /* Return Statement */
        case IR_RET:
            if (instr->op1.kind != VAL_UNDEF) {
                result = get_value(instr->op1);
                snprintf(buf, sizeof(buf), "[Return] %lld\n", (long long)result);
                append_output(buf);
            }
            *next_block = NULL; /* Signal end of function */
            return;
            
        /* Memory Operations */
        case IR_ALLOCA:
            /* Return fake address = result_id + MEMORY_OFFSET */
            interp_set(result_id, result_id + MEMORY_OFFSET);
            break;
            
        case IR_STORE:
            val1 = get_value(instr->op1);
            if (instr->op2.kind == VAL_VAR) {
                uint32_t ptr_id = instr->op2.storage.id;
                int64_t addr = interp_get(ptr_id);
                interp_set((uint32_t)addr, val1);
            }
            break;
            
        case IR_LOAD:
            if (instr->op1.kind == VAL_VAR) {
                uint32_t ptr_id = instr->op1.storage.id;
                int64_t addr = interp_get(ptr_id);
                result = interp_get((uint32_t)addr);
                interp_set(result_id, result);
            }
            break;
            
        case IR_FIELD_PTR:
            /* For struct field access - compute address offset */
            if (instr->op1.kind == VAL_VAR) {
                int64_t base = get_value(instr->op1);
                int64_t offset = get_value(instr->op2);
                interp_set(result_id, base + offset);
            }
            break;
            
        /* Control Flow */
        case IR_JMP:
            *next_block = instr->target1;
            return;
            
        case IR_BR:
            val1 = get_value(instr->op1);
            if (val1) {
                *next_block = instr->target1;
            } else {
                *next_block = instr->target2;
            }
            return;
            
        /* Actor Operations - Simulation for Playground */
        case IR_SPAWN:
            {
                int pid = 100 + (interp_rand() % 900);
                snprintf(buf, sizeof(buf), "[Actor] Spawned new process <0.%d.0>\n", pid);
                append_output(buf);
                interp_set(result_id, pid);
            }
            break;
            
        case IR_SEND:
            val1 = get_value(instr->op1); /* target PID */
            val2 = get_value(instr->op2); /* message */
            snprintf(buf, sizeof(buf), "[Actor] Sent message %lld to process <%lld>\n", 
                     (long long)val2, (long long)val1);
            append_output(buf);
            break;
            
        case IR_RECEIVE:
            append_output("[Actor] Waiting for message (simulated)...\n");
            /* Simulate receiving a message with tag 0 */
            interp_set(result_id, 0);
            break;
            
        case IR_SELF:
            /* Return current process ID (simulated as 1) */
            interp_set(result_id, 1);
            break;
            
        case IR_MOV:
            val1 = get_value(instr->op1);
            interp_set(result_id, val1);
            break;
            
        default:
            /* Unknown opcode - ignore */
            break;
    }
}

/* Execute a basic block */
static void execute_block(IrBlock* block, IrBlock** next_block) {
    if (!block) {
        *next_block = NULL;
        return;
    }
    
    IrInstr* instr = block->head;
    while (instr) {
        execute_instr(instr, next_block);
        if (*next_block != block->next) {
            /* Control flow change (jump/branch/return) */
            return;
        }
        instr = instr->next;
    }
    
    /* Fall through to next block */
    *next_block = block->next;
}

/* Execute IR function */
static void execute_ir_function(IrFunction* fn) {
    if (!fn || !fn->entry) return;
    
    IrBlock* current = fn->entry;
    IrBlock* next = NULL;
    int max_iterations = 100000; /* Prevent infinite loops */
    
    while (current && max_iterations-- > 0) {
        next = current->next; /* Default to sequential execution */
        execute_block(current, &next);
        current = next;
    }
    
    if (max_iterations <= 0) {
        append_output("[Runtime Error] Maximum execution iterations exceeded (infinite loop?)\n");
    }
}

/* ============================================================
 * Compilation Pipeline
 * ============================================================ */

/* Report parse errors */
static void report_parse_errors(Parser* parser) {
    for (size_t i = 0; i < parser->error_count; i++) {
        char buf[512];
        snprintf(buf, sizeof(buf), "  Line %u: %s\n", 
                 parser->errors[i].span.line,
                 parser->errors[i].message ? parser->errors[i].message : "parse error");
        append_output(buf);
    }
}

/* Report semantic errors */
static void report_sema_errors(SemaContext* sema) {
    for (size_t i = 0; i < sema->error_count; i++) {
        char buf[512];
        snprintf(buf, sizeof(buf), "  Line %u: %s\n",
                 sema->errors[i].span.line,
                 sema->errors[i].message ? sema->errors[i].message : "type error");
        append_output(buf);
    }
}

/* ============================================================
 * Public WASM API
 * ============================================================ */

/*
 * Compile and execute ARNm source code.
 * 
 * @param source    The ARNm source code to compile
 * @return          0 on success, non-zero error code on failure
 *                  1 = Empty source
 *                  2 = Parse error
 *                  3 = Semantic error
 *                  4 = IR generation error
 */
WASM_EXPORT
int arnm_compile_string(const char* source) {
    init_output();
    clear_output();
    interp_init();
    
    /* Validate input */
    if (!source || !*source) {
        append_output("[Error] Empty source code\n");
        return 1;
    }
    
    size_t source_len = strlen(source);
    
    /* ========================================
     * Stage 1: Lexical Analysis
     * ======================================== */
    Lexer lexer;
    lexer_init(&lexer, source, source_len);
    
    /* ========================================
     * Stage 2: AST Arena Setup
     * ======================================== */
    AstArena arena;
    ast_arena_init(&arena, 2 * 1024 * 1024); /* 2MB arena */
    
    /* ========================================
     * Stage 3: Parsing
     * ======================================== */
    Parser parser;
    parser_init(&parser, &lexer, &arena);
    
    AstProgram* ast = parser_parse_program(&parser);
    
    if (!ast || parser.had_error) {
        append_output("[Parse Error] Failed to parse source code\n");
        if (parser.error_count > 0) {
            char buf[128];
            snprintf(buf, sizeof(buf), "  %zu error(s) found:\n", parser.error_count);
            append_output(buf);
            report_parse_errors(&parser);
        }
        ast_arena_destroy(&arena);
        return 2;
    }
    
    /* ========================================
     * Stage 4: Semantic Analysis
     * ======================================== */
    SemaContext sema;
    sema_init(&sema);
    
    if (!sema_analyze(&sema, ast)) {
        append_output("[Semantic Error] Type checking failed\n");
        if (sema.error_count > 0) {
            char buf[128];
            snprintf(buf, sizeof(buf), "  %zu error(s) found:\n", sema.error_count);
            append_output(buf);
            report_sema_errors(&sema);
        }
        sema_destroy(&sema);
        ast_arena_destroy(&arena);
        return 3;
    }
    
    /* ========================================
     * Stage 5: IR Generation
     * ======================================== */
    IrModule ir_mod;
    ir_module_init(&ir_mod);
    
    if (!ir_generate(&sema, ast, &ir_mod)) {
        append_output("[IR Error] Failed to generate intermediate representation\n");
        ir_module_destroy(&ir_mod);
        sema_destroy(&sema);
        ast_arena_destroy(&arena);
        return 4;
    }
    
    /* ========================================
     * Stage 6: Execution
     * ======================================== */
    append_output("[ARNm] Compilation successful\n");
    append_output("[ARNm] Executing...\n\n");
    
    /* Find and execute main function */
    IrFunction* main_fn = NULL;
    IrFunction* fn = ir_mod.funcs;
    while (fn) {
        if (fn->name && strcmp(fn->name, "main") == 0) {
            main_fn = fn;
            break;
        }
        fn = fn->next;
    }
    
    if (main_fn) {
        execute_ir_function(main_fn);
    } else {
        append_output("[Warning] No main function found\n");
    }
    
    /* ========================================
     * Cleanup
     * ======================================== */
    ir_module_destroy(&ir_mod);
    sema_destroy(&sema);
    ast_arena_destroy(&arena);
    
    return 0;
}

/*
 * Get the output from the last compilation/execution.
 * 
 * @return  Pointer to null-terminated output string
 */
WASM_EXPORT
const char* arnm_get_output(void) {
    return output_buffer ? output_buffer : "";
}

/*
 * Free any allocated resources.
 * Should be called when done using the WASM module.
 */
WASM_EXPORT
void arnm_free_output(void) {
    if (output_buffer) {
        free(output_buffer);
        output_buffer = NULL;
        output_size = 0;
        output_capacity = 0;
    }
    
    /* Free string table */
    for (int i = 0; i < interp.string_count; i++) {
        if (interp.strings[i]) {
            free(interp.strings[i]);
            interp.strings[i] = NULL;
        }
    }
    interp.string_count = 0;
}

/*
 * Get the version of the ARNm WASM runtime.
 * 
 * @return  Version string
 */
WASM_EXPORT
const char* arnm_version(void) {
    return "ARNm WASM Runtime v0.2.0";
}
