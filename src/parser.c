#include "../include/parser.h"

/* Helper macros for enhanced error reporting */
#define ERROR_WITH_HINT(parser, line, col, msg, hint) \
    parser_error_with_context((parser)->lexer, line, col, msg, hint)

#define ERROR_WITH_CONTEXT(parser, line, col, msg) \
    parser_error_with_context((parser)->lexer, line, col, msg, NULL)

/* Forward declaration for context-aware error reporting */
static void parser_error_with_context(Lexer *lexer, int line, int col, const char *msg, const char *hint);

static void parser_advance(Parser *parser) {
    token_free(parser->previous);  // Free the old previous token
    parser->previous = parser->current;  // Move current to previous
    parser->current = parser->peek;
    parser->peek = lexer_next_token(parser->lexer);
}

static bool parser_check(Parser *parser, TokenType type) {
    return parser->current->type == type;
}

static bool parser_match(Parser *parser, TokenType type) {
    if (parser_check(parser, type)) {
        parser_advance(parser);
        return true;
    }
    return false;
}

static void parser_expect(Parser *parser, TokenType type, const char *msg) {
    if (!parser_match(parser, type)) {
        // Report error at the end of the previous token (where we expected to find this token)
        if (parser->previous) {
            ERROR_WITH_CONTEXT(parser, parser->previous->line, parser->previous->col, msg);
        } else {
            ERROR_WITH_CONTEXT(parser, parser->current->line, parser->current->col, msg);
        }
    }
}

// Forward declarations
static ASTNode *parse_statement(Parser *parser);
static ASTNode *parse_expression(Parser *parser);
static ASTNode *parse_primary(Parser *parser);
static ASTNode *parse_read(Parser *parser);
static ASTNode *parse_cycle(Parser *parser);
ASTNode *parse_package_import(Parser *parser);
static ASTNode *parse_const_decl(Parser *parser);

/* Extract a single line from source code by line number */
static char *extract_source_line(const char *source, int target_line, int *out_len) {
    if (!source || target_line < 1) {
        return NULL;
    }
    
    int current_line = 1;
    const char *line_start = source;
    
    /* Find the start of the target line */
    for (const char *p = source; *p != '\0'; p++) {
        if (current_line == target_line) {
            line_start = p;
            break;
        }
        if (*p == '\n') {
            current_line++;
            if (current_line > target_line) {
                return NULL;  /* Line not found */
            }
        }
    }
    
    if (current_line != target_line) {
        return NULL;  /* Line not found */
    }
    
    /* Find the end of the line */
    const char *line_end = line_start;
    while (*line_end != '\0' && *line_end != '\n') {
        line_end++;
    }
    
    *out_len = line_end - line_start;
    
    char *result = malloc(*out_len + 1);
    if (!result) {
        return NULL;
    }
    
    strncpy(result, line_start, *out_len);
    result[*out_len] = '\0';
    
    return result;
}

/* Display error with full source context, hints, and line pointer */
static void parser_error_with_context(Lexer *lexer, int line, int col, const char *msg, const char *hint) {
    fprintf(stderr, "\n");
    fprintf(stderr, "ERROR at line %d, column %d:\n", line, col);
    fprintf(stderr, "  %s\n", msg);
    
    if (!lexer || !lexer->source) {
        fprintf(stderr, "\n");
        if (hint) {
            fprintf(stderr, "Hint: %s\n", hint);
        }
        fprintf(stderr, "\n");
        exit(1);
    }
    
    /* Extract and display the problematic line */
    int line_len = 0;
    char *error_line = extract_source_line(lexer->source, line, &line_len);
    
    if (error_line) {
        fprintf(stderr, "\n");
        fprintf(stderr, "  %s\n", error_line);
        
        /* Display caret(s) pointing to the error location */
        fprintf(stderr, "  ");
        for (int i = 1; i < col; i++) {
            fprintf(stderr, " ");
        }
        fprintf(stderr, "^^^ Error location\n");
        
        free(error_line);
    }
    
    if (hint) {
        fprintf(stderr, "\n");
        fprintf(stderr, "Hint: %s\n", hint);
    }
    
    fprintf(stderr, "\n");
    exit(1);
}

Parser *parser_new(Lexer *lexer) {
    Parser *parser = xmalloc(sizeof(Parser));
    parser->lexer = lexer;
    parser->previous = NULL;  // Initialize previous token to NULL
    parser->current = lexer_next_token(lexer);
    parser->peek = lexer_next_token(lexer);
    parser->depth = 0;  // SECURITY: Initialize recursion depth counter
    return parser;
}

void parser_free(Parser *parser) {
    if (parser->previous) {
        token_free(parser->previous);
    }
    token_free(parser->current);
    token_free(parser->peek);
    free(parser);
}

