/*
 * ARNm Compiler - Main Driver
 * 
 * Usage: arnmc [options] <source.arnm>
 * 
 * Options:
 *   --dump-tokens   Print token stream
 *   --dump-ast      Print AST structure
 *   --check         Run semantic analysis only
 *   --help          Show help
 */

#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/sema.h"
#include "../include/irgen.h"
#include "../include/codegen.h"
#include "../include/codegen_x86.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * File Reading
 * ============================================================ */

static char* read_file(const char* path, size_t* out_len) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "error: could not open file '%s'\n", path);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    size_t len = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* buffer = (char*)malloc(len + 1);
    if (!buffer) {
        fprintf(stderr, "error: out of memory\n");
        fclose(file);
        return NULL;
    }
    
    size_t read = fread(buffer, 1, len, file);
    buffer[read] = '\0';
    fclose(file);
    
    *out_len = read;
    return buffer;
}

/* ============================================================
 * AST Printing
 * ============================================================ */

static void print_indent(int depth) {
    for (int i = 0; i < depth; i++) printf("  ");
}

static void print_expr(AstExpr* expr, int depth);
static void print_stmt(AstStmt* stmt, int depth);

static void print_expr(AstExpr* expr, int depth) {
    if (!expr) {
        print_indent(depth);
        printf("(null)\n");
        return;
    }
    
    print_indent(depth);
    switch (expr->kind) {
        case AST_IDENT_EXPR:
            printf("Ident: %.*s\n", expr->as.ident.name_len, expr->as.ident.name);
            break;
        case AST_INT_LIT_EXPR:
            printf("Int: %lld\n", (long long)expr->as.int_lit.value);
            break;
        case AST_FLOAT_LIT_EXPR:
            printf("Float: %f\n", expr->as.float_lit.value);
            break;
        case AST_STRING_LIT_EXPR:
            printf("String: %.*s\n", expr->as.string_lit.value_len, expr->as.string_lit.value);
            break;
        case AST_BOOL_LIT_EXPR:
            printf("Bool: %s\n", expr->as.bool_lit.value ? "true" : "false");
            break;
        case AST_NIL_EXPR:
            printf("Nil\n");
            break;
        case AST_SELF_EXPR:
            printf("Self\n");
            break;
        case AST_BINARY_EXPR:
            printf("Binary op: %d\n", expr->as.binary.op);
            print_expr(expr->as.binary.left, depth + 1);
            print_expr(expr->as.binary.right, depth + 1);
            break;
        case AST_UNARY_EXPR:
            printf("Unary op: %d\n", expr->as.unary.op);
            print_expr(expr->as.unary.operand, depth + 1);
            break;
        case AST_CALL_EXPR:
            printf("Call:\n");
            print_expr(expr->as.call.callee, depth + 1);
            for (size_t i = 0; i < expr->as.call.arg_count; i++) {
                print_expr(expr->as.call.args[i], depth + 1);
            }
            break;
        case AST_SEND_EXPR:
            printf("Send:\n");
            print_indent(depth + 1); printf("target:\n");
            print_expr(expr->as.send.target, depth + 2);
            print_indent(depth + 1); printf("message:\n");
            print_expr(expr->as.send.message, depth + 2);
            break;
        case AST_FIELD_EXPR:
            printf("Field: %.*s\n", expr->as.field.field_name_len, expr->as.field.field_name);
            print_expr(expr->as.field.object, depth + 1);
            break;
        case AST_GROUP_EXPR:
            printf("Group:\n");
            print_expr(expr->as.group.inner, depth + 1);
            break;
        default:
            printf("Unknown expr kind: %d\n", expr->kind);
    }
}

static void print_block(AstBlock* block, int depth) {
    if (!block) return;
    for (size_t i = 0; i < block->stmt_count; i++) {
        print_stmt(block->stmts[i], depth);
    }
}

