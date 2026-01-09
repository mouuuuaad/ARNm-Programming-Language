/*
 * ARNm Compiler - Lexer Implementation
 * 
 * DESIGN LOG:
 * - State-machine based, no heap allocations
 * - ASCII fast-path for common characters
 * - Keywords recognized via binary search (sorted table)
 * - Single-pass tokenization, no backtracking
 * 
 * PERFORMANCE NOTES:
 * - Hot path is the main scan loop (while + switch)
 * - Keyword lookup is O(log n) via binary search
 * - Branch prediction friendly: common cases first
 */

#include "../include/lexer.h"
#include <string.h>

/* ============================================================
 * Keyword Table (Sorted for Binary Search)
 * ============================================================ */
typedef struct {
    const char* keyword;
    TokenKind   kind;
} KeywordEntry;

static const KeywordEntry keywords[] = {
    {"actor",    TOK_ACTOR},
    {"break",    TOK_BREAK},
    {"const",    TOK_CONST},
    {"continue", TOK_CONTINUE},
    {"else",     TOK_ELSE},
    {"enum",     TOK_ENUM},
    {"false",    TOK_FALSE},
    {"fn",       TOK_FN},
    {"for",      TOK_FOR},
    {"if",       TOK_IF},
    {"immut",    TOK_IMMUT},
    {"let",      TOK_LET},
    {"loop",     TOK_LOOP},
    {"match",    TOK_MATCH},
    {"mut",      TOK_MUT},
    {"nil",      TOK_NIL},
    {"receive",  TOK_RECEIVE},
    {"return",   TOK_RETURN},
    {"self",     TOK_SELF},
    {"shared",   TOK_SHARED},
    {"spawn",    TOK_SPAWN},
    {"struct",   TOK_STRUCT},
    {"true",     TOK_TRUE},
    {"type",     TOK_TYPE},
    {"unique",   TOK_UNIQUE},
    {"while",    TOK_WHILE},
};

#define KEYWORD_COUNT (sizeof(keywords) / sizeof(keywords[0]))

/* Binary search for keyword */
static TokenKind lookup_keyword(const char* start, size_t len) {
    int lo = 0;
    int hi = KEYWORD_COUNT - 1;
    
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        const char* kw = keywords[mid].keyword;
        size_t kw_len = strlen(kw);
        
        /* Compare strings lexicographically */
        size_t min_len = len < kw_len ? len : kw_len;
        int cmp = memcmp(start, kw, min_len);
        
        if (cmp == 0) {
            /* Prefix matches, compare lengths */
            if (len < kw_len) {
                cmp = -1;
            } else if (len > kw_len) {
                cmp = 1;
            } else {
                return keywords[mid].kind; /* Exact match */
            }
        }
        
        if (cmp < 0) {
            hi = mid - 1;
        } else {
            lo = mid + 1;
        }
    }
    
    return TOK_IDENT; /* Not a keyword */
}

/* ============================================================
 * Token Kind Names (for debugging/error messages)
 * ============================================================ */
