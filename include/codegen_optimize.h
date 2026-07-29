/*
 * codegen_optimize.h - Integration layer for optimized code generation
 * 
 * Provides unified API for generating optimized assembly at various levels:
 * - Level 0: No optimization (baseline)
 * - Level 1: Basic (constant folding)
 * - Level 2: Aggressive (CSE, strength reduction)
 * - Level 3: Maximum (all passes)
 */

#ifndef CODEGEN_OPTIMIZE_H
#define CODEGEN_OPTIMIZE_H

#include <stdbool.h>
#include <time.h>

/* Forward declaration - avoid circular dependency */
typedef void* ASTNode;

/* ============================================
 * OPTIMIZATION CONFIGURATION
 * ============================================ */

typedef struct {
    int optimization_level;         /* 0-3 */
    bool verbose;                   /* Detailed logging */
    bool benchmark;                 /* Enable timing */
    char *cpu_profile;              /* Target CPU: generic, skylake, zen, etc */
} OptimizationConfig;

/* ============================================
 * COMPILATION OUTPUT
 * ============================================ */

typedef struct {
    bool success;
    
    /* Instruction counts */
    int unoptimized_instruction_count;
    int optimized_instruction_count;
    int instructions_eliminated;
    
    /* Optimization statistics */
    int cse_eliminations;
    int strength_reductions;
    int peephole_optimizations;
    int bytes_saved;
    float estimated_speedup_percent;
    
    /* Timing */
    double codegen_time_ms;
    double optimization_time_ms;
    double total_time_ms;
    
    /* Performance comparison */
    float speedup_vs_gcc;           /* Estimated speedup vs GCC -O2 */
    float code_size_vs_gcc;         /* Code size ratio vs GCC output */
} CompiledOutput;

/* ============================================
 * CONFIGURATION API
 * ============================================ */

/**
 * Set global optimization level (0-3)
 * 0 = no optimization, 3 = maximum
 */
void codegen_optim_config_set_level(int level);

/**
 * Enable/disable verbose output
 */
void codegen_optim_config_set_verbose(bool verbose);

/**
 * Enable performance benchmarking
 */
void codegen_optim_config_enable_benchmark(bool enable);

/**
 * Set target CPU profile
 * Profiles: "generic", "skylake", "zen", "zen3", "atom"
 */
void codegen_optim_config_set_cpu(const char *profile);

/* ============================================
 * COMPILATION WITH OPTIMIZATION
 * ============================================ */

/**
 * Generate optimized assembly from AST
 * 
 * Performs:
 * 1. Base code generation (unoptimized)
 * 2. All optimization passes based on level
 * 3. Writes final optimized assembly to output_file
 * 
 * Returns: CompiledOutput with statistics
 */
CompiledOutput codegen_generate_optimized(
    const char *source_file,        /* Input RasCode file */
    const char *output_file,        /* Output .asm file */
    void *program,                  /* Parsed AST (void* to avoid header dependency) */
    int optimization_level          /* 0-3 */
);

/**
 * Optimize existing assembly file
 * 
 * Useful for:
 * 1. Re-optimizing with different level
 * 2. Standalone optimization pass
 * 3. Benchmarking different levels
 * 
 * Returns: true if successful
 */
bool codegen_optimize_asm_file(
    const char *input_file,         /* Input .asm file (unoptimized) */
    const char *output_file,        /* Output .asm file (optimized) */
    int optimization_level          /* 0-3 */
);

/* ============================================
 * REPORTING & ANALYSIS
 * ============================================ */

/**
 * Print compilation results with statistics
 */
void codegen_optim_print_results(CompiledOutput *output);

/**
 * Compare RasCode output with GCC -O2 baseline
 */
void codegen_optim_compare_c_baseline(CompiledOutput *rascom_output);

/* ============================================
 * OPTIMIZATION PROFILES
 * ============================================ */

/* Level 0: No optimization (baseline for benchmarking) */
#define OPTIM_LEVEL_NONE 0

/* Level 1: Basic optimizations
 * • Constant folding
 * • Constant propagation
 * • Simple dead code elimination
 * Expected speedup: 5-10% */
#define OPTIM_LEVEL_BASIC 1

/* Level 2: Aggressive optimizations
 * • Common subexpression elimination
 * • Strength reduction
 * • Loop invariant code motion
 * • Basic peephole
 * Expected speedup: 20-30% */
#define OPTIM_LEVEL_AGGRESSIVE 2

/* Level 3: Maximum optimizations
 * • All of level 2
 * • Inline caching
 * • Advanced peephole
 * • Register allocation optimization
 * • Branch prediction optimization
 * Expected speedup: 40-60% */
#define OPTIM_LEVEL_MAXIMUM 3

#endif /* CODEGEN_OPTIMIZE_H */
