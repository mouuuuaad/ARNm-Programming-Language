/*
 * ARNm Compiler - Parser Implementation
 * 
 * Recursive descent parser with Pratt expression parsing.
 */

#include "../include/parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================
 * Arena Implementation
 * ============================================================ */

void ast_arena_init(AstArena* arena, size_t capacity) {
    arena->buffer = (char*)malloc(capacity);
    arena->capacity = capacity;
    arena->used = 0;
}

void* ast_arena_alloc(AstArena* arena, size_t size) {
    /* Align to 8 bytes */
    size = (size + 7) & ~7;
    
    if (arena->used + size > arena->capacity) {
        return NULL;
    }
    
    void* ptr = arena->buffer + arena->used;
    arena->used += size;
    return ptr;
}

void ast_arena_destroy(AstArena* arena) {
    free(arena->buffer);
    arena->buffer = NULL;
    arena->capacity = 0;
    arena->used = 0;
}

/* ============================================================
 * Parser Helpers
 * ============================================================ */

static void advance(Parser* parser) {
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
}

static bool check(Parser* parser, TokenKind kind) {
    return parser->current.kind == kind;
}

static bool match(Parser* parser, TokenKind kind) {
    if (!check(parser, kind)) return false;
    advance(parser);
    return true;
}

static void error_at(Parser* parser, Token* token, const char* message) {
    if (parser->panic_mode) return;
    parser->panic_mode = true;
    parser->had_error = true;
    
    if (parser->error_count < MAX_PARSE_ERRORS) {
        parser->errors[parser->error_count].code = PARSE_ERR_UNEXPECTED_TOKEN;
        parser->errors[parser->error_count].message = message;
        parser->errors[parser->error_count].span = token->span;
        parser->error_count++;
    }
}

static void error(Parser* parser, const char* message) {
    error_at(parser, &parser->previous, message);
}

static void error_current(Parser* parser, const char* message) {
    error_at(parser, &parser->current, message);
}

static void consume(Parser* parser, TokenKind kind, const char* message) {
    if (parser->current.kind == kind) {
        advance(parser);
        return;
    }
    error_current(parser, message);
}

static void synchronize(Parser* parser) {
    parser->panic_mode = false;
    
    while (parser->current.kind != TOK_EOF) {
        if (parser->previous.kind == TOK_SEMI) return;
        
        switch (parser->current.kind) {
            case TOK_FN:
            case TOK_ACTOR:
            case TOK_LET:
            case TOK_IF:
            case TOK_WHILE:
            case TOK_FOR:
            case TOK_RETURN:
            case TOK_SPAWN:
            case TOK_RECEIVE:
                return;
            default:
                break;
        }
        advance(parser);
    }
}

/* ============================================================
 * Expression Parsing (Pratt Parser)
 * ============================================================ */

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,    /* = += -= *= /= */
    PREC_OR,            /* || */
    PREC_AND,           /* && */
    PREC_EQUALITY,      /* == != */
    PREC_COMPARISON,    /* < > <= >= */
    PREC_SEND,          /* ! (message send) */
    PREC_TERM,          /* + - */
    PREC_FACTOR,        /* * / % */
    PREC_UNARY,         /* ! - ~ */
    PREC_CALL,          /* . () [] */
    PREC_PRIMARY
} Precedence;

static AstExpr* parse_precedence(Parser* parser, Precedence prec);

static AstExpr* parse_number(Parser* parser) {
    AstExpr* expr = AST_NEW(parser->arena, AstExpr);
    if (!expr) return NULL;
    
    if (parser->previous.kind == TOK_INT_LIT) {
        expr->kind = AST_INT_LIT_EXPR;
        expr->as.int_lit.common.span = parser->previous.span;
        expr->as.int_lit.value = strtoll(parser->previous.lexeme, NULL, 0);
    } else {
        expr->kind = AST_FLOAT_LIT_EXPR;
        expr->as.float_lit.common.span = parser->previous.span;
        expr->as.float_lit.value = strtod(parser->previous.lexeme, NULL);
    }
    return expr;
}

static AstExpr* parse_string(Parser* parser) {
    AstExpr* expr = AST_NEW(parser->arena, AstExpr);
    if (!expr) return NULL;
    
    expr->kind = AST_STRING_LIT_EXPR;
    expr->as.string_lit.common.span = parser->previous.span;
    expr->as.string_lit.value = parser->previous.lexeme;
    expr->as.string_lit.value_len = parser->previous.length;
    return expr;
}

