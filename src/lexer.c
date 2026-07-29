#include "../include/lexer.h"

static bool is_keyword(const char *str, TokenType *type) {
    struct { const char *word; TokenType type; } keywords[] = {
        {"fnc", TOK_FNC},
        {"get", TOK_GET},
        {"loop", TOK_LOOP},
        {"if", TOK_IF},
        {"or", TOK_OR},
        {"else", TOK_ELSE},
        {"while", TOK_WHILE},
        {"cycle", TOK_CYCLE},
        {"when", TOK_WHEN},
        {"fixed", TOK_FIXED},
        {"check", TOK_CHECK},
        {"read", TOK_READ},
        {"show", TOK_SHOW},
        {"showf", TOK_SHOWF},
        {"group", TOK_GROUP},
        {"set", TOK_SET},
        {"arr", TOK_ARR},
        {"map", TOK_MAP},
        {"pkg", TOK_PKG},        // Package naming keyword
        {"use", TOK_USE},        // Package usage keyword
        {"const", TOK_CONST},    // Global constant keyword
        {"break", TOK_BREAK},    // Break from loop
        {"continue", TOK_CONTINUE}, // Continue loop
        {"defer", TOK_DEFER},    // Deferred execution at scope exit
        {"emit", TOK_EMIT},      // Emit value from cycle expression
        {"manme", TOK_MANME},    // manme block for HDCE memory management
        // Types
        {"int", TOK_INT},
        {"deci", TOK_DECI},
        {"char", TOK_CHAR},
        {"str", TOK_STR},
        {"bool", TOK_BOOL},
        {"byte", TOK_BYTE},
        {"ubyte", TOK_UBYTE},
        {"none", TOK_NONE},
        {"true", TOK_TRUE},
        {"false", TOK_FALSE},
        {"and", TOK_AND_KW},     // Logical AND keyword
        {"xor", TOK_XOR},        // Logical XOR keyword
        {"not", TOK_NOT},        // Logical NOT keyword
        {NULL, TOK_ERROR}
    };
    
    for (int i = 0; keywords[i].word != NULL; i++) {
        if (strcmp(str, keywords[i].word) == 0) {
            *type = keywords[i].type;
            return true;
        }
    }
    return false;
}

Lexer *lexer_new(const char *source) {
    Lexer *lexer = xmalloc(sizeof(Lexer));
    lexer->source = source;
    lexer->pos = 0;
    lexer->line = 1;
    lexer->col = 1;
    lexer->length = strlen(source);
    return lexer;
}

void lexer_free(Lexer *lexer) {
    free(lexer);
}

void token_free(Token *token) {
    if (token) {
        free(token->value);
        free(token);
    }
}

static char lexer_current(Lexer *lexer) {
    if (lexer->pos >= lexer->length) return '\0';
    return lexer->source[lexer->pos];
}

static char lexer_peek(Lexer *lexer, int offset) {
    int pos = lexer->pos + offset;
    if (pos >= lexer->length) return '\0';
    return lexer->source[pos];
}

static void lexer_advance(Lexer *lexer) {
    if (lexer->pos >= lexer->length) return;
    
    if (lexer->source[lexer->pos] == '\n') {
        lexer->line++;
        lexer->col = 1;
    } else {
        lexer->col++;
    }
    lexer->pos++;
}

static void lexer_skip_whitespace(Lexer *lexer) {
    while (isspace(lexer_current(lexer))) {
        lexer_advance(lexer);
    }
}

static void lexer_skip_line_comment(Lexer *lexer) {
    // Skip until end of line
    while (lexer_current(lexer) != '\n' && lexer_current(lexer) != '\0') {
        lexer_advance(lexer);
    }
}

static void lexer_skip_multiline_comment(Lexer *lexer) {
    // Skip the opening "//>"
    lexer_advance(lexer); // skip second '/'
    lexer_advance(lexer); // skip '>'
    
    // Look for closing "<//"
    while (true) {
        if (lexer_current(lexer) == '\0') {
            error_at(lexer->line, lexer->col, "Unterminated multi-line comment");
        }
        
        if (lexer_current(lexer) == '<' && 
            lexer_peek(lexer, 1) == '/' && 
            lexer_peek(lexer, 2) == '/') {
            lexer_advance(lexer); // skip '<'
            lexer_advance(lexer); // skip '/'
            lexer_advance(lexer); // skip '/'
            break;
        }
        
        lexer_advance(lexer);
    }
}