// Parse primary expression
static ASTNode *parse_primary(Parser *parser) {
    ASTNode *node;
    
    // Number literal
    if (parser_check(parser, TOK_NUMBER)) {
        node = ast_node_new(AST_LITERAL);
        node->literal.value = xstrdup(parser->current->value);
        node->literal.type = xstrdup("int");
        parser_advance(parser);
        return node;
    }
    
    // Decimal literal (floating point)
    if (parser_check(parser, TOK_DECIMAL)) {
        node = ast_node_new(AST_LITERAL);
        node->literal.value = xstrdup(parser->current->value);
        node->literal.type = xstrdup("deci");
        parser_advance(parser);
        return node;
    }
    
    // String literal
    if (parser_check(parser, TOK_STRING)) {
        node = ast_node_new(AST_LITERAL);
        node->literal.value = xstrdup(parser->current->value);
        node->literal.type = xstrdup("str");
        parser_advance(parser);
        return node;
    }
    
    // Character literal: 'A', '!', '@', etc.
    if (parser_check(parser, TOK_CHAR_LIT)) {
        node = ast_node_new(AST_LITERAL);
        // Convert character to its ASCII numeric value
        char ch = parser->current->value[0];
        char ascii_value[16];
        snprintf(ascii_value, sizeof(ascii_value), "%d", (unsigned char)ch);
        node->literal.value = xstrdup(ascii_value);
        node->literal.type = xstrdup("char");
        parser_advance(parser);
        return node;
    }
    
    // Boolean literals
    if (parser_check(parser, TOK_TRUE) || parser_check(parser, TOK_FALSE)) {
        node = ast_node_new(AST_LITERAL);
        node->literal.value = xstrdup(parser->current->value);
        node->literal.type = xstrdup("bool");
        parser_advance(parser);
        return node;
    }
    
    // Read expression: read["prompt"]
    if (parser_check(parser, TOK_READ)) {
        return parse_read(parser);
    }
    
    // Cycle expression: cycle[value] { when[case]: { emit[result]; } ... fixed: { emit[result]; } }
    if (parser_check(parser, TOK_CYCLE)) {
        return parse_cycle(parser);
    }
    
    // Builtin function call: @function[args] or @type[args]::target_type
    if (parser_check(parser, TOK_AT)) {
        parser_advance(parser); // consume '@'
        
        // Accept both identifiers and keywords as builtin function names
        // (e.g., @type, @len, etc.)
        if (!parser_check(parser, TOK_IDENT) && 
            !parser_check(parser, TOK_INT) &&
            !parser_check(parser, TOK_DECI) &&
            !parser_check(parser, TOK_BYTE) &&
            !parser_check(parser, TOK_BOOL) &&
            !parser_check(parser, TOK_STR) &&
            !parser_check(parser, TOK_CHAR) &&
            !parser_check(parser, TOK_SHOW) &&
            !parser_check(parser, TOK_READ)) {
            ERROR_WITH_HINT(parser, parser->current->line, parser->current->col, 
                    "Expected builtin function name after '@'",
                    "Builtin functions: @type, @len, @size, @addr, @peek, @poke, @show, @read, etc.");
        }
        
        char *builtin_name = xstrdup(parser->current->value);
        parser_advance(parser);
        
        parser_expect(parser, TOK_LBRACKET, "Expected '[' after builtin function name");
        
        node = ast_node_new(AST_BUILTIN_CALL);
        node->builtin_call.name = builtin_name;
        node->builtin_call.args = ast_list_new();
        
        if (!parser_check(parser, TOK_RBRACKET)) {
            do {
                ASTNode *arg = parse_expression(parser);
                ast_list_add(node->builtin_call.args, arg);
            } while (parser_match(parser, TOK_COMMA));
        }
        
        parser_expect(parser, TOK_RBRACKET, "Expected ']' after builtin function arguments");
        
        // Check for :: type specification (only for @type builtin)
        if (strcmp(builtin_name, "type") == 0 && parser_match(parser, TOK_COLONCOLON)) {
            // Parse target types separated by commas
            if (!parser_check(parser, TOK_IDENT) && !parser_check(parser, TOK_INT) &&
                !parser_check(parser, TOK_DECI) && !parser_check(parser, TOK_BYTE) &&
                !parser_check(parser, TOK_BOOL) && !parser_check(parser, TOK_STR) &&
                !parser_check(parser, TOK_CHAR)) {
                error_at(parser->current->line, parser->current->col, "Expected type name after '::'");
            }
            
            char *target_types = xstrdup(parser->current->value);
            parser_advance(parser);
            
            // Allow multiple types separated by commas
            while (parser_match(parser, TOK_COMMA)) {
                if (!parser_check(parser, TOK_IDENT) && !parser_check(parser, TOK_INT) &&
                    !parser_check(parser, TOK_DECI) && !parser_check(parser, TOK_BYTE) &&
                    !parser_check(parser, TOK_BOOL) && !parser_check(parser, TOK_STR) &&
                    !parser_check(parser, TOK_CHAR)) {
                    error_at(parser->current->line, parser->current->col, "Expected type name after comma");
                }
                
                size_t new_len = strlen(target_types) + strlen(parser->current->value) + 2;
                char *new_types = xmalloc(new_len);
                snprintf(new_types, new_len, "%s,%s", target_types, parser->current->value);
                free(target_types);
                target_types = new_types;
                parser_advance(parser);
            }
            
            node->builtin_call.target_type = target_types;
        }
        
        return node;
    }
    /*
// Identifier or function call or array access or member access
if (parser_check(parser, TOK_IDENT)) {
    char *name = xstrdup(parser->current->value);
    parser_advance(parser);

    // === ARRAY ACCESS IN EXPRESSIONS ===
    if (parser_check(parser, TOK_LBRACE)) {
        parser_advance(parser); // consume '{'
        ASTNode *index = parse_expression(parser);
        parser_expect(parser, TOK_RBRACE, "Expected '}' after array index");
        
        node = ast_node_new(AST_ARRAY_ACCESS);
        node->array_access.name = name;
        node->array_access.index = index;
        
        // Support arr{i}.member
        if (parser_match(parser, TOK_DOT)) {
            if (!parser_check(parser, TOK_IDENT)) {
                error_at(parser->current->line, parser->current->col, "Expected member name after '.'");
            }
            char *member_name = xstrdup(parser->current->value);
            parser_advance(parser);
            
            ASTNode *member_node = ast_node_new(AST_MEMBER_ACCESS);
            member_node->member_access.object = node;
            member_node->member_access.member_name = member_name;
            return member_node;
        }
        return node;
    }

    // Member access chain: name.member ...
    ASTNode *object = ast_node_new(AST_IDENT);
    object->ident.name = name;

    // Handle chained member access
    while (parser_match(parser, TOK_DOT)) {
        if (!parser_check(parser, TOK_IDENT)) {
            error_at(parser->current->line, parser->current->col, "Expected member name after '.'");
        }
        char *member_name = xstrdup(parser->current->value);
        parser_advance(parser);

        ASTNode *member_node = ast_node_new(AST_MEMBER_ACCESS);
        member_node->member_access.object = object;
        member_node->member_access.member_name = member_name;
        object = member_node;
    }
        // If we created any member access nodes, check for function calls on it
        if (object->type == AST_MEMBER_ACCESS) {
            // Check for function call: module.function[args]
            if (parser_check(parser, TOK_LBRACKET)) {
                // Build qualified name from member access chain
                // Walk up the chain to get all parts: base.member1.member2...
                char qualified_name[512] = {0};
                ASTNode *current = object;
                
                // Collect parts in reverse order (from leaf to root)
                ASTList *parts = ast_list_new();
                while (current->type == AST_MEMBER_ACCESS) {
                    ast_list_add(parts, (ASTNode*)current->member_access.member_name);
                    current = current->member_access.object;
                }
                // Add base identifier
                if (current->type == AST_IDENT) {
                    ast_list_add(parts, (ASTNode*)current->ident.name);
                }
                
                // Build name in correct order (root.member1.member2...)
                for (int i = parts->count - 1; i >= 0; i--) {
                    char *part = (char*)parts->nodes[i];
                    if (i < parts->count - 1) strcat(qualified_name, ".");
                    strcat(qualified_name, part);
                }
                
                // Free the temporary list (note: don't free the strings themselves)
                free(parts->nodes);
                free(parts);
                
                parser_advance(parser); // consume '['
                
                node = ast_node_new(AST_CALL);
                node->call.name = xstrdup(qualified_name);
                node->call.args = ast_list_new();
                
                if (!parser_check(parser, TOK_RBRACKET)) {
                    do {
                        ASTNode *arg = parse_expression(parser);
                        ast_list_add(node->call.args, arg);
                    } while (parser_match(parser, TOK_COMMA));
                }
                
                parser_expect(parser, TOK_RBRACKET, "Expected ']' after function arguments");
                
                return node;
            }
            return object;
        }
        
        // Handle other identifier operations (function calls, array access, map operations)
        node = object;
        name = ((ASTNode *)object)->ident.name;  // Get name back from the identifier node
        
        // Map operations: name->get[key], name->has[key]
        if (parser_match(parser, TOK_ARROW)) {  
            // Allow map operation keywords and identifiers: set, get, has, remove
            if (!parser_check(parser, TOK_SET) &&
                !parser_check(parser, TOK_GET) &&
                !parser_check(parser, TOK_IDENT)) {
                error_at(parser->current->line, parser->current->col, "Expected map operation (get, has, set, remove) after '->'");
            }
            
            char *operation = xstrdup(parser->current->value);
            parser_advance(parser);
            
            // Map get: map->get[key]
            if (strcmp(operation, "get") == 0 && parser_check(parser, TOK_LBRACKET)) {
                parser_advance(parser); // consume '['
                
                ASTNode *key = parse_expression(parser);
                parser_expect(parser, TOK_RBRACKET, "Expected ']' after map key");
                
                node = ast_node_new(AST_MAP_GET);
                node->map_get.map_name = name;
                node->map_get.key = key;
                free(operation);
                return node;
            } else if (strcmp(operation, "has") == 0 && parser_check(parser, TOK_LBRACKET)) {
                // Map has: map->has[key]
                parser_advance(parser); // consume '['
                
                ASTNode *key = parse_expression(parser);
                parser_expect(parser, TOK_RBRACKET, "Expected ']' after map key");
                
                node = ast_node_new(AST_MAP_HAS);
                node->map_has.map_name = name;
                node->map_has.key = key;
                free(operation);
                return node;
            } else {
                error_at(parser->current->line, parser->current->col, "Invalid map operation");
            }
        }
        
        // Array access: name{index} or name{index}.member (for group arrays)
        if (parser_match(parser, TOK_LBRACE)) {
            ASTNode *index = parse_expression(parser);
            parser_expect(parser, TOK_RBRACE, "Expected '}' after array index");
            
            node = ast_node_new(AST_ARRAY_ACCESS);
            node->array_access.name = name;
            node->array_access.index = index;
            
            // Check for member access after array access (for group array members)
            if (parser_match(parser, TOK_DOT)) {
                if (!parser_check(parser, TOK_IDENT)) {
                    error_at(parser->current->line, parser->current->col, "Expected member name after '.'");
                }
                
                char *member_name = xstrdup(parser->current->value);
                parser_advance(parser);
                
                // Create member access node with array access as object
                ASTNode *member_node = ast_node_new(AST_MEMBER_ACCESS);
                member_node->member_access.object = node;
                member_node->member_access.member_name = member_name;
                
                // Handle additional chaining (e.g., arr{i}.member1.member2)
                while (parser_match(parser, TOK_DOT)) {
                    if (!parser_check(parser, TOK_IDENT)) {
                        error_at(parser->current->line, parser->current->col, "Expected member name after '.'");
                    }
                    
                    char *next_member = xstrdup(parser->current->value);
                    parser_advance(parser);
                    
                    ASTNode *next_member_node = ast_node_new(AST_MEMBER_ACCESS);
                    next_member_node->member_access.object = member_node;
                    next_member_node->member_access.member_name = next_member;
                    
                    member_node = next_member_node;
                }
                
                return member_node;
            }
            
            return node;
        }
        
// Function call: name[]
    if (parser_match(parser, TOK_LBRACKET)) {
        node = ast_node_new(AST_CALL);
        node->call.name = name;
        node->call.args = ast_list_new();
        if (!parser_check(parser, TOK_RBRACKET)) {
            do {
                ASTNode *arg = parse_expression(parser);
                ast_list_add(node->call.args, arg);
            } while (parser_match(parser, TOK_COMMA));
        }
        parser_expect(parser, TOK_RBRACKET, "Expected ']' after function arguments");
        return node;
    }

    // Simple identifier
    return node;
}*/

// Identifier or function call or array access or member access
if (parser_check(parser, TOK_IDENT)) {
    char *name = xstrdup(parser->current->value);
    parser_advance(parser);

    // === ARRAY ACCESS IN EXPRESSIONS ===
    if (parser_check(parser, TOK_LBRACE)) {
        parser_advance(parser); // consume '{'
        ASTNode *index = parse_expression(parser);
        parser_expect(parser, TOK_RBRACE, "Expected '}' after array index");
        
        node = ast_node_new(AST_ARRAY_ACCESS);
        node->array_access.name = name;
        node->array_access.index = index;
        
        // Support arr{i}.member
        if (parser_match(parser, TOK_DOT)) {
            if (!parser_check(parser, TOK_IDENT)) {
                error_at(parser->current->line, parser->current->col, "Expected member name after '.'");
            }
            char *member_name = xstrdup(parser->current->value);
            parser_advance(parser);
            
            ASTNode *member_node = ast_node_new(AST_MEMBER_ACCESS);
            member_node->member_access.object = node;
            member_node->member_access.member_name = member_name;
            return member_node;
        }
        return node;
    }

    // Member access chain: name.member ...
    ASTNode *object = ast_node_new(AST_IDENT);
    object->ident.name = name;

    while (parser_match(parser, TOK_DOT)) {
        if (!parser_check(parser, TOK_IDENT)) {
            error_at(parser->current->line, parser->current->col, "Expected member name after '.'");
        }
        char *member_name = xstrdup(parser->current->value);
        parser_advance(parser);

        ASTNode *member_node = ast_node_new(AST_MEMBER_ACCESS);
        member_node->member_access.object = object;
        member_node->member_access.member_name = member_name;
        object = member_node;
    }

    // Function call on member access
    if (object->type == AST_MEMBER_ACCESS) {
        if (parser_check(parser, TOK_LBRACKET)) {
            // Build qualified name
            char qualified_name[512] = {0};
            ASTNode *current = object;
            ASTList *parts = ast_list_new();
            while (current->type == AST_MEMBER_ACCESS) {
                ast_list_add(parts, (ASTNode*)current->member_access.member_name);
                current = current->member_access.object;
            }
            if (current->type == AST_IDENT) {
                ast_list_add(parts, (ASTNode*)current->ident.name);
            }
            for (int i = parts->count - 1; i >= 0; i--) {
                char *part = (char*)parts->nodes[i];
                if (i < parts->count - 1) strcat(qualified_name, ".");
                strcat(qualified_name, part);
            }
            free(parts->nodes);
            free(parts);

            parser_advance(parser); // consume '['
            node = ast_node_new(AST_CALL);
            node->call.name = xstrdup(qualified_name);
            node->call.args = ast_list_new();
            if (!parser_check(parser, TOK_RBRACKET)) {
                do {
                    ASTNode *arg = parse_expression(parser);
                    ast_list_add(node->call.args, arg);
                } while (parser_match(parser, TOK_COMMA));
            }
            parser_expect(parser, TOK_RBRACKET, "Expected ']' after function arguments");
            return node;
        }
        return object;
    }

    // Regular identifier
    node = object;
    name = object->ident.name;

    // Map operations
    if (parser_match(parser, TOK_ARROW)) {
        if (!parser_check(parser, TOK_SET) &&
            !parser_check(parser, TOK_GET) &&
            !parser_check(parser, TOK_IDENT)) {
            error_at(parser->current->line, parser->current->col, "Expected map operation (get, has, set, remove) after '->'");
        }
        char *operation = xstrdup(parser->current->value);
        parser_advance(parser);

        if (strcmp(operation, "get") == 0 && parser_check(parser, TOK_LBRACKET)) {
            parser_advance(parser);
            ASTNode *key = parse_expression(parser);
            parser_expect(parser, TOK_RBRACKET, "Expected ']' after map key");
            node = ast_node_new(AST_MAP_GET);
            node->map_get.map_name = name;
            node->map_get.key = key;
            free(operation);
            return node;
        } else if (strcmp(operation, "has") == 0 && parser_check(parser, TOK_LBRACKET)) {
            parser_advance(parser);
            ASTNode *key = parse_expression(parser);
            parser_expect(parser, TOK_RBRACKET, "Expected ']' after map key");
            node = ast_node_new(AST_MAP_HAS);
            node->map_has.map_name = name;
            node->map_has.key = key;
            free(operation);
            return node;
        } else {
            error_at(parser->current->line, parser->current->col, "Invalid map operation");
        }
    }

    // Function call: name[]
    if (parser_match(parser, TOK_LBRACKET)) {
        node = ast_node_new(AST_CALL);
        node->call.name = name;
        node->call.args = ast_list_new();
        if (!parser_check(parser, TOK_RBRACKET)) {
            do {
                ASTNode *arg = parse_expression(parser);
                ast_list_add(node->call.args, arg);
            } while (parser_match(parser, TOK_COMMA));
        }
        parser_expect(parser, TOK_RBRACKET, "Expected ']' after function arguments");
        return node;
    }

    // Simple identifier
    return node;
}
    
    // Parenthesized expression
    if (parser_match(parser, TOK_LPAREN)) {
        node = parse_expression(parser);
        parser_expect(parser, TOK_RPAREN, "Expected ')' after expression");
        return node;
    }
    
    parser_error_with_context(parser->lexer, parser->current->line, parser->current->col, 
            "Unexpected token in expression",
            "Expected: literal, identifier, '(', '@builtin', or valid expression start");
    return NULL;
}