static AstExpr* parse_identifier(Parser* parser) {
    AstExpr* expr = AST_NEW(parser->arena, AstExpr);
    if (!expr) return NULL;
    
    expr->kind = AST_IDENT_EXPR;
    expr->as.ident.common.span = parser->previous.span;
    expr->as.ident.name = parser->previous.lexeme;
    expr->as.ident.name_len = parser->previous.length;
    return expr;
}

static AstExpr* parse_grouping(Parser* parser) {
    AstExpr* inner = parse_expression(parser);
    consume(parser, TOK_RPAREN, "expected ')' after expression");
    
    AstExpr* expr = AST_NEW(parser->arena, AstExpr);
    if (!expr) return NULL;
    
    expr->kind = AST_GROUP_EXPR;
    expr->as.group.inner = inner;
    return expr;
}

static AstExpr* parse_unary(Parser* parser) {
    TokenKind op_kind = parser->previous.kind;
    Span op_span = parser->previous.span;
    
    AstExpr* operand = parse_precedence(parser, PREC_UNARY);
    
    AstExpr* expr = AST_NEW(parser->arena, AstExpr);
    if (!expr) return NULL;
    
    expr->kind = AST_UNARY_EXPR;
    expr->as.unary.common.span = op_span;
    expr->as.unary.operand = operand;
    
    switch (op_kind) {
        case TOK_MINUS: expr->as.unary.op = UNARY_NEG; break;
        case TOK_BANG:  expr->as.unary.op = UNARY_NOT; break;
        case TOK_TILDE: expr->as.unary.op = UNARY_BITNOT; break;
        default: break;
    }
    return expr;
}

static AstExpr* parse_literal(Parser* parser) {
    AstExpr* expr = AST_NEW(parser->arena, AstExpr);
    if (!expr) return NULL;
    
    switch (parser->previous.kind) {
        case TOK_TRUE:
            expr->kind = AST_BOOL_LIT_EXPR;
            expr->as.bool_lit.value = true;
            break;
        case TOK_FALSE:
            expr->kind = AST_BOOL_LIT_EXPR;
            expr->as.bool_lit.value = false;
            break;
        case TOK_NIL:
            expr->kind = AST_NIL_EXPR;
            break;
        case TOK_SELF:
            expr->kind = AST_SELF_EXPR;
            break;
        default:
            return NULL;
    }
    expr->as.bool_lit.common.span = parser->previous.span;
    return expr;
}

static BinaryOp token_to_binary_op(TokenKind kind) {
    switch (kind) {
        case TOK_PLUS:     return BINARY_ADD;
        case TOK_MINUS:    return BINARY_SUB;
        case TOK_STAR:     return BINARY_MUL;
        case TOK_SLASH:    return BINARY_DIV;
        case TOK_PERCENT:  return BINARY_MOD;
        case TOK_EQ_EQ:    return BINARY_EQ;
        case TOK_BANG_EQ:  return BINARY_NE;
        case TOK_LT:       return BINARY_LT;
        case TOK_LT_EQ:    return BINARY_LE;
        case TOK_GT:       return BINARY_GT;
        case TOK_GT_EQ:    return BINARY_GE;
        case TOK_AND_AND:  return BINARY_AND;
        case TOK_PIPE_PIPE: return BINARY_OR;
        case TOK_AMP:      return BINARY_BITAND;
        case TOK_PIPE:     return BINARY_BITOR;
        case TOK_CARET:    return BINARY_BITXOR;
        case TOK_EQ:       return BINARY_ASSIGN;
        case TOK_PLUS_EQ:  return BINARY_ADD;  /* Compound: desugar in sema */
        case TOK_MINUS_EQ: return BINARY_SUB;
        case TOK_STAR_EQ:  return BINARY_MUL;
        case TOK_SLASH_EQ: return BINARY_DIV;
        case TOK_BANG:     return BINARY_SEND;
        default:           return BINARY_ADD;
    }
}

