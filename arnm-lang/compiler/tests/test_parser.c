/*
 * ARNm Parser Tests
 */

#include "../include/lexer.h"
#include "../include/parser.h"
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

#define ASSERT_EQ(a, b) ASSERT((a) == (b))

static AstProgram* parse_source(const char* src, AstArena* arena, Parser* parser) {
    Lexer lexer;
    lexer_init(&lexer, src, strlen(src));
    ast_arena_init(arena, 64 * 1024);
    parser_init(parser, &lexer, arena);
    return parser_parse_program(parser);
}

/* ============================================================
 * Tests
 * ============================================================ */

TEST(empty_program) {
    AstArena arena;
    Parser parser;
    AstProgram* prog = parse_source("", &arena, &parser);
    
    ASSERT(parser_success(&parser));
    ASSERT(prog != NULL);
    ASSERT_EQ(prog->decl_count, 0);
    
    ast_arena_destroy(&arena);
}

TEST(simple_function) {
    const char* src = "fn main() { }";
    AstArena arena;
    Parser parser;
    AstProgram* prog = parse_source(src, &arena, &parser);
    
    ASSERT(parser_success(&parser));
    ASSERT(prog != NULL);
    ASSERT_EQ(prog->decl_count, 1);
    ASSERT_EQ(prog->decls[0]->kind, AST_FN_DECL);
    ASSERT_EQ(prog->decls[0]->as.fn_decl.param_count, 0);
    
    ast_arena_destroy(&arena);
}

TEST(function_with_params) {
    const char* src = "fn add(a: i32, b: i32) -> i32 { return a + b; }";
    AstArena arena;
    Parser parser;
    AstProgram* prog = parse_source(src, &arena, &parser);
    
    ASSERT(parser_success(&parser));
    ASSERT(prog != NULL);
    ASSERT_EQ(prog->decl_count, 1);
    
    AstFnDecl* fn = &prog->decls[0]->as.fn_decl;
    ASSERT_EQ(fn->param_count, 2);
    ASSERT(fn->return_type != NULL);
    ASSERT(fn->body != NULL);
    ASSERT_EQ(fn->body->stmt_count, 1);
    
    ast_arena_destroy(&arena);
}

TEST(actor_declaration) {
    const char* src = "actor Counter { fn inc() { } }";
    AstArena arena;
    Parser parser;
    AstProgram* prog = parse_source(src, &arena, &parser);
    
    ASSERT(parser_success(&parser));
    ASSERT(prog != NULL);
    ASSERT_EQ(prog->decl_count, 1);
    ASSERT_EQ(prog->decls[0]->kind, AST_ACTOR_DECL);
    ASSERT_EQ(prog->decls[0]->as.actor_decl.method_count, 1);
    
    ast_arena_destroy(&arena);
}

TEST(let_statement) {
    const char* src = "fn main() { let x = 42; let mut y: i32 = 0; }";
    AstArena arena;
    Parser parser;
    AstProgram* prog = parse_source(src, &arena, &parser);
    
    ASSERT(parser_success(&parser));
    ASSERT_EQ(prog->decl_count, 1);
    
    AstBlock* body = prog->decls[0]->as.fn_decl.body;
    ASSERT_EQ(body->stmt_count, 2);
    
    AstStmt* s1 = body->stmts[0];
    ASSERT_EQ(s1->kind, AST_LET_STMT);
    ASSERT_EQ(s1->as.let_stmt.is_mut, false);
    
    AstStmt* s2 = body->stmts[1];
    ASSERT_EQ(s2->kind, AST_LET_STMT);
    ASSERT_EQ(s2->as.let_stmt.is_mut, true);
    
    ast_arena_destroy(&arena);
}

TEST(if_statement) {
    const char* src = "fn main() { if x { } else { } }";
    AstArena arena;
    Parser parser;
    AstProgram* prog = parse_source(src, &arena, &parser);
    
    ASSERT(parser_success(&parser));
    AstBlock* body = prog->decls[0]->as.fn_decl.body;
    ASSERT_EQ(body->stmt_count, 1);
    ASSERT_EQ(body->stmts[0]->kind, AST_IF_STMT);
    ASSERT(body->stmts[0]->as.if_stmt.else_branch != NULL);
    
    ast_arena_destroy(&arena);
}