// ============================================================================
// Expression Parsing with Proper Operator Precedence
// ============================================================================
// Precedence (lowest to highest):
// 1. Ternary (?:)
// 2. Logical OR (||, xor)
// 3. Logical AND (&&, and)
// 4. Bitwise OR (|)
// 5. Bitwise XOR (^)
// 6. Bitwise AND (&)
// 7. Equality (==, !=)
// 8. Comparison (<, >, <=, >=)
// 9. Shift (<<, >>)
// 10. Additive (+, -)
// 11. Multiplicative (*, /, %)
// 12. Unary (+x, -x, !x, ~x, ++x, --x)
// 13. Primary (literals, identifiers, etc.)

// Forward declaration for recursion
static ASTNode *parse_expression(Parser *parser);

// Parse unary expressions: +x, -x, !x, ~x, ++x, --x
static ASTNode *parse_postfix(Parser *parser) {
    ASTNode *node = parse_primary(parser);
    
    // Check for postfix operators (++, --)
    while (parser_check(parser, TOK_PLUSPLUS) || parser_check(parser, TOK_MINUSMINUS)) {
        char *op = xstrdup(parser->current->value);
        parser_advance(parser);
        
        ASTNode *unary_node = ast_node_new(AST_UNARY_OP);
        unary_node->unary.op = op;
        unary_node->unary.operand = node;
        unary_node->unary.is_postfix = true;
        
        node = unary_node;
    }
    
    return node;
}

static ASTNode *parse_unary(Parser *parser) {
    // Check for prefix unary operators
    if (parser_check(parser, TOK_PLUS) || parser_check(parser, TOK_MINUS) ||
        parser_check(parser, TOK_NOT) || parser_check(parser, TOK_BIT_NOT) ||
        parser_check(parser, TOK_PLUSPLUS) || parser_check(parser, TOK_MINUSMINUS)) {
        
        char *op = xstrdup(parser->current->value);
        parser_advance(parser);
        
        ASTNode *operand = parse_unary(parser); // Right-associative recursion
        
        ASTNode *node = ast_node_new(AST_UNARY_OP);
        node->unary.op = op;
        node->unary.operand = operand;
        node->unary.is_postfix = false;  // Prefix by default
        return node;
    }
    
    // Otherwise, parse postfix expression
    return parse_postfix(parser);
}

// Parse multiplicative: *, /, %
static ASTNode *parse_multiplicative(Parser *parser) {
    ASTNode *left = parse_unary(parser);
    
    while (parser_check(parser, TOK_STAR) || parser_check(parser, TOK_SLASH) ||
           parser_check(parser, TOK_PERCENT)) {
        char *op = xstrdup(parser->current->value);
        parser_advance(parser);
        
        ASTNode *right = parse_unary(parser);
        
        ASTNode *node = ast_node_new(AST_BINARY_OP);
        node->binary.op = op;
        node->binary.left = left;
        node->binary.right = right;
        left = node;
    }
    
    return left;
}

// Parse additive: +, -
static ASTNode *parse_additive(Parser *parser) {
    ASTNode *left = parse_multiplicative(parser);
    
    while (parser_check(parser, TOK_PLUS) || parser_check(parser, TOK_MINUS)) {
        char *op = xstrdup(parser->current->value);
        parser_advance(parser);
        
        ASTNode *right = parse_multiplicative(parser);
        
        ASTNode *node = ast_node_new(AST_BINARY_OP);
        node->binary.op = op;
        node->binary.left = left;
        node->binary.right = right;
        left = node;
    }
    
    return left;
}

// Parse shift: <<, >>
static ASTNode *parse_shift(Parser *parser) {
    ASTNode *left = parse_additive(parser);
    
    while (parser_check(parser, TOK_LSHIFT) || parser_check(parser, TOK_RSHIFT)) {
        char *op = xstrdup(parser->current->value);
        parser_advance(parser);
        
        ASTNode *right = parse_additive(parser);
        
        ASTNode *node = ast_node_new(AST_BINARY_OP);
        node->binary.op = op;
        node->binary.left = left;
        node->binary.right = right;
        left = node;
    }
    
    return left;
}

// Parse comparison: <, >, <=, >=
static ASTNode *parse_comparison(Parser *parser) {
    ASTNode *left = parse_shift(parser);
    
    while (parser_check(parser, TOK_LT) || parser_check(parser, TOK_GT) ||
           parser_check(parser, TOK_LTE) || parser_check(parser, TOK_GTE)) {
        char *op = xstrdup(parser->current->value);
        parser_advance(parser);
        
        ASTNode *right = parse_shift(parser);
        
        ASTNode *node = ast_node_new(AST_BINARY_OP);
        node->binary.op = op;
        node->binary.left = left;
        node->binary.right = right;
        left = node;
    }
    
    return left;
}

// Parse equality: ==, !=
static ASTNode *parse_equality(Parser *parser) {
    ASTNode *left = parse_comparison(parser);
    
    while (parser_check(parser, TOK_EQ) || parser_check(parser, TOK_NEQ)) {
        char *op = xstrdup(parser->current->value);
        parser_advance(parser);
        
        ASTNode *right = parse_comparison(parser);
        
        ASTNode *node = ast_node_new(AST_BINARY_OP);
        node->binary.op = op;
        node->binary.left = left;
        node->binary.right = right;
        left = node;
    }
    
    return left;
}

// Parse bitwise AND: &
static ASTNode *parse_bitwise_and(Parser *parser) {
    ASTNode *left = parse_equality(parser);
    
    while (parser_check(parser, TOK_BIT_AND)) {
        char *op = xstrdup(parser->current->value);
        parser_advance(parser);
        
        ASTNode *right = parse_equality(parser);
        
        ASTNode *node = ast_node_new(AST_BINARY_OP);
        node->binary.op = op;
        node->binary.left = left;
        node->binary.right = right;
        left = node;
    }
    
    return left;
}

// Parse bitwise XOR: ^
static ASTNode *parse_bitwise_xor(Parser *parser) {
    ASTNode *left = parse_bitwise_and(parser);
    
    while (parser_check(parser, TOK_BIT_XOR)) {
        char *op = xstrdup(parser->current->value);
        parser_advance(parser);
        
        ASTNode *right = parse_bitwise_and(parser);
        
        ASTNode *node = ast_node_new(AST_BINARY_OP);
        node->binary.op = op;
        node->binary.left = left;
        node->binary.right = right;
        left = node;
    }
    
    return left;
}

// Parse bitwise OR: |
static ASTNode *parse_bitwise_or(Parser *parser) {
    ASTNode *left = parse_bitwise_xor(parser);
    
    while (parser_check(parser, TOK_BIT_OR)) {
        char *op = xstrdup(parser->current->value);
        parser_advance(parser);
        
        ASTNode *right = parse_bitwise_xor(parser);
        
        ASTNode *node = ast_node_new(AST_BINARY_OP);
        node->binary.op = op;
        node->binary.left = left;
        node->binary.right = right;
        left = node;
    }
    
    return left;
}

// Parse logical AND: &&, 'and'
static ASTNode *parse_logical_and(Parser *parser) {
    ASTNode *left = parse_bitwise_or(parser);
    
    while (parser_check(parser, TOK_AND) || parser_check(parser, TOK_AND_KW)) {
        char *op = xstrdup(parser->current->value);
        parser_advance(parser);
        
        ASTNode *right = parse_bitwise_or(parser);
        
        ASTNode *node = ast_node_new(AST_BINARY_OP);
        node->binary.op = op;
        node->binary.left = left;
        node->binary.right = right;
        left = node;
    }
    
    return left;
}

// Parse logical OR: ||, xor
static ASTNode *parse_logical_or(Parser *parser) {
    ASTNode *left = parse_logical_and(parser);
    
    while (parser_check(parser, TOK_LOG_OR) || parser_check(parser, TOK_XOR)) {
        char *op = xstrdup(parser->current->value);
        parser_advance(parser);
        
        ASTNode *right = parse_logical_and(parser);
        
        ASTNode *node = ast_node_new(AST_BINARY_OP);
        node->binary.op = op;
        node->binary.left = left;
        node->binary.right = right;
        left = node;
    }
    
    return left;
}

// Parse ternary conditional: condition ? true_expr : false_expr
static ASTNode *parse_ternary(Parser *parser) {
    ASTNode *condition = parse_logical_or(parser);
    
    if (parser_match(parser, TOK_QUESTION)) {
        ASTNode *true_expr = parse_expression(parser);
        parser_expect(parser, TOK_COLON, "Expected ':' in ternary expression");
        ASTNode *false_expr = parse_ternary(parser); // Right-associative
        
        ASTNode *node = ast_node_new(AST_TERNARY_OP);
        node->ternary.condition = condition;
        node->ternary.true_expr = true_expr;
        node->ternary.false_expr = false_expr;
        return node;
    }
    
    return condition;
}

// Main expression parser - entry point for all expression parsing
static ASTNode *parse_expression(Parser *parser) {
    // SECURITY: Check recursion depth to prevent stack overflow attacks
    const int MAX_DEPTH = 1000;  // Prevent deeply nested expressions
    if (parser->depth > MAX_DEPTH) {
        error_at(parser->current->line, parser->current->col, 
                 "Expression nesting too deep - max 1000 levels");
        return NULL;
    }
    
    parser->depth++;
    ASTNode *node = parse_ternary(parser);
    parser->depth--;
    
    return node;
}

// Parse block
static ASTNode *parse_block(Parser *parser) {
    parser_expect(parser, TOK_LBRACE, "Expected '{' to start block");
    
    ASTNode *block = ast_node_new(AST_BLOCK);
    block->block.statements = ast_list_new();
    
    while (!parser_check(parser, TOK_RBRACE) && !parser_check(parser, TOK_EOF)) {
        ASTNode *stmt = parse_statement(parser);
        if (stmt) {
            // If statement is a block (from interpolation), flatten it
            if (stmt->type == AST_BLOCK) {
                for (int i = 0; i < stmt->block.statements->count; i++) {
                    ast_list_add(block->block.statements, stmt->block.statements->nodes[i]);
                }
                // Don't free the inner nodes, just the container
                free(stmt->block.statements->nodes);
                free(stmt->block.statements);
                free(stmt);
            } else {
                ast_list_add(block->block.statements, stmt);
            }
        }
    }
    
    parser_expect(parser, TOK_RBRACE, "Expected '}' to end block");
    
    return block;
}

