#include "../include/common.h"
#include <limits.h>
#include <stdint.h>

/* SECURITY: Detect integer overflow in multiplication */
bool safe_multiply_check(size_t count, size_t size) {
    if (size > 0 && count > (SIZE_MAX / size)) {
        return false;  /* Overflow would occur */
    }
    return true;
}

void error(const char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
    exit(1);
}

void error_at(int line, int col, const char *msg) {
    fprintf(stderr, "Error at line %d, col %d: %s\n", line, col, msg);
    exit(1);
}

/* Stub implementation - actual implementation in parser.c */
void error_with_context(struct Lexer *lexer, int line, int col, const char *msg, const char *hint) {
    /* This will be implemented via parser-specific function */
    (void)lexer;  // Parameter kept for interface compatibility
    (void)hint;   // Parameter kept for interface compatibility
    error_at(line, col, msg);
}

void *xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        error("Out of memory");
    }
    return ptr;
}

void *xrealloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        error("Out of memory");
    }
    return new_ptr;
}

char *xstrdup(const char *str) {
    char *new_str = strdup(str);
    if (!new_str) {
        error("Out of memory");
    }
    return new_str;
}

// ============================================
// SECURITY HARDENING: Runtime Safety (ALWAYS ENABLED)
// ============================================
// SECURITY FIX 1: Overflow checking MANDATORY for all arithmetic
int g_overflow_check_enabled = 1;   // ENABLED: Security-critical, always check
// SECURITY FIX 2: Bounds checking MANDATORY for all arrays
int g_bounds_check_enabled = 1;     // ENABLED: Safety-critical security feature
// SECURITY FIX 3: Stack checking MANDATORY for recursion safety
int g_stack_check_enabled = 1;      // ENABLED: Prevent stack exhaustion DoS
// SECURITY FIX 4: Enforce strict exit on bounds violations (not graceful degradation)
int g_enforce_strict_security = 1;  // NEW: Exit immediately on any violation