TEST(spawn_statement) {
    const char* src = "fn main() { spawn worker(); }";
    AstArena arena;
    Parser parser;
    AstProgram* prog = parse_source(src, &arena, &parser);
    
    ASSERT(parser_success(&parser));
    AstBlock* body = prog->decls[0]->as.fn_decl.body;
    ASSERT_EQ(body->stmt_count, 1);
    ASSERT_EQ(body->stmts[0]->kind, AST_SPAWN_STMT);
    
    ast_arena_destroy(&arena);
}

TEST(message_send) {
    const char* src = "fn main() { worker ! msg; }";
    AstArena arena;
    Parser parser;
    AstProgram* prog = parse_source(src, &arena, &parser);
    
    ASSERT(parser_success(&parser));
    AstBlock* body = prog->decls[0]->as.fn_decl.body;
    ASSERT_EQ(body->stmt_count, 1);
    ASSERT_EQ(body->stmts[0]->kind, AST_EXPR_STMT);
    
    AstExpr* expr = body->stmts[0]->as.expr_stmt.expr;
    ASSERT_EQ(expr->kind, AST_SEND_EXPR);
    
    ast_arena_destroy(&arena);
}

TEST(receive_block) {
    const char* src = "fn main() { receive { ping => { } pong => { } } }";
    AstArena arena;
    Parser parser;
    AstProgram* prog = parse_source(src, &arena, &parser);
    
    ASSERT(parser_success(&parser));
    AstBlock* body = prog->decls[0]->as.fn_decl.body;
    ASSERT_EQ(body->stmt_count, 1);
    ASSERT_EQ(body->stmts[0]->kind, AST_RECEIVE_STMT);
    ASSERT_EQ(body->stmts[0]->as.receive_stmt.arm_count, 2);
    
    ast_arena_destroy(&arena);
}

TEST(binary_expressions) {
    const char* src = "fn main() { let x = 1 + 2 * 3; }";
    AstArena arena;
    Parser parser;
    AstProgram* prog = parse_source(src, &arena, &parser);
    
    ASSERT(parser_success(&parser));
    AstBlock* body = prog->decls[0]->as.fn_decl.body;
    AstExpr* init = body->stmts[0]->as.let_stmt.init;
    
    /* Should be: (+) with left=1, right=(* 2 3) */
    ASSERT_EQ(init->kind, AST_BINARY_EXPR);
    ASSERT_EQ(init->as.binary.op, BINARY_ADD);
    ASSERT_EQ(init->as.binary.right->kind, AST_BINARY_EXPR);
    ASSERT_EQ(init->as.binary.right->as.binary.op, BINARY_MUL);
    
    ast_arena_destroy(&arena);
}

TEST(call_expression) {
    const char* src = "fn main() { foo(1, 2, 3); }";
    AstArena arena;
    Parser parser;
    AstProgram* prog = parse_source(src, &arena, &parser);
    
    ASSERT(parser_success(&parser));
    AstBlock* body = prog->decls[0]->as.fn_decl.body;
    AstExpr* expr = body->stmts[0]->as.expr_stmt.expr;
    
    ASSERT_EQ(expr->kind, AST_CALL_EXPR);
    ASSERT_EQ(expr->as.call.arg_count, 3);
    
    ast_arena_destroy(&arena);
}

/* ============================================================
 * Main
 * ============================================================ */

int main(void) {
    printf("Running parser tests:\n");
    
    RUN_TEST(empty_program);
    RUN_TEST(simple_function);
    RUN_TEST(function_with_params);
    RUN_TEST(actor_declaration);
    RUN_TEST(let_statement);
    RUN_TEST(if_statement);
    RUN_TEST(spawn_statement);
    RUN_TEST(message_send);
    RUN_TEST(receive_block);
    RUN_TEST(binary_expressions);
    RUN_TEST(call_expression);
    
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