static Token *token_new(TokenType type, const char *value, int line, int col) {
    Token *token = xmalloc(sizeof(Token));
    token->type = type;
    token->value = value ? xstrdup(value) : NULL;
    token->line = line;
    token->col = col;
    return token;
}

static Token *lexer_read_number(Lexer *lexer) {
    int start_line = lexer->line;
    int start_col = lexer->col;
    int start = lexer->pos;
    bool is_decimal = false;
    
    // Check for hex (0x), octal (0), or binary (0b) prefixes
    if (lexer_current(lexer) == '0') {
        char next = lexer_peek(lexer, 1);
        
        // Hexadecimal: 0x or 0X
        if (next == 'x' || next == 'X') {
            lexer_advance(lexer); // skip '0'
            lexer_advance(lexer); // skip 'x' or 'X'
            
            // Read hex digits
            while (isxdigit(lexer_current(lexer))) {
                lexer_advance(lexer);
            }
            
            int len = lexer->pos - start;
            char *value = xmalloc(len + 1);
            strncpy(value, &lexer->source[start], len);
            value[len] = '\0';
            
            Token *token = token_new(TOK_NUMBER, value, start_line, start_col);
            free(value);
            return token;
        }
        
        // Binary: 0b or 0B
        if (next == 'b' || next == 'B') {
            lexer_advance(lexer); // skip '0'
            lexer_advance(lexer); // skip 'b' or 'B'
            
            // Read binary digits
            while (lexer_current(lexer) == '0' || lexer_current(lexer) == '1') {
                lexer_advance(lexer);
            }
            
            int len = lexer->pos - start;
            char *value = xmalloc(len + 1);
            strncpy(value, &lexer->source[start], len);
            value[len] = '\0';
            
            Token *token = token_new(TOK_NUMBER, value, start_line, start_col);
            free(value);
            return token;
        }
        
        // Octal: 0 followed by octal digits (or just 0)
        // Check if it looks like octal (0 followed by digits 0-7)
        if (isdigit(next) && next != '8' && next != '9') {
            // Could be octal
            lexer_advance(lexer); // skip first '0'
            
            // Read octal digits
            while (isdigit(lexer_current(lexer)) && lexer_current(lexer) != '8' && lexer_current(lexer) != '9') {
                lexer_advance(lexer);
            }
            
            // If we hit an 8 or 9, treat as decimal (e.g., 089 → error, but we'll let it be handled)
            if (lexer_current(lexer) == '8' || lexer_current(lexer) == '9' || lexer_current(lexer) == '.') {
                // Fall through to decimal handling
                while (isdigit(lexer_current(lexer)) || lexer_current(lexer) == '.') {
                    if (lexer_current(lexer) == '.') {
                        if (is_decimal) break;
                        is_decimal = true;
                    }
                    lexer_advance(lexer);
                }
            }
            
            int len = lexer->pos - start;
            char *value = xmalloc(len + 1);
            strncpy(value, &lexer->source[start], len);
            value[len] = '\0';
            
            Token *token = token_new(is_decimal ? TOK_DECIMAL : TOK_NUMBER, value, start_line, start_col);
            free(value);
            return token;
        }
    }
    
    // Decimal number (standard handling)
    while (isdigit(lexer_current(lexer)) || lexer_current(lexer) == '.') {
        if (lexer_current(lexer) == '.') {
            if (is_decimal) break;
            is_decimal = true;
        }
        lexer_advance(lexer);
    }
    
    int len = lexer->pos - start;
    char *value = xmalloc(len + 1);
    strncpy(value, &lexer->source[start], len);
    value[len] = '\0';
    
    Token *token = token_new(is_decimal ? TOK_DECIMAL : TOK_NUMBER, value, start_line, start_col);
    free(value);
    return token;
}

