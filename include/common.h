#ifndef COMMON_H
#define COMMON_H

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Version info
#define RASCODE_VERSION "b-0.0.1"
#define RASCODE_TARGET "Linux x86_64"

// Error handling
void error(const char *msg);
void error_at(int line, int col, const char *msg);

// Enhanced error reporting with source context (implemented in parser.c or called via macro)
struct Lexer;  // Forward declaration to avoid circular dependency
void error_with_context(struct Lexer *lexer, int line, int col, const char *msg, const char *hint);

// Memory utilities
void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *str);

/* SECURITY: Check for integer overflow in multiplication */
bool safe_multiply_check(size_t count, size_t size);

// ============================================
// PRIORITY 2: Runtime Safety Enhancements
// ============================================
extern int g_overflow_check_enabled;   // Set by -foverflow-check flag
extern int g_bounds_check_enabled;     // Set by -fbounds-check flag (default: enabled)
extern int g_stack_check_enabled;      // Set by -fstack-check flag (default: enabled)

#define MAX_RECURSION_DEPTH 100000       // Maximum function call depth (increased for deep recursion tests)
#define STACK_WARN_THRESHOLD 90000      // Warn at 90% of max depth (90000/100000)

#endif // COMMON_H
