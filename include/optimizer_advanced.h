/*
 * optimizer_advanced.h - Advanced optimization passes for rascom
 * 
 * Super-fast code generation through aggressive optimizations:
 * - Constant folding & propagation
 * - Dead code elimination  
 * - Common subexpression elimination
 * - Loop invariant code motion
 * - Strength reduction
 * - Peephole optimization
 * - Register allocation optimization
 * - Branch prediction optimization
 * - Inline caching
 * 
 * Goal: Generated code FASTER than C
 */

#ifndef OPTIMIZER_ADVANCED_H
#define OPTIMIZER_ADVANCED_H

#include <stdint.h>
#include <stdbool.h>

/* ============================================
 * OPTIMIZATION RESULT STRUCTURE
 * ============================================ */

typedef struct {
    /* Input/Output */
    char **optimized_instructions;      /* Optimized assembly */
    int optimized_instruction_count;
    int original_instruction_count;
    
    /* Optimization Level */
    int optimization_level;             /* 0-3 */
    
    /* Statistics */
    int constants_folded;
    int dead_code_removed;
    int cse_eliminations;               /* Common subexpression eliminations */
    int strength_reductions;            /* Expensive op -> cheap op */
    int peephole_optimizations;
    int loop_invariant_motions;
    int functions_inlined;
    int register_optimizations;
    int branch_optimizations;
    int inline_caches_created;
    
    /* Performance Metrics */
    int bytes_saved;
    float estimated_speedup_percent;    /* Conservative 10-40% */
    int cycle_savings;                  /* Estimated cycle count reduction */
    
    /* Cache metrics */
    int l1_cache_hits;
    int memory_bandwidth_saved;
} OptimizationResult;

/* ============================================
 * PUBLIC API
 * ============================================ */

/**
 * Apply all optimization passes to assembly
 * 
 * opt_level:
 *   0 - No optimizations (baseline)
 *   1 - Basic (constant fold, constant propagation)
 *   2 - Aggressive (+ CSE, strength reduction, dead code)
 *   3 - Maximum (+ loop motion, peephole, inline caching)
 * 
 * Returns optimization statistics and optimized code
 */
OptimizationResult optim_advanced_compile(
    const char **asm_instructions,
    int count,
    int opt_level
);

/**
 * Pretty-print optimization statistics
 */
void optim_result_print_stats(OptimizationResult *result);

/* ============================================
 * INDIVIDUAL OPTIMIZATION PASSES
 * ============================================ */

/**
 * PASS 1: Constant Folding & Propagation
 * 
 * Evaluates constant expressions at compile time
 * Example: mov rax, 100; imul rax, 2 => mov rax, 200
 * 
 * Returns: 1 if folded, -1 if cannot fold
 */
int optim_constant_fold_asm(const char *instr, char *result);

/**
 * PASS 2: Dead Code Elimination
 * 
 * Removes instructions whose results are never used
 * Example: mov rax, 100; [never use rax] => removed
 * 
 * Returns: Count of optimized instructions
 */
int optim_dead_code_elimination(
    const char **instructions,
    int count,
    char **optimized_out
);

/**
 * PASS 3: Common Subexpression Elimination (CSE)
 * 
 * Caches repeated computations
 * Example: imul rax, 2; ... imul rbx, 2 => imul rax, 2; mov rbx, rax
 * Speedup: 1 cycle vs 3 cycles (198% improvement)
 * 
 * Returns: Count of optimized instructions
 */
int optim_cse(
    const char **instructions,
    int count,
    char **optimized_out
);

/**
 * PASS 4: Loop Invariant Code Motion
 * 
 * Moves constant computations outside loops
 * Example: mov rax, 100; jmp .loop; add rax, 1; ... => 
 *          mov rax, 100; .loop: add rax, 1; ...
 * 
 * Returns: Count of optimized instructions
 */
int optim_loop_invariant_motion(
    const char **instructions,
    int count,
    char **optimized_out
);

/**
 * PASS 5: Strength Reduction
 * 
 * Replace expensive operations with cheaper ones
 * Examples:
 *   x * 2 => x + x (1 cycle vs 3)
 *   x * 4 => shl x, 2 (1 cycle vs 3)
 *   x / 2 => shr x, 1 (1 cycle vs 10)
 * 
 * Returns: Count of optimized instructions
 */
int optim_strength_reduction(
    const char **instructions,
    int count,
    char **optimized_out
);

/**
 * PASS 6: Peephole Optimization
 * 
 * Optimize small instruction sequences
 * Examples:
 *   add rax, 0 => (remove)
 *   mov rax, 1; mov rax, 2 => mov rax, 2 (keep only last)
 *   jmp .end; .unused: ... => remove dead code
 * 
 * Returns: Count of optimized instructions
 */
int optim_peephole(
    const char **instructions,
    int count,
    char **optimized_out
);

/**
 * PASS 7: Register Allocation Optimization
 * 
 * Optimize register usage patterns
 * Strategies:
 *   - Keep loop-critical values in registers
 *   - Use caller-saved (rax, rcx, rdx) for temporaries
 *   - Use callee-saved (rbx, r12-15) for long-lived values
 *   - Minimize stack spills
 * 
 * Returns: Count of optimized instructions
 */
int optim_register_allocation(
    const char **instructions,
    int count,
    char **optimized_out
);

/**
 * PASS 8: Branch Prediction Optimization
 * 
 * Optimize patterns for CPU branch predictor
 * Strategies:
 *   - Likely branches taken forward (loops go backward)
 *   - Unlikely branches taken backward
 *   - Conditional moves instead of branches where possible
 * 
 * Returns: Count of optimized instructions
 */
int optim_branch_prediction(
    const char **instructions,
    int count,
    char **optimized_out
);

/**
 * PASS 9: Inline Caching
 * 
 * Cache repeated variable/function accesses
 * Example: mov rax, var; ... ; mov rax, var => mov rax, var; ... (reuse rax)
 * 
 * Returns: Count of optimized instructions
 */
int optim_inline_caching(
    const char **instructions,
    int count,
    char **optimized_out
);

/* ============================================
 * PERFORMANCE ANALYSIS
 * ============================================ */

/**
 * Estimate x86-64 cycle cost for an instruction
 */
int optim_get_instruction_cycles(const char *instruction);

/**
 * Analyze instruction dependencies for scheduling
 */
void optim_analyze_dependencies(const char *instr, char **deps, int *dep_count);

/**
 * Detect SIMD optimization opportunities
 */
bool optim_detect_simd_vector(const char **instructions, int count);

/* ============================================
 * CONFIGURATION & TUNING
 * ============================================ */

/**
 * Set target CPU profile for optimization
 * Profiles: "generic", "skylake", "zen", "zen3", "atom"
 */
void optim_set_cpu_profile(const char *profile);

/**
 * Set cache line size for optimization (typically 64 bytes)
 */
void optim_set_cache_line_size(int bytes);

/**
 * Enable/disable specific optimization passes
 */
void optim_enable_pass(const char *pass_name, bool enable);

#endif /* OPTIMIZER_ADVANCED_H */