static Precedence get_precedence(TokenKind kind) {
    switch (kind) {
        case TOK_EQ:
        case TOK_PLUS_EQ:
        case TOK_MINUS_EQ:
        case TOK_STAR_EQ:
        case TOK_SLASH_EQ: return PREC_ASSIGNMENT;
        case TOK_PIPE_PIPE: return PREC_OR;
        case TOK_AND_AND:  return PREC_AND;
        case TOK_EQ_EQ:
        case TOK_BANG_EQ:  return PREC_EQUALITY;
        case TOK_LT:
        case TOK_GT:
        case TOK_LT_EQ:
        case TOK_GT_EQ:    return PREC_COMPARISON;
        case TOK_BANG:     return PREC_SEND;
        case TOK_PLUS:
        case TOK_MINUS:    return PREC_TERM;
        case TOK_STAR:
        case TOK_SLASH:
        case TOK_PERCENT:  return PREC_FACTOR;
        case TOK_LPAREN:
        case TOK_LBRACKET:
        case TOK_DOT:      return PREC_CALL;
        default:           return PREC_NONE;
    }
}

static AstExpr* parse_binary(Parser* parser, AstExpr* left) {
    TokenKind op_kind = parser->previous.kind;
    Precedence prec = get_precedence(op_kind);
    
    AstExpr* right = parse_precedence(parser, (Precedence)(prec + 1));
    
    AstExpr* expr = AST_NEW(parser->arena, AstExpr);
    if (!expr) return NULL;
    
    if (op_kind == TOK_BANG) {
        expr->kind = AST_SEND_EXPR;
        expr->as.send.target = left;
        expr->as.send.message = right;
    } else {
        expr->kind = AST_BINARY_EXPR;
        expr->as.binary.op = token_to_binary_op(op_kind);
        expr->as.binary.left = left;
        expr->as.binary.right = right;
    }
    return expr;
}

static AstExpr* parse_call(Parser* parser, AstExpr* callee) {
    /* Parse arguments */
    AstExpr* args[64];
    size_t arg_count = 0;
    
    if (!check(parser, TOK_RPAREN)) {
        do {
            if (arg_count >= 64) {
                error(parser, "too many arguments");
                break;
            }
            args[arg_count++] = parse_expression(parser);
        } while (match(parser, TOK_COMMA));
    }
    consume(parser, TOK_RPAREN, "expected ')' after arguments");
    
    AstExpr* expr = AST_NEW(parser->arena, AstExpr);
    if (!expr) return NULL;
    
    expr->kind = AST_CALL_EXPR;
    expr->as.call.callee = callee;
    expr->as.call.arg_count = arg_count;
    if (arg_count > 0) {
        expr->as.call.args = AST_NEW_ARRAY(parser->arena, AstExpr*, arg_count);
        memcpy(expr->as.call.args, args, sizeof(AstExpr*) * arg_count);
    }
    return expr;
}

static AstExpr* parse_index(Parser* parser, AstExpr* object) {
    AstExpr* index = parse_expression(parser);
    consume(parser, TOK_RBRACKET, "expected ']' after index");
    
    AstExpr* expr = AST_NEW(parser->arena, AstExpr);
    if (!expr) return NULL;
    
    expr->kind = AST_INDEX_EXPR;
    expr->as.index.object = object;
    expr->as.index.index = index;
    return expr;
}

static AstExpr* parse_field(Parser* parser, AstExpr* object) {
    consume(parser, TOK_IDENT, "expected field name after '.'");
    
    AstExpr* expr = AST_NEW(parser->arena, AstExpr);
    if (!expr) return NULL;
    
    expr->kind = AST_FIELD_EXPR;
    expr->as.field.object = object;
    expr->as.field.field_name = parser->previous.lexeme;
    expr->as.field.field_name_len = parser->previous.length;
    return expr;
}

static AstExpr* parse_spawn_expr(Parser* parser) {
    Span start = parser->previous.span;
    AstExpr* inner = parse_precedence(parser, PREC_UNARY);
    
    AstExpr* expr = AST_NEW(parser->arena, AstExpr);
    if (!expr) return NULL;
    
    expr->kind = AST_SPAWN_EXPR;
    expr->as.spawn_expr.common.span = start;
    expr->as.spawn_expr.expr = inner;
    return expr;
}

static AstExpr* parse_prefix(Parser* parser) {
    switch (parser->previous.kind) {
        case TOK_INT_LIT:
        case TOK_FLOAT_LIT:   return parse_number(parser);
        case TOK_STRING_LIT:  return parse_string(parser);
        case TOK_IDENT:       return parse_identifier(parser);
        case TOK_LPAREN:      return parse_grouping(parser);
        case TOK_MINUS:
        case TOK_BANG:
        case TOK_TILDE:       return parse_unary(parser);
        case TOK_TRUE:
        case TOK_FALSE:
        case TOK_NIL:
        case TOK_SELF:        return parse_literal(parser);
        case TOK_SPAWN:       return parse_spawn_expr(parser);
        default:
            error(parser, "expected expression");
            return NULL;
    }
}