// Helper: Parse interpolated string into multiple show statements
// Currently unused but kept for potential future use in advanced interpolation handling
__attribute__((unused))
static ASTNode *parse_interpolated_string_simple(Parser *parser, const char *str) {
    ASTNode *block = ast_node_new(AST_BLOCK);
    block->block.statements = ast_list_new();
    
    char buffer[4096];
    int buf_pos = 0;
    int i = 0;
    
    while (str[i] != '\0') {
        if (str[i] == '$' && str[i+1] != '\0') {
            // Flush any accumulated literal
            if (buf_pos > 0) {
                buffer[buf_pos] = '\0';
                ASTNode *show = ast_node_new(AST_SHOW);
                ASTNode *lit = ast_node_new(AST_LITERAL);
                lit->literal.type = xstrdup("str");
                lit->literal.value = xstrdup(buffer);
                show->show.value = lit;
                ast_list_add(block->block.statements, show);
                buf_pos = 0;
            }
            
            i++; // skip $
            
            // Check for ${expression}
            if (str[i] == '{') {
                i++; // skip {
                int expr_start = i;
                int brace_count = 1;
                
                // Find matching }
                while (str[i] != '\0' && brace_count > 0) {
                    if (str[i] == '{') brace_count++;
                    else if (str[i] == '}') brace_count--;
                    if (brace_count > 0) i++;
                }
                
                if (brace_count != 0) {
                    error_at(parser->current->line, parser->current->col, 
                            "Unclosed interpolation expression");
                }
                
                // Extract expression
                int expr_len = i - expr_start;
                char *expr_str = xmalloc(expr_len + 1);
                strncpy(expr_str, &str[expr_start], expr_len);
                expr_str[expr_len] = '\0';
                
                // Parse expression (create a temporary lexer/parser)
                Lexer *temp_lexer = lexer_new(expr_str);
                Parser *temp_parser = parser_new(temp_lexer);
                ASTNode *expr = parse_expression(temp_parser);
                
                if (expr) {
                    // Create show statement for expression
                    ASTNode *show = ast_node_new(AST_SHOW);
                    show->show.value = expr;
                    ast_list_add(block->block.statements, show);
                } else {
                    // On parse error, output the raw text
                    ASTNode *show = ast_node_new(AST_SHOW);
                    ASTNode *lit = ast_node_new(AST_LITERAL);
                    lit->literal.type = xstrdup("str");
                    char err_buf[512];
                    snprintf(err_buf, sizeof(err_buf), "${%s}", expr_str);
                    lit->literal.value = xstrdup(err_buf);
                    show->show.value = lit;
                    ast_list_add(block->block.statements, show);
                }
                
                parser_free(temp_parser);
                lexer_free(temp_lexer);
                free(expr_str);
                
                i++; // skip }
            } else {
                // Simple $identifier
                int ident_start = i;
                while (str[i] != '\0' && (isalnum(str[i]) || str[i] == '_')) {
                    i++;
                }
                
                int ident_len = i - ident_start;
                if (ident_len == 0) {
                    error_at(parser->current->line, parser->current->col,
                            "Expected identifier after $");
                }
                
                char *ident = xmalloc(ident_len + 1);
                strncpy(ident, &str[ident_start], ident_len);
                ident[ident_len] = '\0';
                
                // Create identifier node
                ASTNode *id = ast_node_new(AST_IDENT);
                id->ident.name = ident;
                
                // Create show statement
                ASTNode *show = ast_node_new(AST_SHOW);
                show->show.value = id;
                ast_list_add(block->block.statements, show);
            }
        } else {
            // Regular character
            buffer[buf_pos++] = str[i++];
            if (buf_pos >= 4095) {
                error_at(parser->current->line, parser->current->col,
                        "String literal too long");
            }
        }
    }
    
    // Flush remaining literal
    if (buf_pos > 0) {
        buffer[buf_pos] = '\0';
        ASTNode *show = ast_node_new(AST_SHOW);
        ASTNode *lit = ast_node_new(AST_LITERAL);
        lit->literal.type = xstrdup("str");
        lit->literal.value = xstrdup(buffer);
        show->show.value = lit;
        ast_list_add(block->block.statements, show);
    }
    
    return block;
}

// Parse statements
static ASTNode *parse_show(Parser *parser) {
    parser_advance(parser); // consume 'show'
    parser_expect(parser, TOK_LBRACKET, "Expected '[' after 'show'");
    
    // Temporarily disable interpolation in show[] to debug the block issue
    // if (parser_check(parser, TOK_STRING) && has_interpolation(parser->current->value)) {
    //     ...interpolation code...
    // }
    
    ASTNode *node = ast_node_new(AST_SHOW);
    node->show.value = parse_expression(parser);
    
    parser_expect(parser, TOK_RBRACKET, "Expected ']' after show expression");
    parser_expect(parser, TOK_SEMICOLON, "Expected ';' after show statement");
    
    return node;
}

// Helper: Parse an expression from a string substring
static ASTNode *parse_expression_from_string(const char *expr_str) {
    if (!expr_str || strlen(expr_str) == 0) {
        return NULL;
    }
    
    // Create a temporary lexer for the expression string
    Lexer *temp_lexer = lexer_new(expr_str);
    if (!temp_lexer) {
        return NULL;
    }
    
    // Create a temporary parser
    Parser *temp_parser = parser_new(temp_lexer);
    if (!temp_parser) {
        lexer_free(temp_lexer);
        return NULL;
    }
    
    // Parse the expression
    ASTNode *expr = parse_expression(temp_parser);
    
    // Cleanup temporary parser and lexer
    parser_free(temp_parser);
    lexer_free(temp_lexer);
    
    return expr;
}

// Parse showf statement with automatic newline - NO BLOCKS
static ASTNode *parse_showf(Parser *parser) {
    parser_advance(parser); // consume 'showf'
    parser_expect(parser, TOK_LBRACKET, "Expected '[' after 'showf'");
    
    if (!parser_check(parser, TOK_STRING)) {
        error_at(parser->current->line, parser->current->col, 
                "Expected string literal in showf[]");
    }
    
    // Make a copy of the string value BEFORE advancing the parser, as parser_advance will free the token
    const char *str_value = xstrdup(parser->current->value);
    parser_advance(parser); // consume string
    
    parser_expect(parser, TOK_RBRACKET, "Expected ']' after showf string");
    parser_expect(parser, TOK_SEMICOLON, "Expected ';' after showf statement");
    
    // Create a showf node with the interpolated string
    ASTNode *node = ast_node_new(AST_SHOWF);
    ASTNode *interp = ast_node_new(AST_INTERPOLATED_STRING);
    
    // Parse the string into chunks and expressions
    interp->interpolated_string.chunks = ast_list_new();
    interp->interpolated_string.expressions = ast_list_new();
    
    char buffer[4096];
    int buf_pos = 0;
    int i = 0;
    
    while (str_value[i] != '\0') {
        // Check for ${expression} syntax
        if (str_value[i] == '$' && str_value[i+1] == '{') {
            // Flush accumulated literal (or empty first chunk)
            buffer[buf_pos] = '\0';
            ASTNode *lit = ast_node_new(AST_LITERAL);
            lit->literal.type = xstrdup("str");
            lit->literal.value = xstrdup(buffer);
            ast_list_add(interp->interpolated_string.chunks, lit);
            buf_pos = 0;
            
            i += 2; // skip ${
            
            // Find the matching closing brace
            int expr_start = i;
            int brace_depth = 1;
            while (str_value[i] != '\0' && brace_depth > 0) {
                if (str_value[i] == '{') {
                    brace_depth++;
                } else if (str_value[i] == '}') {
                    brace_depth--;
                }
                if (brace_depth > 0) {
                    i++;
                }
            }
            
            if (brace_depth == 0) {
                // Extract expression string
                int expr_len = i - expr_start;
                char expr_buf[4096];
                strncpy(expr_buf, &str_value[expr_start], expr_len);
                expr_buf[expr_len] = '\0';
                
                // Parse the expression
                ASTNode *expr = parse_expression_from_string(expr_buf);
                if (expr) {
                    ast_list_add(interp->interpolated_string.expressions, expr);
                }
                
                i++; // skip closing }
            } else {
                error_at(0, 0, "Unclosed ${expression} in showf[]");
                free((char *)str_value);
                return NULL;
            }
        }
        // Check for simple $identifier syntax
        else if (str_value[i] == '$' && str_value[i+1] != '\0' && str_value[i+1] != '{') {
            // Flush accumulated literal (or empty first chunk)
            buffer[buf_pos] = '\0';
            ASTNode *lit = ast_node_new(AST_LITERAL);
            lit->literal.type = xstrdup("str");
            lit->literal.value = xstrdup(buffer);
            ast_list_add(interp->interpolated_string.chunks, lit);
            buf_pos = 0;
            
            i++; // skip $
            
            // Extract identifier
            int ident_start = i;
            while (str_value[i] != '\0' && (isalnum(str_value[i]) || str_value[i] == '_')) {
                i++;
            }
            
            int ident_len = i - ident_start;
            if (ident_len > 0) {
                char ident[256];
                strncpy(ident, &str_value[ident_start], ident_len);
                ident[ident_len] = '\0';
                
                ASTNode *id = ast_node_new(AST_IDENT);
                id->ident.name = xstrdup(ident);
                ast_list_add(interp->interpolated_string.expressions, id);
            }
        } 
        else {
            buffer[buf_pos++] = str_value[i++];
        }
    }
    
    // Flush remaining literal (or final chunk after last expression)
    buffer[buf_pos] = '\0';
    ASTNode *lit = ast_node_new(AST_LITERAL);
    lit->literal.type = xstrdup("str");
    lit->literal.value = xstrdup(buffer);
    ast_list_add(interp->interpolated_string.chunks, lit);
    
    // Set chunk count: should always be expressions->count + 1
    interp->interpolated_string.chunk_count = interp->interpolated_string.expressions->count + 1;
    
    node->showf.interpolated = interp;
    
    // Free the copied string value
    free((char *)str_value);
    
    return node;
}

static ASTNode *parse_return(Parser *parser) {
    parser_advance(parser); // consume 'get'
    parser_expect(parser, TOK_LBRACKET, "Expected '[' after 'get'");
    
    ASTNode *node = ast_node_new(AST_RETURN);
    node->ret.value = parse_expression(parser);
    
    parser_expect(parser, TOK_RBRACKET, "Expected ']' after get expression");
    parser_expect(parser, TOK_SEMICOLON, "Expected ';' after get statement");
    
    return node;
}

static ASTNode *parse_if(Parser *parser) {
    parser_advance(parser); // consume 'if'
    parser_expect(parser, TOK_LBRACKET, "Expected '[' after 'if'");
    
    ASTNode *node = ast_node_new(AST_IF);
    node->if_stmt.condition = parse_expression(parser);
    
    parser_expect(parser, TOK_RBRACKET, "Expected ']' after if condition");
    
    node->if_stmt.then_block = parse_block(parser);
    node->if_stmt.else_block = NULL;
    
    // Handle else-if and else chains (supports both 'or' and 'else' for final else)
    while (parser_check(parser, TOK_OR) || parser_check(parser, TOK_ELSE)) {
        parser_advance(parser); // consume 'or' or 'else'
        
        if (parser_check(parser, TOK_LBRACKET)) {
            // or[condition] - else if
            parser_advance(parser);
            
            // Create nested if statement for else-if
            ASTNode *elif_node = ast_node_new(AST_IF);
            elif_node->if_stmt.condition = parse_expression(parser);
            parser_expect(parser, TOK_RBRACKET, "Expected ']' after condition");
            elif_node->if_stmt.then_block = parse_block(parser);
            elif_node->if_stmt.else_block = NULL;
            
            // Attach to the last else clause
            ASTNode *current = node;
            while (current->if_stmt.else_block != NULL && 
                   current->if_stmt.else_block->type == AST_IF) {
                current = current->if_stmt.else_block;
            }
            current->if_stmt.else_block = elif_node;
        } else if (parser_check(parser, TOK_LBRACE)) {
            // or / else without condition - final else
            ASTNode *else_block = parse_block(parser);
            
            // Attach to the last else clause
            ASTNode *current = node;
            while (current->if_stmt.else_block != NULL && 
                   current->if_stmt.else_block->type == AST_IF) {
                current = current->if_stmt.else_block;
            }
            current->if_stmt.else_block = else_block;
            break; // No more else clauses after final else
        } else {
            error_at(parser->current->line, parser->current->col, 
                    "Expected '[' or '{' after 'or'/'else'");
        }
    }
    
    return node;
}

