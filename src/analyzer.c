/*
 * analyzer.c - Intelligent Language Analysis Implementation
 *
 * Provides smart code completion, error prediction, performance hints,
 * and type inference for RasLang developers
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <ctype.h>
#include "../include/analyzer.h"
//#include "../include/lexer.h"
#include "../include/ast.h"

// ============================================
// Global Warning Configuration (Priority 1.3)
// ============================================

WarningConfig g_warning_config = {
    .wall = 0,
    .wextra = 0,
    .werror = 0,
    .wpedantic = 0,
    .unused_vars = 0,
    .type_mismatch = 0,
    .unreachable = 0,
    .shadowing = 0
};

// Forward declarations for warning detection (Priority 1.3)
static void detect_unused_variables(ASTNode *func, PredictionList *list);
static void detect_shadowing_variables(ASTNode *func, PredictionList *list);
static int contains_identifier(ASTNode *node, const char *name);

// ============================================
// Helper Functions
// ============================================

static void *safe_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "ERROR: Memory allocation failed (%zu bytes)\n", size);
        exit(1);
    }
    return ptr;
}

static char *safe_strdup(const char *str) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char *dup = safe_malloc(len);
    strncpy(dup, str, len);
    dup[len - 1] = '\0';  /* Ensure null termination */
    return dup;
}

static int string_starts_with(const char *str, const char *prefix) {
    if (!str || !prefix) return 0;
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

// Type-State Graph types for HDCE Escape Analysis
typedef struct PointerState_s {
    char *var_name;
    int is_from_manme;
    int manme_depth;
    struct PointerState_s *next;
} PointerState;

typedef struct {
    PointerState *pointers;
    int current_manme_depth;
} TypeStateContext;

// Full Type-State Graph for HDCE Escape Analysis
static bool contains_escaping_pointer(ASTNode *node, TypeStateContext *state) {
    if (!node || !state) return false;

    switch (node->type) {
        case AST_MANME:
            state->current_manme_depth++;
            bool escaped = contains_escaping_pointer(node->manme.body, state);
            state->current_manme_depth--;
            return escaped;

        case AST_ASSIGN:
            // If assigning from a manme pointer to outer variable
            if (node->assign.value && node->assign.value->type == AST_IDENT) {
                const char *src = node->assign.value->ident.name;
                
                // Check if source was from current manme scope
                PointerState *p = state->pointers;
                while (p) {
                    if (strcmp(p->var_name, src) == 0 && p->is_from_manme) {
                        if (p->manme_depth == state->current_manme_depth) {
                            // Pointer is escaping!
                            return true;
                        }
                    }
                    p = p->next;
                }
            }
            break;

        case AST_VAR_DECL:
            if (node->var_decl.value && node->var_decl.value->type == AST_IDENT) {
                // Track new pointer
                PointerState *new_state = safe_malloc(sizeof(PointerState));
                new_state->var_name = xstrdup(node->var_decl.name);
                new_state->is_from_manme = (state->current_manme_depth > 0);
                new_state->manme_depth = state->current_manme_depth;
                new_state->next = state->pointers;
                state->pointers = new_state;
            }
            break;

        case AST_BLOCK:
            if (node->block.statements) {
                for (int i = 0; i < node->block.statements->count; i++) {
                    if (contains_escaping_pointer(node->block.statements->nodes[i], state)) {
                        return true;
                    }
                }
            }
            break;

        default:
            break;
    }
    return false;
}

// ============================================
// Warning Configuration (Priority 1.3)
// ============================================

void analyzer_set_warning_flags(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-Wall") == 0) {
            g_warning_config.wall = 1;
            g_warning_config.unused_vars = 1;
            g_warning_config.type_mismatch = 1;
            g_warning_config.unreachable = 1;
            g_warning_config.shadowing = 1;
        } else if (strcmp(argv[i], "-Wextra") == 0) {
            g_warning_config.wextra = 1;
            g_warning_config.wall = 1;    // Implies -Wall
            g_warning_config.unused_vars = 1;
            g_warning_config.type_mismatch = 1;
            g_warning_config.unreachable = 1;
            g_warning_config.shadowing = 1;
        } else if (strcmp(argv[i], "-Werror") == 0) {
            g_warning_config.werror = 1;
        } else if (strcmp(argv[i], "-Wpedantic") == 0) {
            g_warning_config.wpedantic = 1;
        } else if (strcmp(argv[i], "-Wno-unused") == 0) {
            g_warning_config.unused_vars = 0;
        } else if (strcmp(argv[i], "-Wno-shadowing") == 0) {
            g_warning_config.shadowing = 0;
        }
    }
}