static AstExpr* parse_infix(Parser* parser, AstExpr* left) {
    switch (parser->previous.kind) {
        case TOK_PLUS:
        case TOK_MINUS:
        case TOK_STAR:
        case TOK_SLASH:
        case TOK_PERCENT:
        case TOK_EQ_EQ:
        case TOK_BANG_EQ:
        case TOK_LT:
        case TOK_GT:
        case TOK_LT_EQ:
        case TOK_GT_EQ:
        case TOK_AND_AND:
        case TOK_PIPE_PIPE:
        case TOK_AMP:
        case TOK_PIPE:
        case TOK_CARET:
        case TOK_EQ:
        case TOK_PLUS_EQ:
        case TOK_MINUS_EQ:
        case TOK_STAR_EQ:
        case TOK_SLASH_EQ:
        case TOK_BANG:        return parse_binary(parser, left);
        case TOK_LPAREN:      return parse_call(parser, left);
        case TOK_LBRACKET:    return parse_index(parser, left);
        case TOK_DOT:         return parse_field(parser, left);
        default:              return NULL;
    }
}

static AstExpr* parse_precedence(Parser* parser, Precedence prec) {
    advance(parser);
    AstExpr* left = parse_prefix(parser);
    if (!left) return NULL;
    
    while (prec <= get_precedence(parser->current.kind)) {
        advance(parser);
        left = parse_infix(parser, left);
        if (!left) return NULL;
    }
    return left;
}

AstExpr* parse_expression(Parser* parser) {
    return parse_precedence(parser, PREC_ASSIGNMENT);
}

/* ============================================================
 * Statement Parsing
 * ============================================================ */

AstBlock* parse_block(Parser* parser) {
    consume(parser, TOK_LBRACE, "expected '{'");
    
    AstStmt* stmts[256];
    size_t stmt_count = 0;
    
    while (!check(parser, TOK_RBRACE) && !check(parser, TOK_EOF)) {
        if (stmt_count >= 256) {
            error(parser, "too many statements in block");
            break;
        }
        AstStmt* stmt = parse_statement(parser);
        if (stmt) stmts[stmt_count++] = stmt;
        
        if (parser->panic_mode) synchronize(parser);
    }
    
    consume(parser, TOK_RBRACE, "expected '}'");
    
    AstBlock* block = AST_NEW(parser->arena, AstBlock);
    if (!block) return NULL;
    
    block->stmt_count = stmt_count;
    if (stmt_count > 0) {
        block->stmts = AST_NEW_ARRAY(parser->arena, AstStmt*, stmt_count);
        memcpy(block->stmts, stmts, sizeof(AstStmt*) * stmt_count);
    }
    return block;
}

static AstStmt* parse_let_stmt(Parser* parser) {
    Span start = parser->previous.span;
    bool is_mut = match(parser, TOK_MUT);
    
    consume(parser, TOK_IDENT, "expected variable name");
    const char* name = parser->previous.lexeme;
    uint32_t name_len = parser->previous.length;
    
    AstType* type_ann = NULL;
    if (match(parser, TOK_COLON)) {
        type_ann = parse_type(parser);
    }
    
    AstExpr* init = NULL;
    if (match(parser, TOK_EQ)) {
        init = parse_expression(parser);
    }
    
    consume(parser, TOK_SEMI, "expected ';' after variable declaration");
    
    AstStmt* stmt = AST_NEW(parser->arena, AstStmt);
    if (!stmt) return NULL;
    
    stmt->kind = AST_LET_STMT;
    stmt->as.let_stmt.common.span = start;
    stmt->as.let_stmt.name = name;
    stmt->as.let_stmt.name_len = name_len;
    stmt->as.let_stmt.is_mut = is_mut;
    stmt->as.let_stmt.type_ann = type_ann;
    stmt->as.let_stmt.init = init;
    return stmt;
}

static AstStmt* parse_return_stmt(Parser* parser) {
    Span start = parser->previous.span;
    
    AstExpr* value = NULL;
    if (!check(parser, TOK_SEMI)) {
        value = parse_expression(parser);
    }
    consume(parser, TOK_SEMI, "expected ';' after return");
    
    AstStmt* stmt = AST_NEW(parser->arena, AstStmt);
    if (!stmt) return NULL;
    
    stmt->kind = AST_RETURN_STMT;
    stmt->as.return_stmt.common.span = start;
    stmt->as.return_stmt.value = value;
    return stmt;
}