static Token *lexer_read_string(Lexer *lexer) {
    int start_line = lexer->line;
    int start_col = lexer->col;
    
    lexer_advance(lexer); // skip opening "
    
    // Build the string with dynamic allocation and bounds checking
    int capacity = 256;
    int result_len = 0;
    char *result = xmalloc(capacity);
    int has_interpolation = 0;
    (void)has_interpolation;  // May be used for future features
    
    // Maximum string size: 1MB to prevent DoS
    const int MAX_STRING_SIZE = 1024 * 1024;
    
    while (lexer_current(lexer) != '"' && lexer_current(lexer) != '\0') {
        // Safety check: prevent extremely large strings
        if (result_len >= MAX_STRING_SIZE) {
            free(result);
            error_at(start_line, start_col, "String literal exceeds maximum size (1MB)");
        }
        
        // Resize buffer if needed
        if (result_len >= capacity - 1) {
            capacity *= 2;
            if (capacity > MAX_STRING_SIZE) capacity = MAX_STRING_SIZE;
            result = xrealloc(result, capacity);
        }
        
        if (lexer_current(lexer) == '\\') {
            lexer_advance(lexer); // skip backslash
            char next = lexer_current(lexer);
            
            // Handle all escape sequences
            if (next == '$') {
                result[result_len++] = '$';
                lexer_advance(lexer);
            } else if (next == 'n') {
                result[result_len++] = '\n';  // 0x0A
                lexer_advance(lexer);
            } else if (next == 't') {
                result[result_len++] = '\t';  // 0x09
                lexer_advance(lexer);
            } else if (next == 'r') {
                result[result_len++] = '\r';  // 0x0D
                lexer_advance(lexer);
            } else if (next == 'b') {
                result[result_len++] = '\b';  // 0x08
                lexer_advance(lexer);
            } else if (next == 'v') {
                result[result_len++] = '\v';  // 0x0B
                lexer_advance(lexer);
            } else if (next == 'f') {
                result[result_len++] = '\f';  // 0x0C
                lexer_advance(lexer);
            } else if (next == 'a') {
                result[result_len++] = '\a';  // 0x07
                lexer_advance(lexer);
            } else if (next == '\\') {
                result[result_len++] = '\\';
                lexer_advance(lexer);
            } else if (next == '"') {
                result[result_len++] = '"';
                lexer_advance(lexer);
            } else {
                // Unknown escape, just keep the character
                result[result_len++] = next;
                lexer_advance(lexer);
            }
        } else if (lexer_current(lexer) == '$') {
            // Interpolation marker - keep it in the string for parser to handle
            result[result_len++] = lexer_current(lexer);
            lexer_advance(lexer);
            has_interpolation = 1;
        } else {
            result[result_len++] = lexer_current(lexer);
            lexer_advance(lexer);
        }
    }
    
    if (lexer_current(lexer) == '\0') {
        free(result);
        error_at(start_line, start_col, "Unterminated string literal");
    }
    
    result[result_len] = '\0';
    lexer_advance(lexer); // skip closing "
    
    Token *token = token_new(TOK_STRING, result, start_line, start_col);
    free(result);
    return token;
}

static Token *lexer_read_char(Lexer *lexer) {
    int start_line = lexer->line;
    int start_col = lexer->col;
    
    lexer_advance(lexer); // skip opening '
    
    char c = lexer_current(lexer);
    if (c == '\\') {
        lexer_advance(lexer);
        c = lexer_current(lexer);
    }
    
    char value[2] = {c, '\0'};
    lexer_advance(lexer);
    
    if (lexer_current(lexer) != '\'') {
        error_at(start_line, start_col, "Unterminated character literal");
    }
    lexer_advance(lexer); // skip closing '
    
    return token_new(TOK_CHAR_LIT, value, start_line, start_col);
}

static Token *lexer_read_identifier(Lexer *lexer) {
    int start_line = lexer->line;
    int start_col = lexer->col;
    int start = lexer->pos;
    
    while (isalnum(lexer_current(lexer)) || lexer_current(lexer) == '_') {
        lexer_advance(lexer);
    }
    
    int len = lexer->pos - start;
    char *value = xmalloc(len + 1);
    strncpy(value, &lexer->source[start], len);
    value[len] = '\0';
    
    TokenType type;
    if (is_keyword(value, &type)) {
        Token *token = token_new(type, value, start_line, start_col);
        free(value);
        return token;
    }
    
    Token *token = token_new(TOK_IDENT, value, start_line, start_col);
    free(value);
    return token;
}