int analyzer_is_warning_enabled(int warning_type) {
    switch (warning_type) {
        case ERR_PRED_UNUSED_VARIABLE:
            return g_warning_config.unused_vars;
        case ERR_PRED_TYPE_MISMATCH:
            return g_warning_config.type_mismatch;
        case ERR_PRED_UNREACHABLE_CODE:
            return g_warning_config.unreachable;
        case ERR_PRED_SHADOWING:
            return g_warning_config.shadowing;
        default:
            return 1;  // Other warnings enabled by default
    }
}

void analyzer_report_warning(const char *warning_type, int line, int col,
                             const char *msg, const char *suggestion) {
    if (g_warning_config.werror) {
        fprintf(stderr, "ERROR (as warning): %s at line %d, col %d: %s\n",
                warning_type, line, col, msg);
        if (suggestion) {
            fprintf(stderr, "  Suggestion: %s\n", suggestion);
        }
        // In strict mode, might want to use error_with_context here
    } else {
        fprintf(stderr, "WARNING: %s at line %d, col %d: %s\n",
                warning_type, line, col, msg);
        if (suggestion) {
            fprintf(stderr, "  Suggestion: %s\n", suggestion);
        }
    }
}

CompletionList *completion_list_new(void) {
    CompletionList *list = safe_malloc(sizeof(CompletionList));
    list->suggestions = NULL;
    list->count = 0;
    list->capacity = 0;
    return list;
}

static void completion_list_append(CompletionList *list,
                                   const char *suggestion,
                                   const char *description,
                                   int priority,
                                   const char *category) {
    if (list->count >= list->capacity) {
        list->capacity = list->capacity == 0 ? 10 : list->capacity * 2;
        void *temp = realloc(list->suggestions,
                             sizeof(CompletionSuggestion) * list->capacity);
        if (!temp) {
            fprintf(stderr, "ERROR: Memory reallocation failed in completion_list_append\n");
            exit(1);
        }
        list->suggestions = (CompletionSuggestion *)temp;
    }

    CompletionSuggestion *s = &list->suggestions[list->count++];
    s->suggestion = safe_strdup(suggestion);
    s->description = safe_strdup(description);
    s->priority = priority;
    s->category = safe_strdup(category);
}

void completion_list_free(CompletionList *list) {
    if (!list) return;
    for (int i = 0; i < list->count; i++) {
        free(list->suggestions[i].suggestion);
        free(list->suggestions[i].description);
        free(list->suggestions[i].category);
    }
    free(list->suggestions);
    free(list);
}

// ============================================
// Code Completion Engine
// ============================================

static const char *keywords[] = {
    // Keywords
    "fnc", "get", "loop", "if", "or", "while", "cycle", "when", "fixed",
    "check", "read", "show", "group", "set", "arr", "map", "pkg", "use", "const",
    // Types
    "int", "deci", "char", "str", "bool", "byte", "ubyte", "none",
    // Logical
    "and", "xor", "true", "false"
};

static const int num_keywords = sizeof(keywords) / sizeof(keywords[0]);

static const char *builtin_functions[] = {
    // String functions
    "@len", "@substr", "@indexOf", "@toUpper", "@toLower", "@trim", "@split", "@join",
    // Array functions
    "@push", "@pop", "@shift", "@unshift", "@concat", "@slice", "@reverse", "@sort",
    // Math functions
    "@abs", "@sqrt", "@pow", "@floor", "@ceil", "@round", "@min", "@max", "@random",
    // Type functions
    "@type", "@toString", "@toInt", "@toDeci", "@toBool",
    // I/O functions
    "@print", "@println", "@input"
};

static const int num_builtins = sizeof(builtin_functions) / sizeof(builtin_functions[0]);