static AstStmt* parse_if_stmt(Parser* parser) {
    Span start = parser->previous.span;
    
    AstExpr* condition = parse_expression(parser);
    AstBlock* then_block = parse_block(parser);
    
    AstStmt* else_branch = NULL;
    if (match(parser, TOK_ELSE)) {
        if (check(parser, TOK_IF)) {
            advance(parser);
            else_branch = parse_if_stmt(parser);
        } else {
            AstStmt* else_stmt = AST_NEW(parser->arena, AstStmt);
            if (else_stmt) {
                else_stmt->kind = AST_BLOCK;
                AstBlock* block = parse_block(parser);
                if (block) {
                    else_stmt->as.block = *block;
                }
                else_branch = else_stmt;
            }
        }
    }
    
    AstStmt* stmt = AST_NEW(parser->arena, AstStmt);
    if (!stmt) return NULL;
    
    stmt->kind = AST_IF_STMT;
    stmt->as.if_stmt.common.span = start;
    stmt->as.if_stmt.condition = condition;
    stmt->as.if_stmt.then_block = then_block;
    stmt->as.if_stmt.else_branch = else_branch;
    return stmt;
}

static AstStmt* parse_while_stmt(Parser* parser) {
    Span start = parser->previous.span;
    
    AstExpr* condition = parse_expression(parser);
    AstBlock* body = parse_block(parser);
    
    AstStmt* stmt = AST_NEW(parser->arena, AstStmt);
    if (!stmt) return NULL;
    
    stmt->kind = AST_WHILE_STMT;
    stmt->as.while_stmt.common.span = start;
    stmt->as.while_stmt.condition = condition;
    stmt->as.while_stmt.body = body;
    return stmt;
}

static AstStmt* parse_for_stmt(Parser* parser) {
    Span start = parser->previous.span;
    
    /* for x in expr { body } */
    consume(parser, TOK_IDENT, "expected iterator variable after 'for'");
    const char* var_name = parser->previous.lexeme;
    uint32_t var_name_len = parser->previous.length;
    
    if (!match(parser, TOK_IDENT) || 
        parser->previous.length != 2 ||
        parser->previous.lexeme[0] != 'i' ||
        parser->previous.lexeme[1] != 'n') {
        error(parser, "expected 'in' after iterator variable");
    }
    
    AstExpr* iterable = parse_expression(parser);
    AstBlock* body = parse_block(parser);
    
    AstStmt* stmt = AST_NEW(parser->arena, AstStmt);
    if (!stmt) return NULL;
    
    stmt->kind = AST_FOR_STMT;
    stmt->as.for_stmt.common.span = start;
    stmt->as.for_stmt.var_name = var_name;
    stmt->as.for_stmt.var_name_len = var_name_len;
    stmt->as.for_stmt.iterable = iterable;
    stmt->as.for_stmt.body = body;
    return stmt;
}

static AstStmt* parse_loop_stmt(Parser* parser) {
    Span start = parser->previous.span;
    
    AstBlock* body = parse_block(parser);
    
    AstStmt* stmt = AST_NEW(parser->arena, AstStmt);
    if (!stmt) return NULL;
    
    stmt->kind = AST_LOOP_STMT;
    stmt->as.loop_stmt.common.span = start;
    stmt->as.loop_stmt.body = body;
    return stmt;
}

static AstStmt* parse_spawn_stmt(Parser* parser) {
    Span start = parser->previous.span;
    
    AstExpr* expr = parse_expression(parser);
    consume(parser, TOK_SEMI, "expected ';' after spawn");
    
    AstStmt* stmt = AST_NEW(parser->arena, AstStmt);
    if (!stmt) return NULL;
    
    stmt->kind = AST_SPAWN_STMT;
    stmt->as.spawn_stmt.common.span = start;
    stmt->as.spawn_stmt.expr = expr;
    return stmt;
}