static void print_stmt(AstStmt* stmt, int depth) {
    if (!stmt) return;
    
    print_indent(depth);
    switch (stmt->kind) {
        case AST_LET_STMT:
            printf("Let: %.*s%s\n", 
                stmt->as.let_stmt.name_len, 
                stmt->as.let_stmt.name,
                stmt->as.let_stmt.is_mut ? " (mut)" : "");
            if (stmt->as.let_stmt.init) {
                print_expr(stmt->as.let_stmt.init, depth + 1);
            }
            break;
        case AST_RETURN_STMT:
            printf("Return:\n");
            if (stmt->as.return_stmt.value) {
                print_expr(stmt->as.return_stmt.value, depth + 1);
            }
            break;
        case AST_IF_STMT:
            printf("If:\n");
            print_indent(depth + 1); printf("condition:\n");
            print_expr(stmt->as.if_stmt.condition, depth + 2);
            print_indent(depth + 1); printf("then:\n");
            print_block(stmt->as.if_stmt.then_block, depth + 2);
            if (stmt->as.if_stmt.else_branch) {
                print_indent(depth + 1); printf("else:\n");
                print_stmt(stmt->as.if_stmt.else_branch, depth + 2);
            }
            break;
        case AST_WHILE_STMT:
            printf("While:\n");
            print_expr(stmt->as.while_stmt.condition, depth + 1);
            print_block(stmt->as.while_stmt.body, depth + 1);
            break;
        case AST_SPAWN_STMT:
            printf("Spawn:\n");
            print_expr(stmt->as.spawn_stmt.expr, depth + 1);
            break;
        case AST_RECEIVE_STMT:
            printf("Receive: %zu arms\n", stmt->as.receive_stmt.arm_count);
            break;
        case AST_EXPR_STMT:
            printf("ExprStmt:\n");
            print_expr(stmt->as.expr_stmt.expr, depth + 1);
            break;
        case AST_BREAK_STMT:
            printf("Break\n");
            break;
        case AST_CONTINUE_STMT:
            printf("Continue\n");
            break;
        case AST_BLOCK:
            printf("Block:\n");
            print_block(&stmt->as.block, depth + 1);
            break;
        default:
            printf("Unknown stmt kind: %d\n", stmt->kind);
    }
}

static void print_fn(AstFnDecl* fn, int depth) {
    print_indent(depth);
    printf("Function: %.*s (params: %zu)\n", fn->name_len, fn->name, fn->param_count);
    if (fn->body) {
        print_block(fn->body, depth + 1);
    }
}

static void print_actor(AstActorDecl* actor, int depth) {
    print_indent(depth);
    printf("Actor: %.*s (methods: %zu)\n", actor->name_len, actor->name, actor->method_count);
    for (size_t i = 0; i < actor->method_count; i++) {
        print_fn(actor->methods[i], depth + 1);
    }
}

static void print_ast(AstProgram* program) {
    printf("=== AST ===\n");
    printf("Program: %zu declarations\n", program->decl_count);
    for (size_t i = 0; i < program->decl_count; i++) {
        AstDecl* decl = program->decls[i];
        switch (decl->kind) {
            case AST_FN_DECL:
                print_fn(&decl->as.fn_decl, 1);
                break;
            case AST_ACTOR_DECL:
                print_actor(&decl->as.actor_decl, 1);
                break;
            default:
                printf("  Unknown decl kind: %d\n", decl->kind);
        }
    }
}

/* ============================================================
 * Token Printing
 * ============================================================ */

static void print_tokens(const char* source, size_t len) {
    Lexer lexer;
    lexer_init(&lexer, source, len);
    
    printf("=== Tokens ===\n");
    Token tok;
    do {
        tok = lexer_next_token(&lexer);
        printf("%3u:%2u  %-15s  '%.*s'\n",
            tok.span.line, tok.span.column,
            token_kind_name(tok.kind),
            tok.length, tok.lexeme);
    } while (tok.kind != TOK_EOF && tok.kind != TOK_ERROR);
}

/* ============================================================
 * Main
 * ============================================================ */