Token *lexer_next_token(Lexer *lexer) {
    lexer_skip_whitespace(lexer);
    
    // Handle comments
    if (lexer_current(lexer) == '/' && lexer_peek(lexer, 1) == '/') {
        // Check for multi-line comment //> ... <//
        if (lexer_peek(lexer, 2) == '>') {
            lexer_advance(lexer); // skip first '/'
            lexer_skip_multiline_comment(lexer);
            return lexer_next_token(lexer);
        }
        // Single-line comment
        lexer_skip_line_comment(lexer);
        return lexer_next_token(lexer);
    }
    
    int line = lexer->line;
    int col = lexer->col;
    char c = lexer_current(lexer);
    
    if (c == '\0') {
        return token_new(TOK_EOF, NULL, line, col);
    }
    
    // Numbers
    if (isdigit(c)) {
        return lexer_read_number(lexer);
    }
    
    // Strings
    if (c == '"') {
        return lexer_read_string(lexer);
    }
    
    // Characters
    if (c == '\'') {
        return lexer_read_char(lexer);
    }
    
    // Identifiers and keywords
    if (isalpha(c) || c == '_') {
        return lexer_read_identifier(lexer);
    }
    
    // Three-character operators
    if (c == '<' && lexer_peek(lexer, 1) == '<' && lexer_peek(lexer, 2) == '=') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        lexer_advance(lexer);
        return token_new(TOK_LSHIFT_ASSIGN, "<<=", line, col);
    }
    
    if (c == '>' && lexer_peek(lexer, 1) == '>' && lexer_peek(lexer, 2) == '=') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        lexer_advance(lexer);
        return token_new(TOK_RSHIFT_ASSIGN, ">>=", line, col);
    }
    
    // Two-character operators
    if (c == ':' && lexer_peek(lexer, 1) == ':') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return token_new(TOK_COLONCOLON, "::", line, col);
    }
    
    if (c == '=' && lexer_peek(lexer, 1) == '=') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return token_new(TOK_EQ, "==", line, col);
    }
    
    if (c == '!' && lexer_peek(lexer, 1) == '=') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return token_new(TOK_NEQ, "!=", line, col);
    }
    
    if (c == '<' && lexer_peek(lexer, 1) == '=') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return token_new(TOK_LTE, "<=", line, col);
    }
    
    if (c == '>' && lexer_peek(lexer, 1) == '=') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return token_new(TOK_GTE, ">=", line, col);
    }
    
    if (c == '+' && lexer_peek(lexer, 1) == '+') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return token_new(TOK_PLUSPLUS, "++", line, col);
    }
    
    if (c == '-' && lexer_peek(lexer, 1) == '-') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return token_new(TOK_MINUSMINUS, "--", line, col);
    }
    
    // Logical operators
    if (c == '&' && lexer_peek(lexer, 1) == '&') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return token_new(TOK_AND, "&&", line, col);
    }
    
    if (c == '|' && lexer_peek(lexer, 1) == '|') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return token_new(TOK_LOG_OR, "||", line, col);
    }
    
    // Shift operators
    if (c == '<' && lexer_peek(lexer, 1) == '<') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return token_new(TOK_LSHIFT, "<<", line, col);
    }
    
    if (c == '>' && lexer_peek(lexer, 1) == '>') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return token_new(TOK_RSHIFT, ">>", line, col);
    }
    
    // Compound assignment operators
    if (c == '+' && lexer_peek(lexer, 1) == '=') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return token_new(TOK_PLUS_ASSIGN, "+=", line, col);
    }
    
    if (c == '-' && lexer_peek(lexer, 1) == '>') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return token_new(TOK_ARROW, "->", line, col);
    }
    
    if (c == '-' && lexer_peek(lexer, 1) == '=') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return token_new(TOK_MINUS_ASSIGN, "-=", line, col);
    }
    
    if (c == '*' && lexer_peek(lexer, 1) == '=') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return token_new(TOK_STAR_ASSIGN, "*=", line, col);
    }
    
    if (c == '/' && lexer_peek(lexer, 1) == '=') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return token_new(TOK_SLASH_ASSIGN, "/=", line, col);
    }
    
    if (c == '%' && lexer_peek(lexer, 1) == '=') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return token_new(TOK_PERCENT_ASSIGN, "%=", line, col);
    }
    
    if (c == '&' && lexer_peek(lexer, 1) == '=') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return token_new(TOK_AND_ASSIGN, "&=", line, col);
    }
    
    if (c == '|' && lexer_peek(lexer, 1) == '=') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return token_new(TOK_OR_ASSIGN, "|=", line, col);
    }
    
    if (c == '^' && lexer_peek(lexer, 1) == '=') {
        lexer_advance(lexer);
        lexer_advance(lexer);
        return token_new(TOK_XOR_ASSIGN, "^=", line, col);
    }
    
    // Single-character tokens
    lexer_advance(lexer);
    switch (c) {
        case '+': return token_new(TOK_PLUS, "+", line, col);
        case '-': return token_new(TOK_MINUS, "-", line, col);
        case '*': return token_new(TOK_STAR, "*", line, col);
        case '/': return token_new(TOK_SLASH, "/", line, col);
        case '%': return token_new(TOK_PERCENT, "%", line, col);
        case '=': return token_new(TOK_ASSIGN, "=", line, col);
        case '<': return token_new(TOK_LT, "<", line, col);
        case '>': return token_new(TOK_GT, ">", line, col);
        case '!': return token_new(TOK_NOT, "!", line, col);
        case '&': return token_new(TOK_BIT_AND, "&", line, col);
        case '|': return token_new(TOK_BIT_OR, "|", line, col);
        case '^': return token_new(TOK_BIT_XOR, "^", line, col);
        case '~': return token_new(TOK_BIT_NOT, "~", line, col);
        case '?': return token_new(TOK_QUESTION, "?", line, col);
        case '(': return token_new(TOK_LPAREN, "(", line, col);
        case ')': return token_new(TOK_RPAREN, ")", line, col);
        case '[': return token_new(TOK_LBRACKET, "[", line, col);
        case ']': return token_new(TOK_RBRACKET, "]", line, col);
        case '{': return token_new(TOK_LBRACE, "{", line, col);
        case '}': return token_new(TOK_RBRACE, "}", line, col);
        case ';': return token_new(TOK_SEMICOLON, ";", line, col);
        case ':': return token_new(TOK_COLON, ":", line, col);
        case ',': return token_new(TOK_COMMA, ",", line, col);
        case '.': return token_new(TOK_DOT, ".", line, col);
        case '@': return token_new(TOK_AT, "@", line, col);
        default:
            fprintf(stderr, "Unknown character: '%c'\n", c);
            return token_new(TOK_ERROR, NULL, line, col);
    }
}