static AstStmt* parse_receive_stmt(Parser* parser) {
    Span start = parser->previous.span;
    consume(parser, TOK_LBRACE, "expected '{' after receive");
    
    ReceiveArm arms[32];
    size_t arm_count = 0;
    
    while (!check(parser, TOK_RBRACE) && !check(parser, TOK_EOF)) {
        if (arm_count >= 32) {
            error(parser, "too many receive arms");
            break;
        }
        
        /* Parse pattern (identifier or integer) */
        if (match(parser, TOK_IDENT) || match(parser, TOK_INT_LIT)) {
            arms[arm_count].pattern = parser->previous.lexeme;
            arms[arm_count].pattern_len = parser->previous.length;
        } else {
            error(parser, "expected pattern (identifier or number)");
        }
        
        consume(parser, TOK_FAT_ARROW, "expected '=>' after pattern");
        arms[arm_count].body = parse_block(parser);
        arm_count++;
    }
    
    consume(parser, TOK_RBRACE, "expected '}' after receive arms");
    
    AstStmt* stmt = AST_NEW(parser->arena, AstStmt);
    if (!stmt) return NULL;
    
    stmt->kind = AST_RECEIVE_STMT;
    stmt->as.receive_stmt.common.span = start;
    stmt->as.receive_stmt.arm_count = arm_count;
    if (arm_count > 0) {
        stmt->as.receive_stmt.arms = AST_NEW_ARRAY(parser->arena, ReceiveArm, arm_count);
        memcpy(stmt->as.receive_stmt.arms, arms, sizeof(ReceiveArm) * arm_count);
    }
    return stmt;
}

AstStmt* parse_statement(Parser* parser) {
    if (match(parser, TOK_LET))     return parse_let_stmt(parser);
    if (match(parser, TOK_RETURN))  return parse_return_stmt(parser);
    if (match(parser, TOK_IF))      return parse_if_stmt(parser);
    if (match(parser, TOK_WHILE))   return parse_while_stmt(parser);
    if (match(parser, TOK_FOR))     return parse_for_stmt(parser);
    if (match(parser, TOK_LOOP))    return parse_loop_stmt(parser);
    if (match(parser, TOK_SPAWN))   return parse_spawn_stmt(parser);
    if (match(parser, TOK_RECEIVE)) return parse_receive_stmt(parser);
    
    if (match(parser, TOK_BREAK)) {
        consume(parser, TOK_SEMI, "expected ';' after break");
        AstStmt* stmt = AST_NEW(parser->arena, AstStmt);
        if (stmt) stmt->kind = AST_BREAK_STMT;
        return stmt;
    }
    
    if (match(parser, TOK_CONTINUE)) {
        consume(parser, TOK_SEMI, "expected ';' after continue");
        AstStmt* stmt = AST_NEW(parser->arena, AstStmt);
        if (stmt) stmt->kind = AST_CONTINUE_STMT;
        return stmt;
    }
    
    /* Expression statement */
    AstExpr* expr = parse_expression(parser);
    consume(parser, TOK_SEMI, "expected ';' after expression");
    
    AstStmt* stmt = AST_NEW(parser->arena, AstStmt);
    if (!stmt) return NULL;
    
    stmt->kind = AST_EXPR_STMT;
    stmt->as.expr_stmt.expr = expr;
    return stmt;
}

/* ============================================================
 * Type Parsing
 * ============================================================ */

AstType* parse_type(Parser* parser) {
    consume(parser, TOK_IDENT, "expected type name");
    
    AstType* type = AST_NEW(parser->arena, AstType);
    if (!type) return NULL;
    
    type->kind = AST_TYPE_IDENT;
    type->as.ident.common.span = parser->previous.span;
    type->as.ident.name = parser->previous.lexeme;
    type->as.ident.name_len = parser->previous.length;
    
    /* Handle optional type: Type? */
    if (match(parser, TOK_QUESTION)) {
        AstType* optional = AST_NEW(parser->arena, AstType);
        if (optional) {
            optional->kind = AST_TYPE_OPTIONAL;
            optional->as.optional.inner = type;
            return optional;
        }
    }
    
    /* Handle array type: [Type] */
    if (match(parser, TOK_LBRACKET)) {
        consume(parser, TOK_RBRACKET, "expected ']'");
        AstType* array = AST_NEW(parser->arena, AstType);
        if (array) {
            array->kind = AST_TYPE_ARRAY;
            array->as.array.element_type = type;
            return array;
        }
    }
    
    return type;
}

/* ============================================================
 * Declaration Parsing
 * ============================================================ */

