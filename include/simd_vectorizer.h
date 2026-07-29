/*
 * simd_vectorizer.h - SIMD Vectorization Pass
 * 
 * Detects loops suitable for SIMD vectorization and generates optimized AVX2 code
 */

#ifndef SIMD_VECTORIZER_H
#define SIMD_VECTORIZER_H

#include <stdbool.h>

typedef enum {
    SIMD_LOOP_NONE = 0,
    SIMD_LOOP_REDUCTION = 1,      // Accumulation: sum += expr
    SIMD_LOOP_ELEMENTWISE = 2,    // Element-wise: a[i] = expr
    SIMD_LOOP_DOT_PRODUCT = 3,    // Dot product: sum += a[i]*b[i]
} SIMDLoopType;

typedef struct {
    bool is_vectorizable;
    SIMDLoopType loop_type;
    const char *accumulator_var;
    const char *loop_var;
    int vector_width;              // 4 for doubles (32-bit vectors)
    int estimated_iterations;
    bool has_memory_access;
    bool has_function_calls;
    bool safe_to_vectorize;
    const char *reason;
} SIMDLoopInfo;

// Main API
char *simd_vectorize_loop(const char *loop_assembly, SIMDLoopInfo *info_out);
float estimate_speedup(SIMDLoopInfo info);

#endif // SIMD_VECTORIZER_H