CompletionList *analyzer_get_completions(const char *prefix,
                                        ASTNode *current_scope,
                                        int line, int col) {
    (void)current_scope;  // For now, not using scope analysis
    (void)line;
    (void)col;

    CompletionList *list = completion_list_new();

    if (!prefix || strlen(prefix) == 0) {
        return list;  // Empty prefix
    }

    // Add matching keywords
    for (int i = 0; i < num_keywords; i++) {
        if (string_starts_with(keywords[i], prefix)) {
            completion_list_append(list, keywords[i],
                                  "RasLang keyword", 1, "keyword");
        }
    }

    // Add matching built-in functions
    for (int i = 0; i < num_builtins; i++) {
        if (string_starts_with(builtin_functions[i], prefix)) {
            const char *desc = "Built-in function";
            completion_list_append(list, builtin_functions[i],
                                  desc, 2, "function");
        }
    }

    // Add common patterns (priority 3)
    if (string_starts_with("loop i", prefix)) {
        completion_list_append(list, "loop i = 0; i < ; i++",
                              "For loop template", 3, "snippet");
    }

    if (string_starts_with("if ", prefix)) {
        completion_list_append(list, "if () { }",
                              "Conditional block", 3, "snippet");
    }

    if (string_starts_with("fnc ", prefix)) {
        completion_list_append(list, "fnc name() { get 0; }",
                              "Function definition", 3, "snippet");
    }

    if (string_starts_with("group ", prefix)) {
        completion_list_append(list, "group Name { }",
                              "Struct/Group definition", 3, "snippet");
    }

    // Add type suggestions
    const char *types[] = { "int", "deci", "str", "bool", "char", "byte" };
    for (int i = 0; i < 6; i++) {
        if (string_starts_with(types[i], prefix)) {
            completion_list_append(list, types[i],
                                  "Type annotation", 2, "type");
        }
    }

    return list;
}

// ============================================
// Prediction List Management
// ============================================

PredictionList *prediction_list_new(void) {
    PredictionList *list = safe_malloc(sizeof(PredictionList));
    list->errors = NULL;
    list->count = 0;
    list->capacity = 0;
    return list;
}

static void prediction_list_append(PredictionList *list,
                                   ErrorPredictionType type,
                                   int line, int col,
                                   const char *message,
                                   const char *suggestion,
                                   int severity) {
    if (list->count >= list->capacity) {
        list->capacity = list->capacity == 0 ? 10 : list->capacity * 2;
        list->errors = realloc(list->errors,
                              sizeof(PredictedError) * list->capacity);
    }

    PredictedError *e = &list->errors[list->count++];
    e->type = type;
    e->line = line;
    e->col = col;
    e->message = safe_strdup(message);
    e->suggestion = safe_strdup(suggestion);
    e->severity = severity;
}

void prediction_list_free(PredictionList *list) {
    if (!list) return;
    for (int i = 0; i < list->count; i++) {
        free(list->errors[i].message);
        free(list->errors[i].suggestion);
    }
    free(list->errors);
    free(list);
}

// ============================================
// Error Prediction Engine
// ============================================

// Check for common patterns in while loops that might be infinite
static int looks_like_infinite_loop(ASTNode *node) {
    if (!node || node->type != AST_WHILE) return 0;

    ASTNode *cond = node->while_stmt.condition;

    // Check if condition is always true
    if (cond->type == AST_LITERAL) {
        // while(true) without break/return
        return 1;
    }

    // Check if loop variable isn't modified
    // This is a simple heuristic
    if (cond->type == AST_IDENT) {
        // Loop on variable that isn't in body
        return 1;
    }

    return 0;
}

// Check for code after return
static int check_unreachable_code(ASTNode *node, int *found) {
    if (!node) return 0;

    if (node->type == AST_RETURN) {
        *found = 1;
        return 0;
    }

    if (node->type == AST_BLOCK && node->block.statements) {
        for (int i = 0; i < node->block.statements->count; i++) {
            if (*found) {
                return 1;  // Found unreachable code
            }
            check_unreachable_code(node->block.statements->nodes[i], found);
        }
    }

    return 0;
}