const char *token_type_to_string(TokenType type) {
    switch (type) {
        case TOK_FNC: return "TOK_FNC";
        case TOK_GET: return "TOK_GET";
        case TOK_SHOW: return "TOK_SHOW";
        case TOK_SHOWF: return "TOK_SHOWF";
        case TOK_IF: return "TOK_IF";
        case TOK_OR: return "TOK_OR";
        case TOK_WHILE: return "TOK_WHILE";
        case TOK_LOOP: return "TOK_LOOP";
        case TOK_BREAK: return "TOK_BREAK";
        case TOK_CONTINUE: return "TOK_CONTINUE";
        case TOK_DEFER: return "TOK_DEFER";
        case TOK_INT: return "TOK_INT";
        case TOK_STR: return "TOK_STR";
        case TOK_NONE: return "TOK_NONE";
        case TOK_IDENT: return "TOK_IDENT";
        case TOK_NUMBER: return "TOK_NUMBER";
        case TOK_STRING: return "TOK_STRING";
        case TOK_LBRACE: return "TOK_LBRACE";
        case TOK_RBRACE: return "TOK_RBRACE";
        case TOK_SEMICOLON: return "TOK_SEMICOLON";
        case TOK_DOT: return "TOK_DOT";
        case TOK_ARROW: return "TOK_ARROW";
        case TOK_EOF: return "TOK_EOF";
        default: return "UNKNOWN";
    }
}