const char* token_kind_name(TokenKind kind) {
    static const char* names[] = {
        [TOK_IDENT]        = "IDENT",
        [TOK_INT_LIT]      = "INT_LIT",
        [TOK_FLOAT_LIT]    = "FLOAT_LIT",
        [TOK_STRING_LIT]   = "STRING_LIT",
        [TOK_CHAR_LIT]     = "CHAR_LIT",
        [TOK_FN]           = "fn",
        [TOK_ACTOR]        = "actor",
        [TOK_LET]          = "let",
        [TOK_MUT]          = "mut",
        [TOK_CONST]        = "const",
        [TOK_IF]           = "if",
        [TOK_ELSE]         = "else",
        [TOK_MATCH]        = "match",
        [TOK_WHILE]        = "while",
        [TOK_FOR]          = "for",
        [TOK_LOOP]         = "loop",
        [TOK_BREAK]        = "break",
        [TOK_CONTINUE]     = "continue",
        [TOK_RETURN]       = "return",
        [TOK_SPAWN]        = "spawn",
        [TOK_RECEIVE]      = "receive",
        [TOK_SELF]         = "self",
        [TOK_UNIQUE]       = "unique",
        [TOK_SHARED]       = "shared",
        [TOK_IMMUT]        = "immut",
        [TOK_TYPE]         = "type",
        [TOK_STRUCT]       = "struct",
        [TOK_ENUM]         = "enum",
        [TOK_TRUE]         = "true",
        [TOK_FALSE]        = "false",
        [TOK_NIL]          = "nil",
        [TOK_PLUS]         = "+",
        [TOK_MINUS]        = "-",
        [TOK_STAR]         = "*",
        [TOK_SLASH]        = "/",
        [TOK_PERCENT]      = "%",
        [TOK_BANG]         = "!",
        [TOK_EQ]           = "=",
        [TOK_LT]           = "<",
        [TOK_GT]           = ">",
        [TOK_AMP]          = "&",
        [TOK_PIPE]         = "|",
        [TOK_CARET]        = "^",
        [TOK_TILDE]        = "~",
        [TOK_DOT]          = ".",
        [TOK_COMMA]        = ",",
        [TOK_COLON]        = ":",
        [TOK_SEMI]         = ";",
        [TOK_AT]           = "@",
        [TOK_HASH]         = "#",
        [TOK_QUESTION]     = "?",
        [TOK_ARROW]        = "->",
        [TOK_FAT_ARROW]    = "=>",
        [TOK_DOUBLE_COLON] = "::",
        [TOK_EQ_EQ]        = "==",
        [TOK_BANG_EQ]      = "!=",
        [TOK_LT_EQ]        = "<=",
        [TOK_GT_EQ]        = ">=",
        [TOK_AND_AND]      = "&&",
        [TOK_PIPE_PIPE]    = "||",
        [TOK_PLUS_EQ]      = "+=",
        [TOK_MINUS_EQ]     = "-=",
        [TOK_STAR_EQ]      = "*=",
        [TOK_SLASH_EQ]     = "/=",
        [TOK_DOT_DOT]      = "..",
        [TOK_DOT_DOT_EQ]   = "..=",
        [TOK_LPAREN]       = "(",
        [TOK_RPAREN]       = ")",
        [TOK_LBRACE]       = "{",
        [TOK_RBRACE]       = "}",
        [TOK_LBRACKET]     = "[",
        [TOK_RBRACKET]     = "]",
        [TOK_EOF]          = "EOF",
        [TOK_ERROR]        = "ERROR",
    };
    
    if (kind < TOK_COUNT) {
        return names[kind] ? names[kind] : "UNKNOWN";
    }
    return "UNKNOWN";
}

const char* lexer_error_message(LexerError err) {
    switch (err) {
        case LEX_OK:                    return "no error";
        case LEX_ERR_UNEXPECTED_CHAR:   return "unexpected character";
        case LEX_ERR_UNTERMINATED_STRING: return "unterminated string literal";
        case LEX_ERR_UNTERMINATED_CHAR: return "unterminated character literal";
        case LEX_ERR_INVALID_ESCAPE:    return "invalid escape sequence";
        case LEX_ERR_INVALID_NUMBER:    return "invalid number literal";
        case LEX_ERR_UNTERMINATED_COMMENT: return "unterminated block comment";
        default:                        return "unknown error";
    }
}

/* ============================================================
 * Lexer Implementation
 * ============================================================ */

void lexer_init(Lexer* lexer, const char* source, size_t source_len) {
    lexer->source = source;
    lexer->source_len = source_len;
    lexer->cursor = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->has_peek = false;
    
    /* Initialize current token to EOF */
    lexer->current.kind = TOK_EOF;
    lexer->current.lexeme = source + source_len;
    lexer->current.length = 0;
    lexer->current.span = (Span){
        .start = (uint32_t)source_len,
        .end = (uint32_t)source_len,
        .line = 1,
        .column = 1
    };
}

/* Peek at current character without advancing */
static inline char peek_char(Lexer* lexer) {
    if (lexer->cursor >= lexer->source_len) {
        return '\0';
    }
    return lexer->source[lexer->cursor];
}

/* Peek at character at offset from cursor */
static inline char peek_char_at(Lexer* lexer, size_t offset) {
    if (lexer->cursor + offset >= lexer->source_len) {
        return '\0';
    }
    return lexer->source[lexer->cursor + offset];
}

/* Advance cursor by one byte */
static inline void advance(Lexer* lexer) {
    if (lexer->cursor < lexer->source_len) {
        if (lexer->source[lexer->cursor] == '\n') {
            lexer->line++;
            lexer->column = 1;
        } else {
            lexer->column++;
        }
        lexer->cursor++;
    }
}