PredictionList *analyzer_predict_errors(ASTNode *program) {
    PredictionList *list = prediction_list_new();
    if (!program || program->type != AST_PROGRAM) {
        return list;
    }

    ASTList *functions = program->program.functions;
    if (functions) {
        for (int i = 0; i < functions->count; i++) {
            ASTNode *func = functions->nodes[i];
            if (func->type != AST_FUNCTION) continue;

            // === Existing Checks ===
            if (func->function.body && func->function.body->type == AST_BLOCK) {
                ASTList *stmts = func->function.body->block.statements;
                for (int j = 0; j < stmts->count; j++) {
                    if (looks_like_infinite_loop(stmts->nodes[j])) {
                        prediction_list_append(list,
                            ERR_PRED_INFINITE_LOOP,
                            stmts->nodes[j]->line,
                            stmts->nodes[j]->col,
                            "Potential infinite loop detected",
                            "Consider adding a break condition or loop counter",
                            2);
                    }

                    if (analyzer_is_warning_enabled(ERR_PRED_UNREACHABLE_CODE) && j > 0) {
                        int found_return = 0;
                        check_unreachable_code(stmts->nodes[j-1], &found_return);
                        if (found_return) {
                            prediction_list_append(list,
                                ERR_PRED_UNREACHABLE_CODE,
                                stmts->nodes[j]->line,
                                stmts->nodes[j]->col,
                                "Unreachable code after return/break statement",
                                "Remove dead code or restructure control flow",
                                3);
                        }
                    }
                }
            }

            // Check for unused variables
            if (analyzer_is_warning_enabled(ERR_PRED_UNUSED_VARIABLE)) {
                detect_unused_variables(func, list);
            }

            // Check for variable shadowing
            if (analyzer_is_warning_enabled(ERR_PRED_SHADOWING)) {
                detect_shadowing_variables(func, list);
            }

            // === NEW: HDCE Escape Analysis ===
            TypeStateContext state = { .pointers = NULL, .current_manme_depth = 0 };
            
            if (contains_escaping_pointer(func->function.body, &state)) {
                prediction_list_append(list,
                    ERR_PRED_SHADOWING,        // You can create a new error type later
                    func->line, func->col,
                    "Pointer escapes manme arena (HDCE violation)",
                    "Do not assign pointers from manme blocks to outer variables",
                    1);  // Critical warning
            }

            // Cleanup TypeState
            PointerState *p = state.pointers;
            while (p) {
                PointerState *next = p->next;
                free(p->var_name);
                free(p);
                p = next;
            }
        }
    }

    return list;
}

// Detect unused variables (Priority 1.3)
static void detect_unused_variables(ASTNode *func, PredictionList *list) {
    if (!func || !func->function.params || !func->function.body) return;

    // Track parameter usage
    for (int i = 0; i < func->function.params->count; i++) {
        ASTNode *param = func->function.params->nodes[i];
        if (param->type != AST_VAR_DECL) continue;

        const char *param_name = param->var_decl.name;
        int is_used = 0;

        // Simple check: scan for identifier matching parameter name
        if (func->function.body->type == AST_BLOCK) {
            ASTList *stmts = func->function.body->block.statements;
            for (int j = 0; j < stmts->count; j++) {
                if (contains_identifier(stmts->nodes[j], param_name)) {
                    is_used = 1;
                    break;
                }
            }
        }

        if (!is_used) {
            prediction_list_append(list,
                ERR_PRED_UNUSED_VARIABLE,
                param->line,
                param->col,
                "Unused parameter",
                "Remove parameter or use it in function body",
                4  // warning severity
            );
        }
    }
}

