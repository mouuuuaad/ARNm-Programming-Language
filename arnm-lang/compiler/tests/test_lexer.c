/*
 * ARNm Lexer Tests
 */

#include "../include/lexer.h"
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

/* ============================================================
 * Tests
 * ============================================================ */

TEST(keywords) {
    const char* src = "fn actor let const mut spawn receive if else while return";
    Lexer lexer;
    lexer_init(&lexer, src, strlen(src));
    
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_FN);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_ACTOR);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_LET);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_CONST);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_MUT);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_SPAWN);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_RECEIVE);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_IF);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_ELSE);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_WHILE);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_RETURN);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_EOF);
}

TEST(operators) {
    const char* src = "+-*/% == != < <= > >= && || -> => ! :: :=";
    Lexer lexer;
    lexer_init(&lexer, src, strlen(src));
    
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_PLUS);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_MINUS);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_STAR);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_SLASH);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_PERCENT);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_EQ_EQ);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_BANG_EQ);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_LT);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_LT_EQ);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_GT);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_GT_EQ);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_AND_AND);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_PIPE_PIPE);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_ARROW);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_FAT_ARROW);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_BANG);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_DOUBLE_COLON);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_COLON_EQ);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_EOF);
}

TEST(delimiters) {
    const char* src = "(){}[],;:";
    Lexer lexer;
    lexer_init(&lexer, src, strlen(src));
    
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_LPAREN);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_RPAREN);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_LBRACE);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_RBRACE);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_LBRACKET);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_RBRACKET);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_COMMA);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_SEMI);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_COLON);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_EOF);
}

TEST(numbers) {
    const char* src = "42 3.14 0xFF 0b1010 1e10";
    Lexer lexer;
    lexer_init(&lexer, src, strlen(src));
    
    Token t1 = lexer_next_token(&lexer);
    ASSERT_EQ(t1.kind, TOK_INT_LIT);
    ASSERT_EQ(t1.length, 2);
    
    Token t2 = lexer_next_token(&lexer);
    ASSERT_EQ(t2.kind, TOK_FLOAT_LIT);
    
    Token t3 = lexer_next_token(&lexer);
    ASSERT_EQ(t3.kind, TOK_INT_LIT);
    
    Token t4 = lexer_next_token(&lexer);
    ASSERT_EQ(t4.kind, TOK_INT_LIT);
    
    Token t5 = lexer_next_token(&lexer);
    ASSERT_EQ(t5.kind, TOK_FLOAT_LIT);
    
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_EOF);
}

TEST(strings) {
    const char* src = "\"hello\" \"world\\n\"";
    Lexer lexer;
    lexer_init(&lexer, src, strlen(src));
    
    Token t1 = lexer_next_token(&lexer);
    ASSERT_EQ(t1.kind, TOK_STRING_LIT);
    ASSERT_EQ(t1.length, 7);
    
    Token t2 = lexer_next_token(&lexer);
    ASSERT_EQ(t2.kind, TOK_STRING_LIT);
    
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_EOF);
}

TEST(identifiers) {
    const char* src = "foo bar_baz _private myActor123";
    Lexer lexer;
    lexer_init(&lexer, src, strlen(src));
    
    Token t1 = lexer_next_token(&lexer);
    ASSERT_EQ(t1.kind, TOK_IDENT);
    ASSERT_EQ(t1.length, 3);
    
    Token t2 = lexer_next_token(&lexer);
    ASSERT_EQ(t2.kind, TOK_IDENT);
    ASSERT_EQ(t2.length, 7);
    
    Token t3 = lexer_next_token(&lexer);
    ASSERT_EQ(t3.kind, TOK_IDENT);
    ASSERT_EQ(t3.length, 8);
    
    Token t4 = lexer_next_token(&lexer);
    ASSERT_EQ(t4.kind, TOK_IDENT);
    ASSERT_EQ(t4.length, 10);
    
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_EOF);
}

TEST(comments) {
    const char* src = "foo // comment\nbar /* block */ baz";
    Lexer lexer;
    lexer_init(&lexer, src, strlen(src));
    
    Token t1 = lexer_next_token(&lexer);
    ASSERT_EQ(t1.kind, TOK_IDENT);
    ASSERT(memcmp(t1.lexeme, "foo", 3) == 0);
    
    Token t2 = lexer_next_token(&lexer);
    ASSERT_EQ(t2.kind, TOK_IDENT);
    ASSERT(memcmp(t2.lexeme, "bar", 3) == 0);
    
    Token t3 = lexer_next_token(&lexer);
    ASSERT_EQ(t3.kind, TOK_IDENT);
    ASSERT(memcmp(t3.lexeme, "baz", 3) == 0);
    
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_EOF);
}

TEST(line_tracking) {
    const char* src = "foo\nbar\nbaz";
    Lexer lexer;
    lexer_init(&lexer, src, strlen(src));
    
    Token t1 = lexer_next_token(&lexer);
    ASSERT_EQ(t1.span.line, 1);
    ASSERT_EQ(t1.span.column, 1);
    
    Token t2 = lexer_next_token(&lexer);
    ASSERT_EQ(t2.span.line, 2);
    ASSERT_EQ(t2.span.column, 1);
    
    Token t3 = lexer_next_token(&lexer);
    ASSERT_EQ(t3.span.line, 3);
    ASSERT_EQ(t3.span.column, 1);
}

TEST(message_send) {
    const char* src = "actor ! message";
    Lexer lexer;
    lexer_init(&lexer, src, strlen(src));
    
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_ACTOR);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_BANG);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_IDENT);
    ASSERT_EQ(lexer_next_token(&lexer).kind, TOK_EOF);
}

/* ============================================================
 * Main
 * ============================================================ */

int main(void) {
    printf("Running lexer tests:\n");
    
    RUN_TEST(keywords);
    RUN_TEST(operators);
    RUN_TEST(delimiters);
    RUN_TEST(numbers);
    RUN_TEST(strings);
    RUN_TEST(identifiers);
    RUN_TEST(comments);
    RUN_TEST(line_tracking);
    RUN_TEST(message_send);
    
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