static AstFnDecl* parse_function_inner(Parser* parser) {
    Span start = parser->previous.span;
    
    consume(parser, TOK_IDENT, "expected function name");
    const char* name = parser->previous.lexeme;
    uint32_t name_len = parser->previous.length;
    
    consume(parser, TOK_LPAREN, "expected '(' after function name");
    
    /* Parse parameters */
    FnParam params[32];
    size_t param_count = 0;
    
    if (!check(parser, TOK_RPAREN)) {
        do {
            if (param_count >= 32) {
                error(parser, "too many parameters");
                break;
            }
            
            params[param_count].is_mut = match(parser, TOK_MUT);
            consume(parser, TOK_IDENT, "expected parameter name");
            params[param_count].name = parser->previous.lexeme;
            params[param_count].name_len = parser->previous.length;
            
            consume(parser, TOK_COLON, "expected ':' after parameter name");
            params[param_count].type = parse_type(parser);
            param_count++;
        } while (match(parser, TOK_COMMA));
    }
    
    consume(parser, TOK_RPAREN, "expected ')' after parameters");
    
    /* Parse return type */
    AstType* return_type = NULL;
    if (match(parser, TOK_ARROW)) {
        return_type = parse_type(parser);
    }
    
    /* Parse body */
    AstBlock* body = parse_block(parser);
    
    AstFnDecl* fn = AST_NEW(parser->arena, AstFnDecl);
    if (!fn) return NULL;
    
    fn->common.span = start;
    fn->name = name;
    fn->name_len = name_len;
    fn->param_count = param_count;
    if (param_count > 0) {
        fn->params = AST_NEW_ARRAY(parser->arena, FnParam, param_count);
        memcpy(fn->params, params, sizeof(FnParam) * param_count);
    }
    fn->return_type = return_type;
    fn->body = body;
    return fn;
}

static AstActorDecl* parse_actor_inner(Parser* parser) {
    Span start = parser->previous.span;
    
    consume(parser, TOK_IDENT, "expected actor name");
    const char* name = parser->previous.lexeme;
    uint32_t name_len = parser->previous.length;
    
    consume(parser, TOK_LBRACE, "expected '{' after actor name");
    
    AstFnDecl* methods[64];
    size_t method_count = 0;
    AstLetStmt* fields[64];
    size_t field_count = 0;
    AstReceiveStmt* receive_block = NULL;
    
    while (!check(parser, TOK_RBRACE) && !check(parser, TOK_EOF)) {
        if (match(parser, TOK_FN)) {
            if (method_count >= 64) {
                error(parser, "too many methods in actor");
                break;
            }
            methods[method_count++] = parse_function_inner(parser);
        } else if (match(parser, TOK_RECEIVE)) {
            AstStmt* recv = parse_receive_stmt(parser);
            if (recv) receive_block = &recv->as.receive_stmt;
        } else if (match(parser, TOK_LET)) {
            /* Parse field: let x: Type [= init]; */
            AstStmt* stmt = parse_let_stmt(parser);
            if (stmt && stmt->kind == AST_LET_STMT) {
                if (field_count >= 64) {
                    error(parser, "too many fields in actor");
                } else {
                    /* Copy to heap or just pointer? 
                       parse_let_stmt allocates AstStmt. 
                       We need AstLetStmt*.
                       AST_NEW allocates, so stmt is on arena.
                    */
                    /* AstLetStmt is embedded in union usually? 
                       Wait, parse_let_stmt returns AstStmt*.
                       Stmt has union.
                       We want to store the pointer to the LetStmt part or the whole Stmt?
                       AstActorDecl fields is AstLetStmt**.
                       We can just point to &stmt->as.let_stmt if stmt is allocated.
                       But stmt is a pointer to a struct containing the union.
                       So &stmt->as.let_stmt is valid address inside that struct.
                    */
                    /* Actually `parse_let_stmt` likely calls `AST_NEW(AstStmt)`.
                       So `stmt` is valid.
                    */
                    fields[field_count++] = &stmt->as.let_stmt;
                }
            }
        } else {
            error_current(parser, "expected 'fn', 'receive', or 'let' in actor");
            advance(parser);
        }
    }
    
    consume(parser, TOK_RBRACE, "expected '}' after actor body");
    
    AstActorDecl* actor = AST_NEW(parser->arena, AstActorDecl);
    if (!actor) return NULL;
    
    actor->common.span = start;
    actor->name = name;
    actor->name_len = name_len;
    
    actor->field_count = field_count;
    if (field_count > 0) {
        actor->fields = AST_NEW_ARRAY(parser->arena, AstLetStmt*, field_count);
        memcpy(actor->fields, fields, sizeof(AstLetStmt*) * field_count);
    }

    actor->method_count = method_count;
    if (method_count > 0) {
        actor->methods = AST_NEW_ARRAY(parser->arena, AstFnDecl*, method_count);
        memcpy(actor->methods, methods, sizeof(AstFnDecl*) * method_count);
    }
    actor->receive_block = receive_block;
    return actor;
}


