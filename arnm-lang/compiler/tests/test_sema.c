/*
 * ARNm Semantic Analysis Tests
 */

#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/sema.h"
#include <stdio.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) static void test_##name(void)
#define RUN_TEST(name) do { \
    printf("  %s...", #name); \
    tests_run++; \
    test_##name(); \
    tests_passed++; \
    printf(" OK\n"); \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf(" FAIL\n    Assertion failed: %s\n", #cond); \
        return; \
    } \
} while(0)

static AstProgram* parse_and_analyze(const char* src, SemaContext* ctx) {
    Lexer lexer;
    lexer_init(&lexer, src, strlen(src));
    
    AstArena arena;
    ast_arena_init(&arena, 64 * 1024);
    
    Parser parser;
    parser_init(&parser, &lexer, &arena);
    AstProgram* prog = parser_parse_program(&parser);
    
    if (!parser_success(&parser)) return NULL;
    
    sema_init(ctx);
    sema_analyze(ctx, prog);
    
    return prog;
}

/* ============================================================
 * Tests
 * ============================================================ */

TEST(simple_function) {
    SemaContext ctx;
    AstProgram* prog = parse_and_analyze("fn main() { }", &ctx);
    ASSERT(prog != NULL);
    ASSERT(!ctx.had_error);
    sema_destroy(&ctx);
}

TEST(variable_declaration) {
    SemaContext ctx;
    AstProgram* prog = parse_and_analyze("fn main() { let x = 42; }", &ctx);
    ASSERT(prog != NULL);
    ASSERT(!ctx.had_error);
    sema_destroy(&ctx);
}

TEST(undefined_variable) {
    SemaContext ctx;
    parse_and_analyze("fn main() { let x = y; }", &ctx);
    ASSERT(ctx.had_error);
    sema_destroy(&ctx);
}

TEST(binary_operators) {
    SemaContext ctx;
    AstProgram* prog = parse_and_analyze("fn main() { let x = 1 + 2; }", &ctx);
    ASSERT(prog != NULL);
    ASSERT(!ctx.had_error);
    sema_destroy(&ctx);
}

TEST(comparison_returns_bool) {
    SemaContext ctx;
    AstProgram* prog = parse_and_analyze(
        "fn main() { let x = 1 < 2; if x { } }", &ctx);
    ASSERT(prog != NULL);
    ASSERT(!ctx.had_error);
    sema_destroy(&ctx);
}

TEST(function_call) {
    SemaContext ctx;
    AstProgram* prog = parse_and_analyze(
        "fn foo() { } fn main() { foo(); }", &ctx);
    ASSERT(prog != NULL);
    ASSERT(!ctx.had_error);
    sema_destroy(&ctx);
}

TEST(spawn_expression) {
    SemaContext ctx;
    AstProgram* prog = parse_and_analyze(
        "fn worker() { } fn main() { let p = spawn worker(); }", &ctx);
    ASSERT(prog != NULL);
    ASSERT(!ctx.had_error);
    sema_destroy(&ctx);
}

TEST(message_send) {
    SemaContext ctx;
    AstProgram* prog = parse_and_analyze(
        "fn main() { let p = spawn worker(); p ! msg; } fn worker() { }", &ctx);
    ASSERT(prog != NULL);
    /* May have errors for undefined 'msg' */
    sema_destroy(&ctx);
}

TEST(actor_definition) {
    SemaContext ctx;
    AstProgram* prog = parse_and_analyze(
        "actor Counter { fn inc() { } }", &ctx);
    ASSERT(prog != NULL);
    ASSERT(!ctx.had_error);
    sema_destroy(&ctx);
}

TEST(break_outside_loop) {
    SemaContext ctx;
    parse_and_analyze("fn main() { break; }", &ctx);
    ASSERT(ctx.had_error);
    sema_destroy(&ctx);
}

TEST(break_inside_loop) {
    SemaContext ctx;
    AstProgram* prog = parse_and_analyze(
        "fn main() { while true { break; } }", &ctx);
    ASSERT(prog != NULL);
    ASSERT(!ctx.had_error);
    sema_destroy(&ctx);
}

/* ============================================================
 * Main
 * ============================================================ */

int main(void) {
    printf("Running sema tests:\n");
    
    RUN_TEST(simple_function);
    RUN_TEST(variable_declaration);
    RUN_TEST(undefined_variable);
    RUN_TEST(binary_operators);
    RUN_TEST(comparison_returns_bool);
    RUN_TEST(function_call);
    RUN_TEST(spawn_expression);
    RUN_TEST(message_send);
    RUN_TEST(actor_definition);
    RUN_TEST(break_outside_loop);
    RUN_TEST(break_inside_loop);
    
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
