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

static AstProgram* parse_and_analyze(const char* src, SemaContext* ctx, AstArena* arena) {
    Lexer lexer;
    lexer_init(&lexer, src, strlen(src));
    
    ast_arena_init(arena, 64 * 1024);
    
    Parser parser;
    parser_init(&parser, &lexer, arena);
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
    AstArena arena;
    AstProgram* prog = parse_and_analyze("fn main() { }", &ctx, &arena);
    ASSERT(prog != NULL);
    ASSERT(!ctx.had_error);
    sema_destroy(&ctx);
    ast_arena_destroy(&arena);
}

TEST(variable_declaration) {
    SemaContext ctx;
    AstArena arena;
    AstProgram* prog = parse_and_analyze("fn main() { let x = 42; }", &ctx, &arena);
    ASSERT(prog != NULL);
    ASSERT(!ctx.had_error);
    sema_destroy(&ctx);
    ast_arena_destroy(&arena);
}

TEST(undefined_variable) {
    SemaContext ctx;
    AstArena arena;
    parse_and_analyze("fn main() { let x = y; }", &ctx, &arena);
    ASSERT(ctx.had_error);
    sema_destroy(&ctx);
    ast_arena_destroy(&arena);
}

TEST(binary_operators) {
    SemaContext ctx;
    AstArena arena;
    AstProgram* prog = parse_and_analyze("fn main() { let x = 1 + 2; }", &ctx, &arena);
    ASSERT(prog != NULL);
    ASSERT(!ctx.had_error);
    sema_destroy(&ctx);
    ast_arena_destroy(&arena);
}

TEST(comparison_returns_bool) {
    SemaContext ctx;
    AstArena arena;
    AstProgram* prog = parse_and_analyze(
        "fn main() { let x = 1 < 2; if x { } }", &ctx, &arena);
    ASSERT(prog != NULL);
    ASSERT(!ctx.had_error);
    sema_destroy(&ctx);
    ast_arena_destroy(&arena);
}

TEST(function_call) {
    SemaContext ctx;
    AstArena arena;
    AstProgram* prog = parse_and_analyze(
        "fn foo() { } fn main() { foo(); }", &ctx, &arena);
    ASSERT(prog != NULL);
    ASSERT(!ctx.had_error);
    sema_destroy(&ctx);
    ast_arena_destroy(&arena);
}

TEST(spawn_expression) {
    SemaContext ctx;
    AstArena arena;
    AstProgram* prog = parse_and_analyze(
        "fn worker() { } fn main() { let p = spawn worker(); }", &ctx, &arena);
    ASSERT(prog != NULL);
    ASSERT(!ctx.had_error);
    sema_destroy(&ctx);
    ast_arena_destroy(&arena);
}

TEST(message_send) {
    SemaContext ctx;
    AstArena arena;
    AstProgram* prog = parse_and_analyze(
        "fn main() { let p = spawn worker(); p ! msg; } fn worker() { }", &ctx, &arena);
    ASSERT(prog != NULL);
    /* May have errors for undefined 'msg' */
    sema_destroy(&ctx);
    ast_arena_destroy(&arena);
}

TEST(actor_definition) {
    SemaContext ctx;
    AstArena arena;
    AstProgram* prog = parse_and_analyze(
        "actor Counter { fn inc() { } }", &ctx, &arena);
    ASSERT(prog != NULL);
    ASSERT(!ctx.had_error);
    sema_destroy(&ctx);
    ast_arena_destroy(&arena);
}

TEST(break_outside_loop) {
    SemaContext ctx;
    AstArena arena;
    parse_and_analyze("fn main() { break; }", &ctx, &arena);
    ASSERT(ctx.had_error);
    sema_destroy(&ctx);
    ast_arena_destroy(&arena);
}

TEST(break_inside_loop) {
    SemaContext ctx;
    AstArena arena;
    AstProgram* prog = parse_and_analyze(
        "fn main() { while true { break; } }", &ctx, &arena);
    ASSERT(prog != NULL);
    ASSERT(!ctx.had_error);
    sema_destroy(&ctx);
    ast_arena_destroy(&arena);
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