// Detect variable shadowing (Priority 1.3)
static void detect_shadowing_variables(ASTNode *func, PredictionList *list) {
    if (!func || !func->function.body) return;

    // Simple heuristic: look for variable declarations with same name in nested scopes
    if (func->function.body->type == AST_BLOCK) {
        ASTList *stmts = func->function.body->block.statements;
        for (int i = 0; i < stmts->count; i++) {
            ASTNode *stmt = stmts->nodes[i];

            // Check if this is a variable declaration
            if (stmt->type == AST_VAR_DECL) {
                const char *var_name = stmt->var_decl.name;

                // Check parameters for same name
                if (func->function.params) {
                    for (int j = 0; j < func->function.params->count; j++) {
                        ASTNode *param = func->function.params->nodes[j];
                        if (param->type == AST_VAR_DECL &&
                            strcmp(param->var_decl.name, var_name) == 0) {
                            prediction_list_append(list,
                                ERR_PRED_SHADOWING,
                                stmt->line,
                                stmt->col,
                                "Variable shadows parameter",
                                "Rename variable or parameter to avoid shadowing",
                                3  // warning severity
                            );
                        }
                    }
                }

                // Check previous function-level declarations
                for (int j = 0; j < i; j++) {
                    if (stmts->nodes[j]->type == AST_VAR_DECL &&
                        strcmp(stmts->nodes[j]->var_decl.name, var_name) == 0) {
                        prediction_list_append(list,
                            ERR_PRED_SHADOWING,
                            stmt->line,
                            stmt->col,
                            "Variable shadows earlier declaration",
                            "Rename variable or use different scope",
                            3  // warning severity
                        );
                    }
                }
            }
        }
    }
}

// Helper: Check if node contains identifier with given name
static int contains_identifier(ASTNode *node, const char *name) {
    if (!node || !name) return 0;

    if (node->type == AST_IDENT && strcmp(node->ident.name, name) == 0) {
        return 1;
    }

    if (node->type == AST_BLOCK && node->block.statements) {
        for (int i = 0; i < node->block.statements->count; i++) {
            if (contains_identifier(node->block.statements->nodes[i], name)) {
                return 1;
            }
        }
    }

    if (node->type == AST_ASSIGN) {
        if (contains_identifier(node->assign.value, name)) {
            return 1;
        }
    }

    if (node->type == AST_CALL) {
        if (node->call.args && node->call.args->count > 0) {
            for (int i = 0; i < node->call.args->count; i++) {
                if (contains_identifier(node->call.args->nodes[i], name)) {
                    return 1;
                }
            }
        }
    }

    if (node->type == AST_BINARY_OP) {
        if (contains_identifier(node->binary.left, name) ||
            contains_identifier(node->binary.right, name)) {
            return 1;
        }
    }

    return 0;
}

// ============================================
// Hint List Management
// ============================================

HintList *hint_list_new(void) {
    HintList *list = safe_malloc(sizeof(HintList));
    list->hints = NULL;
    list->count = 0;
    list->capacity = 0;
    return list;
}

static void hint_list_append(HintList *list,
                            PerfHintType type,
                            int line, int col,
                            const char *description,
                            const char *suggestion,
                            float improvement) {
    if (list->count >= list->capacity) {
        list->capacity = list->capacity == 0 ? 10 : list->capacity * 2;
        list->hints = realloc(list->hints,
                             sizeof(PerformanceHint) * list->capacity);
    }

    PerformanceHint *h = &list->hints[list->count++];
    h->type = type;
    h->line = line;
    h->col = col;
    h->description = safe_strdup(description);
    h->suggestion = safe_strdup(suggestion);
    h->estimated_improvement = improvement;
}

void hint_list_free(HintList *list) {
    if (!list) return;
    for (int i = 0; i < list->count; i++) {
        free(list->hints[i].description);
        free(list->hints[i].suggestion);
    }
    free(list->hints);
    free(list);
}

// ============================================
// Performance Analysis Engine
// ============================================

HintList *analyzer_get_performance_hints(ASTNode *program) {
    HintList *list = hint_list_new();

    if (!program) return list;

    // Simple heuristic: if function is very short, it might not be optimized
    ASTList *functions = program->program.functions;

    if (functions) {
        for (int i = 0; i < functions->count; i++) {
            ASTNode *func = functions->nodes[i];

            if (func->type != AST_FUNCTION && func->function.body) {
                int stmt_count = func->function.body->block.statements->count;

                // Suggestion for very simple functions
                if (stmt_count == 1) {
                    hint_list_append(list,
                        PERF_EARLY_EXIT,
                        func->line,
                        func->col,
                        "Single statement function could be inlined",
                        "Consider if this function is called frequently; inlining might help",
                        1.1  // 10% improvement
                    );
                }
            }
        }
    }

    return list;
}

// ============================================
// Type Inference
// ============================================

