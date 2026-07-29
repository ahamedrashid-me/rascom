/*
 * memory_safety.h - RasCode Memory Safety API
 * 
 * Rust/Zig-equivalent compile-time and runtime safety
 */

#ifndef RASCOM_MEMORY_SAFETY_H
#define RASCOM_MEMORY_SAFETY_H

#include <stdint.h>
#include <stdbool.h>

/* ============================================
 * MANDATORY BOUNDS CHECKING
 * ============================================ */

/**
 * Validate array access - ALWAYS checked, never optional
 * Equivalent to Rust's arr[index] bounds checking
 */
bool validate_array_access(const void *array_ptr, uint32_t element_size,
                          int64_t index, uint32_t array_length,
                          const char *location);

/**
 * Check for integer overflow in multiplication
 */
bool check_multiplication_overflow(uint64_t count, uint64_t element_size);

/* ============================================
 * USE-AFTER-FREE DETECTION
 * ============================================ */

/**
 * Allocate with version-based use-after-free detection
 */
void *safe_alloc_versioned(uint32_t size, const char *location);

/**
 * Free with version invalidation
 */
void safe_free_versioned(void *ptr, const char *location);

/**
 * Validate pointer has not been freed
 */
bool validate_pointer_dereference(const void *ptr, const char *location);

/* ============================================
 * SAFE ARRAY TYPE
 * ============================================ */

typedef struct {
    void *data;
    uint32_t len;
    uint32_t capacity;
    uint32_t element_size;
    const char *allocation_site;
} SafeArray;

SafeArray *safe_array_new(uint32_t capacity, uint32_t element_size,
                         const char *location);

void *safe_array_get(SafeArray *arr, uint64_t index, const char *location);

void safe_array_set(SafeArray *arr, uint64_t index, const void *value,
                   const char *location);

void safe_array_free(SafeArray *arr);

/* ============================================
 * INITIALIZATION
 * ============================================ */

void memory_safety_init(bool enable);
void memory_safety_set_verbose(bool verbose);
void memory_safety_stats(void);

#endif /* RASCOM_MEMORY_SAFETY_H */
