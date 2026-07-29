#ifndef CODEGEN_H
#define CODEGEN_H

#include "common.h"
#include "ast.h"
#include "elfgen.h"
#include <stdarg.h>

// Group field definition
typedef struct GroupField {
    char *name;
    char *type;
    int offset;              // Offset within the struct
    int size;                // Size of this field
    struct GroupField *next;
} GroupField;

// Group type definition
typedef struct GroupType {
    char *name;
    int total_size;          // Total size of the struct
    GroupField *fields;      // Linked list of fields
    struct GroupType *next;
} GroupType;

// Symbol table entry for tracking variables
typedef struct Symbol {
    char *name;
    int offset;         // Stack offset from rbp
    int size;           // Total size in bytes
    char *type;         // Variable type (or group type name)
    bool is_array;
    bool is_array_param;  // True if array passed as parameter (pointer), false if local array
    int array_size;     // Number of elements if array
    int element_size;   // Size of each element if array
    bool is_group;      // True if this is a group variable
    bool is_map;        // True if this is a map variable
    char *key_type;     // Map key type
    char *value_type;   // Map value type
    int capacity;       // Map capacity (number of buckets)
    struct Symbol *next;
} Symbol;

// Function signature tracking for return type and parameter info
typedef struct FunctionSignature {
    char *name;
    char *return_type;
    char **param_types;
    int param_count;
    int required_param_count;   // Number of parameters without defaults
    ASTNode **default_values;   // Array of default expressions (NULL if no default)
    struct FunctionSignature *next;
} FunctionSignature;

typedef struct {
    FILE *output;
    ELFGenerator *elf_gen;  // ELF generator for direct machine code
    int label_count;
    int stack_offset;
    Symbol *symbols;     // Symbol table (linked list)
    GroupType *groups;   // Group type definitions
    ASTList *constants;  // Global constants list
    FunctionSignature *functions;  // Function signatures (for return type lookup)
    const char *current_function_return_type;  // Current function's return type (for codegen_return)
    int current_function_epilogue;  // Current function's epilogue label for returns
    int use_elf_gen;     // 1 = use ELF generator, 0 = use FILE output (legacy)
    
    // Loop context for break/continue support
    int loop_break_label;      // Label to jump to on break
    int loop_continue_label;   // Label to jump to on continue
    
    // Defer statement tracking (simplified: store as-is for block scope)
    // In a full implementation, this would use a stack
    // For now, we'll handle defers at the block/function level
    ASTList *deferred_stmts;  // Queue of deferred statements
    
    // SIMD Vectorization Support
    bool enable_simd;          // Global flag to enable SIMD optimizations
    bool in_simd_loop;         // Currently processing a SIMD-optimized loop
    int vector_width;          // 4 for 256-bit (4 doubles), 8 for 512-bit
    int unroll_iteration;      // Current iteration in unrolled loop (0-3), -1 if not unrolled
    
    // Performance Optimization Tracking
    int loop_depth;            // Current nesting depth (0 = not in loop, 1+ = nested loop level)
    bool enable_prefetch;      // Enable prefetch hints for cache optimization
    int last_prefetch_rip;     // Last RIP offset where prefetch was emitted
    
    // Phase 3: Register Allocation for Loop Variables
    bool enable_reg_alloc;     // Enable hot loop variable in-register allocation
    const char *loop_var_i;    // Loop variable allocated to r8 (i)
    const char *loop_var_j;    // Loop variable allocated to r9 (j)
    const char *loop_var_k;    // Loop variable allocated to r10 (k)
    const char *loop_var_sum;  // Accumulator allocated to r11 (sum/result)
} CodeGen;

// Code generator functions
CodeGen *codegen_new(const char *output_file);
void codegen_free(CodeGen *gen);
void codegen_generate(CodeGen *gen, ASTNode *ast);

#endif // CODEGEN_H