/* Skip whitespace and comments */
static void skip_whitespace_and_comments(Lexer* lexer) {
    while (lexer->cursor < lexer->source_len) {
        char c = peek_char(lexer);
        
        /* Whitespace */
        if (is_whitespace(c)) {
            advance(lexer);
            continue;
        }
        
        /* Line comment: // */
        if (c == '/' && peek_char_at(lexer, 1) == '/') {
            advance(lexer); /* skip / */
            advance(lexer); /* skip / */
            while (peek_char(lexer) != '\n' && peek_char(lexer) != '\0') {
                advance(lexer);
            }
            continue;
        }
        
        /* Block comment */
        if (c == '/' && peek_char_at(lexer, 1) == '*') {
            advance(lexer); /* skip / */
            advance(lexer); /* skip * */
            int depth = 1;
            while (depth > 0 && lexer->cursor < lexer->source_len) {
                if (peek_char(lexer) == '/' && peek_char_at(lexer, 1) == '*') {
                    advance(lexer);
                    advance(lexer);
                    depth++;
                } else if (peek_char(lexer) == '*' && peek_char_at(lexer, 1) == '/') {
                    advance(lexer);
                    advance(lexer);
                    depth--;
                } else {
                    advance(lexer);
                }
            }
            continue;
        }
        
        break; /* Not whitespace or comment */
    }
}

/* Create a token at current position */
static Token make_token(Lexer* lexer, TokenKind kind, size_t start, uint32_t start_line, uint16_t start_col) {
    Token tok;
    tok.kind = kind;
    tok.lexeme = lexer->source + start;
    tok.length = (uint32_t)(lexer->cursor - start);
    tok.span.start = (uint32_t)start;
    tok.span.end = (uint32_t)lexer->cursor;
    tok.span.line = start_line;
    tok.span.column = start_col;
    return tok;
}

/* Create an error token */
static Token make_error(Lexer* lexer, const char* message, size_t start, uint32_t start_line, uint16_t start_col) {
    Token tok;
    tok.kind = TOK_ERROR;
    tok.lexeme = message;
    tok.length = (uint32_t)strlen(message);
    tok.span.start = (uint32_t)start;
    tok.span.end = (uint32_t)lexer->cursor;
    tok.span.line = start_line;
    tok.span.column = start_col;
    return tok;
}

/* Scan identifier or keyword */
static Token scan_identifier(Lexer* lexer, size_t start, uint32_t start_line, uint16_t start_col) {
    while (is_ident_cont(peek_char(lexer))) {
        advance(lexer);
    }
    
    /* Check if it's a keyword */
    size_t len = lexer->cursor - start;
    TokenKind kind = lookup_keyword(lexer->source + start, len);
    
    return make_token(lexer, kind, start, start_line, start_col);
}

/* Scan number literal (int or float) */
static Token scan_number(Lexer* lexer, size_t start, uint32_t start_line, uint16_t start_col) {
    bool is_float = false;
    
    /* Check for hex/binary/octal prefix */
    if (peek_char(lexer) == '0') {
        char next = peek_char_at(lexer, 1);
        if (next == 'x' || next == 'X') {
            advance(lexer); /* 0 */
            advance(lexer); /* x */
            while (is_hex_digit(peek_char(lexer))) {
                advance(lexer);
            }
            return make_token(lexer, TOK_INT_LIT, start, start_line, start_col);
        }
        if (next == 'b' || next == 'B') {
            advance(lexer); /* 0 */
            advance(lexer); /* b */
            while (peek_char(lexer) == '0' || peek_char(lexer) == '1') {
                advance(lexer);
            }
            return make_token(lexer, TOK_INT_LIT, start, start_line, start_col);
        }
        if (next == 'o' || next == 'O') {
            advance(lexer); /* 0 */
            advance(lexer); /* o */
            while (peek_char(lexer) >= '0' && peek_char(lexer) <= '7') {
                advance(lexer);
            }
            return make_token(lexer, TOK_INT_LIT, start, start_line, start_col);
        }
    }
    
    /* Decimal digits */
    while (is_digit(peek_char(lexer))) {
        advance(lexer);
    }
    
    /* Decimal point */
    if (peek_char(lexer) == '.' && is_digit(peek_char_at(lexer, 1))) {
        is_float = true;
        advance(lexer); /* . */
        while (is_digit(peek_char(lexer))) {
            advance(lexer);
        }
    }
    
    /* Exponent */
    char e = peek_char(lexer);
    if (e == 'e' || e == 'E') {
        is_float = true;
        advance(lexer); /* e */
        if (peek_char(lexer) == '+' || peek_char(lexer) == '-') {
            advance(lexer);
        }
        while (is_digit(peek_char(lexer))) {
            advance(lexer);
        }
    }
    
    return make_token(lexer, is_float ? TOK_FLOAT_LIT : TOK_INT_LIT, start, start_line, start_col);
}

