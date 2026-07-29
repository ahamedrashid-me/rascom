/*
 * optimization_pass_simd.h - Integration of SIMD + Register Allocation
 */

#ifndef OPTIMIZATION_PASS_SIMD_H
#define OPTIMIZATION_PASS_SIMD_H

typedef struct {
    char *original_asm;
    char *optimized_asm;
    
    bool simd_applied;
    float simd_speedup;
    
    bool register_allocation_applied;
    float register_allocation_speedup;
    
    float total_estimated_speedup;
    float actual_measured_speedup;
    
    int instructions_before;
    int instructions_after;
    int bytes_saved;
} OptimizationResult;

// Main optimization pass
OptimizationResult *optimize_with_simd_and_registers(const char *assembly_code, int optimization_level);

// Diagnostic output
void print_optimization_report(const OptimizationResult *result);

// Cleanup
void optimization_result_free(OptimizationResult *result);

#endif // OPTIMIZATION_PASS_SIMD_H