static void print_usage(const char* program) {
    printf("Usage: %s [options] <source.arnm>\n", program);
    printf("\nOptions:\n");
    printf("  --dump-tokens   Print token stream\n");
    printf("  --dump-ast      Print AST structure\n");
    printf("  --check         Run semantic analysis only\n");
    printf("  --emit-ir       Emit SSA Intermediate Representation\n");
    printf("  --emit-llvm     Emit LLVM IR (.ll)\n");
    printf("  --emit-asm      Emit x86_64 Assembly (.s)\n");
    printf("  --help          Show this help\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char* source_file = NULL;
    bool dump_tokens = false;
    bool dump_ast = false;
    bool check_only = false;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--dump-tokens") == 0) {
            dump_tokens = true;
        } else if (strcmp(argv[i], "--dump-ast") == 0) {
            dump_ast = true;
        } else if (strcmp(argv[i], "--check") == 0) {
            check_only = true;
        } else if (strcmp(argv[i], "--emit-ir") == 0) {
             /* Handled later */
        } else if (strcmp(argv[i], "--emit-llvm") == 0) {
             /* Handled later */
        } else if (strcmp(argv[i], "--emit-asm") == 0) {
             /* Handled later */
        } else if (argv[i][0] != '-') {
            source_file = argv[i];
        } else {
            fprintf(stderr, "error: unknown option '%s'\n", argv[i]);
            return 1;
        }
    }
    
    if (!source_file) {
        fprintf(stderr, "error: no source file specified\n");
        return 1;
    }
    
    /* Read source file */
    size_t source_len;
    char* source = read_file(source_file, &source_len);
    if (!source) return 1;
    
    fprintf(stderr, "ARNm Compiler v0.2.0\n");
    fprintf(stderr, "Compiling: %s (%zu bytes)\n\n", source_file, source_len);
    
    /* Dump tokens if requested */
    if (dump_tokens) {
        print_tokens(source, source_len);
        printf("\n");
    }
    
    /* Parse */
    Lexer lexer;
    lexer_init(&lexer, source, source_len);
    
    AstArena arena;
    ast_arena_init(&arena, 1024 * 1024); /* 1MB arena */
    
    Parser parser;
    parser_init(&parser, &lexer, &arena);
    
    AstProgram* program = parser_parse_program(&parser);
    
    if (!parser_success(&parser)) {
        fprintf(stderr, "\nParse errors:\n");
        for (size_t i = 0; i < parser.error_count; i++) {
            fprintf(stderr, "  %u:%u: %s\n",
                parser.errors[i].span.line,
                parser.errors[i].span.column,
                parser.errors[i].message);
        }
        ast_arena_destroy(&arena);
        free(source);
        return 1;
    }
    
    fprintf(stderr, "Parse successful: %zu declarations\n", program->decl_count);
    
    /* Dump AST if requested */
    if (dump_ast) {
        printf("\n");
        print_ast(program);
    }
    
    /* Semantic analysis */
    SemaContext sema;
    sema_init(&sema);
    bool sema_ok = sema_analyze(&sema, program);
    
    if (!sema_ok) {
        fprintf(stderr, "\nSemantic errors:\n");
        for (size_t i = 0; i < sema.error_count; i++) {
            fprintf(stderr, "  %u:%u: %s\n",
                sema.errors[i].span.line,
                sema.errors[i].span.column,
                sema.errors[i].message);
        }
        sema_destroy(&sema);
        ast_arena_destroy(&arena);
        free(source);
        return 1;
    }
    
    fprintf(stderr, "Semantic analysis: OK (%zu symbols)\n", sema.symbols.symbol_count);
    
    if (check_only) {
        fprintf(stderr, "\nCheck complete. No errors.\n");
        sema_destroy(&sema);
        ast_arena_destroy(&arena);
        free(source);
        return 0;
    }

    /* Code Generation */
    bool emit_ir = false;
    bool emit_llvm = false;
    bool emit_asm = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--emit-ir") == 0) emit_ir = true;
        if (strcmp(argv[i], "--emit-llvm") == 0) emit_llvm = true;
        if (strcmp(argv[i], "--emit-asm") == 0) emit_asm = true;
    }

    IrModule ir_mod;
    if (!ir_generate(&sema, program, &ir_mod)) {
        fprintf(stderr, "Error: IR generation failed\n");
        sema_destroy(&sema);
        ast_arena_destroy(&arena);
        free(source);
        return 1;
    }

    if (emit_ir) {
        fprintf(stderr, "\n--- ARNm IR ---\n");
        ir_dump_module(&ir_mod); /* This usually dumps to stdout? Check later */
    }
    
    if (emit_llvm) {
        if (emit_ir) fprintf(stderr, "\n");
        fprintf(stderr, "--- LLVM IR ---\n");
        codegen_emit(&ir_mod, stdout);
    }

    if (emit_asm) {
        if (emit_ir || emit_llvm) fprintf(stderr, "\n");
        fprintf(stderr, "--- x86_64 Assembly ---\n");
        x86_emit(&ir_mod, stdout);
    }
    
    ir_module_destroy(&ir_mod);
    
    /* Cleanup */
    sema_destroy(&sema);
    ast_arena_destroy(&arena);
    free(source);
    
    return 0;
}