/* Scan string literal */
static Token scan_string(Lexer* lexer, size_t start, uint32_t start_line, uint16_t start_col) {
    advance(lexer); /* skip opening " */
    
    while (peek_char(lexer) != '"' && peek_char(lexer) != '\0') {
        if (peek_char(lexer) == '\\') {
            advance(lexer); /* skip \ */
            if (peek_char(lexer) == '\0') {
                return make_error(lexer, "unterminated string", start, start_line, start_col);
            }
        }
        advance(lexer);
    }
    
    if (peek_char(lexer) == '\0') {
        return make_error(lexer, "unterminated string", start, start_line, start_col);
    }
    
    advance(lexer); /* skip closing " */
    return make_token(lexer, TOK_STRING_LIT, start, start_line, start_col);
}

/* Scan character literal */
static Token scan_char(Lexer* lexer, size_t start, uint32_t start_line, uint16_t start_col) {
    advance(lexer); /* skip opening ' */
    
    if (peek_char(lexer) == '\\') {
        advance(lexer); /* skip \ */
        if (peek_char(lexer) == '\0') {
            return make_error(lexer, "unterminated char", start, start_line, start_col);
        }
        advance(lexer); /* skip escaped char */
    } else if (peek_char(lexer) != '\'' && peek_char(lexer) != '\0') {
        advance(lexer); /* skip char */
    }
    
    if (peek_char(lexer) != '\'') {
        return make_error(lexer, "unterminated char", start, start_line, start_col);
    }
    
    advance(lexer); /* skip closing ' */
    return make_token(lexer, TOK_CHAR_LIT, start, start_line, start_col);
}

