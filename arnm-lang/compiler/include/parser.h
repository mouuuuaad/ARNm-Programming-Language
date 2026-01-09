/*
 * ARNm Compiler - Parser Interface
 */

#ifndef ARNM_PARSER_H
#define ARNM_PARSER_H

#include "lexer.h"
#include "ast.h"

typedef enum {
    PARSE_OK = 0,
    PARSE_ERR_UNEXPECTED_TOKEN,
    PARSE_ERR_EXPECTED_IDENT,
    PARSE_ERR_EXPECTED_EXPR,
    PARSE_ERR_EXPECTED_BLOCK,
    PARSE_ERR_UNCLOSED_PAREN,
    PARSE_ERR_UNCLOSED_BRACE,
    PARSE_ERR_MEMORY,
} ParseError;

typedef struct {
    ParseError  code;
    const char* message;
    Span        span;
} ParseDiagnostic;

#define MAX_PARSE_ERRORS 64

typedef struct {
    Lexer*          lexer;
    AstArena*       arena;
    Token           current;
    Token           previous;
    bool            had_error;
    bool            panic_mode;
    ParseDiagnostic errors[MAX_PARSE_ERRORS];
    size_t          error_count;
} Parser;

void parser_init(Parser* parser, Lexer* lexer, AstArena* arena);
AstProgram* parser_parse_program(Parser* parser);

static inline bool parser_success(const Parser* parser) {
    return !parser->had_error;
}

const char* parse_error_message(ParseError err);

/* Parse functions */
AstDecl* parse_declaration(Parser* parser);
AstStmt* parse_statement(Parser* parser);
AstBlock* parse_block(Parser* parser);
AstExpr* parse_expression(Parser* parser);
AstType* parse_type(Parser* parser);

#endif /* ARNM_PARSER_H */
