#ifndef PARSER_H
#define PARSER_H

#include "common.h"
#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer *lexer;
    Token *previous;  // Track previous token for accurate error reporting
    Token *current;
    Token *peek;
    int depth;        // SECURITY: Track recursion depth to prevent stack overflow
} Parser;

// Parser functions
Parser *parser_new(Lexer *lexer);
void parser_free(Parser *parser);
ASTNode *parser_parse(Parser *parser);

#endif // PARSER_H