static ASTNode *parse_while(Parser *parser) {
    parser_advance(parser); // consume 'while'
    parser_expect(parser, TOK_LBRACKET, "Expected '[' after 'while'");
    
    ASTNode *node = ast_node_new(AST_WHILE);
    node->while_stmt.condition = parse_expression(parser);
    
    parser_expect(parser, TOK_RBRACKET, "Expected ']' after while condition");
    
    node->while_stmt.body = parse_block(parser);
    
    return node;
}

static ASTNode *parse_cycle(Parser *parser) {
    parser_advance(parser); // consume 'cycle'
    parser_expect(parser, TOK_LBRACKET, "Expected '[' after 'cycle'");
    
    ASTNode *node = ast_node_new(AST_CYCLE);
    node->cycle.value = parse_expression(parser);
    node->cycle.cases = ast_list_new();
    node->cycle.default_case = NULL;
    
    parser_expect(parser, TOK_RBRACKET, "Expected ']' after cycle expression");
    parser_expect(parser, TOK_LBRACE, "Expected '{' to start cycle body");
    
    // Parse when cases
    while (!parser_check(parser, TOK_RBRACE) && !parser_check(parser, TOK_EOF)) {
        if (parser_check(parser, TOK_WHEN)) {
            parser_advance(parser); // consume 'when'
            parser_expect(parser, TOK_LBRACKET, "Expected '[' after 'when'");
            
            ASTNode *when_node = ast_node_new(AST_WHEN);
            when_node->when.value = parse_expression(parser);
            
            parser_expect(parser, TOK_RBRACKET, "Expected ']' after when value");
            parser_expect(parser, TOK_COLON, "Expected ':' after when condition");
            
            when_node->when.body = parse_block(parser);
            ast_list_add(node->cycle.cases, when_node);
        } else if (parser_check(parser, TOK_FIXED)) {
            parser_advance(parser); // consume 'fixed'
            parser_expect(parser, TOK_COLON, "Expected ':' after 'fixed'");
            node->cycle.default_case = parse_block(parser);
        } else {
            error_at(parser->current->line, parser->current->col, 
                    "Expected 'when' or 'fixed' in cycle block");
        }
    }
    
    parser_expect(parser, TOK_RBRACE, "Expected '}' to end cycle");
    
    return node;
}

static ASTNode *parse_check(Parser *parser) {
    parser_advance(parser); // consume 'check'
    
    ASTNode *node = ast_node_new(AST_CHECK);
    node->check.try_block = parse_block(parser);
    node->check.handlers = ast_list_new();
    
    // Parse when handlers (error handling)
    while (parser_match(parser, TOK_WHEN)) {
        parser_expect(parser, TOK_LBRACKET, "Expected '[' after 'when'");
        
        ASTNode *handler = ast_node_new(AST_WHEN);
        
        // Error type or identifier
        if (parser_check(parser, TOK_IDENT)) {
            handler->when.value = ast_node_new(AST_IDENT);
            handler->when.value->ident.name = xstrdup(parser->current->value);
            parser_advance(parser);
        } else {
            handler->when.value = NULL; // catch all
        }
        
        parser_expect(parser, TOK_RBRACKET, "Expected ']' after when condition");
        parser_expect(parser, TOK_COLON, "Expected ':' after when");
        
        handler->when.body = parse_block(parser);
        ast_list_add(node->check.handlers, handler);
    }
    
    return node;
}

static ASTNode *parse_read(Parser *parser) {
    parser_advance(parser); // consume 'read'
    parser_expect(parser, TOK_LBRACKET, "Expected '[' after 'read'");
    
    ASTNode *node = ast_node_new(AST_READ);
    node->read.target = NULL;
    node->read.prompt = NULL;
    node->read.return_type = NULL;
    
    // Detect which form: read[type][prompt], read[prompt], or read[var]
    bool is_type_keyword = false;
    const char *type_names[] = {"int", "str", "deci", "char", "byte", "ubyte", "bool", "none", NULL};
    
    if (parser_check(parser, TOK_IDENT)) {
        // Check if it's a type keyword
        const char *name = parser->current->value;
        for (int i = 0; type_names[i]; i++) {
            if (strcmp(name, type_names[i]) == 0) {
                is_type_keyword = true;
                break;
            }
        }
    }
    
    if (is_type_keyword) {
        // Form: read[type][prompt] - reads with explicit type conversion
        node->read.return_type = xstrdup(parser->current->value);
        parser_advance(parser); // consume type
        parser_expect(parser, TOK_RBRACKET, "Expected ']' after read type");
        
        // Now expect second bracket with prompt
        parser_expect(parser, TOK_LBRACKET, "Expected '[' after read type");
        if (parser_check(parser, TOK_STRING)) {
            node->read.prompt = ast_node_new(AST_LITERAL);
            node->read.prompt->literal.value = xstrdup(parser->current->value);
            node->read.prompt->literal.type = xstrdup("str");
            parser_advance(parser);
        } else {
            error_at(parser->current->line, parser->current->col, 
                    "Expected prompt string in read[type][prompt] form");
        }
        parser_expect(parser, TOK_RBRACKET, "Expected ']' after read prompt");
    } 
    else if (parser_check(parser, TOK_STRING)) {
        // Form: read["prompt"] - returns string
        node->read.prompt = ast_node_new(AST_LITERAL);
        node->read.prompt->literal.value = xstrdup(parser->current->value);
        node->read.prompt->literal.type = xstrdup("str");
        node->read.return_type = xstrdup("str");  // Default return type is string
        parser_advance(parser);
        parser_expect(parser, TOK_RBRACKET, "Expected ']' after read prompt");
    } 
    else if (parser_check(parser, TOK_IDENT)) {
        // Old form: read[variable] - statement form
        node->read.target = ast_node_new(AST_IDENT);
        node->read.target->ident.name = xstrdup(parser->current->value);
        parser_advance(parser);
        parser_expect(parser, TOK_RBRACKET, "Expected ']' after read variable");
        parser_expect(parser, TOK_SEMICOLON, "Expected ';' after read statement");
    } 
    else {
        error_at(parser->current->line, parser->current->col, 
                "Expected prompt string, type, or variable name after 'read['");
    }
    
    // For expression forms (not statement), don't expect semicolon
    // Semicolon is only expected for old statement form read[var];
    if (node->read.target == NULL && !parser_check(parser, TOK_SEMICOLON)) {
        // This is fine - expression form, no semicolon needed
    }
    
    return node;
}

static ASTNode *parse_loop(Parser *parser) {
    parser_advance(parser); // consume 'loop'
    parser_expect(parser, TOK_LBRACKET, "Expected '[' after 'loop'");
    
    ASTNode *node = ast_node_new(AST_LOOP);
    
    // Parse init
    node->loop_stmt.init = parse_statement(parser);
    
    // Parse condition
    node->loop_stmt.condition = parse_expression(parser);
    parser_expect(parser, TOK_SEMICOLON, "Expected ';' after loop condition");
    
    // Parse increment - can be:
    // 1. Simple: i++ or i--
    // 2. Complex: i = expression
    if (parser_check(parser, TOK_IDENT)) {
        char *name = xstrdup(parser->current->value);
        parser_advance(parser);
        
        if (parser_match(parser, TOK_PLUSPLUS)) {
            // i++
            ASTNode *one = ast_node_new(AST_LITERAL);
            one->literal.value = xstrdup("1");
            one->literal.type = xstrdup("int");
            
            ASTNode *ident = ast_node_new(AST_IDENT);
            ident->ident.name = xstrdup(name);
            
            ASTNode *add = ast_node_new(AST_BINARY_OP);
            add->binary.op = xstrdup("+");
            add->binary.left = ident;
            add->binary.right = one;
            
            node->loop_stmt.increment = ast_node_new(AST_ASSIGN);
            node->loop_stmt.increment->assign.name = name;
            node->loop_stmt.increment->assign.value = add;
        } else if (parser_match(parser, TOK_MINUSMINUS)) {
            // i--
            ASTNode *one = ast_node_new(AST_LITERAL);
            one->literal.value = xstrdup("1");
            one->literal.type = xstrdup("int");
            
            ASTNode *ident = ast_node_new(AST_IDENT);
            ident->ident.name = xstrdup(name);
            
            ASTNode *sub = ast_node_new(AST_BINARY_OP);
            sub->binary.op = xstrdup("-");
            sub->binary.left = ident;
            sub->binary.right = one;
            
            node->loop_stmt.increment = ast_node_new(AST_ASSIGN);
            node->loop_stmt.increment->assign.name = name;
            node->loop_stmt.increment->assign.value = sub;
        } else if (parser_match(parser, TOK_ASSIGN)) {
            // i = expression
            ASTNode *value = parse_expression(parser);
            
            node->loop_stmt.increment = ast_node_new(AST_ASSIGN);
            node->loop_stmt.increment->assign.name = name;
            node->loop_stmt.increment->assign.value = value;
        } else {
            free(name);
        }
    }
    
    parser_expect(parser, TOK_RBRACKET, "Expected ']' after loop increment");
    
    node->loop_stmt.body = parse_block(parser);
    
    return node;
}
// implementing manme block for HDCE memory management model
static ASTNode *parse_manme(Parser *parser) {
    parser_expect(parser, TOK_MANME, "Expected 'manme' keyword");
    parser_expect(parser, TOK_LBRACE, "Expected '{' after manme");
    
    ASTNode *node = ast_node_new(AST_MANME);
    node->manme.body = parse_block(parser);   // reuse existing block parser
    
    parser_expect(parser, TOK_RBRACE, "Expected '}' to close manme block");
    return node;
}

static ASTNode *parse_var_decl(Parser *parser) {
    char *type = xstrdup(parser->current->value);
    parser_advance(parser);
    
    if (!parser_check(parser, TOK_IDENT)) {
        error_at(parser->current->line, parser->current->col, "Expected identifier after type");
    }
    
    char *name = xstrdup(parser->current->value);
    parser_advance(parser);
    
    ASTNode *node = ast_node_new(AST_VAR_DECL);
    node->var_decl.type = type;
    node->var_decl.name = name;
    
    if (parser_match(parser, TOK_ASSIGN)) {
        node->var_decl.value = parse_expression(parser);
    } else {
        node->var_decl.value = NULL;
    }
    
    // Don't consume semicolon here if we're in a loop init
    if (parser_check(parser, TOK_SEMICOLON)) {
        parser_advance(parser);
    }
    
    return node;
}

