/*
 * analyzer.h - Intelligent Language Analysis for RasCode
 * 
 * Built-in features for RasCode developers:
 * - Smart code completion
 * - Error prediction and prevention
 * - Performance hints and optimizations
 * - Type inference helpers
 * 
 * No external dependencies - pure analysis
 */

#ifndef ANALYZER_H
#define ANALYZER_H

#include "common.h"
#include "ast.h"
#include "lexer.h"

// ============================================
// Code Completion
// ============================================

typedef struct {
    char *suggestion;    // Suggested code
    char *description;   // What it does
    int priority;        // 1 (high) to 5 (low)
    char *category;      // "keyword", "function", "variable"
} CompletionSuggestion;

typedef struct {
    CompletionSuggestion *suggestions;
    int count;
    int capacity;
} CompletionList;

// Get smart completions based on context
CompletionList *analyzer_get_completions(
    const char *prefix,           // Partial word being typed
    ASTNode *current_scope,       // Current AST scope
    int line,                     // Current line
    int col                       // Current column
);

void completion_list_free(CompletionList *list);

// ============================================
// Error Prediction
// ============================================

typedef enum {
    ERR_PRED_UNINITIALIZED,      // Use of uninitialized variable
    ERR_PRED_TYPE_MISMATCH,      // Type mismatch in assignment
    ERR_PRED_ARRAY_BOUNDS,       // Possible out-of-bounds access
    ERR_PRED_NULL_DEREFERENCE,   // Possible null pointer
    ERR_PRED_INFINITE_LOOP,      // Detected infinite loop pattern
    ERR_PRED_UNREACHABLE_CODE,   // Code after return
    ERR_PRED_UNUSED_VARIABLE,    // Variable assigned but never used
    ERR_PRED_SHADOWING,          // Variable shadows outer scope
} ErrorPredictionType;

typedef struct {
    ErrorPredictionType type;
    int line;
    int col;
    char *message;               // Human description
    char *suggestion;            // How to fix it
    int severity;                // 1 (critical) to 5 (info)
} PredictedError;

typedef struct {
    PredictedError *errors;
    int count;
    int capacity;
} PredictionList;

// Analyze AST for potential errors
PredictionList *analyzer_predict_errors(ASTNode *program);

void prediction_list_free(PredictionList *list);

// ============================================
// Performance Hints
// ============================================

typedef enum {
    PERF_LOOP_INVARIANT,         // Move calculation outside loop
    PERF_ARRAY_OPTIMIZATION,     // Use array instead of repeated calls
    PERF_CACHE_OPTIMIZATION,     // Improve memory locality
    PERF_ALGORITHM_SUGGESTION,   // More efficient algorithm exists
    PERF_STRING_CONCAT,          // Optimize string concatenation
    PERF_EARLY_EXIT,             // Add early exit condition
} PerfHintType;

typedef struct {
    PerfHintType type;
    int line;
    int col;
    char *description;           // What's inefficient
    char *suggestion;            // How to optimize
    float estimated_improvement; // 1.2 = 20% faster
} PerformanceHint;

typedef struct {
    PerformanceHint *hints;
    int count;
    int capacity;
} HintList;

// Analyze AST for performance opportunities
HintList *analyzer_get_performance_hints(ASTNode *program);

void hint_list_free(HintList *list);

// ============================================
// Type Inference
// ============================================

typedef enum {
    TYPE_UNKNOWN,
    TYPE_INT,
    TYPE_DECI,
    TYPE_CHAR,
    TYPE_STR,
    TYPE_BOOL,
    TYPE_BYTE,
    TYPE_UBYTE,
    TYPE_ARRAY,
    TYPE_GROUP,
    TYPE_MAP,
} InferredType;

typedef struct {
    InferredType type;
    char *base_type_name;        // For arrays/groups: "int[]", "MyGroup"
    int is_nullable;             // Can be NULL/none
} TypeInfo;

// Infer type of an expression
TypeInfo *analyzer_infer_type(ASTNode *expr, ASTNode *scope);

void type_info_free(TypeInfo *info);

// Type compatibility check
int analyzer_types_compatible(TypeInfo *from, TypeInfo *to);

// ============================================
// Symbol Table Management (for scope analysis)
// ============================================

typedef struct AnalyzerSymbol {
    char *name;                  // Variable/function name
    TypeInfo *type;              // Its type
    int line;                    // Where declared
    int is_used;                 // Has been referenced
    int is_modified;             // Has been assigned
    ASTNode *definition;         // AST node defining it
} AnalyzerSymbol;

typedef struct SymbolTable_s {
    AnalyzerSymbol **symbols;
    int count;
    int capacity;
    struct SymbolTable_s *parent;  // Parent scope
} SymbolTable;

SymbolTable *symbol_table_new(SymbolTable *parent);
void symbol_table_free(SymbolTable *table);

int symbol_table_add(SymbolTable *table, const char *name, TypeInfo *type, int line);
AnalyzerSymbol *symbol_table_lookup(SymbolTable *table, const char *name);
AnalyzerSymbol *symbol_table_lookup_local(SymbolTable *table, const char *name);

// ============================================
// Analysis Context
// ============================================

typedef struct {
    SymbolTable *global_scope;
    ASTNode *current_scope;
    int error_count;
    int warning_count;
    int hint_count;
} AnalysisContext;

AnalysisContext *analyzer_context_new(void);
void analyzer_context_free(AnalysisContext *ctx);

// ============================================
// Public API
// ============================================

// Run full analysis and print recommendations
void analyzer_analyze_and_report(ASTNode *program);

// Get formatted completion string for IDE integration
char *analyzer_format_completions(CompletionList *list);

// Get formatted error predictions for IDE integration
char *analyzer_format_predictions(PredictionList *list);

// Get formatted hints for IDE integration
char *analyzer_format_hints(HintList *list);

// ============================================
// Warnings Configuration (Priority 1.3)
// ============================================

typedef enum {
    WARN_LEVEL_NONE = 0,      // No warnings
    WARN_LEVEL_ALL = 1,       // -Wall: common warnings
    WARN_LEVEL_EXTRA = 2,     // -Wextra: extra warnings
    WARN_LEVEL_ERROR = 4,     // -Werror: treat warnings as errors
    WARN_LEVEL_PEDANTIC = 8,  // -Wpedantic: very strict
} WarningLevel;

typedef struct {
    int wall;          // Enable common warnings (-Wall)
    int wextra;        // Enable extra warnings (-Wextra)
    int werror;        // Treat warnings as errors (-Werror)
    int wpedantic;     // Pedantic warnings (-Wpedantic)
    int unused_vars;   // Warn about unused variables
    int type_mismatch; // Warn about type mismatches
    int unreachable;   // Warn about unreachable code
    int shadowing;     // Warn about variable shadowing
} WarningConfig;

// Global warning configuration
extern WarningConfig g_warning_config;

// Set warning configuration from command line flags
void analyzer_set_warning_flags(int argc, char **argv);

// Check if a specific warning is enabled
int analyzer_is_warning_enabled(int warning_type);

// Report warning message with source context
void analyzer_report_warning(const char *warning_type, int line, int col, 
                             const char *msg, const char *suggestion);

#endif // ANALYZER_H