static AstStructDecl* parse_struct_inner(Parser* parser) {
    Span start = parser->previous.span;
    
    consume(parser, TOK_IDENT, "expected struct name");
    const char* name = parser->previous.lexeme;
    uint32_t name_len = parser->previous.length;
    
    consume(parser, TOK_LBRACE, "expected '{' after struct name");
    
    FnParam fields[64];
    size_t field_count = 0;
    
    if (!check(parser, TOK_RBRACE)) {
        do {
            if (field_count >= 64) {
                error(parser, "too many fields in struct");
                break;
            }
            
            fields[field_count].is_mut = match(parser, TOK_MUT);
            consume(parser, TOK_IDENT, "expected field name");
            fields[field_count].name = parser->previous.lexeme;
            fields[field_count].name_len = parser->previous.length;
            
            consume(parser, TOK_COLON, "expected ':' after field name");
            fields[field_count].type = parse_type(parser);
            
            field_count++;
        } while (match(parser, TOK_COMMA));
    }
    
    consume(parser, TOK_RBRACE, "expected '}' after struct body");
    
    AstStructDecl* decl = AST_NEW(parser->arena, AstStructDecl);
    if (!decl) return NULL;
    
    decl->common.span = start;
    decl->name = name;
    decl->name_len = name_len;
    decl->field_count = field_count;
    
    if (field_count > 0) {
        decl->fields = AST_NEW_ARRAY(parser->arena, FnParam, field_count);
        memcpy(decl->fields, fields, sizeof(FnParam) * field_count);
    }
    return decl;
}

AstDecl* parse_declaration(Parser* parser) {
    AstDecl* decl = AST_NEW(parser->arena, AstDecl);
    if (!decl) return NULL;
    
    if (match(parser, TOK_FN)) {
        decl->kind = AST_FN_DECL;
        AstFnDecl* fn = parse_function_inner(parser);
        if (fn) decl->as.fn_decl = *fn;
        return decl;
    }
    
    if (match(parser, TOK_ACTOR)) {
        decl->kind = AST_ACTOR_DECL;
        AstActorDecl* actor = parse_actor_inner(parser);
        if (actor) decl->as.actor_decl = *actor;
        return decl;
    }

    if (match(parser, TOK_STRUCT)) {
        decl->kind = AST_STRUCT_DECL;
        AstStructDecl* s = parse_struct_inner(parser);
        if (s) decl->as.struct_decl = *s;
        return decl;
    }
    
    error_current(parser, "expected 'fn' or 'actor'");
    return NULL;
}

/* ============================================================
 * Program Parsing
 * ============================================================ */

void parser_init(Parser* parser, Lexer* lexer, AstArena* arena) {
    parser->lexer = lexer;
    parser->arena = arena;
    parser->had_error = false;
    parser->panic_mode = false;
    parser->error_count = 0;
    
    /* Prime the parser with first token */
    advance(parser);
}

AstProgram* parser_parse_program(Parser* parser) {
    AstDecl* decls[256];
    size_t decl_count = 0;
    
    while (!check(parser, TOK_EOF)) {
        if (decl_count >= 256) {
            error(parser, "too many declarations");
            break;
        }
        
        AstDecl* decl = parse_declaration(parser);
        if (decl) decls[decl_count++] = decl;
        
        if (parser->panic_mode) synchronize(parser);
    }
    
    AstProgram* program = AST_NEW(parser->arena, AstProgram);
    if (!program) return NULL;
    
    program->decl_count = decl_count;
    if (decl_count > 0) {
        program->decls = AST_NEW_ARRAY(parser->arena, AstDecl*, decl_count);
        memcpy(program->decls, decls, sizeof(AstDecl*) * decl_count);
    }
    return program;
}

const char* parse_error_message(ParseError err) {
    switch (err) {
        case PARSE_OK:                  return "no error";
        case PARSE_ERR_UNEXPECTED_TOKEN: return "unexpected token";
        case PARSE_ERR_EXPECTED_IDENT:  return "expected identifier";
        case PARSE_ERR_EXPECTED_EXPR:   return "expected expression";
        case PARSE_ERR_EXPECTED_BLOCK:  return "expected block";
        case PARSE_ERR_UNCLOSED_PAREN:  return "unclosed parenthesis";
        case PARSE_ERR_UNCLOSED_BRACE:  return "unclosed brace";
        case PARSE_ERR_MEMORY:          return "out of memory";
        default:                        return "unknown error";
    }
}
