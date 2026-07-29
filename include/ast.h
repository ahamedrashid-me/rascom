#ifndef AST_H
#define AST_H

#include "common.h"

// AST Node types
typedef enum {
    AST_PROGRAM,
    AST_CONST_DECL,  // Global constant declaration
    AST_FUNCTION,
    AST_BLOCK,
    AST_RETURN,      // get
    AST_IF,
    AST_WHILE,
    AST_LOOP,
    AST_BREAK,      // break statement
    AST_CONTINUE,   // continue statement
    AST_DEFER,      // defer statement (deferred cleanup)
    AST_EMIT,       // emit statement (cycle expression value setter)
    AST_MANME,      // manme block (HDCE memory management)
    AST_SHOW,
    AST_READ,
    AST_CYCLE,       // cycle (switch)
    AST_WHEN,        // when (case)
    AST_CHECK,       // check (try)
    AST_VAR_DECL,
    AST_ASSIGN,
    AST_ARRAY_ASSIGN, // Array element assignment
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_TERNARY_OP,  // Ternary conditional operator
    AST_CALL,           // use to import packages from internet or from another source, outside of stdlib
    AST_LITERAL,
    AST_IDENT,
    AST_ARRAY_DECL,  // Array declaration
    AST_ARRAY_ACCESS,// Array element access
    AST_GROUP_DEF,   // Group (struct) definition
    AST_GROUP_DECL,  // Group variable declaration
    AST_MEMBER_ACCESS, // Group member access (dot operator)
    AST_MEMBER_ASSIGN, // Group member assignment
    AST_MAP_DECL,    // Map declaration
    AST_MAP_SET,     // Map set operation
    AST_MAP_GET,     // Map get operation
    AST_MAP_HAS,     // Map has operation
    AST_MAP_REMOVE,  // Map remove operation
    AST_BUILTIN_CALL, // Builtin function call (@function)
    AST_PACKAGE_IMPORT, // Package import (pkg:<name>;)
    AST_SHOWF,       // showf with interpolated strings
    AST_INTERPOLATED_STRING, // String with {expr} interpolation
} ASTNodeType;

// Forward declarations
typedef struct ASTNode ASTNode;
typedef struct ASTList ASTList;

// AST List (for multiple children)
struct ASTList {
    ASTNode **nodes;
    int count;
    int capacity;
};

// AST Node
struct ASTNode {
    ASTNodeType type;
    int line;
    int col;
    
    union {
        // Program
        struct {
            ASTList *imports;    // Package imports
            ASTList *constants;  // Global constants
            ASTList *groups;     // Group definitions
            ASTList *functions;  // Function definitions
        } program;
        
        // Function
        struct {
            char *name;
            ASTList *params;
            char *return_type;
            ASTNode *body;
        } function;
        
        // Block
        struct {
            ASTList *statements;
        } block;
        
        // Return (get)
        struct {
            ASTNode *value;
        } ret;
        
        // If
        struct {
            ASTNode *condition;
            ASTNode *then_block;
            ASTNode *else_block;
        } if_stmt;
        
        // While
        struct {
            ASTNode *condition;
            ASTNode *body;
        } while_stmt;
        
        // Loop (for)
        struct {
            ASTNode *init;
            ASTNode *condition;
            ASTNode *increment;
            ASTNode *body;
        } loop_stmt;
        
        // Show (output without newline - use \n in strings for newlines)
        struct {
            ASTNode *value;
        } show;
        
        // Read (now supports both statement and expression forms)
        struct {
            ASTNode *target;     // Variable to read into (for statement form: read[var];)
            ASTNode *prompt;     // Prompt string (can be optional)
            char *return_type;   // Return type for expression form (int, str, deci, etc.) - NULL = infer from assignment
        } read;
        
        // Cycle (switch)
        struct {
            ASTNode *value;
            ASTList *cases;      // List of AST_WHEN nodes
            ASTNode *default_case; // AST_BLOCK for fixed
        } cycle;
        
        // When (case)
        struct {
            ASTNode *value;
            ASTNode *body;
        } when;
        
        // Check (try-catch)
        struct {
            ASTNode *try_block;
            ASTList *handlers;   // List of when handlers
        } check;
        
        // Defer (deferred statement execution)
        struct {
            ASTNode *stmt;       // Statement to defer until scope exit
        } defer;
        
        // Emit (cycle expression value setter)
        struct {
            ASTNode *value;      // Value to emit from cycle
        } emit;
        
