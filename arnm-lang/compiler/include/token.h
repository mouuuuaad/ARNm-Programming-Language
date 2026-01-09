/*
 * ARNm Compiler - Token Definitions
 * 
 * DESIGN DECISION: Tokens reference source memory directly.
 * Rationale: Zero-copy tokenization minimizes allocator pressure
 * and ensures cache-friendly sequential access during parsing.
 * 
 * Trade-off: Source buffer must outlive all tokens.
 */

#ifndef ARNM_TOKEN_H
#define ARNM_TOKEN_H

#include <stddef.h>
#include <stdint.h>

/* ============================================================
 * Token Kinds
 * ============================================================
 * Ordered by frequency of occurrence for branch prediction.
 * Identifiers and literals appear first (hot path).
 */
typedef enum {
    /* Identifiers and Literals */
    TOK_IDENT,          /* foo, bar, myActor */
    TOK_INT_LIT,        /* 42, 0xFF, 0b1010 */
    TOK_FLOAT_LIT,      /* 3.14, 1e-10 */
    TOK_STRING_LIT,     /* "hello" */
    TOK_CHAR_LIT,       /* 'a' */

    /* Keywords - Declarations */
    TOK_FN,             /* fn */
    TOK_ACTOR,          /* actor */
    TOK_LET,            /* let */
    TOK_MUT,            /* mut */
    TOK_CONST,          /* const */

    /* Keywords - Control Flow */
    TOK_IF,             /* if */
    TOK_ELSE,           /* else */
    TOK_MATCH,          /* match */
    TOK_WHILE,          /* while */
    TOK_FOR,            /* for */
    TOK_LOOP,           /* loop */
    TOK_BREAK,          /* break */
    TOK_CONTINUE,       /* continue */
    TOK_RETURN,         /* return */

    /* Keywords - Concurrency */
    TOK_SPAWN,          /* spawn */
    TOK_RECEIVE,        /* receive */
    TOK_SELF,           /* self */

    /* Keywords - Types & Ownership */
    TOK_UNIQUE,         /* unique */
    TOK_SHARED,         /* shared */
    TOK_IMMUT,          /* immut (immutable) */
    TOK_TYPE,           /* type */
    TOK_STRUCT,         /* struct */
    TOK_ENUM,           /* enum */
    TOK_TRUE,           /* true */
    TOK_FALSE,          /* false */
    TOK_NIL,            /* nil */

    /* Operators - Single Character */
    TOK_PLUS,           /* + */
    TOK_MINUS,          /* - */
    TOK_STAR,           /* * */
    TOK_SLASH,          /* / */
    TOK_PERCENT,        /* % */
    TOK_BANG,           /* ! (message send) */
    TOK_EQ,             /* = */
    TOK_LT,             /* < */
    TOK_GT,             /* > */
    TOK_AMP,            /* & */
    TOK_PIPE,           /* | */
    TOK_CARET,          /* ^ */
    TOK_TILDE,          /* ~ */
    TOK_DOT,            /* . */
    TOK_COMMA,          /* , */
    TOK_COLON,          /* : */
    TOK_SEMI,           /* ; */
    TOK_AT,             /* @ */
    TOK_HASH,           /* # */
    TOK_QUESTION,       /* ? */

    /* Operators - Multi-Character */
    TOK_ARROW,          /* -> */
    TOK_FAT_ARROW,      /* => */
    TOK_DOUBLE_COLON,   /* :: */
    TOK_EQ_EQ,          /* == */
    TOK_BANG_EQ,        /* != */
    TOK_LT_EQ,          /* <= */
    TOK_GT_EQ,          /* >= */
    TOK_AND_AND,        /* && */
    TOK_PIPE_PIPE,      /* || */
    TOK_PLUS_EQ,        /* += */
    TOK_MINUS_EQ,       /* -= */
    TOK_STAR_EQ,        /* *= */
    TOK_SLASH_EQ,       /* /= */
    TOK_DOT_DOT,        /* .. */
    TOK_DOT_DOT_EQ,     /* ..= */

    /* Delimiters */
    TOK_LPAREN,         /* ( */
    TOK_RPAREN,         /* ) */
    TOK_LBRACE,         /* { */
    TOK_RBRACE,         /* } */
    TOK_LBRACKET,       /* [ */
    TOK_RBRACKET,       /* ] */

    /* Special */
    TOK_EOF,            /* End of file */
    TOK_ERROR,          /* Lexer error */

    TOK_COUNT           /* Total token count (for arrays) */
} TokenKind;

/* ============================================================
 * Source Span
 * ============================================================
 * Tracks exact location in source for error messages and LSP.
 * Uses byte offsets, not character offsets (UTF-8 aware).
 */
typedef struct {
    uint32_t start;     /* Byte offset from source start */
    uint32_t end;       /* Byte offset (exclusive) */
    uint32_t line;      /* 1-indexed line number */
    uint16_t column;    /* 1-indexed column (byte offset in line) */
} Span;

/* ============================================================
 * Token
 * ============================================================
 * 24 bytes on 64-bit systems (fits in half a cache line).
 * 
 * INVARIANT: lexeme points into source buffer.
 * INVARIANT: length == span.end - span.start
 */
typedef struct {
    TokenKind   kind;       /* 4 bytes */
    uint32_t    length;     /* 4 bytes: lexeme length */
    const char* lexeme;     /* 8 bytes: pointer into source */
    Span        span;       /* 10 bytes (padded to 12) */
} Token;

/* ============================================================
 * Token Utilities
 * ============================================================ */

/* Returns human-readable name for token kind */
const char* token_kind_name(TokenKind kind);

/* Returns true if token is a keyword */
static inline int token_is_keyword(TokenKind kind) {
    return kind >= TOK_FN && kind <= TOK_NIL;
}

/* Returns true if token is an operator */
static inline int token_is_operator(TokenKind kind) {
    return kind >= TOK_PLUS && kind <= TOK_DOT_DOT_EQ;
}

/* Returns true if token is a delimiter */
static inline int token_is_delimiter(TokenKind kind) {
    return kind >= TOK_LPAREN && kind <= TOK_RBRACKET;
}

/* Returns true if token represents a literal value */
static inline int token_is_literal(TokenKind kind) {
    return kind >= TOK_INT_LIT && kind <= TOK_CHAR_LIT;
}

#endif /* ARNM_TOKEN_H */
