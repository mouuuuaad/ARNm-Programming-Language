/*
 * ARNm Compiler - Lexer Interface
 * 
 * DESIGN DECISION: State-machine based lexer with zero allocations.
 * The lexer maintains a cursor into the source buffer and produces
 * tokens on demand. No lookahead buffer is maintained (single-pass).
 * 
 * UTF-8 STRATEGY: ASCII fast-path for common characters (0x00-0x7F).
 * Multi-byte sequences are handled correctly but not optimized.
 * Identifiers may contain Unicode letters (validated separately).
 */

#ifndef ARNM_LEXER_H
#define ARNM_LEXER_H

#include "token.h"
#include <stdbool.h>

/* ============================================================
 * Lexer State
 * ============================================================
 * All state needed for tokenization. No heap allocations.
 * 
 * Size: ~40 bytes (fits in a cache line with room to spare)
 */
typedef struct {
    const char* source;     /* Source buffer (not owned) */
    size_t      source_len; /* Total source length in bytes */
    size_t      cursor;     /* Current byte position */
    uint32_t    line;       /* Current line (1-indexed) */
    uint16_t    column;     /* Current column (1-indexed) */
    Token       current;    /* Most recently produced token */
    Token       peek;       /* Lookahead token (lazy) */
    bool        has_peek;   /* True if peek is valid */
} Lexer;

/* ============================================================
 * Lexer Lifecycle
 * ============================================================ */

/*
 * Initialize lexer with source buffer.
 * 
 * PRECONDITION: source must remain valid for lexer lifetime.
 * PRECONDITION: source_len must be accurate.
 * 
 * The lexer does not take ownership of the source buffer.
 */
void lexer_init(Lexer* lexer, const char* source, size_t source_len);

/*
 * Advance to and return the next token.
 * 
 * Returns TOK_EOF when input is exhausted.
 * Returns TOK_ERROR for malformed input (lexeme contains error message).
 * 
 * This is the primary tokenization entry point.
 */
Token lexer_next_token(Lexer* lexer);

/*
 * Peek at the next token without consuming it.
 * 
 * Multiple calls return the same token until lexer_next_token() is called.
 * Useful for LL(1) parsing decisions.
 */
Token lexer_peek_token(Lexer* lexer);

/*
 * Return the current token (last token returned by lexer_next_token).
 */
static inline Token lexer_current(const Lexer* lexer) {
    return lexer->current;
}

/* ============================================================
 * UTF-8 Classification (Inline for Performance)
 * ============================================================ */

/* ASCII fast-path: single-byte characters */
static inline bool is_ascii(char c) {
    return (unsigned char)c < 0x80;
}

/* Whitespace (ASCII only for simplicity) */
static inline bool is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

/* Digit [0-9] */
static inline bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

/* Hex digit [0-9a-fA-F] */
static inline bool is_hex_digit(char c) {
    return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

/* ASCII letter [a-zA-Z] */
static inline bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

/* Valid identifier start: letter or underscore */
static inline bool is_ident_start(char c) {
    return is_alpha(c) || c == '_';
}

/* Valid identifier continuation: letter, digit, or underscore */
static inline bool is_ident_cont(char c) {
    return is_alpha(c) || is_digit(c) || c == '_';
}

/* ============================================================
 * Error Reporting
 * ============================================================ */

/* Error codes for lexer diagnostics */
typedef enum {
    LEX_OK = 0,
    LEX_ERR_UNEXPECTED_CHAR,
    LEX_ERR_UNTERMINATED_STRING,
    LEX_ERR_UNTERMINATED_CHAR,
    LEX_ERR_INVALID_ESCAPE,
    LEX_ERR_INVALID_NUMBER,
    LEX_ERR_UNTERMINATED_COMMENT,
} LexerError;

/* Get human-readable error message */
const char* lexer_error_message(LexerError err);

#endif /* ARNM_LEXER_H */