Token lexer_next_token(Lexer* lexer) {
    /* If we have a peeked token, consume it */
    if (lexer->has_peek) {
        lexer->has_peek = false;
        lexer->current = lexer->peek;
        return lexer->current;
    }
    
    skip_whitespace_and_comments(lexer);
    
    if (lexer->cursor >= lexer->source_len) {
        lexer->current = make_token(lexer, TOK_EOF, lexer->cursor, lexer->line, lexer->column);
        return lexer->current;
    }
    
    size_t start = lexer->cursor;
    uint32_t start_line = lexer->line;
    uint16_t start_col = lexer->column;
    char c = peek_char(lexer);
    
    /* Identifier or keyword */
    if (is_ident_start(c)) {
        lexer->current = scan_identifier(lexer, start, start_line, start_col);
        return lexer->current;
    }
    
    /* Number literal */
    if (is_digit(c)) {
        lexer->current = scan_number(lexer, start, start_line, start_col);
        return lexer->current;
    }
    
    /* String literal */
    if (c == '"') {
        lexer->current = scan_string(lexer, start, start_line, start_col);
        return lexer->current;
    }
    
    /* Character literal */
    if (c == '\'') {
        lexer->current = scan_char(lexer, start, start_line, start_col);
        return lexer->current;
    }
    
    /* Operators and delimiters */
    advance(lexer);
    
    switch (c) {
        /* Single-character tokens */
        case '(': lexer->current = make_token(lexer, TOK_LPAREN, start, start_line, start_col); break;
        case ')': lexer->current = make_token(lexer, TOK_RPAREN, start, start_line, start_col); break;
        case '{': lexer->current = make_token(lexer, TOK_LBRACE, start, start_line, start_col); break;
        case '}': lexer->current = make_token(lexer, TOK_RBRACE, start, start_line, start_col); break;
        case '[': lexer->current = make_token(lexer, TOK_LBRACKET, start, start_line, start_col); break;
        case ']': lexer->current = make_token(lexer, TOK_RBRACKET, start, start_line, start_col); break;
        case ',': lexer->current = make_token(lexer, TOK_COMMA, start, start_line, start_col); break;
        case ';': lexer->current = make_token(lexer, TOK_SEMI, start, start_line, start_col); break;
        case '~': lexer->current = make_token(lexer, TOK_TILDE, start, start_line, start_col); break;
        case '@': lexer->current = make_token(lexer, TOK_AT, start, start_line, start_col); break;
        case '#': lexer->current = make_token(lexer, TOK_HASH, start, start_line, start_col); break;
        case '?': lexer->current = make_token(lexer, TOK_QUESTION, start, start_line, start_col); break;
        case '^': lexer->current = make_token(lexer, TOK_CARET, start, start_line, start_col); break;
        case '%': lexer->current = make_token(lexer, TOK_PERCENT, start, start_line, start_col); break;
        
        /* Potential multi-character tokens */
        case '+':
            if (peek_char(lexer) == '=') {
                advance(lexer);
                lexer->current = make_token(lexer, TOK_PLUS_EQ, start, start_line, start_col);
            } else {
                lexer->current = make_token(lexer, TOK_PLUS, start, start_line, start_col);
            }
            break;
            
        case '-':
            if (peek_char(lexer) == '>') {
                advance(lexer);
                lexer->current = make_token(lexer, TOK_ARROW, start, start_line, start_col);
            } else if (peek_char(lexer) == '=') {
                advance(lexer);
                lexer->current = make_token(lexer, TOK_MINUS_EQ, start, start_line, start_col);
            } else {
                lexer->current = make_token(lexer, TOK_MINUS, start, start_line, start_col);
            }
            break;
            
        case '*':
            if (peek_char(lexer) == '=') {
                advance(lexer);
                lexer->current = make_token(lexer, TOK_STAR_EQ, start, start_line, start_col);
            } else {
                lexer->current = make_token(lexer, TOK_STAR, start, start_line, start_col);
            }
            break;
            
        case '/':
            if (peek_char(lexer) == '=') {
                advance(lexer);
                lexer->current = make_token(lexer, TOK_SLASH_EQ, start, start_line, start_col);
            } else {
                lexer->current = make_token(lexer, TOK_SLASH, start, start_line, start_col);
            }
            break;
            
        case '=':
            if (peek_char(lexer) == '=') {
                advance(lexer);
                lexer->current = make_token(lexer, TOK_EQ_EQ, start, start_line, start_col);
            } else if (peek_char(lexer) == '>') {
                advance(lexer);
                lexer->current = make_token(lexer, TOK_FAT_ARROW, start, start_line, start_col);
            } else {
                lexer->current = make_token(lexer, TOK_EQ, start, start_line, start_col);
            }
            break;
            
        case '!':
            if (peek_char(lexer) == '=') {
                advance(lexer);
                lexer->current = make_token(lexer, TOK_BANG_EQ, start, start_line, start_col);
            } else {
                lexer->current = make_token(lexer, TOK_BANG, start, start_line, start_col);
            }
            break;
            
        case '<':
            if (peek_char(lexer) == '=') {
                advance(lexer);
                lexer->current = make_token(lexer, TOK_LT_EQ, start, start_line, start_col);
            } else {
                lexer->current = make_token(lexer, TOK_LT, start, start_line, start_col);
            }
            break;
            
        case '>':
            if (peek_char(lexer) == '=') {
                advance(lexer);
                lexer->current = make_token(lexer, TOK_GT_EQ, start, start_line, start_col);
            } else {
                lexer->current = make_token(lexer, TOK_GT, start, start_line, start_col);
            }
            break;
            
        case '&':
            if (peek_char(lexer) == '&') {
                advance(lexer);
                lexer->current = make_token(lexer, TOK_AND_AND, start, start_line, start_col);
            } else {
                lexer->current = make_token(lexer, TOK_AMP, start, start_line, start_col);
            }
            break;
            
        case '|':
            if (peek_char(lexer) == '|') {
                advance(lexer);
                lexer->current = make_token(lexer, TOK_PIPE_PIPE, start, start_line, start_col);
            } else {
                lexer->current = make_token(lexer, TOK_PIPE, start, start_line, start_col);
            }
            break;
            
        case ':':
            if (peek_char(lexer) == ':') {
                advance(lexer);
                lexer->current = make_token(lexer, TOK_DOUBLE_COLON, start, start_line, start_col);
            } else {
                lexer->current = make_token(lexer, TOK_COLON, start, start_line, start_col);
            }
            break;
            
        case '.':
            if (peek_char(lexer) == '.') {
                advance(lexer);
                if (peek_char(lexer) == '=') {
                    advance(lexer);
                    lexer->current = make_token(lexer, TOK_DOT_DOT_EQ, start, start_line, start_col);
                } else {
                    lexer->current = make_token(lexer, TOK_DOT_DOT, start, start_line, start_col);
                }
            } else {
                lexer->current = make_token(lexer, TOK_DOT, start, start_line, start_col);
            }
            break;
            
        default:
            lexer->current = make_error(lexer, "unexpected character", start, start_line, start_col);
            break;
    }
    
    return lexer->current;
}

Token lexer_peek_token(Lexer* lexer) {
    if (!lexer->has_peek) {
        /* Save current state */
        Token saved = lexer->current;
        
        /* Get next token */
        lexer->peek = lexer_next_token(lexer);
        
        /* Restore current and mark peek as valid */
        lexer->current = saved;
        lexer->has_peek = true;
    }
    return lexer->peek;
}