        // Manme (HDCE memory management block)
        struct {
            ASTNode *body;       // Block body
        } manme;
        
        // Variable declaration (also used for function parameters)
        struct {
            char *type;
            char *name;
            ASTNode *value;
            
            // Array parameter support: when is_array=true, these hold array info
            bool is_array;           // true if this is an array parameter
            char *array_element_type;// Element type (int, str, etc.)
            ASTNode *array_size;     // Size expression (e.g., literal or variable)
        } var_decl;
        
        // Constant declaration (global)
        struct {
            char *name;
            ASTNode *value;
        } const_decl;
        
        // Assignment
        struct {
            char *name;
            ASTNode *value;
        } assign;
        
        // Array element assignment
        struct {
            char *name;
            ASTNode *index;
            ASTNode *value;
        } array_assign;
        
        // Binary operation
        struct {
            char *op;
            ASTNode *left;
            ASTNode *right;
        } binary;
        
        // Unary operation
        struct {
            char *op;
            ASTNode *operand;
            bool is_postfix;  // true for postfix (x++), false for prefix (++x)
        } unary;
        
        // Ternary operation (conditional expression)
        struct {
            ASTNode *condition;
            ASTNode *true_expr;
            ASTNode *false_expr;
        } ternary;
        
        // Function call
        struct {
            char *name;
            ASTList *args;
        } call;
        
        // Literal
        struct {
            char *value;
            char *type;  // "int", "str", etc.
        } literal;
        
        // Identifier
        struct {
            char *name;
        } ident;
        
        // Array declaration
        struct {
            char *element_type;  // Type of elements (int, str, etc.)
            char *name;
            ASTNode *size;       // Size expression (constant or variable)
            ASTList *initializer; // Optional initial values
        } array_decl;
        
        // Array access
        struct {
            char *name;
            ASTNode *index;      // Index expression
        } array_access;
        
        // Group definition
        struct {
            char *name;          // Group type name
            ASTList *fields;     // List of field declarations (VAR_DECL nodes)
        } group_def;
        
        // Group variable declaration
        struct {
            char *type_name;     // Name of the group type
            char *var_name;      // Variable name
        } group_decl;
        
        // Member access (dot operator)
        struct {
            ASTNode *object;     // Object being accessed (can be nested member access)
            char *member_name;   // Member field name
        } member_access;
        
        // Member assignment
        struct {
            ASTNode *object;     // Object to assign to (var or nested member access)
            char *member_name;   // Member field name
            ASTNode *value;      // Value to assign
        } member_assign;
        
        // Map declaration
        struct {
            char *key_type;      // Key type (int, str, etc.)
            char *value_type;    // Value type
            char *name;          // Map variable name
        } map_decl;
        
        // Map set: map.set[key, value]
        struct {
            char *map_name;      // Map variable name
            ASTNode *key;        // Key expression
            ASTNode *value;      // Value expression
        } map_set;
        
        // Map get: map.get[key]
        struct {
            char *map_name;      // Map variable name
            ASTNode *key;        // Key expression
        } map_get;
        
        // Map has: map.has[key]
        struct {
            char *map_name;      // Map variable name
            ASTNode *key;        // Key expression
        } map_has;
        
        // Map remove: map.remove[key]
        struct {
            char *map_name;      // Map variable name
            ASTNode *key;        // Key expression
        } map_remove;
        
        // Builtin function call (@function)
        struct {
            char *name;          // Builtin function name (without @)
            ASTList *args;       // Arguments
            char *target_type;   // Target type for @type[x]::type conversions (NULL if not a type conversion)
        } builtin_call;
        
        // Package import (pkg:<name>;)
        struct {
            char *package_name;  // Name of the package to import
            char *alias;         // Optional alias (NULL if no alias)
        } package_import;
        
        // showf with interpolated strings
        struct {
            ASTNode *interpolated; // Interpolated string node
        } showf;
        
        // Interpolated string: contains string chunks and expressions
        struct {
            ASTList *chunks;     // String literal chunks (str type literals)
            ASTList *expressions; // Expression nodes to interpolate
            int chunk_count;     // Number of chunks (always expressions->count + 1)
        } interpolated_string;
    };
};

// AST functions
ASTNode *ast_node_new(ASTNodeType type);
ASTList *ast_list_new(void);
void ast_list_add(ASTList *list, ASTNode *node);
void ast_node_free(ASTNode *node);
void ast_list_free(ASTList *list);

#endif // AST_H