static ASTNode *parse_statement(Parser *parser) {
    // Builtin function call statement: @function[args]; or @type[args]::type;
    if (parser_check(parser, TOK_AT)) {
        parser_advance(parser); // consume '@'
        
        // Accept both identifiers and keywords as builtin function names
        if (!parser_check(parser, TOK_IDENT) && 
            !parser_check(parser, TOK_INT) &&
            !parser_check(parser, TOK_DECI) &&
            !parser_check(parser, TOK_BYTE) &&
            !parser_check(parser, TOK_BOOL) &&
            !parser_check(parser, TOK_STR) &&
            !parser_check(parser, TOK_CHAR) &&
            !parser_check(parser, TOK_SHOW) &&
            !parser_check(parser, TOK_READ)) {
            error_at(parser->current->line, parser->current->col, "Expected builtin function name after '@'");
        }
        
        char *builtin_name = xstrdup(parser->current->value);
        parser_advance(parser);
        
        parser_expect(parser, TOK_LBRACKET, "Expected '[' after builtin function name");
        
        ASTNode *node = ast_node_new(AST_BUILTIN_CALL);
        node->builtin_call.name = builtin_name;
        node->builtin_call.args = ast_list_new();
        
        if (!parser_check(parser, TOK_RBRACKET)) {
            do {
                ASTNode *arg = parse_expression(parser);
                ast_list_add(node->builtin_call.args, arg);
            } while (parser_match(parser, TOK_COMMA));
        }
        
        parser_expect(parser, TOK_RBRACKET, "Expected ']' after builtin function arguments");
        
        // Check for :: type specification (only for @type builtin)
        if (strcmp(builtin_name, "type") == 0 && parser_match(parser, TOK_COLONCOLON)) {
            // Parse target types separated by commas
            if (!parser_check(parser, TOK_IDENT) && !parser_check(parser, TOK_INT) &&
                !parser_check(parser, TOK_DECI) && !parser_check(parser, TOK_BYTE) &&
                !parser_check(parser, TOK_BOOL) && !parser_check(parser, TOK_STR) &&
                !parser_check(parser, TOK_CHAR)) {
                error_at(parser->current->line, parser->current->col, "Expected type name after '::'");
            }
            
            char *target_types = xstrdup(parser->current->value);
            parser_advance(parser);
            
            // Allow multiple types separated by commas
            while (parser_match(parser, TOK_COMMA)) {
                if (!parser_check(parser, TOK_IDENT) && !parser_check(parser, TOK_INT) &&
                    !parser_check(parser, TOK_DECI) && !parser_check(parser, TOK_BYTE) &&
                    !parser_check(parser, TOK_BOOL) && !parser_check(parser, TOK_STR) &&
                    !parser_check(parser, TOK_CHAR)) {
                    error_at(parser->current->line, parser->current->col, "Expected type name after comma");
                }
                
                size_t new_len = strlen(target_types) + strlen(parser->current->value) + 2;
                char *new_types = xmalloc(new_len);
                snprintf(new_types, new_len, "%s,%s", target_types, parser->current->value);
                free(target_types);
                target_types = new_types;
                parser_advance(parser);
            }
            
            node->builtin_call.target_type = target_types;
        }
        
        parser_expect(parser, TOK_SEMICOLON, "Expected ';' after builtin function call");
        return node;
    }
    
    // show statement (no newline)
    if (parser_check(parser, TOK_SHOW)) {
        return parse_show(parser);
    }
    
    // showf statement (formatted output with interpolation)
    if (parser_check(parser, TOK_SHOWF)) {
        return parse_showf(parser);
    }
    
    // get (return) statement
    if (parser_check(parser, TOK_GET)) {
        return parse_return(parser);
    }
    
    // if statement
    if (parser_check(parser, TOK_IF)) {
        return parse_if(parser);
    }
    
    // while statement
    if (parser_check(parser, TOK_WHILE)) {
        return parse_while(parser);
    }
    
    // loop statement
    if (parser_check(parser, TOK_LOOP)) {
        return parse_loop(parser);
    }
    
    // cycle statement (switch)
    if (parser_check(parser, TOK_CYCLE)) {
        return parse_cycle(parser);
    }
    
    // check statement (try-catch)
    if (parser_check(parser, TOK_CHECK)) {
        return parse_check(parser);
    }
    
    // break statement
    if (parser_check(parser, TOK_BREAK)) {
        parser_advance(parser); // consume 'break'
        parser_expect(parser, TOK_SEMICOLON, "Expected ';' after 'break'");
        ASTNode *node = ast_node_new(AST_BREAK);
        return node;
    }
    
    // continue statement
    if (parser_check(parser, TOK_CONTINUE)) {
        parser_advance(parser); // consume 'continue'
        parser_expect(parser, TOK_SEMICOLON, "Expected ';' after 'continue'");
        ASTNode *node = ast_node_new(AST_CONTINUE);
        return node;
    }
    
    // defer statement
    if (parser_check(parser, TOK_DEFER)) {
        parser_advance(parser); // consume 'defer'
        ASTNode *stmt = parse_statement(parser);
        
        ASTNode *node = ast_node_new(AST_DEFER);
        node->defer.stmt = stmt;
        return node;
    }
    
    // emit statement (cycle expression value setter)
    if (parser_check(parser, TOK_EMIT)) {
        parser_advance(parser); // consume 'emit'
        parser_expect(parser, TOK_LBRACKET, "Expected '[' after 'emit'");
        ASTNode *value = parse_expression(parser);
        parser_expect(parser, TOK_RBRACKET, "Expected ']' after emit expression");
        parser_expect(parser, TOK_SEMICOLON, "Expected ';' after emit statement");
        
        ASTNode *node = ast_node_new(AST_EMIT);
        node->emit.value = value;
        return node;
    }
    // read statement
    if (parser_check(parser, TOK_READ)) {
        return parse_read(parser);
    }

    // manme block for HDCE
    if (parser_check(parser, TOK_MANME)) {
        return parse_manme(parser);
    }

    // Variable declaration
    if (parser_check(parser, TOK_INT) || parser_check(parser, TOK_DECI) ||
        parser_check(parser, TOK_CHAR) || parser_check(parser, TOK_STR) ||
        parser_check(parser, TOK_BOOL) || parser_check(parser, TOK_BYTE) ||
        parser_check(parser, TOK_UBYTE)) {
        return parse_var_decl(parser);
    }
    
    // Group variable declaration: TypeName varname;
    // Check if this is a group type (identifier followed by identifier)
    if (parser_check(parser, TOK_IDENT) && parser->peek->type == TOK_IDENT) {
        char *type_name = xstrdup(parser->current->value);
        parser_advance(parser);
        
        char *var_name = xstrdup(parser->current->value);
        parser_advance(parser);
        
        parser_expect(parser, TOK_SEMICOLON, "Expected ';' after group declaration");
        
        ASTNode *node = ast_node_new(AST_GROUP_DECL);
        node->group_decl.type_name = type_name;
        node->group_decl.var_name = var_name;
        return node;
    }
    
    // Array declaration: arr{type, size} name
    if (parser_check(parser, TOK_ARR)) {
        parser_advance(parser); // consume 'arr'
        parser_expect(parser, TOK_LBRACE, "Expected '{' after 'arr'");
        
        // Parse element type - can be primitive or group type (IDENT)
        if (!parser_check(parser, TOK_INT) && !parser_check(parser, TOK_STR) &&
            !parser_check(parser, TOK_DECI) && !parser_check(parser, TOK_BOOL) &&
            !parser_check(parser, TOK_CHAR) && !parser_check(parser, TOK_BYTE) &&
            !parser_check(parser, TOK_UBYTE) && !parser_check(parser, TOK_IDENT)) {
            error_at(parser->current->line, parser->current->col, 
                    "Expected type after 'arr{' (primitive or group type)");
        }
        
        ASTNode *node = ast_node_new(AST_ARRAY_DECL);
        node->array_decl.element_type = xstrdup(parser->current->value);
        parser_advance(parser);
        
        parser_expect(parser, TOK_COMMA, "Expected ',' after array element type");
        
        // Parse size
        node->array_decl.size = parse_expression(parser);
        
        parser_expect(parser, TOK_RBRACE, "Expected '}' after array size");
        
        // Parse array name
        if (!parser_check(parser, TOK_IDENT)) {
            error_at(parser->current->line, parser->current->col, 
                    "Expected array name");
        }
        
        node->array_decl.name = xstrdup(parser->current->value);
        parser_advance(parser);
        
        // Optional initializer
        node->array_decl.initializer = NULL;
        if (parser_match(parser, TOK_ASSIGN)) {
            parser_expect(parser, TOK_LBRACE, "Expected '{' for array initializer");
            
            node->array_decl.initializer = ast_list_new();
            
            if (!parser_check(parser, TOK_RBRACE)) {
                do {
                    ASTNode *value = parse_expression(parser);
                    ast_list_add(node->array_decl.initializer, value);
                } while (parser_match(parser, TOK_COMMA));
            }
            
            parser_expect(parser, TOK_RBRACE, "Expected '}' after array initializer");
        }
        
        parser_expect(parser, TOK_SEMICOLON, "Expected ';' after array declaration");
        
        return node;
    }
    
    // Map declaration: map{keyType, valueType} name;
    if (parser_check(parser, TOK_MAP)) {
        parser_advance(parser); // consume 'map'
        parser_expect(parser, TOK_LBRACE, "Expected '{' after 'map'");
        
        // Parse key type
        if (!parser_check(parser, TOK_INT) && !parser_check(parser, TOK_STR) &&
            !parser_check(parser, TOK_DECI) && !parser_check(parser, TOK_BOOL) &&
            !parser_check(parser, TOK_CHAR) && !parser_check(parser, TOK_BYTE) &&
            !parser_check(parser, TOK_UBYTE)) {
            error_at(parser->current->line, parser->current->col, 
                    "Expected key type after 'map{'");
        }
        
        char *key_type = xstrdup(parser->current->value);
        parser_advance(parser);
        
        parser_expect(parser, TOK_COMMA, "Expected ',' after map key type");
        
        // Parse value type
        if (!parser_check(parser, TOK_INT) && !parser_check(parser, TOK_STR) &&
            !parser_check(parser, TOK_DECI) && !parser_check(parser, TOK_BOOL) &&
            !parser_check(parser, TOK_CHAR) && !parser_check(parser, TOK_BYTE) &&
            !parser_check(parser, TOK_UBYTE)) {
            error_at(parser->current->line, parser->current->col, 
                    "Expected value type after ','");
        }
        
        char *value_type = xstrdup(parser->current->value);
        parser_advance(parser);
        
        parser_expect(parser, TOK_RBRACE, "Expected '}' after map value type");
        
        // Parse map name
        if (!parser_check(parser, TOK_IDENT)) {
            error_at(parser->current->line, parser->current->col, 
                    "Expected map name");
        }
        
        ASTNode *node = ast_node_new(AST_MAP_DECL);
        node->map_decl.key_type = key_type;
        node->map_decl.value_type = value_type;
        node->map_decl.name = xstrdup(parser->current->value);
        parser_advance(parser);
        
        parser_expect(parser, TOK_SEMICOLON, "Expected ';' after map declaration");
        return node;
    }
    
    // Assignment or function call or array element assignment
    if (parser_check(parser, TOK_IDENT)) {
        char *name = xstrdup(parser->current->value);
        parser_advance(parser);
        
        // Standalone increment: name++;
        if (parser_match(parser, TOK_PLUSPLUS)) {
            parser_expect(parser, TOK_SEMICOLON, "Expected ';' after increment");
            ASTNode *node = ast_node_new(AST_ASSIGN);
            node->assign.name = name;
            
            // Create: name = name + 1
            ASTNode *binary = ast_node_new(AST_BINARY_OP);
            binary->binary.op = xstrdup("+");
            
            ASTNode *left = ast_node_new(AST_IDENT);
            left->ident.name = xstrdup(name);
            binary->binary.left = left;
            
            ASTNode *right = ast_node_new(AST_LITERAL);
            right->literal.type = xstrdup("int");
            right->literal.value = xstrdup("1");
            binary->binary.right = right;
            
            node->assign.value = binary;
            return node;
        }
        
        // Standalone decrement: name--;
        if (parser_match(parser, TOK_MINUSMINUS)) {
            parser_expect(parser, TOK_SEMICOLON, "Expected ';' after decrement");
            ASTNode *node = ast_node_new(AST_ASSIGN);
            node->assign.name = name;
            
            // Create: name = name - 1
            ASTNode *binary = ast_node_new(AST_BINARY_OP);
            binary->binary.op = xstrdup("-");
            
            ASTNode *left = ast_node_new(AST_IDENT);
            left->ident.name = xstrdup(name);
            binary->binary.left = left;
            
            ASTNode *right = ast_node_new(AST_LITERAL);
            right->literal.type = xstrdup("int");
            right->literal.value = xstrdup("1");
            binary->binary.right = right;
            
            node->assign.value = binary;
            return node;
        }
        
        // Member access or assignment: name.member or name.member.member2 etc
        if (parser_check(parser, TOK_DOT)) {
            // Create the initial identifier node
            ASTNode *object = ast_node_new(AST_IDENT);
            object->ident.name = name;
            
            // Handle chained member access (e.g., a.b.c)
            while (parser_match(parser, TOK_DOT)) {
                if (!parser_check(parser, TOK_IDENT)) {
                    error_at(parser->current->line, parser->current->col, "Expected member name after '.'");
                }
                
                char *member_name = xstrdup(parser->current->value);
                parser_advance(parser);
                
                // Check if this is the final member and it's an assignment
                if (parser_check(parser, TOK_ASSIGN)) {
                    parser_advance(parser); // consume '='
                    
                    // Member assignment with nested object
                    ASTNode *node = ast_node_new(AST_MEMBER_ASSIGN);
                    node->member_assign.object = object;
                    node->member_assign.member_name = member_name;
                    node->member_assign.value = parse_expression(parser);
                    parser_expect(parser, TOK_SEMICOLON, "Expected ';' after member assignment");
                    return node;
                }
                
                // Create a member access node with the previous object
                ASTNode *member_node = ast_node_new(AST_MEMBER_ACCESS);
                member_node->member_access.object = object;
                member_node->member_access.member_name = member_name;
                
                // Update object to be this member access for the next iteration
                object = member_node;
            }
            
            error_at(parser->current->line, parser->current->col, "Expected '=' or another member after member access");
        }
        
        // Map operations: name->set[key, value], name->remove[key]
        if (parser_match(parser, TOK_ARROW)) {
            if (!parser_check(parser, TOK_SET) &&
                !parser_check(parser, TOK_IDENT)) {
                error_at(parser->current->line, parser->current->col, "Expected map operation (set, remove) after '->'");
            }
            
            char *operation = xstrdup(parser->current->value);
            parser_advance(parser);
            
            if (strcmp(operation, "set") == 0 && parser_check(parser, TOK_LBRACKET)) {
                // Map set: map->set[key, value];
                parser_advance(parser); // consume '['
                
                ASTNode *key = parse_expression(parser);
                parser_expect(parser, TOK_COMMA, "Expected ',' after map key");
                
                ASTNode *value = parse_expression(parser);
                parser_expect(parser, TOK_RBRACKET, "Expected ']' after map value");
                parser_expect(parser, TOK_SEMICOLON, "Expected ';' after map->set");
                
                ASTNode *node = ast_node_new(AST_MAP_SET);
                node->map_set.map_name = name;
                node->map_set.key = key;
                node->map_set.value = value;
                free(operation);
                return node;
            } else if (strcmp(operation, "remove") == 0 && parser_check(parser, TOK_LBRACKET)) {
                // Map remove: map->remove[key];
                parser_advance(parser); // consume '['
                
                ASTNode *key = parse_expression(parser);
                parser_expect(parser, TOK_RBRACKET, "Expected ']' after map key");
                parser_expect(parser, TOK_SEMICOLON, "Expected ';' after map->remove");
                
                ASTNode *node = ast_node_new(AST_MAP_REMOVE);
                node->map_remove.map_name = name;
                node->map_remove.key = key;
                free(operation);
                return node;
            } else {
                error_at(parser->current->line, parser->current->col, "Invalid map operation");
            }
        }
        
        // Array element assignment: name{index} = value
        // OR member access through array: name{index}.member = value
        if (parser_check(parser, TOK_LBRACE)) {
            parser_advance(parser); // consume '{'
            
            ASTNode *index = parse_expression(parser);
            parser_expect(parser, TOK_RBRACE, "Expected '}'");
            
            // Check for member access after array access: arr{i}.member
            if (parser_check(parser, TOK_DOT)) {
                // Create array access node as the object
                ASTNode *array_access = ast_node_new(AST_ARRAY_ACCESS);
                array_access->array_access.name = name;
                array_access->array_access.index = index;
                
                ASTNode *object = array_access;
                
                // Handle chained member access (e.g., arr{i}.a.b.c)
                while (parser_match(parser, TOK_DOT)) {
                    if (!parser_check(parser, TOK_IDENT)) {
                        error_at(parser->current->line, parser->current->col, "Expected member name after '.'");
                    }
                    
                    char *member_name = xstrdup(parser->current->value);
                    parser_advance(parser);
                    
                    // Check if this is the final member and it's an assignment
                    if (parser_check(parser, TOK_ASSIGN)) {
                        parser_advance(parser); // consume '='
                        
                        // Member assignment with array access as object
                        ASTNode *node = ast_node_new(AST_MEMBER_ASSIGN);
                        node->member_assign.object = object;
                        node->member_assign.member_name = member_name;
                        node->member_assign.value = parse_expression(parser);
                        parser_expect(parser, TOK_SEMICOLON, "Expected ';' after member assignment");
                        return node;
                    }
                    
                    // Create a member access node with the previous object
                    ASTNode *member_node = ast_node_new(AST_MEMBER_ACCESS);
                    member_node->member_access.object = object;
                    member_node->member_access.member_name = member_name;
                    
                    // Update object to be this member access for the next iteration
                    object = member_node;
                }
                
                error_at(parser->current->line, parser->current->col, "Expected '=' after member access");
            } else if (parser_match(parser, TOK_ASSIGN)) {
                // Simple array element assignment
                ASTNode *node = ast_node_new(AST_ARRAY_ASSIGN);
                node->array_assign.name = name;
                node->array_assign.index = index;
                node->array_assign.value = parse_expression(parser);
                parser_expect(parser, TOK_SEMICOLON, "Expected ';' after array assignment");
                return node;
            } else {
                error_at(parser->current->line, parser->current->col, "Expected '=' or '.' after array index");
            }
        }
        
        if (parser_match(parser, TOK_ASSIGN)) {
            ASTNode *node = ast_node_new(AST_ASSIGN);
            node->assign.name = name;
            node->assign.value = parse_expression(parser);
            parser_expect(parser, TOK_SEMICOLON, "Expected ';' after assignment");
            return node;
        } else if (parser_match(parser, TOK_LBRACKET)) {
            // Function call as statement
            ASTNode *node = ast_node_new(AST_CALL);
            node->call.name = name;
            node->call.args = ast_list_new();
            
            if (!parser_check(parser, TOK_RBRACKET)) {
                do {
                    ASTNode *arg = parse_expression(parser);
                    ast_list_add(node->call.args, arg);
                } while (parser_match(parser, TOK_COMMA));
            }
            
            parser_expect(parser, TOK_RBRACKET, "Expected ']' after function arguments");
            parser_expect(parser, TOK_SEMICOLON, "Expected ';' after function call");
            return node;
        }
        
        free(name);
    }
    
    error_at(parser->current->line, parser->current->col, "Unexpected token in statement");
    return NULL;
}