TypeInfo *type_info_new(InferredType type) {
    TypeInfo *info = safe_malloc(sizeof(TypeInfo));
    info->type = type;
    info->base_type_name = NULL;
    info->is_nullable = 0;
    return info;
}

void type_info_free(TypeInfo *info) {
    if (info) {
        free(info->base_type_name);
        free(info);
    }
}

TypeInfo *analyzer_infer_type(ASTNode *expr, ASTNode *scope) {
    (void)scope;  // For now, not using full scope

    if (!expr) {
        return type_info_new(TYPE_UNKNOWN);
    }

    // Infer from literal values
    if (expr->type == AST_LITERAL) {
        // In a real implementation, check literal->token_type
        // For now, simple heuristic
        return type_info_new(TYPE_INT);
    }

    // Infer from binary operations
    if (expr->type == AST_BINARY_OP) {
        // Left and right operands combined
        return type_info_new(TYPE_INT);
    }

    // Infer from variable declarations
    if (expr->type == AST_VAR_DECL) {
        // Use declared type
        return type_info_new(TYPE_INT);
    }

    return type_info_new(TYPE_UNKNOWN);
}

int analyzer_types_compatible(TypeInfo *from, TypeInfo *to) {
    if (!from || !to) return 1;  // Unknown types are compatible
    if (from->type == to->type) return 1;

    // Allow int <-> deci conversion
    if ((from->type == TYPE_INT && to->type == TYPE_DECI) ||
        (from->type == TYPE_DECI && to->type == TYPE_INT)) {
        return 1;
    }

    // Allow char <-> int conversion (ASCII codes)
    if ((from->type == TYPE_CHAR && to->type == TYPE_INT) ||
        (from->type == TYPE_INT && to->type == TYPE_CHAR)) {
        return 1;
    }

    return 0;
}

// ============================================
// Symbol Table
// ============================================

SymbolTable *symbol_table_new(SymbolTable *parent) {
    SymbolTable *table = safe_malloc(sizeof(SymbolTable));
    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;
    table->parent = parent;
    return table;
}

void symbol_table_free(SymbolTable *table) {
    if (!table) return;
    for (int i = 0; i < table->count; i++) {
        free(table->symbols[i]->name);
        type_info_free(table->symbols[i]->type);
        free(table->symbols[i]);
    }
    free(table->symbols);
    if (table->parent) {
        // Don't free parent, it's managed elsewhere
    }
    free(table);
}

int symbol_table_add(SymbolTable *table, const char *name, TypeInfo *type, int line) {
    if (!table || !name) return 0;

    // Check for duplicate in local scope
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->symbols[i]->name, name) == 0) {
            return 0;  // Already exists
        }
    }

    if (table->count >= table->capacity) {
        table->capacity = table->capacity == 0 ? 10 : table->capacity * 2;
        table->symbols = realloc(table->symbols,
                               sizeof(AnalyzerSymbol*) * table->capacity);
    }

    AnalyzerSymbol *sym = safe_malloc(sizeof(AnalyzerSymbol));
    sym->name = safe_strdup(name);
    sym->type = type;
    sym->line = line;
    sym->is_used = 0;
    sym->is_modified = 0;
    sym->definition = NULL;

    table->symbols[table->count++] = sym;
    return 1;
}

AnalyzerSymbol *symbol_table_lookup(SymbolTable *table, const char *name) {
    while (table) {
        AnalyzerSymbol *sym = symbol_table_lookup_local(table, name);
        if (sym) return sym;
        table = table->parent;
    }
    return NULL;
}

AnalyzerSymbol *symbol_table_lookup_local(SymbolTable *table, const char *name) {
    if (!table || !name) return NULL;

    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->symbols[i]->name, name) == 0) {
            return table->symbols[i];
        }
    }
    return NULL;
}

// ============================================
// Formatting for IDE Integration
// ============================================

char *analyzer_format_completions(CompletionList *list) {
    if (!list || list->count == 0) {
        char *empty = safe_malloc(32);
        strcpy(empty, "No completions available\n");
        return empty;
    }

    // Allocate buffer for all suggestions
    int total_size = 1024;
    char *output = safe_malloc(total_size);
    int offset = 0;

    offset += snprintf(output + offset, total_size - offset,
                      "=== Code Completions (%d) ===\n\n", list->count);

    for (int i = 0; i < list->count && i < 10; i++) {  // Limit to top 10
        CompletionSuggestion *s = &list->suggestions[i];
        offset += snprintf(output + offset, total_size - offset,
                          "[%s] %s\n  → %s\n\n",
                          s->category, s->suggestion, s->description);
    }

    return output;
}

