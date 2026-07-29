#ifndef LEXER_H
#define LEXER_H

#include "common.h"

// Token types
typedef enum {
    // Keywords
    TOK_FNC,        // fnc
    TOK_GET,        // get
    TOK_LOOP,       // loop
    TOK_IF,         // if
    TOK_OR,         // or (else-if/else, legacy)
    TOK_ELSE,       // else (else/else-if)
    TOK_WHILE,      // while
    TOK_CYCLE,      // cycle
    TOK_WHEN,       // when
    TOK_FIXED,      // fixed
    TOK_CHECK,      // check
    TOK_READ,       // read
    TOK_SHOW,       // show
    TOK_SHOWF,      // showf
    TOK_GROUP,      // group
    TOK_SET,        // set
    TOK_ARR,        // arr
    TOK_MAP,        // map
    TOK_PKG,        // pkg (package import)
    TOK_USE,        // use (package usage)
    TOK_CONST,      // const (global constant)
    TOK_BREAK,      // break (loop break)
    TOK_CONTINUE,   // continue (loop continue)
    TOK_DEFER,      // defer (deferred statement execution)
    TOK_EMIT,       // emit (cycle expression value setter)
    TOK_MANME,      // manme (HDCE memory management block)
    
    // Types
    TOK_INT,        // int
    TOK_DECI,       // deci
    TOK_CHAR,       // char
    TOK_STR,        // str
    TOK_BOOL,       // bool
    TOK_BYTE,       // byte
    TOK_UBYTE,      // ubyte
    TOK_NONE,       // none
    
    // Literals
    TOK_NUMBER,     // 123
    TOK_DECIMAL,    // 3.14
    TOK_STRING,     // "hello"
    TOK_CHAR_LIT,   // 'a'
    TOK_TRUE,       // true
    TOK_FALSE,      // false
    
    // Identifiers
    TOK_IDENT,      // variable/function names
    
    // Operators - Arithmetic
    TOK_PLUS,       // +
    TOK_MINUS,      // -
    TOK_STAR,       // *
    TOK_SLASH,      // /
    TOK_PERCENT,    // %
    
    // Operators - Comparison
    TOK_EQ,         // ==
    TOK_NEQ,        // !=
    TOK_LT,         // <
    TOK_GT,         // >
    TOK_LTE,        // <=
    TOK_GTE,        // >=
    
    // Operators - Logical
    TOK_AND,        // &&
    TOK_LOG_OR,     // || (TOK_OR conflicts with 'or' keyword)
    TOK_NOT,        // !
    TOK_AND_KW,     // and (keyword)
    TOK_XOR,        // xor (keyword)
    
    // Operators - Bitwise
    TOK_BIT_AND,    // &
    TOK_BIT_OR,     // |
    TOK_BIT_XOR,    // ^
    TOK_BIT_NOT,    // ~
    TOK_LSHIFT,     // <<
    TOK_RSHIFT,     // >>
    
    // Operators - Assignment
    TOK_ASSIGN,     // =
    TOK_PLUS_ASSIGN,  // +=
    TOK_MINUS_ASSIGN, // -=
    TOK_STAR_ASSIGN,  // *=
    TOK_SLASH_ASSIGN, // /=
    TOK_PERCENT_ASSIGN, // %=
    TOK_AND_ASSIGN,   // &=
    TOK_OR_ASSIGN,    // |=
    TOK_XOR_ASSIGN,   // ^=
    TOK_LSHIFT_ASSIGN, // <<=
    TOK_RSHIFT_ASSIGN, // >>=
    
    // Operators - Increment/Decrement
    TOK_PLUSPLUS,   // ++
    TOK_MINUSMINUS, // --
    
    // Operators - Ternary
    TOK_QUESTION,   // ?
    
    // Delimiters
    TOK_LPAREN,     // (
    TOK_RPAREN,     // )
    TOK_LBRACKET,   // [
    TOK_RBRACKET,   // ]
    TOK_LBRACE,     // {
    TOK_RBRACE,     // }
    TOK_SEMICOLON,  // ;
    TOK_COLON,      // :
    TOK_COLONCOLON, // ::
    TOK_COMMA,      // ,
    TOK_DOT,        // .
    TOK_ARROW,      // -> (map access)
    TOK_AT,         // @ (builtin function prefix)
    
    // Special
    TOK_EOF,        // End of file
    TOK_ERROR       // Error token
} TokenType;

typedef struct {
    TokenType type;
    char *value;
    int line;
    int col;
} Token;

typedef struct {
    const char *source;
    int pos;
    int line;
    int col;
    int length;
} Lexer;

// Lexer functions
Lexer *lexer_new(const char *source);
void lexer_free(Lexer *lexer);
Token *lexer_next_token(Lexer *lexer);
void token_free(Token *token);
const char *token_type_to_string(TokenType type);

#endif // LEXER_H