// Parse group definition
static ASTNode *parse_group(Parser *parser) {
    parser_expect(parser, TOK_GROUP, "Expected 'group'");
    
    if (!parser_check(parser, TOK_IDENT)) {
        error_at(parser->current->line, parser->current->col, "Expected group name");
    }
    
    char *group_name = xstrdup(parser->current->value);
    parser_advance(parser);
    
    parser_expect(parser, TOK_LBRACE, "Expected '{' after group name");
    
    ASTNode *node = ast_node_new(AST_GROUP_DEF);
    node->group_def.name = group_name;
    node->group_def.fields = ast_list_new();
    
    // Parse fields
    while (!parser_check(parser, TOK_RBRACE) && !parser_check(parser, TOK_EOF)) {
        // Field format: type name;
        // Type can be a keyword (int, str, etc.) or identifier (custom group)
        char *field_type = NULL;
        
        if (parser_check(parser, TOK_INT) || parser_check(parser, TOK_DECI) ||
            parser_check(parser, TOK_CHAR) || parser_check(parser, TOK_STR) ||
            parser_check(parser, TOK_BOOL) || parser_check(parser, TOK_BYTE) ||
            parser_check(parser, TOK_UBYTE) || parser_check(parser, TOK_IDENT)) {
            field_type = xstrdup(parser->current->value);
            parser_advance(parser);
        } else {
            error_at(parser->current->line, parser->current->col, "Expected field type");
        }
        
        if (!parser_check(parser, TOK_IDENT)) {
            error_at(parser->current->line, parser->current->col, "Expected field name");
        }
        
        char *field_name = xstrdup(parser->current->value);
        parser_advance(parser);
        
        parser_expect(parser, TOK_SEMICOLON, "Expected ';' after field declaration");
        
        // Create a VAR_DECL node for the field
        ASTNode *field = ast_node_new(AST_VAR_DECL);
        field->var_decl.type = field_type;
        field->var_decl.name = field_name;
        field->var_decl.value = NULL;
        
        ast_list_add(node->group_def.fields, field);
    }
    
    parser_expect(parser, TOK_RBRACE, "Expected '}' after group fields");
    
    return node;
}