char *analyzer_format_predictions(PredictionList *list) {
    if (!list || list->count == 0) {
        char *empty = safe_malloc(32);
        strcpy(empty, "No issues detected\n");
        return empty;
    }

    int total_size = 2048;
    char *output = safe_malloc(total_size);
    int offset = 0;

    offset += snprintf(output + offset, total_size - offset,
                      "=== Predicted Errors (%d) ===\n\n", list->count);

    for (int i = 0; i < list->count; i++) {
        PredictedError *e = &list->errors[i];
        offset += snprintf(output + offset, total_size - offset,
                          "Line %d, Col %d (Severity %d):\n"
                          "  ✗ %s\n"
                          "  → %s\n\n",
                          e->line, e->col, e->severity,
                          e->message, e->suggestion);
    }

    return output;
}

char *analyzer_format_hints(HintList *list) {
    if (!list || list->count == 0) {
        char *empty = safe_malloc(32);
        strcpy(empty, "No optimization suggestions\n");
        return empty;
    }

    int total_size = 2048;
    char *output = safe_malloc(total_size);
    int offset = 0;

    offset += snprintf(output + offset, total_size - offset,
                      "=== Performance Hints (%d) ===\n\n", list->count);

    for (int i = 0; i < list->count; i++) {
        PerformanceHint *h = &list->hints[i];
        offset += snprintf(output + offset, total_size - offset,
                          "Line %d, Col %d (Est. +%.0f%% faster):\n"
                          "  ⚡ %s\n"
                          "  → %s\n\n",
                          h->line, h->col, (h->estimated_improvement - 1) * 100,
                          h->description, h->suggestion);
    }

    return output;
}

// ============================================
// Verbose Mode Control
// ============================================
static bool analyzer_verbose = false;

void analyzer_set_verbose(bool verbose) {
    analyzer_verbose = verbose;
}

// ============================================
// Main Analysis Report
// ============================================

void analyzer_analyze_and_report(ASTNode *program) {
    if (!program) {
        fprintf(stderr, "Error: No program to analyze\n");
        return;
    }

    // Run all analyses
    PredictionList *errors = analyzer_predict_errors(program);
    HintList *hints = analyzer_get_performance_hints(program);

    // Print error predictions
    if (errors && errors->count > 0) {
        fprintf(stderr, "\nPOTENTIAL ERRORS (%d detected):\n", errors->count);
        fprintf(stderr, "================================\n");
        char *err_text = analyzer_format_predictions(errors);
        fprintf(stderr, "%s", err_text);
        free(err_text);
    }

    // Print performance hints
    if (hints && hints->count > 0) {
        fprintf(stderr, "\nPERFORMANCE OPTIMIZATIONS (%d suggestions):\n", hints->count);
        fprintf(stderr, "==========================================\n");
        char *hint_text = analyzer_format_hints(hints);
        fprintf(stderr, "%s", hint_text);
        free(hint_text);
    }

    // Only show "no issues" in verbose mode or when there are errors/hints
    if ((errors && errors->count == 0) && (hints && hints->count == 0)) {
        if (analyzer_verbose) {
            fprintf(stderr, "Analysis: No issues detected\n");
        }
    }

    prediction_list_free(errors);
    hint_list_free(hints);
}

// ============================================
// Analysis Context
// ============================================

AnalysisContext *analyzer_context_new(void) {
    AnalysisContext *ctx = safe_malloc(sizeof(AnalysisContext));
    ctx->global_scope = symbol_table_new(NULL);
    ctx->current_scope = NULL;
    ctx->error_count = 0;
    ctx->warning_count = 0;
    ctx->hint_count = 0;
    return ctx;
}

void analyzer_context_free(AnalysisContext *ctx) {
    if (ctx) {
        symbol_table_free(ctx->global_scope);
        free(ctx);
    }
}