// Parse function
static ASTNode *parse_function(Parser *parser) {
    parser_expect(parser, TOK_FNC, "Expected 'fnc' to start function");
    
    if (!parser_check(parser, TOK_IDENT)) {
        ERROR_WITH_CONTEXT(parser, parser->current->line, parser->current->col, "Expected function name");
    }
    
    ASTNode *node = ast_node_new(AST_FUNCTION);
    node->function.name = xstrdup(parser->current->value);
    parser_advance(parser);
    
    parser_expect(parser, TOK_LBRACKET, "Expected '[' after function name");
    
    // Parse parameters
    node->function.params = ast_list_new();
    
    if (!parser_check(parser, TOK_RBRACKET)) {
        do {
            ASTNode *param = ast_node_new(AST_VAR_DECL);
            param->var_decl.is_array = false;
            
            // Check for array parameter: arr{type, size}
            if (parser_check(parser, TOK_ARR)) {
                parser_advance(parser); // consume 'arr'
                parser_expect(parser, TOK_LBRACE, "Expected '{' after 'arr'");
                
                // Parse element type - can be primitive or group type (IDENT)
                if (!parser_check(parser, TOK_INT) && !parser_check(parser, TOK_STR) &&
                    !parser_check(parser, TOK_DECI) && !parser_check(parser, TOK_BOOL) &&
                    !parser_check(parser, TOK_CHAR) && !parser_check(parser, TOK_BYTE) &&
                    !parser_check(parser, TOK_UBYTE) && !parser_check(parser, TOK_IDENT)) {
                    ERROR_WITH_HINT(parser, parser->current->line, parser->current->col, 
                            "Expected element type after 'arr{'",
                            "Array parameters require: arr{type, size} where type is int, str, deci, bool, char, byte, ubyte, or a group name");
                }
                
                param->var_decl.is_array = true;
                param->var_decl.array_element_type = xstrdup(parser->current->value);
                parser_advance(parser);
                
                parser_expect(parser, TOK_COMMA, "Expected ',' after array element type");
                
                // Parse size
                param->var_decl.array_size = parse_expression(parser);
                
                parser_expect(parser, TOK_RBRACE, "Expected '}' after array size");
                
                // Parse parameter name
                if (!parser_check(parser, TOK_IDENT)) {
                    ERROR_WITH_HINT(parser, parser->current->line, parser->current->col, 
                            "Expected parameter name after array type",
                            "Syntax: arr{type, size} paramName - provide a name for the array parameter");
                }
                
                param->var_decl.name = xstrdup(parser->current->value);
                parser_advance(parser);
                
                // Default value (optional): = expression
                if (parser_match(parser, TOK_ASSIGN)) {
                    param->var_decl.value = parse_expression(parser);
                } else {
                    param->var_decl.value = NULL;
                }
            } else {
                // Regular parameter: type name
                // Type - support all data types AND group types (which appear as IDENT tokens)
                // Primitive types: int, str, deci, bool, char, byte, ubyte
                // OR identifiers that could be group type names
                if (!parser_check(parser, TOK_INT) && !parser_check(parser, TOK_STR) &&
                    !parser_check(parser, TOK_DECI) && !parser_check(parser, TOK_BOOL) &&
                    !parser_check(parser, TOK_CHAR) && !parser_check(parser, TOK_BYTE) &&
                    !parser_check(parser, TOK_UBYTE) && !parser_check(parser, TOK_IDENT)) {
                    ERROR_WITH_HINT(parser, parser->current->line, parser->current->col, 
                            "Expected parameter type",
                            "Valid types: int, str, deci, bool, char, byte, ubyte, or group type names");
                }
                
                param->var_decl.type = xstrdup(parser->current->value);
                parser_advance(parser);
                
                // Name
                if (!parser_check(parser, TOK_IDENT)) {
                    ERROR_WITH_CONTEXT(parser, parser->current->line, parser->current->col, "Expected parameter name");
                }
                
                param->var_decl.name = xstrdup(parser->current->value);
                parser_advance(parser);
                
                // Default value (optional): = expression
                if (parser_match(parser, TOK_ASSIGN)) {
                    param->var_decl.value = parse_expression(parser);
                } else {
                    param->var_decl.value = NULL;
                }
            }
            
            ast_list_add(node->function.params, param);
        } while (parser_match(parser, TOK_COMMA));
    }
    
    parser_expect(parser, TOK_RBRACKET, "Expected ']' after function parameters");
    parser_expect(parser, TOK_COLONCOLON, "Expected '::' before return type");
    
    // Return type (can be simple like 'int' or compound like 'arr{int, 2}')
    if (parser_check(parser, TOK_ARR)) {
        // Array return type: arr{type, size}
        parser_advance(parser); // consume 'arr'
        parser_expect(parser, TOK_LBRACE, "Expected '{' after 'arr'");
        
        char return_str[256];
        strcpy(return_str, "arr{");
        
        // Element type
        if (!parser_check(parser, TOK_INT) && !parser_check(parser, TOK_STR) &&
            !parser_check(parser, TOK_DECI) && !parser_check(parser, TOK_BOOL) &&
            !parser_check(parser, TOK_CHAR) && !parser_check(parser, TOK_BYTE) &&
            !parser_check(parser, TOK_UBYTE) && !parser_check(parser, TOK_IDENT)) {
            error_at(parser->current->line, parser->current->col, "Expected type after 'arr{'");
        }
        
        strcat(return_str, parser->current->value);
        parser_advance(parser);
        
        parser_expect(parser, TOK_COMMA, "Expected ',' after array element type");
        strcat(return_str, ",");
        
        // Size (should be a number)
        if (!parser_check(parser, TOK_NUMBER)) {
            error_at(parser->current->line, parser->current->col, "Expected array size");
        }
        
        strcat(return_str, parser->current->value);
        parser_advance(parser);
        
        parser_expect(parser, TOK_RBRACE, "Expected '}' after array size");
        strcat(return_str, "}");
        
        node->function.return_type = xstrdup(return_str);
    } else {
        // Simple return type: int, str, deci, bool, etc., or group type (IDENT)
        if (!parser_check(parser, TOK_INT) && !parser_check(parser, TOK_STR) &&
            !parser_check(parser, TOK_DECI) && !parser_check(parser, TOK_BOOL) &&
            !parser_check(parser, TOK_CHAR) && !parser_check(parser, TOK_BYTE) &&
            !parser_check(parser, TOK_UBYTE) && !parser_check(parser, TOK_NONE) &&
            !parser_check(parser, TOK_IDENT)) {
            error_at(parser->current->line, parser->current->col, "Expected return type");
        }
        
        node->function.return_type = xstrdup(parser->current->value);
        parser_advance(parser);
    }
    
    // Function body
    node->function.body = parse_block(parser);
    
    return node;
}


// Parse module import: use:path; or use:"path";
static ASTNode *parse_use_statement(Parser *parser) {
    ASTNode *node = ast_node_new(AST_PACKAGE_IMPORT);
    node->line = parser->current->line;
    node->col = parser->current->col;

    parser_expect(parser, TOK_USE, "Expected 'use' keyword");
    parser_expect(parser, TOK_COLON, "Expected ':' after 'use'");

    // === CRITICAL FIX: Handle file paths vs package names ===
    if (parser_check(parser, TOK_STRING)) {
        // Case 1: Direct file path: use:"b.ras" or use:"../lib/utils.ras"
        const char *path = parser->current->value;
        node->package_import.package_name = xstrdup(path);
        parser_advance(parser);
    } else {
        // Case 2: Package name (like use:math)
        char path_buffer[1024] = {0};
        int path_index = 0;

        while (path_index < 1023) {
            if (parser_check(parser, TOK_IDENT)) {
                int len = strlen(parser->current->value);
                if (path_index + len < 1024) {
                    strcat(path_buffer, parser->current->value);
                    path_index += len;
                    parser_advance(parser);
                } else break;
            } else if (parser_check(parser, TOK_DOT)) {
                strcat(path_buffer, ".");
                path_index++;
                parser_advance(parser);
                if (parser_check(parser, TOK_DOT)) {
                    strcat(path_buffer, ".");
                    path_index++;
                    parser_advance(parser);
                }
            } else if (parser_check(parser, TOK_SLASH)) {
                strcat(path_buffer, "/");
                path_index++;
                parser_advance(parser);
            } else {
                break;
            }
        }

        if (path_buffer[0] == '\0') {
            error_at(parser->current->line, parser->current->col, 
                    "Expected module path or identifier after 'use:'");
        }
        node->package_import.package_name = xstrdup(path_buffer);
    }

    // Optional alias: as <alias>
    node->package_import.alias = NULL;
    if (parser_check(parser, TOK_IDENT) && strcmp(parser->current->value, "as") == 0) {
        parser_advance(parser); // consume 'as'
        if (!parser_check(parser, TOK_IDENT)) {
            error_at(parser->current->line, parser->current->col,
                    "Expected alias name after 'as'");
        }
        node->package_import.alias = xstrdup(parser->current->value);
        parser_advance(parser);
    }

    parser_expect(parser, TOK_SEMICOLON, "Expected ';' after use statement");
    return node;
}

// Parse global constant declaration
static ASTNode *parse_const_decl(Parser *parser) {
    ASTNode *node = ast_node_new(AST_CONST_DECL);
    node->line = parser->current->line;
    node->col = parser->current->col;
    
    parser_expect(parser, TOK_CONST, "Expected 'const' keyword");
    
    // Get constant name
    if (!parser_check(parser, TOK_IDENT)) {
        error_at(parser->current->line, parser->current->col, 
                "Expected constant name after 'const'");
    }
    
    node->const_decl.name = xstrdup(parser->current->value);
    parser_advance(parser);
    
    // Expect '='
    parser_expect(parser, TOK_ASSIGN, "Expected '=' after constant name");
    
    // Parse constant value (must be a literal or expression)
    node->const_decl.value = parse_expression(parser);
    
    // Expect ';'
    parser_expect(parser, TOK_SEMICOLON, "Expected ';' after constant declaration");
    
    return node;
}

// Parse program
ASTNode *parser_parse(Parser *parser) {
    ASTNode *program = ast_node_new(AST_PROGRAM);
    program->program.imports = ast_list_new();
    program->program.constants = ast_list_new();
    program->program.groups = ast_list_new();
    program->program.functions = ast_list_new();
    
    while (!parser_check(parser, TOK_EOF)) {
        if (parser_check(parser, TOK_PKG)) {
            // Module declaration: pkg:name;
            ASTNode *import = parse_package_import(parser);
            ast_list_add(program->program.imports, import);
        } else if (parser_check(parser, TOK_USE)) {
            // Module import: use:path; or use:"path";
            ASTNode *import = parse_use_statement(parser);
            ast_list_add(program->program.imports, import);
        } else if (parser_check(parser, TOK_CONST)) {
            ASTNode *constant = parse_const_decl(parser);
            ast_list_add(program->program.constants, constant);
        } else if (parser_check(parser, TOK_GROUP)) {
            ASTNode *group = parse_group(parser);
            ast_list_add(program->program.groups, group);
        } else if (parser_check(parser, TOK_FNC)) {
            ASTNode *func = parse_function(parser);
            ast_list_add(program->program.functions, func);
        } else {
            error_at(parser->current->line, parser->current->col, 
                    "Expected package declaration, module import, constant, group, or function");
        }
    }
    
    return program;}
    
// Parse package import (module declaration): pkg:name;
ASTNode *parse_package_import(Parser *parser) {
    ASTNode *node = ast_node_new(AST_PACKAGE_IMPORT);
    node->line = parser->current->line;
    node->col = parser->current->col;

    parser_expect(parser, TOK_PKG, "Expected 'pkg' keyword");
    parser_expect(parser, TOK_COLON, "Expected ':' after 'pkg'");

    // Get module name (identifier only for pkg declaration)
    if (!parser_check(parser, TOK_IDENT)) {
        error_at(parser->current->line, parser->current->col,
                "Expected module name after 'pkg:'");
    }

    node->package_import.package_name = xstrdup(parser->current->value);
    parser_advance(parser);

    // Optional alias: as <alias>
    node->package_import.alias = NULL;
    if (parser_check(parser, TOK_IDENT) && strcmp(parser->current->value, "as") == 0) {
        parser_advance(parser); // consume 'as'
        if (!parser_check(parser, TOK_IDENT)) {
            error_at(parser->current->line, parser->current->col,
                    "Expected alias name after 'as'");
        }
        node->package_import.alias = xstrdup(parser->current->value);
        parser_advance(parser);
    }

    parser_expect(parser, TOK_SEMICOLON, "Expected ';' after package declaration");
    return node;
}
