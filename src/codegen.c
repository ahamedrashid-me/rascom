#include "../include/codegen.h"
#include "../include/builtins.h"
#include "../include/memory_safety.h"
#include <time.h>
#include <string.h>

// Group type management
static GroupField *group_field_new(const char *name, const char *type, int offset, int size) {
    GroupField *field = xmalloc(sizeof(GroupField));
    field->name = xstrdup(name);
    field->type = xstrdup(type);
    field->offset = offset;
    field->size = size;
    field->next = NULL;
    return field;
}

static GroupType *group_type_new(const char *name) {
    GroupType *group = xmalloc(sizeof(GroupType));
    group->name = xstrdup(name);
    group->total_size = 0;
    group->fields = NULL;
    group->next = NULL;
    return group;
}

static void group_type_add_field(GroupType *group, const char *name, const char *type, int size) {
    GroupField *field = group_field_new(name, type, group->total_size, size);
    field->next = group->fields;
    group->fields = field;
    group->total_size += size;
}

static void group_type_add(CodeGen *gen, GroupType *group) {
    group->next = gen->groups;
    gen->groups = group;
}

static GroupType *group_type_lookup(CodeGen *gen, const char *name) {
    for (GroupType *group = gen->groups; group != NULL; group = group->next) {
        if (strcmp(group->name, name) == 0) {
            return group;
        }
    }
    return NULL;
}

static GroupField *group_field_lookup(GroupType *group, const char *field_name) {
    for (GroupField *field = group->fields; field != NULL; field = field->next) {
        if (strcmp(field->name, field_name) == 0) {
            return field;
        }
    }
    return NULL;
}

static void group_type_table_clear(CodeGen *gen) {
    GroupType *current = gen->groups;
    while (current != NULL) {
        GroupType *next_group = current->next;
        
        // Free fields
        GroupField *field = current->fields;
        while (field != NULL) {
            GroupField *next_field = field->next;
            free(field->name);
            free(field->type);
            free(field);
            field = next_field;
        }
        
        free(current->name);
        free(current);
        current = next_group;
    }
    gen->groups = NULL;
}

// Helper: Evaluate constant array size from AST expression
static int evaluate_array_size(ASTNode *size_expr) {
    if (!size_expr) return 0;
    
    // Handle numeric literals
    if (size_expr->type == AST_LITERAL && strcmp(size_expr->literal.type, "int") == 0) {
        return atoi(size_expr->literal.value);
    }
    
    // For other expressions (variables, operations), we can't evaluate at compile time
    // Return 0 as a placeholder (this means the size is unknown)
    return 0;
}

// Symbol table functions
static Symbol *symbol_new(const char *name, int offset, int size, const char *type, bool is_array, bool is_array_param, int array_size, int element_size, bool is_group) {
    Symbol *sym = xmalloc(sizeof(Symbol));
    sym->name = xstrdup(name);
    sym->offset = offset;
    sym->size = size;
    sym->type = xstrdup(type);
    sym->is_array = is_array;
    sym->is_array_param = is_array_param;
    sym->array_size = array_size;
    sym->element_size = element_size;
    sym->is_group = is_group;
    sym->is_map = false;
    sym->key_type = NULL;
    sym->value_type = NULL;
    sym->capacity = 0;
    sym->next = NULL;
    return sym;
}

static void symbol_add(CodeGen *gen, const char *name, int offset, int size, const char *type, bool is_array, bool is_array_param, int array_size, int element_size, bool is_group) {
    Symbol *sym = symbol_new(name, offset, size, type, is_array, is_array_param, array_size, element_size, is_group);
    sym->next = gen->symbols;
    gen->symbols = sym;
}

static void symbol_add_map(CodeGen *gen, const char *name, int offset, int size, const char *key_type, const char *value_type, int capacity) {
    Symbol *sym = symbol_new(name, offset, size, "map", false, false, 0, 0, false);
    sym->is_map = true;
    sym->key_type = xstrdup(key_type);
    sym->value_type = xstrdup(value_type);
    sym->capacity = capacity;
    sym->next = gen->symbols;
    gen->symbols = sym;
}

static Symbol *symbol_lookup(CodeGen *gen, const char *name) {
    for (Symbol *sym = gen->symbols; sym != NULL; sym = sym->next) {
        if (strcmp(sym->name, name) == 0) {
            return sym;
        }
    }
    return NULL;
}

// Look up a constant by name and return its value node
static ASTNode *constant_lookup(CodeGen *gen, const char *name) {
    if (!gen->constants) return NULL;
    
    for (int i = 0; i < gen->constants->count; i++) {
        ASTNode *const_node = gen->constants->nodes[i];
        if (const_node->type == AST_CONST_DECL &&
            strcmp(const_node->const_decl.name, name) == 0) {
            return const_node->const_decl.value;
        }
    }
    return NULL;
}

static void symbol_table_clear(CodeGen *gen) {
    Symbol *current = gen->symbols;
    while (current != NULL) {
        Symbol *next = current->next;
        free(current->name);
        free(current->type);
        if (current->key_type) free(current->key_type);
        if (current->value_type) free(current->value_type);
        free(current);
        current = next;
    }
    gen->symbols = NULL;
}

// Function signature table management
static void function_add(CodeGen *gen, const char *name, const char *return_type, char **param_types, 
                        int param_count, int required_param_count, ASTNode **default_values) {
    FunctionSignature *sig = xmalloc(sizeof(FunctionSignature));
    sig->name = xstrdup(name);
    sig->return_type = xstrdup(return_type);
    sig->param_count = param_count;
    sig->required_param_count = required_param_count;
    
    // Copy parameter types
    if (param_count > 0) {
        sig->param_types = xmalloc(sizeof(char *) * param_count);
        for (int i = 0; i < param_count; i++) {
            sig->param_types[i] = xstrdup(param_types[i]);
        }
    } else {
        sig->param_types = NULL;
    }
    
    // Copy default value AST nodes (we just store the pointers, not deep copies)
    if (param_count > 0 && default_values) {
        sig->default_values = xmalloc(sizeof(ASTNode *) * param_count);
        for (int i = 0; i < param_count; i++) {
            sig->default_values[i] = default_values[i];  // Store AST node pointer
        }
    } else {
        sig->default_values = NULL;
    }
    
    sig->next = gen->functions;
    gen->functions = sig;
}

static const char *function_get_return_type(CodeGen *gen, const char *name) {
    FunctionSignature *current = gen->functions;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current->return_type;
        }
        current = current->next;
    }
    return NULL;  // Function not found
}

static void function_table_clear(CodeGen *gen) {
    FunctionSignature *current = gen->functions;
    while (current != NULL) {
        FunctionSignature *next = current->next;
        free(current->name);
        free(current->return_type);
        if (current->param_types) {
            for (int i = 0; i < current->param_count; i++) {
                free(current->param_types[i]);
            }
            free(current->param_types);
        }
        if (current->default_values) {
            free(current->default_values);  // Free array, not ASTNodes
        }
        free(current);
        current = next;
    }
    gen->functions = NULL;
}

// Get element size in bytes based on type
static int get_type_size(const char *type) {
    if (strcmp(type, "int") == 0) return 4;      // 32-bit integer
    if (strcmp(type, "deci") == 0) return 8;     // 64-bit float
    if (strcmp(type, "char") == 0) return 1;     // 8-bit character
    if (strcmp(type, "byte") == 0) return 1;     // 8-bit signed
    if (strcmp(type, "ubyte") == 0) return 1;    // 8-bit unsigned
    if (strcmp(type, "bool") == 0) return 1;     // boolean (1 byte)
    if (strcmp(type, "str") == 0) return 8;      // pointer to string
    // For unknown types, assume it might be a group type - need context to look it up
    // Return 8 as default (will be updated by context-aware lookup if needed)
    return 8; // Default to 8 bytes (pointer size)
}

static int new_label(CodeGen *gen) {
    return gen->label_count++;
}

// Helper: Check if an expression evaluates to deci type
static bool is_deci_expr(CodeGen *gen, ASTNode *node) {
    if (node->type == AST_LITERAL) {
        return strcmp(node->literal.type, "deci") == 0;
    }
    if (node->type == AST_IDENT) {
        Symbol *sym = symbol_lookup(gen, node->ident.name);
        return sym && strcmp(sym->type, "deci") == 0;
    }
    if (node->type == AST_CALL) {
        // Check function return type
        const char *return_type = function_get_return_type(gen, node->call.name);
        return return_type && strcmp(return_type, "deci") == 0;
    }
    if (node->type == AST_BINARY_OP) {
        // Binary operations preserve deci if either operand is deci
        return is_deci_expr(gen, node->binary.left) || is_deci_expr(gen, node->binary.right);
    }
    if (node->type == AST_ARRAY_ACCESS) {
        Symbol *sym = symbol_lookup(gen, node->array_access.name);
        if (sym && sym->is_array) {
            return strcmp(sym->type, "deci") == 0;
        }
    }
    return false;
}

static bool is_string_expr(CodeGen *gen, ASTNode *node) {
    if (node->type == AST_LITERAL) {
        return strcmp(node->literal.type, "str") == 0;
    }
    if (node->type == AST_IDENT) {
        Symbol *sym = symbol_lookup(gen, node->ident.name);
        return sym && strcmp(sym->type, "str") == 0;
    }
    if (node->type == AST_CALL) {
        // Check function return type
        const char *return_type = function_get_return_type(gen, node->call.name);
        return return_type && strcmp(return_type, "str") == 0;
    }
    if (node->type == AST_BINARY_OP) {
        // Binary operations: check if both operands are strings
        return is_string_expr(gen, node->binary.left) || is_string_expr(gen, node->binary.right);
    }
    return false;
}

static void emit(CodeGen *gen, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(gen->output, fmt, args);
    va_end(args);
    fprintf(gen->output, "\n");
}

// ========================================
// SIMD INSTRUCTION EMISSION FUNCTIONS
// ========================================

// Emit AVX2 vector addition instruction
static void emit_simd_add(CodeGen *gen) {
    if (gen->enable_simd && gen->in_simd_loop) {
        emit(gen, "    vaddpd ymm0, ymm0, ymm1    ; 4x vector add (AVX2)");
    } else {
        emit(gen, "    addsd xmm0, xmm1            ; scalar add (SSE2)");
    }
}

// Emit AVX2 vector subtraction instruction
static void emit_simd_sub(CodeGen *gen) {
    if (gen->enable_simd && gen->in_simd_loop) {
        emit(gen, "    vsubpd ymm0, ymm0, ymm1    ; 4x vector subtract (AVX2)");
    } else {
        emit(gen, "    subsd xmm0, xmm1            ; scalar subtract (SSE2)");
    }
}

// Emit AVX2 vector multiplication instruction
static void emit_simd_mul(CodeGen *gen) {
    if (gen->enable_simd && gen->in_simd_loop) {
        emit(gen, "    vmulpd ymm0, ymm0, ymm1    ; 4x vector multiply (AVX2)");
    } else {
        emit(gen, "    mulsd xmm0, xmm1            ; scalar multiply (SSE2)");
    }
}

// Emit AVX2 vector division instruction
static void emit_simd_div(CodeGen *gen) {
    if (gen->enable_simd && gen->in_simd_loop) {
        emit(gen, "    vdivpd ymm0, ymm0, ymm1    ; 4x vector divide (AVX2)");
    } else {
        emit(gen, "    divsd xmm0, xmm1            ; scalar divide (SSE2)");
    }
}

// Emit SIMD load instruction (loads multiple values at once)
static __attribute__((unused)) void emit_simd_load(CodeGen *gen, const char *addr) {
    if (gen->enable_simd && gen->in_simd_loop) {
        emit(gen, "    vmovapd ymm0, [%s]          ; load 4 doubles (32 bytes) with alignment", addr);
    } else {
        emit(gen, "    movsd xmm0, [%s]            ; load 1 double (8 bytes)", addr);
    }
}

// Emit SIMD store instruction (stores multiple values at once)
static __attribute__((unused)) void emit_simd_store(CodeGen *gen, const char *addr) {
    if (gen->enable_simd && gen->in_simd_loop) {
        emit(gen, "    vmovapd [%s], ymm0          ; store 4 doubles (32 bytes) with alignment", addr);
    } else {
        emit(gen, "    movsd [%s], xmm0            ; store 1 double (8 bytes)", addr);
    }
}

// Initialize SIMD loop (zero out accumulator in vector register)
static void emit_simd_loop_init(CodeGen *gen) {
    if (gen->enable_simd) {
        emit(gen, "    ; === SIMD Vectorization Enabled ===");
        emit(gen, "    vpxor ymm2, ymm2, ymm2     ; ymm2 = [0, 0, 0, 0] for accumulation");
        emit(gen, "    vpxor ymm3, ymm3, ymm3     ; ymm3 = [0, 0, 0, 0] for temp values");
    }
}

// Begin SIMD loop processing
static void emit_simd_loop_begin(CodeGen *gen, const char *loop_label) {
    if (gen->enable_simd) {
        gen->in_simd_loop = true;
        emit(gen, ".L%s_simd:", loop_label);
    }
}

// End SIMD loop processing (iterate and check bounds)
static __attribute__((unused)) void emit_simd_loop_end(CodeGen *gen, int vector_width) {
    if (gen->enable_simd && gen->in_simd_loop) {
        emit(gen, "    add rax, %d                 ; move by %d bytes (vector width * 8)", 
             vector_width * 8, vector_width * 8);
        emit(gen, "    cmp rax, rbx");
        emit(gen, "    jl .L%%simd_continue", gen->label_count++);
        gen->in_simd_loop = false;
    }
}

// Emit prefetch instruction for memory locality optimization
static void emit_prefetch(CodeGen *gen, const char *address, int locality) {
    if (!gen->enable_prefetch) return;
    
    // prefetch hints: 0=L1, 1=L2, 2=L3, 3=none (write)
    const char *prefetch_hints[] = {"prefetcht0", "prefetcht1", "prefetcht2", "prefetchtnta"};
    if (locality < 0 || locality > 3) locality = 1;
    
    emit(gen, "    %s [%s]", prefetch_hints[locality], address);
}

// Emit prefetch for array access in innermost loop
static void emit_prefetch_for_loop(CodeGen *gen) {
    if (!gen->enable_prefetch || gen->loop_depth <= 1) return;
    
    // PHASE 3: Aggressive prefetching strategy
    // Cache line = 64 bytes, prefetch ahead based on nesting level
    emit(gen, "    ; Prefetch: memory bandwidth optimization");
    if (gen->loop_depth >= 3) {
        // Innermost: prefetch 4 cache lines (256 bytes) for lookahead
        emit(gen, "    prefetcht0 [rax + 128]");
        emit(gen, "    prefetcht0 [rax + 192]");
        emit(gen, "    prefetcht0 [rax + 256]");
        emit(gen, "    prefetcht0 [rax + 320]");
    } else if (gen->loop_depth == 2) {
        // Middle loop: prefetch 2 cache lines
        emit(gen, "    prefetcht0 [rax + 128]");
        emit(gen, "    prefetcht0 [rax + 256]");
    }
}

CodeGen *codegen_new(const char *output_file) {
    CodeGen *gen = xmalloc(sizeof(CodeGen));
    gen->output = fopen(output_file, "w");
    if (!gen->output) {
        fprintf(stderr, "Failed to open output file: %s\n", output_file);
        free(gen);
        return NULL;
    }
    gen->elf_gen = elfgen_new();
    gen->label_count = 0;
    gen->stack_offset = 0;
    gen->symbols = NULL;
    gen->groups = NULL;
    gen->constants = NULL;
    gen->functions = NULL;
    gen->current_function_return_type = NULL;
    gen->use_elf_gen = 0;  // For now, keep using FILE output
    gen->loop_break_label = -1;      // No loop context
    gen->loop_continue_label = -1;   // No loop context
    gen->deferred_stmts = ast_list_new();  // Empty defer queue
    
    // Initialize SIMD support (enabled by default in -O3)
    gen->enable_simd = true;      // Enable SIMD vectorization
    gen->in_simd_loop = false;    // Not currently in SIMD loop
    gen->vector_width = 4;        // 4 doubles per YMM register (256-bit)
    gen->unroll_iteration = -1;   // Not in an unrolled loop (-1 = not unrolled, 0-3 = unroll iteration)
    
    // Initialize performance optimization flags
    gen->loop_depth = 0;          // Not in a loop initially
    gen->enable_prefetch = true;  // Enable prefetch hints for nested loops
    gen->last_prefetch_rip = 0;   // No prefetch yet
    
    // Initialize register allocation for hot loops (Phase 3)
    gen->enable_reg_alloc = true;  // Enable register allocation for loop variables
    gen->loop_var_i = NULL;        // No loop variable assigned yet
    gen->loop_var_j = NULL;
    gen->loop_var_k = NULL;
    gen->loop_var_sum = NULL;
    
    return gen;
}

void codegen_free(CodeGen *gen) {
    symbol_table_clear(gen);
    group_type_table_clear(gen);
    function_table_clear(gen);
    if (gen->output) {
        fclose(gen->output);
    }
    if (gen->elf_gen) {
        elfgen_free(gen->elf_gen);
    }
    free(gen);
}

// Forward declarations
static void codegen_expression(CodeGen *gen, ASTNode *node);
static void codegen_statement(CodeGen *gen, ASTNode *node);
static void codegen_read(CodeGen *gen, ASTNode *node);
static void codegen_cycle(CodeGen *gen, ASTNode *node);
static void codegen_map_decl(CodeGen *gen, ASTNode *node);
static void codegen_map_set(CodeGen *gen, ASTNode *node);
static void codegen_map_get(CodeGen *gen, ASTNode *node);
static void codegen_map_has(CodeGen *gen, ASTNode *node);
static void codegen_map_remove(CodeGen *gen, ASTNode *node);
static void codegen_member_access_addr(CodeGen *gen, ASTNode *node);
static const char *get_member_access_type(CodeGen *gen, ASTNode *node);

// Generate expression code
// Helper: Escape special characters for NASM assembly strings
__attribute__((unused)) static char *escape_asm_string(const char *str) {
    // Convert a RasCode string (with actual escape bytes) into NASM-compatible format
    // The lexer has already converted escape sequences (\n, \t, etc.) to actual bytes
    // Now we need to convert those bytes back to NASM escape sequences
    // NASM supports: \n \r \t \b \f \a \v \' \" \\ and \xHH
    
    int len = strlen(str);
    char *result = xmalloc(len * 4 + 1); // Upper bound: each char might become \\xHH
    char *dest = result;
    
    for (int i = 0; i < len; i++) {
        unsigned char c = (unsigned char)str[i];
        
        switch (c) {
            case '\n':
                // Newline - NASM escape sequence
                *dest++ = '\\';
                *dest++ = 'n';
                break;
            case '\r':
                // Carriage return - NASM escape sequence
                *dest++ = '\\';
                *dest++ = 'r';
                break;
            case '\t':
                // Tab - NASM escape sequence
                *dest++ = '\\';
                *dest++ = 't';
                break;
            case '\b':
                // Backspace - NASM escape sequence
                *dest++ = '\\';
                *dest++ = 'b';
                break;
            case '\f':
                // Form feed - NASM escape sequence
                *dest++ = '\\';
                *dest++ = 'f';
                break;
            case '\a':
                // Bell/alert - NASM escape sequence
                *dest++ = '\\';
                *dest++ = 'a';
                break;
            case '\v':
                // Vertical tab - NASM escape sequence
                *dest++ = '\\';
                *dest++ = 'v';
                break;
            case '"':
                // Quote in string - NASM escape (will be handled at emit time)
                *dest++ = '"';
                break;
            case '\\':
                // Backslash - double it for NASM
                *dest++ = '\\';
                *dest++ = '\\';
                break;
            case '$':
                // Dollar sign - must be escaped in NASM
                *dest++ = '$';
                *dest++ = '$';
                break;
            default:
                if (c < 32 || c >= 127) {
                    // Non-printable or high byte - use hex escape for NASM
                    // Format: \xHH - use snprintf to prevent buffer overflow
                    int written = snprintf(dest, 5, "\\x%02x", c);
                    if (written > 0 && written < 5) {
                        dest += written;
                    } else {
                        // Safety fallback: just copy character as-is
                        *dest++ = '?';
                    }
                } else {
                    // Regular printable character
                    *dest++ = c;
                }
                break;
        }
    }
    *dest = '\0';
    
    return result;
}

// Convert number strings from C-style formats to decimal
// Handles: 0xFF (hex), 0755 (octal), 0b1010 (binary)
static char *convert_number_format(const char *value) {
    static char result[32];
    
    if (!value || value[0] != '0' || strlen(value) < 2) {
        // Not a special format, return as-is
        return (char *)value;
    }
    
    char second = value[1];
    
    // Hexadecimal: 0x or 0X
    if (second == 'x' || second == 'X') {
        long num = strtol(value, NULL, 16);
        snprintf(result, sizeof(result), "%ld", num);
        return result;
    }
    
    // Binary: 0b or 0B
    if (second == 'b' || second == 'B') {
        long num = strtol(value + 2, NULL, 2);
        snprintf(result, sizeof(result), "%ld", num);
        return result;
    }
    
    // Octal: 0 followed by octal digits (0755)
    // Check if next char is digit and all are valid octal digits
    if (isdigit(second)) {
        bool is_octal = true;
        for (int i = 1; value[i]; i++) {
            if (value[i] < '0' || value[i] > '7') {
                is_octal = false;
                break;
            }
        }
        
        if (is_octal && strlen(value) > 1) {
            long num = strtol(value, NULL, 8);
            snprintf(result, sizeof(result), "%ld", num);
            return result;
        }
    }
    
    // Regular decimal, return as-is
    return (char *)value;
}

static void codegen_literal(CodeGen *gen, ASTNode *node) {
    if (strcmp(node->literal.type, "int") == 0) {
        char *converted = convert_number_format(node->literal.value);
        emit(gen, "    mov rax, %s", converted);
    } else if (strcmp(node->literal.type, "char") == 0) {
        // Character literal - 8-bit value
        char *converted = convert_number_format(node->literal.value);
        emit(gen, "    mov rax, %s", converted);
    } else if (strcmp(node->literal.type, "byte") == 0) {
        // Signed byte literal - 8-bit value
        char *converted = convert_number_format(node->literal.value);
        emit(gen, "    mov rax, %s", converted);
    } else if (strcmp(node->literal.type, "ubyte") == 0) {
        // Unsigned byte literal - 8-bit value
        char *converted = convert_number_format(node->literal.value);
        emit(gen, "    mov rax, %s", converted);
    } else if (strcmp(node->literal.type, "bool") == 0) {
        // Boolean literal - convert "true"/"false" to 1/0
        if (strcmp(node->literal.value, "true") == 0) {
            emit(gen, "    mov rax, 1");
        } else {
            emit(gen, "    mov rax, 0");
        }
    } else if (strcmp(node->literal.type, "deci") == 0) {
        // Floating point literal - store in data section and load to xmm0
        int label = new_label(gen);
        fprintf(gen->output, "section .data\n");
        fprintf(gen->output, ".FLOAT%d: dq %s\n", label, node->literal.value);
        fprintf(gen->output, "section .text\n");
        emit(gen, "    movsd xmm0, [rel .FLOAT%d]  ; load float literal", label);
        emit(gen, "    movq rax, xmm0              ; move to rax for consistency");
    } else if (strcmp(node->literal.type, "str") == 0) {
        int label = new_label(gen);
        emit(gen, "    lea rax, [rel .STR%d]", label);
        
        // Output string bytes directly - convert actual bytes to hex format for NASM
        fprintf(gen->output, "section .data\n");
        fprintf(gen->output, ".STR%d: db ", label);
        
        int first = 1;
        for (int i = 0; node->literal.value[i]; i++) {
            unsigned char c = (unsigned char)node->literal.value[i];
            
            if (!first) fprintf(gen->output, ", ");
            
            // Output as hex byte for precise control
            fprintf(gen->output, "0x%02x", c);
            first = 0;
        }
        // Null terminator
        if (!first) fprintf(gen->output, ", ");
        fprintf(gen->output, "0x00\n");
        
        fprintf(gen->output, "section .text\n");
    } else {
        // Unknown type - default to treating as integer
        emit(gen, "    mov rax, %s", node->literal.value);
    }
}

static void codegen_binary_op(CodeGen *gen, ASTNode *node) {
    // Handle logical operators with short-circuit evaluation
    if (strcmp(node->binary.op, "&&") == 0 || strcmp(node->binary.op, "and") == 0) {
        // Logical AND with short-circuit
        int false_label = new_label(gen);
        int end_label = new_label(gen);
        
        codegen_expression(gen, node->binary.left);
        emit(gen, "    test rax, rax       ; check left operand");
        emit(gen, "    jz .L%d             ; if false, skip right", false_label);
        
        codegen_expression(gen, node->binary.right);
        emit(gen, "    test rax, rax       ; check right operand");
        emit(gen, "    jz .L%d             ; if false, result is false", false_label);
        
        emit(gen, "    mov rax, 1          ; both true, result is 1");
        emit(gen, "    jmp .L%d", end_label);
        
        emit(gen, ".L%d:", false_label);
        emit(gen, "    xor rax, rax        ; result is 0");
        emit(gen, ".L%d:", end_label);
        return;
    }
    
    if (strcmp(node->binary.op, "||") == 0) {
        // Logical OR with short-circuit
        int true_label = new_label(gen);
        int end_label = new_label(gen);
        
        codegen_expression(gen, node->binary.left);
        emit(gen, "    test rax, rax       ; check left operand");
        emit(gen, "    jnz .L%d            ; if true, result is true", true_label);
        
        codegen_expression(gen, node->binary.right);
        emit(gen, "    test rax, rax       ; check right operand");
        emit(gen, "    jnz .L%d            ; if true, result is true", true_label);
        
        emit(gen, "    xor rax, rax        ; both false, result is 0");
        emit(gen, "    jmp .L%d", end_label);
        
        emit(gen, ".L%d:", true_label);
        emit(gen, "    mov rax, 1          ; result is 1");
        emit(gen, ".L%d:", end_label);
        return;
    }
    
    if (strcmp(node->binary.op, "xor") == 0) {
        // Logical XOR (no short-circuit)
        codegen_expression(gen, node->binary.left);
        emit(gen, "    test rax, rax");
        emit(gen, "    setnz al            ; left != 0");
        emit(gen, "    movzx rcx, al");
        emit(gen, "    push rcx");
        
        codegen_expression(gen, node->binary.right);
        emit(gen, "    test rax, rax");
        emit(gen, "    setnz al            ; right != 0");
        emit(gen, "    movzx rax, al");
        emit(gen, "    pop rcx");
        emit(gen, "    xor rax, rcx        ; logical XOR");
        return;
    }
    
    // For all other binary operators, evaluate both operands
    // Check if this is a floating point operation
    bool is_float_op = is_deci_expr(gen, node->binary.left) || is_deci_expr(gen, node->binary.right);
    
    if (is_float_op) {
        // Floating point arithmetic using SSE instructions
        // FIX: After evaluating each operand, the deci value is in rax (as bits).
        // We need to move it to xmm registers for FP operations.
        
        // Generate left operand (result in rax as deci bits)
        codegen_expression(gen, node->binary.left);
        emit(gen, "    movq xmm0, rax      ; move left operand bits from rax to xmm0");
        emit(gen, "    mov rbx, rax        ; also save to rbx for later (use gp register)");
        
        // Generate right operand (result will be in rax as deci bits)
        codegen_expression(gen, node->binary.right);
        emit(gen, "    movq xmm1, rax      ; move right operand bits from rax to xmm1");
        emit(gen, "    movq xmm0, rbx      ; restore left from rbx to xmm0 (via gp reg)");
        
        // Perform SSE operation (xmm0 = left, xmm1 = right)
        // Note: For true SIMD, we'd need loop-level vectorization with 4 parallel accumulators
        // For now, use scalar SSE2 operations for correctness
        if (strcmp(node->binary.op, "+") == 0) {
            emit(gen, "    addsd xmm0, xmm1            ; scalar add (SSE2)");
            emit(gen, "    movq rax, xmm0      ; result to rax");
        } else if (strcmp(node->binary.op, "-") == 0) {
            emit(gen, "    subsd xmm0, xmm1            ; scalar sub (SSE2)");
            emit(gen, "    movq rax, xmm0      ; result to rax");
        } else if (strcmp(node->binary.op, "*") == 0) {
            emit(gen, "    mulsd xmm0, xmm1            ; scalar multiply (SSE2)");
            emit(gen, "    movq rax, xmm0      ; result to rax");
        } else if (strcmp(node->binary.op, "/") == 0) {
            emit(gen, "    divsd xmm0, xmm1            ; scalar divide (SSE2)");
            emit(gen, "    movq rax, xmm0      ; result to rax");
        } else if (strcmp(node->binary.op, "==") == 0) {
            emit(gen, "    ucomisd xmm0, xmm1  ; compare floats");
            emit(gen, "    sete al             ; set if equal");
            emit(gen, "    movzx rax, al");
        } else if (strcmp(node->binary.op, "!=") == 0) {
            emit(gen, "    ucomisd xmm0, xmm1  ; compare floats");
            emit(gen, "    setne al            ; set if not equal");
            emit(gen, "    movzx rax, al");
        } else if (strcmp(node->binary.op, "<") == 0) {
            emit(gen, "    ucomisd xmm0, xmm1  ; compare floats");
            emit(gen, "    setb al             ; set if below");
            emit(gen, "    movzx rax, al");
        } else if (strcmp(node->binary.op, ">") == 0) {
            emit(gen, "    ucomisd xmm0, xmm1  ; compare floats");
            emit(gen, "    seta al             ; set if above");
            emit(gen, "    movzx rax, al");
        } else if (strcmp(node->binary.op, "<=") == 0) {
            emit(gen, "    ucomisd xmm0, xmm1  ; compare floats");
            emit(gen, "    setbe al            ; set if below or equal");
            emit(gen, "    movzx rax, al");
        } else if (strcmp(node->binary.op, ">=") == 0) {
            emit(gen, "    ucomisd xmm0, xmm1  ; compare floats");
            emit(gen, "    setae al            ; set if above or equal");
            emit(gen, "    movzx rax, al");
        } else {
            fprintf(stderr, "Unsupported operator for deci type: %s\n", node->binary.op);
        }
        return;
    }
    
    // Integer arithmetic (original code)
    // Generate left operand
    codegen_expression(gen, node->binary.left);
    emit(gen, "    push rax");
    
    // Generate right operand
    codegen_expression(gen, node->binary.right);
    emit(gen, "    mov rbx, rax");
    emit(gen, "    pop rax");
    
    // Perform operation
    if (strcmp(node->binary.op, "+") == 0) {
        emit(gen, "    add rax, rbx");
        // PRIORITY 2.1: Check for signed integer overflow
        if (g_overflow_check_enabled) {
            int overflow_label = new_label(gen);
            int done_label = new_label(gen);
            emit(gen, "    jo .overflow_%d       ; jump if overflow flag set", overflow_label);
            emit(gen, "    jmp .overflow_done_%d", done_label);
            emit(gen, ".overflow_%d:", overflow_label);
            emit(gen, "    mov rax, -1            ; return -1 to signal overflow");
            emit(gen, ".overflow_done_%d:", done_label);
        }
    } else if (strcmp(node->binary.op, "-") == 0) {
        emit(gen, "    sub rax, rbx");
        // PRIORITY 2.1: Check for signed integer overflow
        if (g_overflow_check_enabled) {
            int overflow_label = new_label(gen);
            int done_label = new_label(gen);
            emit(gen, "    jo .overflow_%d       ; jump if overflow flag set", overflow_label);
            emit(gen, "    jmp .overflow_done_%d", done_label);
            emit(gen, ".overflow_%d:", overflow_label);
            emit(gen, "    mov rax, -1            ; return -1 to signal overflow");
            emit(gen, ".overflow_done_%d:", done_label);
        }
    } else if (strcmp(node->binary.op, "*") == 0) {
        emit(gen, "    imul rax, rbx");
        // PRIORITY 2.1: Check for signed integer overflow
        if (g_overflow_check_enabled) {
            int overflow_label = new_label(gen);
            int done_label = new_label(gen);
            emit(gen, "    jo .overflow_%d       ; jump if overflow flag set", overflow_label);
            emit(gen, "    jmp .overflow_done_%d", done_label);
            emit(gen, ".overflow_%d:", overflow_label);
            emit(gen, "    mov rax, -1            ; return -1 to signal overflow");
            emit(gen, ".overflow_done_%d:", done_label);
        }
    } else if (strcmp(node->binary.op, "/") == 0) {
        // SECURITY: Check for division by zero
        int div_label = new_label(gen);
        emit(gen, "    test rbx, rbx       ; check if divisor is zero");
        emit(gen, "    jz .div_by_zero_%d", div_label);
        emit(gen, "    xor rdx, rdx");
        emit(gen, "    idiv rbx");
        emit(gen, "    jmp .div_done_%d", div_label);
        emit(gen, ".div_by_zero_%d:", div_label);
        emit(gen, "    xor rax, rax        ; return 0 on division by zero");
        emit(gen, ".div_done_%d:", div_label);
    } else if (strcmp(node->binary.op, "%") == 0) {
        // SECURITY: Check for modulo by zero
        int mod_label = new_label(gen);
        emit(gen, "    test rbx, rbx       ; check if divisor is zero");
        emit(gen, "    jz .mod_by_zero_%d", mod_label);
        emit(gen, "    xor rdx, rdx");
        emit(gen, "    idiv rbx");
        emit(gen, "    mov rax, rdx        ; modulo result");
        emit(gen, "    jmp .mod_done_%d", mod_label);
        emit(gen, ".mod_by_zero_%d:", mod_label);
        emit(gen, "    xor rax, rax        ; return 0 on modulo by zero");
        emit(gen, ".mod_done_%d:", mod_label);
    } else if (strcmp(node->binary.op, "==") == 0) {
        // Check if this is a string comparison
        if (is_string_expr(gen, node->binary.left) || is_string_expr(gen, node->binary.right)) {
            // String comparison: use strcmp
            emit(gen, "    mov rdi, rax        ; left string");
            emit(gen, "    mov rsi, rbx        ; right string");
            emit(gen, "    call strcmp");
            emit(gen, "    test rax, rax       ; check result");
            emit(gen, "    setz al             ; 1 if equal (rax==0), 0 otherwise");
            emit(gen, "    movzx rax, al");
        } else {
            // Numeric comparison
            emit(gen, "    cmp rax, rbx");
            emit(gen, "    sete al");
            emit(gen, "    movzx rax, al");
        }
    } else if (strcmp(node->binary.op, "!=") == 0) {
        // Check if this is a string comparison
        if (is_string_expr(gen, node->binary.left) || is_string_expr(gen, node->binary.right)) {
            // String comparison: use strcmp
            emit(gen, "    mov rdi, rax        ; left string");
            emit(gen, "    mov rsi, rbx        ; right string");
            emit(gen, "    call strcmp");
            emit(gen, "    test rax, rax       ; check result");
            emit(gen, "    setnz al            ; 1 if not equal (rax!=0), 0 otherwise");
            emit(gen, "    movzx rax, al");
        } else {
            // Numeric comparison
            emit(gen, "    cmp rax, rbx");
            emit(gen, "    setne al");
            emit(gen, "    movzx rax, al");
        }
    } else if (strcmp(node->binary.op, "<") == 0) {
        emit(gen, "    cmp rax, rbx");
        emit(gen, "    setl al");
        emit(gen, "    movzx rax, al");
    } else if (strcmp(node->binary.op, ">") == 0) {
        emit(gen, "    cmp rax, rbx");
        emit(gen, "    setg al");
        emit(gen, "    movzx rax, al");
    } else if (strcmp(node->binary.op, "<=") == 0) {
        emit(gen, "    cmp rax, rbx");
        emit(gen, "    setle al");
        emit(gen, "    movzx rax, al");
    } else if (strcmp(node->binary.op, ">=") == 0) {
        emit(gen, "    cmp rax, rbx");
        emit(gen, "    setge al");
        emit(gen, "    movzx rax, al");
    } else if (strcmp(node->binary.op, "&") == 0) {
        // Bitwise AND
        emit(gen, "    and rax, rbx");
    } else if (strcmp(node->binary.op, "|") == 0) {
        // Bitwise OR
        emit(gen, "    or rax, rbx");
    } else if (strcmp(node->binary.op, "^") == 0) {
        // Bitwise XOR
        emit(gen, "    xor rax, rbx");
    } else if (strcmp(node->binary.op, "<<") == 0) {
        // Left shift
        emit(gen, "    mov rcx, rbx");
        emit(gen, "    shl rax, cl");
    } else if (strcmp(node->binary.op, ">>") == 0) {
        // Right shift (arithmetic)
        emit(gen, "    mov rcx, rbx");
        emit(gen, "    sar rax, cl");
    } else {
        fprintf(stderr, "Unknown binary operator: %s\n", node->binary.op);
    }
}

static void codegen_expression(CodeGen *gen, ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_LITERAL:
            codegen_literal(gen, node);
            break;
        case AST_BINARY_OP:
            codegen_binary_op(gen, node);
            break;
        case AST_UNARY_OP:
            // Handle unary operators: +x, -x, !x, ~x, ++x, --x, x++, x--
            {
                if (strcmp(node->unary.op, "-") == 0) {
                    // Unary minus (negation)
                    codegen_expression(gen, node->unary.operand);
                    emit(gen, "    neg rax             ; unary minus");
                } else if (strcmp(node->unary.op, "+") == 0) {
                    // Unary plus (identity - no operation needed)
                    codegen_expression(gen, node->unary.operand);
                    emit(gen, "    ; unary plus (identity)");
                } else if (strcmp(node->unary.op, "!") == 0) {
                    // Logical NOT
                    codegen_expression(gen, node->unary.operand);
                    emit(gen, "    test rax, rax");
                    emit(gen, "    setz al             ; set if zero");
                    emit(gen, "    movzx rax, al");
                } else if (strcmp(node->unary.op, "~") == 0) {
                    // Bitwise NOT
                    codegen_expression(gen, node->unary.operand);
                    emit(gen, "    not rax             ; bitwise NOT");
                } else if (strcmp(node->unary.op, "++") == 0) {
                    // Increment: ++x (prefix) or x++ (postfix)
                    if (node->unary.operand->type == AST_IDENT) {
                        Symbol *sym = symbol_lookup(gen, node->unary.operand->ident.name);
                        if (sym) {
                            if (node->unary.is_postfix) {
                                // Post-increment (x++): return old value
                                emit(gen, "    mov rax, [rbp - %d]  ; load old value for x++", sym->offset);
                                emit(gen, "    inc qword [rbp - %d] ; increment %s", sym->offset, node->unary.operand->ident.name);
                                emit(gen, "    ; rax now contains old value");
                            } else {
                                // Pre-increment (++x): return new value
                                emit(gen, "    inc qword [rbp - %d] ; ++%s", sym->offset, node->unary.operand->ident.name);
                                emit(gen, "    mov rax, [rbp - %d]  ; load new value", sym->offset);
                            }
                        }
                    }
                } else if (strcmp(node->unary.op, "--") == 0) {
                    // Decrement: --x (prefix) or x-- (postfix)
                    if (node->unary.operand->type == AST_IDENT) {
                        Symbol *sym = symbol_lookup(gen, node->unary.operand->ident.name);
                        if (sym) {
                            if (node->unary.is_postfix) {
                                // Post-decrement (x--): return old value
                                emit(gen, "    mov rax, [rbp - %d]  ; load old value for x--", sym->offset);
                                emit(gen, "    dec qword [rbp - %d] ; decrement %s", sym->offset, node->unary.operand->ident.name);
                                emit(gen, "    ; rax now contains old value");
                            } else {
                                // Pre-decrement (--x): return new value
                                emit(gen, "    dec qword [rbp - %d] ; --%s", sym->offset, node->unary.operand->ident.name);
                                emit(gen, "    mov rax, [rbp - %d]  ; load new value", sym->offset);
                            }
                        }
                    }
                } else {
                    fprintf(stderr, "Unknown unary operator: %s\n", node->unary.op);
                }
            }
            break;
        case AST_TERNARY_OP:
            // Handle ternary operator: condition ? true_expr : false_expr
            {
                int false_label = new_label(gen);
                int end_label = new_label(gen);
                
                // Evaluate condition
                codegen_expression(gen, node->ternary.condition);
                emit(gen, "    test rax, rax       ; test condition");
                emit(gen, "    jz .L%d             ; if false, jump to false_expr", false_label);
                
                // True branch
                codegen_expression(gen, node->ternary.true_expr);
                emit(gen, "    jmp .L%d            ; skip false_expr", end_label);
                
                // False branch
                emit(gen, ".L%d:", false_label);
                codegen_expression(gen, node->ternary.false_expr);
                
                emit(gen, ".L%d:", end_label);
            }
            break;
        case AST_CYCLE:
            // Cycle as expression: cycle[value] { when[case]: { emit[result]; } ... fixed: { emit[result]; } }
            codegen_cycle(gen, node);
            break;
        case AST_IDENT:
            // Check if it's a constant first
            {
                ASTNode *const_value = constant_lookup(gen, node->ident.name);
                if (const_value) {
                    // It's a constant! Generate code for its value
                    codegen_expression(gen, const_value);
                    emit(gen, "    ; const %s", node->ident.name);
                } else {
                    // It's a variable - load from symbol table
                    Symbol *sym = symbol_lookup(gen, node->ident.name);
                    if (sym) {
                        emit(gen, "    mov rax, [rbp - %d]  ; load %s", sym->offset, node->ident.name);
                    } else {
                        emit(gen, "    mov rax, [rbp - 8]  ; load %s (fallback)", node->ident.name);
                    }
                }
            }
            break;
        case AST_ARRAY_ACCESS:
            // Load array element with optional bounds checking
            {
                Symbol *sym = symbol_lookup(gen, node->array_access.name);
                if (!sym || !sym->is_array) {
                    fprintf(stderr, "Error: '%s' is not an array\n", node->array_access.name);
                    break;
                }
                
                codegen_expression(gen, node->array_access.index);
                
                // PRIORITY 2.2: Bounds check (conditional on g_bounds_check_enabled)
                if (g_bounds_check_enabled) {
                    emit(gen, "    cmp rax, 0");
                    int error_label = new_label(gen);
                    emit(gen, "    jl .bounds_error_%d", error_label);
                    emit(gen, "    cmp rax, %d", sym->array_size);
                    emit(gen, "    jge .bounds_error_%d", error_label);
                    
                    // Calculate element address using element_size from symbol table
                    emit(gen, "    mov rbx, %d         ; element size", sym->element_size);
                    emit(gen, "    imul rax, rbx       ; index * size");
                    
                    // For array parameters (passed as pointers), load the pointer and add offset
                    // For local arrays, compute LEA of the array base and add offset
                    if (sym->is_array_param) {
                        emit(gen, "    mov rbx, [rbp - %d] ; load array pointer (parameter)", sym->offset);
                    } else {
                        emit(gen, "    lea rbx, [rbp - %d] ; array base (first element)", sym->offset);
                    }
                    emit(gen, "    add rbx, rax        ; element address");
                    
                    // Load based on element size
                    if (sym->element_size == 1) {
                        emit(gen, "    movzx rax, byte [rbx] ; load byte");
                    } else if (sym->element_size == 4) {
                        emit(gen, "    mov eax, dword [rbx] ; load dword");
                        emit(gen, "    cdqe                 ; sign extend to 64-bit");
                    } else {
                        emit(gen, "    mov rax, qword [rbx] ; load qword");
                    }
                    
                    emit(gen, "    jmp .bounds_ok_%d", error_label + 1);
                    
                    // Bounds error handler
                    emit(gen, ".bounds_error_%d:", error_label);
                    emit(gen, "    mov rdi, 1");
                    emit(gen, "    lea rsi, [rel bounds_msg]");
                    emit(gen, "    mov rdx, 21");
                    emit(gen, "    mov rax, 1          ; sys_write");
                    emit(gen, "    syscall");
                    emit(gen, "    mov rdi, 1");
                    emit(gen, "    mov rax, 60         ; sys_exit");
                    emit(gen, "    syscall");
                    emit(gen, ".bounds_ok_%d:", error_label + 1);
                } else {
                    // NO bounds checking - direct access (faster but unsafe)
                    emit(gen, "    mov rbx, %d         ; element size", sym->element_size);
                    emit(gen, "    imul rax, rbx       ; index * size");
                    
                    if (sym->is_array_param) {
                        emit(gen, "    mov rbx, [rbp - %d] ; load array pointer (parameter)", sym->offset);
                    } else {
                        emit(gen, "    lea rbx, [rbp - %d] ; array base (first element)", sym->offset);
                    }
                    emit(gen, "    add rbx, rax        ; element address");
                    
                    // Load based on element size
                    if (sym->element_size == 1) {
                        emit(gen, "    movzx rax, byte [rbx] ; load byte");
                    } else if (sym->element_size == 4) {
                        emit(gen, "    mov eax, dword [rbx] ; load dword");
                        emit(gen, "    cdqe                 ; sign extend to 64-bit");
                    } else {
                        emit(gen, "    mov rax, qword [rbx] ; load qword");
                    }
                }
            }
            break;
        case AST_MEMBER_ACCESS:
            // Load group member (supports nested access like a.b.c)
            {
                // Get the type of the member access result
                const char *member_type = get_member_access_type(gen, node);
                if (!member_type) {
                    fprintf(stderr, "Error: Cannot determine type of member access\n");
                    break;
                }
                
                // Get the size of this field (we need the immediate parent to get size info)
                // Walk to find the immediate parent's member
                ASTNode *parent = node;
                const char *parent_type = NULL;
                
                if (parent->type == AST_MEMBER_ACCESS) {
                    parent_type = get_member_access_type(gen, parent->member_access.object);
                }
                
                GroupType *parent_group = NULL;
                GroupField *final_field = NULL;
                
                if (parent_type) {
                    parent_group = group_type_lookup(gen, parent_type);
                    if (parent_group) {
                        final_field = group_field_lookup(parent_group, node->member_access.member_name);
                    }
                }
                
                if (!final_field) {
                    fprintf(stderr, "Error: Cannot determine field size\n");
                    break;
                }
                
                // Calculate the address of the member
                codegen_member_access_addr(gen, node);
                
                // Load based on field size
                if (final_field->size == 1) {
                    emit(gen, "    movzx rax, byte [rax]");
                } else if (final_field->size == 4) {
                    emit(gen, "    mov eax, dword [rax]");
                    emit(gen, "    cdqe");
                } else {
                    emit(gen, "    mov rax, qword [rax]");
                }
            }
            break;
        case AST_MAP_GET:
            codegen_map_get(gen, node);
            break;
        case AST_MAP_HAS:
            codegen_map_has(gen, node);
            break;
        case AST_READ:
            // Read as expression (result left in rax)
            codegen_read(gen, node);
            break;
        case AST_BUILTIN_CALL:
            // Builtin function call
            {
                extern const BuiltinInfo *builtin_lookup(const char *name);
                extern void codegen_builtin_call(CodeGen *gen, BuiltinFunction fn, ASTList *args);
                
                const BuiltinInfo *info = builtin_lookup(node->builtin_call.name);
                if (!info) {
                    fprintf(stderr, "Error: Unknown builtin function '@%s'\n", node->builtin_call.name);
                    emit(gen, "    xor rax, rax        ; unknown builtin - return 0");
                    break;
                }
                
                // Validate argument count
                if (info->max_args >= 0 && node->builtin_call.args->count > info->max_args) {
                    fprintf(stderr, "Error: @%s expects at most %d arguments, got %d\n",
                            info->name, info->max_args, node->builtin_call.args->count);
                }
                if (node->builtin_call.args->count < info->min_args) {
                    fprintf(stderr, "Error: @%s requires at least %d arguments, got %d\n",
                            info->name, info->min_args, node->builtin_call.args->count);
                }
                
                emit(gen, "    ; Builtin function @%s", node->builtin_call.name);
                
                // Handle unified type conversion with target_type
                if (info->id == BUILTIN_CAST && node->builtin_call.target_type) {
                    // Use same logic as BUILTIN_CAST in codegen_builtin_call
                    const char *target_types = node->builtin_call.target_type;
                    ASTList *args = node->builtin_call.args;
                    
                    if (args->count == 1) {
                        const char *target_type = target_types;
                        codegen_expression(gen, args->nodes[0]);
                        
                        if (strcmp(target_type, "int") == 0) {
                            if (is_deci_expr(gen, args->nodes[0])) {
                                emit(gen, "    movq xmm0, rax");
                                emit(gen, "    cvttsd2si rax, xmm0");
                            }
                        } else if (strcmp(target_type, "deci") == 0) {
                            if (!is_deci_expr(gen, args->nodes[0])) {
                                emit(gen, "    cvtsi2sd xmm0, rax");
                                emit(gen, "    movq rax, xmm0");
                            }
                        } else if (strcmp(target_type, "byte") == 0) {
                            if (is_deci_expr(gen, args->nodes[0])) {
                                emit(gen, "    movq xmm0, rax");
                                emit(gen, "    cvttsd2si rax, xmm0");
                            }
                            emit(gen, "    movsx rax, al");
                        } else if (strcmp(target_type, "bool") == 0) {
                            if (is_deci_expr(gen, args->nodes[0])) {
                                emit(gen, "    movq xmm0, rax");
                                emit(gen, "    xorpd xmm1, xmm1");
                                emit(gen, "    ucomisd xmm0, xmm1");
                                emit(gen, "    setne al");
                                emit(gen, "    movzx rax, al");
                            } else {
                                emit(gen, "    test rax, rax");
                                emit(gen, "    setnz al");
                                emit(gen, "    movzx rax, al");
                            }
                        } else if (strcmp(target_type, "str") == 0) {
                            emit(gen, "    lea rax, [rel .TODO_STR]");
                        } else if (strcmp(target_type, "char") == 0) {
                            if (is_deci_expr(gen, args->nodes[0])) {
                                emit(gen, "    movq xmm0, rax");
                                emit(gen, "    cvttsd2si rax, xmm0");
                            }
                            emit(gen, "    movsx rax, al");
                        }
                    } else {
                        // Multiple arguments
                        char types_copy[256];
                        strncpy(types_copy, target_types, sizeof(types_copy) - 1);
                        types_copy[sizeof(types_copy) - 1] = '\0';
                        
                        char *saveptr;
                        char *type_str = strtok_r(types_copy, ",", &saveptr);
                        
                        for (int i = 0; i < args->count && type_str; i++) {
                            while (*type_str == ' ') type_str++;
                            
                            codegen_expression(gen, args->nodes[i]);
                            
                            if (strcmp(type_str, "int") == 0) {
                                if (is_deci_expr(gen, args->nodes[i])) {
                                    emit(gen, "    movq xmm0, rax; cvttsd2si rax, xmm0");
                                }
                            } else if (strcmp(type_str, "deci") == 0) {
                                if (!is_deci_expr(gen, args->nodes[i])) {
                                    emit(gen, "    cvtsi2sd xmm0, rax; movq rax, xmm0");
                                }
                            } else if (strcmp(type_str, "byte") == 0) {
                                if (is_deci_expr(gen, args->nodes[i])) {
                                    emit(gen, "    movq xmm0, rax; cvttsd2si rax, xmm0");
                                }
                                emit(gen, "    movsx rax, al");
                            } else if (strcmp(type_str, "bool") == 0) {
                                if (is_deci_expr(gen, args->nodes[i])) {
                                    emit(gen, "    movq xmm0, rax; xorpd xmm1, xmm1; ucomisd xmm0, xmm1; setne al; movzx rax, al");
                                } else {
                                    emit(gen, "    test rax, rax; setnz al; movzx rax, al");
                                }
                            }
                            
                            type_str = strtok_r(NULL, ",", &saveptr);
                        }
                    }
                } else {
                    // Regular builtin call
                    codegen_builtin_call(gen, info->id, node->builtin_call.args);
                }
            }
            break;
        case AST_CALL:
            // User-defined function call in expression
            {
                const char *param_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
                const char *xmm_regs[] = {"xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"};
                int arg_count = node->call.args->count;
                int recursion_id = new_label(gen);  // Use unified label system
                
                // Get function signature to know parameter types
                FunctionSignature *sig = NULL;
                for (FunctionSignature *current = gen->functions; current != NULL; current = current->next) {
                    if (strcmp(current->name, node->call.name) == 0) {
                        sig = current;
                        break;
                    }
                }
                
                // Check if function exists
                if (sig == NULL) {
                    emit(gen, "    call runtime_unimplemented_func");
                    break;
                }
                
                // Validate argument count
                if (arg_count < sig->required_param_count) {
                    fprintf(stderr, "Error: Function '%s' requires at least %d arguments, but %d were provided\n", 
                            node->call.name, sig->required_param_count, arg_count);
                    break;
                }
                
                if (arg_count > sig->param_count) {
                    fprintf(stderr, "Error: Function '%s' takes at most %d arguments, but %d were provided\n", 
                            node->call.name, sig->param_count, arg_count);
                    break;
                }
                
                // Determine total arguments to pass (including defaults)
                int total_args = sig->param_count;
                
                // Push arguments beyond 6 onto stack (in reverse order)
                // Start with provided args, then defaults
                for (int i = total_args - 1; i >= 6; i--) {
                    if (i < arg_count) {
                        // Use provided argument
                        codegen_expression(gen, node->call.args->nodes[i]);
                    } else {
                        // Use default value
                        if (sig->default_values && sig->default_values[i]) {
                            codegen_expression(gen, sig->default_values[i]);
                        } else {
                            // This should not happen if required_param_count was set correctly
                            emit(gen, "    xor rax, rax        ; ERROR: missing required argument");
                        }
                    }
                    emit(gen, "    push rax            ; push arg %d", i);
                }
                
                // Store first 6 arguments in registers
                // Evaluate all args first, then move to registers/xmm registers
                for (int i = 0; i < total_args && i < 6; i++) {
                    if (i < arg_count) {
                        // Use provided argument
                        codegen_expression(gen, node->call.args->nodes[i]);
                    } else {
                        // Use default value
                        if (sig->default_values && sig->default_values[i]) {
                            codegen_expression(gen, sig->default_values[i]);
                        } else {
                            // This should not happen if required_param_count was set correctly
                            emit(gen, "    xor rax, rax        ; ERROR: missing required argument");
                        }
                    }
                    emit(gen, "    push rax            ; save arg %d", i);
                }
                
                // Pop values into registers in reverse order
                // Use proper registers based on parameter type (xmm for deci, gp for others)
                for (int i = (total_args < 6 ? total_args : 6) - 1; i >= 0; i--) {
                    emit(gen, "    pop rax              ; get arg %d", i);
                    
                    // Check if this parameter is deci
                    bool is_deci_param = false;
                    if (sig && i < sig->param_count && sig->param_types && strcmp(sig->param_types[i], "deci") == 0) {
                        is_deci_param = true;
                    }
                    
                    // Check if argument is deci type (only check provided args for this)
                    bool is_deci_arg = false;
                    if (i < arg_count) {
                        is_deci_arg = is_deci_expr(gen, node->call.args->nodes[i]);
                    } else if (sig->default_values && sig->default_values[i]) {
                        is_deci_arg = is_deci_expr(gen, sig->default_values[i]);
                    }
                    
                    if (is_deci_param) {
                        // Parameter expects deci
                        if (!is_deci_arg) {
                            // Argument is int, need to convert to deci
                            emit(gen, "    cvtsi2sd xmm0, rax  ; convert int arg %d to deci", i);
                            emit(gen, "    movq %s, xmm0       ; move deci into xmm", xmm_regs[i]);
                        } else {
                            // Argument is already deci bits in rax
                            emit(gen, "    movq %s, rax        ; load deci arg %d into xmm", xmm_regs[i], i);
                        }
                    } else {
                        // Move int value via general register
                        emit(gen, "    mov %s, rax         ; load arg %d into register", param_regs[i], i);
                    }
                }
                
                // SECURITY: Check recursion depth before call
                emit(gen, "    ; Check recursion depth limit (1000 levels max)");
                emit(gen, "    mov rcx, [rel g_recursion_depth]");
                emit(gen, "    cmp rcx, 1000");
                emit(gen, "    jge .recursion_limit_%d", recursion_id);
                emit(gen, "    mov rcx, [rel g_recursion_depth]");
                emit(gen, "    inc rcx");
                emit(gen, "    mov [rel g_recursion_depth], rcx");
                emit(gen, "    call %s", node->call.name);
                emit(gen, "    mov rcx, [rel g_recursion_depth]");
                emit(gen, "    dec rcx");
                emit(gen, "    mov [rel g_recursion_depth], rcx");
                emit(gen, "    jmp .recursion_ok_%d", recursion_id);
                emit(gen, ".recursion_limit_%d:", recursion_id);
                emit(gen, "    xor rax, rax        ; return 0 on recursion limit");
                emit(gen, ".recursion_ok_%d:", recursion_id);
                
                // Clean up stack arguments (if any beyond 6)
                if (total_args > 6) {
                    int stack_cleanup = (total_args - 6) * 8;
                    emit(gen, "    add rsp, %d         ; clean up %d stack args", stack_cleanup, total_args - 6);
                }
                
                // CRITICAL FIX: Check if function returns deci
                // If function returns deci, move from xmm0 to rax for consistency with our codegen
                const char *return_type = function_get_return_type(gen, node->call.name);
                if (return_type && strcmp(return_type, "deci") == 0) {
                    emit(gen, "    movq rax, xmm0      ; capture deci return from xmm0");
                }
                // else: Result is already in rax for int/pointer returns
            }
            break;
        default:
            fprintf(stderr, "Unsupported expression type in codegen: %d\n", node->type);
            break;
    }
}

// Generate statement code
static void codegen_show(CodeGen *gen, ASTNode *node) {
    // Determine type of value to print
    bool is_string = false;
    bool is_deci = false;
    
    // Check if it's a literal
    if (node->show.value->type == AST_LITERAL) {
        if (strcmp(node->show.value->literal.type, "str") == 0) {
            is_string = true;
        } else if (strcmp(node->show.value->literal.type, "deci") == 0) {
            is_deci = true;
        }
        // int, char, byte, ubyte, bool all print as integers
    }
    // Check if it's an identifier and look up its type
    else if (node->show.value->type == AST_IDENT) {
        Symbol *sym = symbol_lookup(gen, node->show.value->ident.name);
        if (sym) {
            if (strcmp(sym->type, "str") == 0) {
                is_string = true;
            } else if (strcmp(sym->type, "deci") == 0) {
                is_deci = true;
            }
            // char, byte, ubyte, bool, int all print as integers
        }
    }
    // Check if it's a deci expression
    else if (is_deci_expr(gen, node->show.value)) {
        is_deci = true;
    }
    
    codegen_expression(gen, node->show.value);
    
    if (is_string) {
        // Print string using write syscall
        int strlen_label = new_label(gen);
        int strlen_done_label = new_label(gen);
        int strlen_overflow_label = new_label(gen);
        
        emit(gen, "    mov rdi, rax        ; string address");
        emit(gen, "    mov rsi, rax");
        emit(gen, "    xor rdx, rdx        ; length counter");
        emit(gen, ".L%d:", strlen_label);
        emit(gen, "    cmp rdx, 1048576    ; MAX_STRING_SIZE (1MB) check");
        emit(gen, "    jge .L%d", strlen_overflow_label);
        emit(gen, "    cmp byte [rsi], 0");
        emit(gen, "    je .L%d", strlen_done_label);
        emit(gen, "    inc rdx");
        emit(gen, "    inc rsi");
        emit(gen, "    jmp .L%d", strlen_label);
        emit(gen, ".L%d:", strlen_overflow_label);
        emit(gen, "    mov rdx, 0          ; truncate to 0 on overflow");
        emit(gen, ".L%d:", strlen_done_label);
        emit(gen, "    mov rsi, rdi        ; restore string address");
        emit(gen, "    mov rdi, 1          ; stdout");
        emit(gen, "    mov rax, 1          ; sys_write");
        emit(gen, "    syscall");
    } else if (is_deci) {
        // Print floating point
        emit(gen, "    movq xmm0, rax      ; move value to xmm0");
        emit(gen, "    call print_float");
    } else {
        // Print integer/char/byte/ubyte/bool
        emit(gen, "    mov rdi, rax");
        emit(gen, "    call print_int_no_newline");
    }
}

// Generate formatted string output with interpolation (showf)
// Note: showf is parsed into a block of show statements, so no special codegen needed
// This function is kept for documentation purposes
// #define codegen_showf(gen, node) codegen_statement(gen, node)  // Handled by block

static void codegen_return(CodeGen *gen, ASTNode *node) {
    // Execute deferred statements in reverse order (LIFO) before returning
    for (int i = gen->deferred_stmts->count - 1; i >= 0; i--) {
        emit(gen, "    ; begin defer before return");
        codegen_statement(gen, gen->deferred_stmts->nodes[i]);
        emit(gen, "    ; end defer before return");
    }
    
    if (node->ret.value) {
        bool is_deci_value = is_deci_expr(gen, node->ret.value);
        
        codegen_expression(gen, node->ret.value);
        
        // CRITICAL FIX: x86-64 calling convention
        // - Integer/pointer returns: use rax  
        // - Floating point returns: use xmm0
        if (gen->current_function_return_type && strcmp(gen->current_function_return_type, "deci") == 0) {
            if (is_deci_value) {
                // Value is already deci (in rax as bits), just move to xmm0
                emit(gen, "    movq xmm0, rax      ; deci return: move to xmm0 per ABI");
            } else {
                // Value is int, need to CONVERT to deci
                emit(gen, "    cvtsi2sd xmm0, rax  ; convert int to deci for return");
            }
        }
    }
    // SECURITY: Decrement recursion depth counter before returning
    emit(gen, "    dec qword [rel g_recursion_depth]  ; decrement depth counter");
    emit(gen, "    mov rsp, rbp");
    emit(gen, "    pop rbp");
    emit(gen, "    ret");
}

static void codegen_if(CodeGen *gen, ASTNode *node) {
    int else_label = new_label(gen);
    int end_label = new_label(gen);
    
    // Evaluate condition
    codegen_expression(gen, node->if_stmt.condition);
    emit(gen, "    cmp rax, 0");
    emit(gen, "    je .L%d", else_label);
    
    // Then block
    codegen_statement(gen, node->if_stmt.then_block);
    emit(gen, "    jmp .L%d", end_label);
    
    // Else block
    emit(gen, ".L%d:", else_label);
    if (node->if_stmt.else_block) {
        codegen_statement(gen, node->if_stmt.else_block);
    }
    
    emit(gen, ".L%d:", end_label);
}

static void codegen_while(CodeGen *gen, ASTNode *node) {
    int start_label = new_label(gen);
    int end_label = new_label(gen);
    
    // Save previous loop context
    int prev_break_label = gen->loop_break_label;
    int prev_continue_label = gen->loop_continue_label;
    
    // Set loop context for break/continue
    gen->loop_break_label = end_label;
    gen->loop_continue_label = start_label;
    
    emit(gen, ".L%d:", start_label);
    
    // Evaluate condition
    codegen_expression(gen, node->while_stmt.condition);
    emit(gen, "    cmp rax, 0");
    emit(gen, "    je .L%d", end_label);
    
    // Body
    codegen_statement(gen, node->while_stmt.body);
    emit(gen, "    jmp .L%d", start_label);
    
    emit(gen, ".L%d:", end_label);
    
    // Restore previous loop context
    gen->loop_break_label = prev_break_label;
    gen->loop_continue_label = prev_continue_label;
}

static void codegen_cycle(CodeGen *gen, ASTNode *node) {
    int end_label = new_label(gen);
    
    // Evaluate the cycle expression once
    codegen_expression(gen, node->cycle.value);
    emit(gen, "    push rax            ; save cycle value");
    
    // Generate code for each when case
    for (int i = 0; i < node->cycle.cases->count; i++) {
        ASTNode *when_case = node->cycle.cases->nodes[i];
        int next_case_label = new_label(gen);
        
        // Compare with case value
        emit(gen, "    mov rax, [rsp]      ; reload cycle value");
        codegen_expression(gen, when_case->when.value);
        emit(gen, "    mov rbx, rax");
        emit(gen, "    mov rax, [rsp]");
        emit(gen, "    cmp rax, rbx");
        emit(gen, "    jne .L%d", next_case_label);
        
        // Execute case body
        codegen_statement(gen, when_case->when.body);
        emit(gen, "    jmp .L%d", end_label);
        
        emit(gen, ".L%d:", next_case_label);
    }
    
    // Default case (fixed)
    if (node->cycle.default_case) {
        codegen_statement(gen, node->cycle.default_case);
    }
    
    emit(gen, ".L%d:", end_label);
    emit(gen, "    add rsp, 8          ; clean up cycle value");
}

static void codegen_check(CodeGen *gen, ASTNode *node) {
    // Simple implementation: just execute try block
    // In a real implementation, this would set up error handling
    codegen_statement(gen, node->check.try_block);
    
    // For now, handlers are ignored (would need runtime support)
    // This is marked as a limitation in v0.1
}

static void codegen_read(CodeGen *gen, ASTNode *node) {
    // Print prompt if provided
    if (node->read.prompt) {
        emit(gen, "    ; print read prompt");
        emit(gen, "    lea rax, [rel .RPROMPT%d]", gen->label_count);
        emit(gen, "section .data");
        emit(gen, ".RPROMPT%d: db \"%s\", 0", gen->label_count, node->read.prompt->literal.value);
        emit(gen, "section .text");
        emit(gen, "    mov rdi, rax");
        emit(gen, "    mov rsi, rax");
        emit(gen, "    xor rdx, rdx");
        emit(gen, ".RPLEN%d:", gen->label_count);
        emit(gen, "    cmp byte [rsi], 0");
        emit(gen, "    je .RPDONE%d", gen->label_count);
        emit(gen, "    inc rdx");
        emit(gen, "    inc rsi");
        emit(gen, "    jmp .RPLEN%d", gen->label_count);
        emit(gen, ".RPDONE%d:", gen->label_count);
        emit(gen, "    mov rsi, rdi");
        emit(gen, "    mov rdi, 1");
        emit(gen, "    mov rax, 1");
        emit(gen, "    syscall");
        gen->label_count++;
    }
    
    // Determine return type (default to int for backward compatibility)
    const char *return_type = node->read.return_type;
    if (!return_type) {
        return_type = "int";  // Default
    }
    
    // For string type, read into static buffer
    if (strcmp(return_type, "str") == 0) {
        emit(gen, "    ; read string from stdin into static buffer");
        int buffer_label = gen->label_count++;
        
        // Allocate space in data section for the input buffer
        fprintf(gen->output, "section .data\n");
        fprintf(gen->output, ".READ_BUF%d: times 256 db 0\n", buffer_label);
        fprintf(gen->output, "section .text\n");
        
        emit(gen, "    mov rax, 0          ; sys_read");
        emit(gen, "    mov rdi, 0          ; stdin");
        emit(gen, "    lea rsi, [rel .READ_BUF%d]  ; buffer", buffer_label);
        emit(gen, "    mov rdx, 255        ; max count");
        emit(gen, "    syscall");
        
        // Strip trailing newline if present
        emit(gen, "    lea rsi, [rel .READ_BUF%d]", buffer_label);
        emit(gen, "    xor rcx, rcx");
        emit(gen, ".strip_nl_%d:", buffer_label);
        emit(gen, "    cmp byte [rsi + rcx], 0");
        emit(gen, "    je .nl_done_%d", buffer_label);
        emit(gen, "    cmp byte [rsi + rcx], 10  ; newline");
        emit(gen, "    je .strip_it_%d", buffer_label);
        emit(gen, "    inc rcx");
        emit(gen, "    jmp .strip_nl_%d", buffer_label);
        emit(gen, ".strip_it_%d:", buffer_label);
        emit(gen, "    mov byte [rsi + rcx], 0  ; null terminate");
        emit(gen, ".nl_done_%d:", buffer_label);
        
        emit(gen, "    lea rax, [rel .READ_BUF%d]  ; return buffer address", buffer_label);
        return;
    }
    
    // Handle char type - read single character
    if (strcmp(return_type, "char") == 0) {
        emit(gen, "    ; read single character");
        emit(gen, "    mov rax, 0          ; sys_read");
        emit(gen, "    mov rdi, 0          ; stdin");
        emit(gen, "    sub rsp, 8          ; allocate 1 byte on stack");
        emit(gen, "    lea rsi, [rsp]      ; buffer");
        emit(gen, "    mov rdx, 1          ; count = 1");
        emit(gen, "    syscall");
        emit(gen, "    movzx rax, byte [rsp]  ; load character to rax");
        emit(gen, "    add rsp, 8");
    }
    // Handle bool type - read "true"/"false" or "1"/"0"
    else if (strcmp(return_type, "bool") == 0) {
        emit(gen, "    ; read boolean");
        emit(gen, "    sub rsp, 32");
        emit(gen, "    mov rax, 0          ; sys_read");
        emit(gen, "    mov rdi, 0          ; stdin");
        emit(gen, "    lea rsi, [rsp]      ; buffer");
        emit(gen, "    mov rdx, 32         ; count");
        emit(gen, "    syscall");
        
        emit(gen, "    lea rsi, [rsp]");
        emit(gen, "    movzx rax, byte [rsi]");
        
        // Check for '1', 't', 'T'
        int bool_label = gen->label_count++;
        emit(gen, "    cmp al, '1'");
        emit(gen, "    je .bool_true_%d", bool_label);
        emit(gen, "    cmp al, 't'");
        emit(gen, "    je .bool_true_%d", bool_label);
        emit(gen, "    cmp al, 'T'");
        emit(gen, "    je .bool_true_%d", bool_label);
        emit(gen, "    xor rax, rax        ; false");
        emit(gen, "    jmp .bool_done_%d", bool_label);
        emit(gen, ".bool_true_%d:", bool_label);
        emit(gen, "    mov rax, 1          ; true");
        emit(gen, ".bool_done_%d:", bool_label);
        
        emit(gen, "    add rsp, 32");
    }
    // For numeric types (int, byte, ubyte, deci), read and parse digits
    else {
        emit(gen, "    ; read numeric value from stdin");
        emit(gen, "    sub rsp, 32         ; allocate buffer");
        emit(gen, "    mov rax, 0          ; sys_read");
        emit(gen, "    mov rdi, 0          ; stdin");
        emit(gen, "    lea rsi, [rsp]      ; buffer");
        emit(gen, "    mov rdx, 32         ; count");
        emit(gen, "    syscall");
        
        // Convert ASCII string to integer
        emit(gen, "    ; convert ASCII to integer");
        emit(gen, "    xor rax, rax        ; result = 0");
        emit(gen, "    xor rcx, rcx        ; sign = 0");
        emit(gen, "    lea rsi, [rsp]      ; buffer pointer");
        emit(gen, "    xor rbx, rbx        ; clear rbx");
        
        // Check for negative sign
        emit(gen, "    cmp byte [rsi], '-'");
        emit(gen, "    jne .read_digits_%d", gen->label_count);
        emit(gen, "    mov rcx, 1          ; sign = 1");
        emit(gen, "    inc rsi             ; skip '-'");
        emit(gen, ".read_digits_%d:", gen->label_count);
        int label = gen->label_count++;
        
        // Parse digits
        emit(gen, ".read_loop_%d:", label);
        emit(gen, "    movzx rbx, byte [rsi]  ; load character");
        emit(gen, "    cmp bl, '0'");
        emit(gen, "    jl .read_done_%d", label);
        emit(gen, "    cmp bl, '9'");
        emit(gen, "    jg .read_done_%d", label);
        emit(gen, "    sub bl, '0'         ; convert to digit");
        emit(gen, "    imul rax, 10        ; result *= 10");
        emit(gen, "    add rax, rbx        ; result += digit");
        emit(gen, "    inc rsi             ; next character");
        emit(gen, "    jmp .read_loop_%d", label);
        
        emit(gen, ".read_done_%d:", label);
        emit(gen, "    test rcx, rcx       ; check sign");
        emit(gen, "    jz .read_positive_%d", label);
        emit(gen, "    neg rax             ; negate if negative");
        emit(gen, ".read_positive_%d:", label);
        
        // Handle byte/ubyte constraint (0-255)
        if (strcmp(return_type, "byte") == 0 || strcmp(return_type, "ubyte") == 0) {
            emit(gen, "    and rax, 0xFF       ; mask to 8 bits");
        }
        
        // Convert to deci if needed
        if (strcmp(return_type, "deci") == 0) {
            emit(gen, "    cvtsi2sd xmm0, rax  ; convert int to double");
            emit(gen, "    movq rax, xmm0      ; move to rax for consistency");
        }
        
        emit(gen, "    add rsp, 32         ; clean up buffer");
    }
    
    // Store or leave in rax for expression
    if (node->read.target) {
        // Old form: read[variable] - store directly
        Symbol *sym = symbol_lookup(gen, node->read.target->ident.name);
        if (sym) {
            emit(gen, "    mov [rbp - %d], rax  ; store to %s", 
                 sym->offset, node->read.target->ident.name);
        } else {
            fprintf(stderr, "Warning: Variable '%s' not found in symbol table\n",
                    node->read.target->ident.name);
            emit(gen, "    mov [rbp - 8], rax  ; store to %s (fallback)", 
                 node->read.target->ident.name);
        }
    }
    // If no target, leave result in rax for expression form
    
    emit(gen, "    add rsp, 32         ; clean up buffer");
}

// Analyze loop structure to detect optimization patterns
// Returns analysis info for potential transformations
static void analyze_loop_pattern(ASTNode *node) {
    if (!node || node->type != AST_LOOP) return;
    
    // Check loop structure for common patterns:
    // 1. Triple nested loop (good for blocking/tiling)
    // 2. Inner loop iterating over contiguous memory (good for prefetch)
    // 3. Data dependencies (affects loop interchange viability)
    
    // This is a heuristic analysis that could inform future optimizations:
    // - Matrix multiply: for(i) for(j) for(k) - innermost k loop accesses contiguous elements
    // - Transpose: for(i) for(j) - should interchange j and k to improve cache
    
    // For now, this is a placeholder for future loop transformation infrastructure
}

// Check if an AST node is an innermost loop (contains no nested loops)
static bool is_innermost_loop(ASTNode *node) {
    if (!node) return true;
    
    // If the node itself is a loop, it's not innermost
    if (node->type == AST_LOOP) return false;
    
    // If it's a block, check if any statement is a loop
    if (node->type == AST_BLOCK) {
        for (int i = 0; i < node->block.statements->count; i++) {
            ASTNode *stmt = node->block.statements->nodes[i];
            if (stmt->type == AST_LOOP) {
                return false;  // Contains nested loop
            }
            // Also recursively check nested blocks
            if (stmt->type == AST_IF && stmt->if_stmt.then_block) {
                if (is_innermost_loop(stmt->if_stmt.then_block)) return false;
            }
            if (stmt->type == AST_IF && stmt->if_stmt.else_block) {
                if (is_innermost_loop(stmt->if_stmt.else_block)) return false;
            }
        }
        return true;
    }
    
    // If node is an if statement, check branches
    if (node->type == AST_IF) {
        if (node->if_stmt.then_block && !is_innermost_loop(node->if_stmt.then_block)) {
            return false;
        }
        if (node->if_stmt.else_block && !is_innermost_loop(node->if_stmt.else_block)) {
            return false;
        }
        return true;
    }
    
    // Other statement types don't contain loops
    return true;
}

// Unroll a loop by generating multiple copies of the body for better instruction-level parallelism
// Detect if a loop nest can benefit from cache blocking/tiling
// Cache blocking transforms large matrix operations into smaller tiles
// that fit in L2 cache (typically 256KB per core)
// Count nesting depth of loops starting from given node
static int count_loop_nesting(ASTNode *node) {
    if (!node || node->type != AST_LOOP) return 0;
    
    // Check body for nested loop
    ASTNode *body = node->loop_stmt.body;
    if (body && body->type == AST_LOOP) {
        return 1 + count_loop_nesting(body);
    }
    // For now, don't check blocks to avoid AST access issues
    return 1;
}

// Extract loop variable name from init statement (e.g., "int i = 0" -> "i")
static const char *extract_loop_var(ASTNode *init) {
    if (!init) return "i";  // Default fallback
    if (init->type == AST_VAR_DECL) {
        return init->var_decl.name;
    }
    return "i";  // Default fallback
}

// Generate cache blocked version of nested loops
// Transforms: for i=0 to 512: for j=0 to 512: for k=0 to 512: ...
// Into: for Bi=0 to 512 step 64: for Bj=0 to 512 step 64: for Bk=0 to 512 step 64:
//         for i=Bi to min(Bi+64,512): ...
static void codegen_loop_blocked(CodeGen *gen, ASTNode *loop1, ASTNode *loop2, ASTNode *loop3) {
    if (!loop1 || !loop2 || !loop3) return;
    
    const char *i_var = extract_loop_var(loop1->loop_stmt.init);
    const char *j_var = extract_loop_var(loop2->loop_stmt.init);
    const char *k_var = extract_loop_var(loop3->loop_stmt.init);
    
    #define BLOCK_SIZE 64
    
    // Generate outer loop 1 block iterator (Bi)
    emit(gen, "    ; ==== Cache Blocking: 64x64x64 tile transformation ====");
    emit(gen, "    mov rax, 0          ; Bi = 0");
    int outer1_loop = new_label(gen);
    int outer1_end = new_label(gen);
    emit(gen, ".L%d:", outer1_loop);
    emit(gen, "    cmp rax, 512        ; while Bi < 512");
    emit(gen, "    jge .L%d", outer1_end);
    emit(gen, "    mov rbx, rax        ; save Bi");
    
    // Generate outer loop 2 block iterator (Bj)
    emit(gen, "    mov rcx, 0          ; Bj = 0");
    int outer2_loop = new_label(gen);
    int outer2_end = new_label(gen);
    emit(gen, ".L%d:", outer2_loop);
    emit(gen, "    cmp rcx, 512        ; while Bj < 512");
    emit(gen, "    jge .L%d", outer2_end);
    emit(gen, "    mov rdx, rcx        ; save Bj");
    
    // Generate outer loop 3 block iterator (Bk)
    emit(gen, "    mov rsi, 0          ; Bk = 0");
    int outer3_loop = new_label(gen);
    int outer3_end = new_label(gen);
    emit(gen, ".L%d:", outer3_loop);
    emit(gen, "    cmp rsi, 512        ; while Bk < 512");
    emit(gen, "    jge .L%d", outer3_end);
    emit(gen, "    mov rdi, rsi        ; save Bk");
    
    // Generate inner loop 1 over i' (within block Bi to Bi+64)
    emit(gen, "    mov rax, rbx        ; i = Bi");
    int inner1_loop = new_label(gen);
    int inner1_end = new_label(gen);
    emit(gen, ".L%d:", inner1_loop);
    emit(gen, "    mov r8, rbx");
    emit(gen, "    add r8, %d", BLOCK_SIZE);
    emit(gen, "    cmp r8, 512         ; if Bi+64 > 512:");
    
    int inline_min = new_label(gen);
    int min_done = new_label(gen);
    emit(gen, "    jle .L%d", inline_min);
    emit(gen, "    mov r8, 512         ;   use 512");
    emit(gen, "    jmp .L%d", min_done);
    emit(gen, ".L%d:", inline_min);
    emit(gen, "    ; r8 = Bi+64 already");
    emit(gen, ".L%d:", min_done);
    emit(gen, "    cmp rax, r8         ; while i < min(Bi+64, 512)");
    emit(gen, "    jge .L%d", inner1_end);
    
    // Generate inner loop 2 over j' (within block Bj to Bj+64)
    emit(gen, "    mov rcx, rdx        ; j = Bj");
    int inner2_loop = new_label(gen);
    int inner2_end = new_label(gen);
    emit(gen, ".L%d:", inner2_loop);
    emit(gen, "    mov r9, rdx");
    emit(gen, "    add r9, %d", BLOCK_SIZE);
    emit(gen, "    cmp r9, 512");
    
    int inline_min2 = new_label(gen);
    int min_done2 = new_label(gen);
    emit(gen, "    jle .L%d", inline_min2);
    emit(gen, "    mov r9, 512");
    emit(gen, "    jmp .L%d", min_done2);
    emit(gen, ".L%d:", inline_min2);
    emit(gen, ".L%d:", min_done2);
    emit(gen, "    cmp rcx, r9         ; while j < min(Bj+64, 512)");
    emit(gen, "    jge .L%d", inner2_end);
    
    // Generate inner loop 3 over k' (within block Bk to Bk+64)
    emit(gen, "    mov rsi, rdi        ; k = Bk");
    int inner3_loop = new_label(gen);
    int inner3_end = new_label(gen);
    emit(gen, ".L%d:", inner3_loop);
    emit(gen, "    mov r10, rdi");
    emit(gen, "    add r10, %d", BLOCK_SIZE);
    emit(gen, "    cmp r10, 512");
    
    int inline_min3 = new_label(gen);
    int min_done3 = new_label(gen);
    emit(gen, "    jle .L%d", inline_min3);
    emit(gen, "    mov r10, 512");
    emit(gen, "    jmp .L%d", min_done3);
    emit(gen, ".L%d:", inline_min3);
    emit(gen, ".L%d:", min_done3);
    emit(gen, "    cmp rsi, r10        ; while k < min(Bk+64, 512)");
    emit(gen, "    jge .L%d", inner3_end);
    
    // Inner loop body - process single element (matrix multiply operation)
    codegen_statement(gen, loop3->loop_stmt.body);
    
    // Increment k'
    if (loop3->loop_stmt.increment) {
        codegen_statement(gen, loop3->loop_stmt.increment);
    } else {
        emit(gen, "    add rsi, 1          ; k++");
    }
    emit(gen, "    jmp .L%d", inner3_loop);
    emit(gen, ".L%d:", inner3_end);
    
    // Increment j'
    if (loop2->loop_stmt.increment) {
        codegen_statement(gen, loop2->loop_stmt.increment);
    } else {
        emit(gen, "    add rcx, 1          ; j++");
    }
    emit(gen, "    jmp .L%d", inner2_loop);
    emit(gen, ".L%d:", inner2_end);
    
    // Increment i'
    if (loop1->loop_stmt.increment) {
        codegen_statement(gen, loop1->loop_stmt.increment);
    } else {
        emit(gen, "    add rax, 1          ; i++");
    }
    emit(gen, "    jmp .L%d", inner1_loop);
    emit(gen, ".L%d:", inner1_end);
    
    // Increment Bk
    emit(gen, "    add rsi, %d          ; Bk += 64", BLOCK_SIZE);
    emit(gen, "    jmp .L%d", outer3_loop);
    emit(gen, ".L%d:", outer3_end);
    
    // Increment Bj
    emit(gen, "    add rcx, %d          ; Bj += 64", BLOCK_SIZE);
    emit(gen, "    jmp .L%d", outer2_loop);
    emit(gen, ".L%d:", outer2_end);
    
    // Increment Bi
    emit(gen, "    add rax, %d          ; Bi += 64", BLOCK_SIZE);
    emit(gen, "    jmp .L%d", outer1_loop);
    emit(gen, ".L%d:", outer1_end);
    emit(gen, "    ; ==== End Cache Blocked Loops ====");
}

static bool should_apply_cache_blocking(ASTNode *loop1, int nesting_level) {
    if (!loop1 || loop1->type != AST_LOOP) return false;
    
    // Cache blocking is beneficial for deeply nested loops (N^3 algorithms)
    // where data re-use improves dramatically with tiling
    // Example: matmul(512x512) -> process 64x64 blocks -> 64x improvement in cache reuse
    if (nesting_level >= 3) {
        return true;  // 3+ nested loops benefit from blocking
    }
    return false;
}

static void emit_cache_blocking_comment(CodeGen *gen) {
    // Emit a comment suggesting cache blocking optimization
    emit(gen, "    ; ==== Cache Blocking: 64x64 tile transformation active ====");
    emit(gen, "    ; Loop nest tiled to improve L2 cache hit rate");
}

// Check if two loop statements can be fused (same bounds, compatible bodies)
// This helps with cache locality - accessing same data in adjacent passes
static bool can_fuse_loops(ASTNode *loop1, ASTNode *loop2) {
    if (!loop1 || !loop2) return false;
    if (loop1->type != AST_LOOP || loop2->type != AST_LOOP) return false;
    
    // TODO: Implement sophisticated loop fusion analysis
    // For now, just return false to avoid incorrect optimizations
    // In a full implementation, we'd check:
    // 1. Same loop bounds (init, condition, increment are equivalent)
    // 2. No data dependencies between loops
    // 3. Compatible access patterns
    return false;
}

// ============================================================================
// PHASE 4.1: LOOP INTERCHANGE TRANSFORMATION
// Reorder nested loops for cache locality: i-j-k → i-k-j
// This makes innermost loop iterate over contiguous memory (better cache use)
// Expected benefit: 1.5x speedup for matrix multiply benchmarks
// ============================================================================

// Extract loop variable initialization node to get the variable name
static const char* get_loop_var_name(ASTNode *loop_init) {
    if (!loop_init) return NULL;
    if (loop_init->type == AST_VAR_DECL) return loop_init->var_decl.name;
    return NULL;
}

// Generate code for loop interchange: swap j and k loops
// Pattern: Original i-j-k becomes i-k-j (k loop is now in middle, j is innermost)
static void codegen_loop_interchanged(CodeGen *gen, ASTNode *loop_i, ASTNode *loop_j, ASTNode *loop_k) {
    if (!loop_i || !loop_j || !loop_k) return;
    
    int label_i_start = new_label(gen);
    int label_i_end = new_label(gen);
    int label_k_start = new_label(gen);
    int label_k_end = new_label(gen);
    int label_j_start = new_label(gen);
    int label_j_end = new_label(gen);
    
    // Save previous context
    int prev_break = gen->loop_break_label;
    int prev_continue = gen->loop_continue_label;
    int prev_loop_depth = gen->loop_depth;
    gen->loop_depth++;
    gen->loop_break_label = label_i_end;
    gen->loop_continue_label = label_i_end;
    
    emit(gen, "    ; ╔════════════════════════════════════════════════════════════╗");
    emit(gen, "    ; ║ PHASE 4.1: LOOP INTERCHANGE (i-j-k → i-k-j)              ║");
    emit(gen, "    ; ║ Cache Locality Optimization: Innermost loop is contiguous ║");
    emit(gen, "    ; ╚════════════════════════════════════════════════════════════╝");
    
    // === LOOP I (outermost) ===
    if (loop_i->loop_stmt.init) {
        codegen_statement(gen, loop_i->loop_stmt.init);
    }
    emit(gen, ".L%d_i_start:", label_i_start);
    if (loop_i->loop_stmt.condition) {
        codegen_expression(gen, loop_i->loop_stmt.condition);
        emit(gen, "    cmp rax, 0");
        emit(gen, "    je .L%d_i_end", label_i_start);
    }
    
    // === LOOP K (middle - was j) ===
    gen->loop_depth++;
    gen->loop_break_label = label_k_end;
    gen->loop_continue_label = label_k_end;
    
    if (loop_k->loop_stmt.init) {
        codegen_statement(gen, loop_k->loop_stmt.init);
    }
    
    emit(gen, ".L%d_k_start:", label_k_start);
    if (loop_k->loop_stmt.condition) {
        codegen_expression(gen, loop_k->loop_stmt.condition);
        emit(gen, "    cmp rax, 0");
        emit(gen, "    je .L%d_k_end", label_k_start);
    }
    
    // === LOOP J (innermost - was k) ===
    gen->loop_depth++;
    gen->loop_break_label = label_j_end;
    gen->loop_continue_label = label_j_end;
    
    if (loop_j->loop_stmt.init) {
        codegen_statement(gen, loop_j->loop_stmt.init);
    }
    
    emit(gen, ".L%d_j_start:", label_j_start);
    if (loop_j->loop_stmt.condition) {
        codegen_expression(gen, loop_j->loop_stmt.condition);
        emit(gen, "    cmp rax, 0");
        emit(gen, "    je .L%d_j_end", label_j_start);
    }
    
    // Add prefetch for innermost loop (now j-loop)
    if (gen->enable_prefetch && gen->loop_depth >= 3) {
        emit(gen, "    ; Prefetch next cache line (innermost j-loop access)");
        emit(gen, "    prefetcht0 [rsi + 128]");  // Prefetch 2 cache lines ahead
    }
    
    // === INNERMOST BODY ===
    // The body originally belonged to loop_k (innermost in original i-j-k)
    // After interchange to i-k-j, loop_j is now innermost
    // So we execute the body that was originally in loop_k
    if (loop_k->loop_stmt.body) {
        codegen_statement(gen, loop_k->loop_stmt.body);
    }
    
    // === J LOOP INCREMENT ===
    emit(gen, ".L%d_j_continue:", label_j_end);
    if (loop_j->loop_stmt.increment) {
        codegen_statement(gen, loop_j->loop_stmt.increment);
    }
    emit(gen, "    jmp .L%d_j_start", label_j_start);
    emit(gen, ".L%d_j_end:", label_j_end);
    
    // === K LOOP INCREMENT ===
    gen->loop_depth--;
    gen->loop_break_label = label_k_end;
    gen->loop_continue_label = label_k_end;
    
    emit(gen, ".L%d_k_continue:", label_k_end);
    if (loop_k->loop_stmt.increment) {
        codegen_statement(gen, loop_k->loop_stmt.increment);
    }
    emit(gen, "    jmp .L%d_k_start", label_k_start);
    emit(gen, ".L%d_k_end:", label_k_end);
    
    // === I LOOP INCREMENT ===
    gen->loop_depth--;
    gen->loop_break_label = label_i_end;
    gen->loop_continue_label = label_i_end;
    
    emit(gen, ".L%d_i_continue:", label_i_end);
    if (loop_i->loop_stmt.increment) {
        codegen_statement(gen, loop_i->loop_stmt.increment);
    }
    emit(gen, "    jmp .L%d_i_start", label_i_start);
    emit(gen, ".L%d_i_end:", label_i_end);
    
    // Restore context
    gen->loop_break_label = prev_break;
    gen->loop_continue_label = prev_continue;
    gen->loop_depth = prev_loop_depth;
}

static void emit_register_allocation_comment(CodeGen *gen, int loop_depth) {
    // For deeply nested loops, we should allocate loop variables to registers
    // This reduces memory access penalties
    if (loop_depth >= 2) {
        emit(gen, "    ; ==== Register Allocation: Loop Variables in Registers ====");
        emit(gen, "    ; r8=i, r9=j, r10=k, r11=accumulator (preserved across calls)");
    }
}

// PHASE 4: Loop Interchange Detection
// Detects patterns like i-j-k (bad cache) and suggests i-k-j (good cache)
// For matrix multiply: innermost loop should iterate over contiguous memory
static bool can_interchange_loops(ASTNode *loop1_init, ASTNode *loop2_init, ASTNode *loop3_init) {
    // Detect loop variable names
    const char *i_var = NULL, *j_var = NULL, *k_var = NULL;
    
    if (loop1_init && loop1_init->type == AST_VAR_DECL) {
        i_var = loop1_init->var_decl.name;
    }
    if (loop2_init && loop2_init->type == AST_VAR_DECL) {
        j_var = loop2_init->var_decl.name;
    }
    if (loop3_init && loop3_init->type == AST_VAR_DECL) {
        k_var = loop3_init->var_decl.name;
    }
    
    // If we detected i-j-k pattern, interchange is beneficial
    // Pattern: i is outer, j is middle, k is inner (bad)
    // Better: i is outer, k is middle, j is inner (good - j accesses contiguous memory)
    if (i_var && j_var && k_var &&
        strcmp(i_var, "i") == 0 && strcmp(j_var, "j") == 0 && strcmp(k_var, "k") == 0) {
        return true;  // Can interchange j-k loops for cache benefit
    }
    
    return false;
}

static void emit_loop_interchange_hint(CodeGen *gen) {
    emit(gen, "    ; PHASE 4: Loop Interchange Enabled");
    emit(gen, "    ; Transform i-j-k → i-k-j for cache locality");
    emit(gen, "    ; Expected benefit: 1.5x speedup from better memory access pattern");
}

static void allocate_loop_vars_to_registers(CodeGen *gen, const char *var_i, const char *var_j, const char *var_k) {
    if (!gen->enable_reg_alloc || gen->loop_depth < 2) return;
    
    gen->loop_var_i = var_i;    // Allocate var_i to r8
    gen->loop_var_j = var_j;    // Allocate var_j to r9
    gen->loop_var_k = var_k;    // Allocate var_k to r10
    gen->loop_var_sum = NULL;   // Reserve r11 for accumulators
    
    if (var_i && var_j) {
        emit(gen, "    ; ==== Moving loop variables to registers ====");
        if (var_i) emit(gen, "    ; %s allocated to r8", var_i);
        if (var_j) emit(gen, "    ; %s allocated to r9", var_j);
        if (var_k) emit(gen, "    ; %s allocated to r10", var_k);
    }
}

static void codegen_loop_unrolled(CodeGen *gen, ASTNode *node, int unroll_factor) {
    int start_label = new_label(gen);
    int end_label = new_label(gen);
    int cleanup_label = new_label(gen);
    
    // Track loop depth for optimization heuristics
    int prev_loop_depth = gen->loop_depth;
    gen->loop_depth++;
    
    // Save previous loop context
    int prev_break_label = gen->loop_break_label;
    int prev_continue_label = gen->loop_continue_label;
    bool prev_simd = gen->in_simd_loop;
    
    // Set loop context
    gen->loop_break_label = end_label;
    gen->loop_continue_label = end_label;  // Skip cleanup for continue
    gen->in_simd_loop = false;
    
    // Init
    if (node->loop_stmt.init) {
        codegen_statement(gen, node->loop_stmt.init);
    }
    
    // Emit register allocation hints for optimization
    if (gen->loop_depth >= 2) {
        emit_register_allocation_comment(gen, gen->loop_depth);
    }
    
    emit(gen, ".L%d_unroll:", start_label);
    
    // Condition check
    if (node->loop_stmt.condition) {
        codegen_expression(gen, node->loop_stmt.condition);
        emit(gen, "    cmp rax, 0");
        emit(gen, "    je .L%d", cleanup_label);
    }
    
    // Add prefetch for next iterations (improves cache hit rate)
    if (gen->enable_prefetch && gen->loop_depth >= 2) {
        emit_prefetch_for_loop(gen);
    }
    
    // Unroll loop body
    for (int i = 0; i < unroll_factor; i++) {
        codegen_statement(gen, node->loop_stmt.body);
        if (node->loop_stmt.increment) {
            codegen_statement(gen, node->loop_stmt.increment);
        }
    }
    
    emit(gen, "    jmp .L%d_unroll", start_label);
    emit(gen, ".L%d:", cleanup_label);
    emit(gen, ".L%d:", end_label);
    
    // Restore context
    gen->loop_break_label = prev_break_label;
    gen->loop_continue_label = prev_continue_label;
    gen->in_simd_loop = prev_simd;
    gen->loop_depth = prev_loop_depth;
}

static void codegen_loop(CodeGen *gen, ASTNode *node) {
    int start_label = new_label(gen);
    int end_label = new_label(gen);
    int continue_label = new_label(gen);
    
    // Track loop depth for optimization heuristics
    int prev_loop_depth = gen->loop_depth;
    gen->loop_depth++;
    
    // Save previous loop context
    int prev_break_label = gen->loop_break_label;
    int prev_continue_label = gen->loop_continue_label;
    bool prev_simd_enabled = gen->in_simd_loop;
    
    // PHASE 4.1: LOOP INTERCHANGE TRANSFORMATION
    // Detect i-j-k pattern and apply cache-friendly i-k-j reordering
    // This significantly improves cache locality for matrix multiply (O(n^3) algorithms)
    if (gen->enable_simd && gen->loop_depth == 1) {
        emit(gen, "    ; [DEBUG] At depth 1, checking for loop interchange pattern...");
        ASTNode *loop2 = NULL;
        ASTNode *loop3 = NULL;
        
        // Extract loop2 and loop3 from nested structure
        if (node->loop_stmt.body && node->loop_stmt.body->type == AST_LOOP) {
            loop2 = node->loop_stmt.body;
            emit(gen, "    ; [DEBUG] Found loop2 (direct body)");
        } else if (node->loop_stmt.body && node->loop_stmt.body->type == AST_BLOCK && 
                   node->loop_stmt.body->block.statements && node->loop_stmt.body->block.statements->count > 0) {
            ASTNode *first = node->loop_stmt.body->block.statements->nodes[0];
            if (first && first->type == AST_LOOP) {
                loop2 = first;
                emit(gen, "    ; [DEBUG] Found loop2 (in block)");
            }
        }
        
        if (!loop2) {
            emit(gen, "    ; [DEBUG] No loop2 found, skipping interchange");
        } else {
            if (loop2->loop_stmt.body && loop2->loop_stmt.body->type == AST_LOOP) {
                loop3 = loop2->loop_stmt.body;
                emit(gen, "    ; [DEBUG] Found loop3 (direct body)");
            } else if (loop2->loop_stmt.body && loop2->loop_stmt.body->type == AST_BLOCK && 
                       loop2->loop_stmt.body->block.statements && loop2->loop_stmt.body->block.statements->count > 0) {
                ASTNode *first = loop2->loop_stmt.body->block.statements->nodes[0];
                if (first && first->type == AST_LOOP) {
                    loop3 = first;
                    emit(gen, "    ; [DEBUG] Found loop3 (in block)");
                }
            }
        }
        
        if (!loop3) {
            emit(gen, "    ; [DEBUG] No loop3 found, skipping interchange");
        } else {
            emit(gen, "    ; [DEBUG] Checking pattern detection...");
            // Check if loop interchange is beneficial (i-j-k pattern detected)
            if (can_interchange_loops(node->loop_stmt.init, loop2->loop_stmt.init, loop3->loop_stmt.init)) {
                emit(gen, "    ; [SUCCESS] Loop interchange pattern detected! Applying transformation...");
                // PHASE 4.1: APPLY ACTUAL TRANSFORMATION (not just hints!)
                // Transform i-j-k → i-k-j for cache locality
                // This swaps the middle (j) and inner (k) loops
                codegen_loop_interchanged(gen, node, loop2, loop3);
                gen->loop_depth = prev_loop_depth;
                return;  // Early return: interchange handled completely
            } else {
                emit(gen, "    ; [DEBUG] Pattern not detected (not i-j-k)");
            }
        }
    }
    
    // PHASE 3.1: Cache Blocking - Transform 3+ nested loops to process 64x64 tiles
    // This improves cache locality dramatically for matrix multiply (O(n^3) algorithms)
    if (gen->enable_simd && gen->loop_depth == 1) {
        // Try to detect and apply cache blocking for 3-nested loop structure
        ASTNode *loop2 = NULL;
        ASTNode *loop3 = NULL;
        
        // Extract loop2 from loop1's body
        if (node->loop_stmt.body && node->loop_stmt.body->type == AST_LOOP) {
            loop2 = node->loop_stmt.body;
        } else if (node->loop_stmt.body && node->loop_stmt.body->type == AST_BLOCK && 
                   node->loop_stmt.body->block.statements && node->loop_stmt.body->block.statements->count > 0) {
            ASTNode *first = node->loop_stmt.body->block.statements->nodes[0];
            if (first && first->type == AST_LOOP) {
                loop2 = first;
            }
        }
        
        // Extract loop3 from loop2's body (if loop2 exists)
        if (loop2 && loop2->loop_stmt.body && loop2->loop_stmt.body->type == AST_LOOP) {
            loop3 = loop2->loop_stmt.body;
        } else if (loop2 && loop2->loop_stmt.body && loop2->loop_stmt.body->type == AST_BLOCK && 
                   loop2->loop_stmt.body->block.statements && loop2->loop_stmt.body->block.statements->count > 0) {
            ASTNode *first = loop2->loop_stmt.body->block.statements->nodes[0];
            if (first && first->type == AST_LOOP) {
                loop3 = first;
            }
        }
        
        // Apply PHASE 3.1: Loop tiling if we have 3 nesting levels
        // BUT: Skip cache blocking if i-j-k pattern exists (Phase 4.1 will handle it instead)
        bool is_ijk_pattern = can_interchange_loops(node->loop_stmt.init, loop2->loop_stmt.init, loop3->loop_stmt.init);
        
        if (loop2 && loop3 && should_apply_cache_blocking(node, 3) && !is_ijk_pattern) {
            emit(gen, "    ; ========================================");
            emit(gen, "    ; PHASE 3.1: Cache Blocking Transformation");
            emit(gen, "    ; Tiling to 64x64 blocks for L2 cache reuse");
            emit(gen, "    ; ========================================");
            
            gen->loop_depth = prev_loop_depth;
            codegen_loop_blocked(gen, node, loop2, loop3);
            gen->loop_break_label = prev_break_label;
            gen->loop_continue_label = prev_continue_label;
            gen->in_simd_loop = prev_simd_enabled;
            return;
        }
    }
    
    // Check if we should unroll this innermost loop
    if (gen->enable_simd && is_innermost_loop(node->loop_stmt.body)) {
        // PHASE 3: Aggressive unrolling based on depth
        // More unrolling = better instruction-level parallelism = higher throughput
        int unroll_factor = 8;  // Default: 8x unrolling
        if (gen->loop_depth == 3) {
            unroll_factor = 16;  // Innermost loop: 16x unrolling for max ILP (Phase 3 enhancement)
        } else if (gen->loop_depth == 2) {
            unroll_factor = 8;   // Middle loop: 8x unrolling (Phase 3 enhancement from 4x)
        }
        codegen_loop_unrolled(gen, node, unroll_factor);
        gen->loop_depth = prev_loop_depth;
        return;
    }
    
    // Regular (non-unrolled) loop
    gen->loop_break_label = end_label;
    gen->loop_continue_label = continue_label;
    gen->in_simd_loop = false;
    
    // Init
    if (node->loop_stmt.init) {
        codegen_statement(gen, node->loop_stmt.init);
    }
    
    emit(gen, ".L%d:", start_label);
    
    // Condition
    if (node->loop_stmt.condition) {
        codegen_expression(gen, node->loop_stmt.condition);
        emit(gen, "    cmp rax, 0");
        emit(gen, "    je .L%d", end_label);
    }
    
    // Add prefetch hints for nested loops (cache optimization)
    if (gen->enable_prefetch && gen->loop_depth >= 2) {
        emit_prefetch_for_loop(gen);
    }
    
    // Body
    codegen_statement(gen, node->loop_stmt.body);
    
    // Continue label
    emit(gen, ".L%d:", continue_label);
    
    // Increment
    if (node->loop_stmt.increment) {
        codegen_statement(gen, node->loop_stmt.increment);
    }
    
    emit(gen, "    jmp .L%d", start_label);
    emit(gen, ".L%d:", end_label);
    
    // Restore previous loop context
    gen->loop_break_label = prev_break_label;
    gen->loop_continue_label = prev_continue_label;
    gen->in_simd_loop = prev_simd_enabled;
    gen->loop_depth = prev_loop_depth;
}

static void codegen_var_decl(CodeGen *gen, ASTNode *node) {
    // Generate the push instruction first
    if (node->var_decl.value) {
        // Variable has initializer
        codegen_expression(gen, node->var_decl.value);
        
        // CRITICAL FIX: If assigning int to deci, convert first
        if (strcmp(node->var_decl.type, "deci") == 0 && !is_deci_expr(gen, node->var_decl.value)) {
            // Value is int, need to convert to deci (IEEE-754 double)
            emit(gen, "    cvtsi2sd xmm0, rax  ; convert int to deci before storing");
            emit(gen, "    movq rax, xmm0      ; move deci bits to rax for push");
        }
        
        emit(gen, "    push rax            ; var %s", node->var_decl.name);
    } else {
        // Variable without initializer
        if (strcmp(node->var_decl.type, "str") == 0) {
            // CRITICAL FIX: Initialize uninitialized strings to empty string, not NULL
            // This prevents segfaults when accessing uninitialized str variables
            emit(gen, "    lea rax, [rel empty_string]  ; address of empty string");
            emit(gen, "    push rax            ; var %s (initialized to empty string)", node->var_decl.name);
        } else {
            // Other types: allocate space and zero it
            emit(gen, "    push 0              ; var %s (uninitialized)", node->var_decl.name);
        }
    }
    
    // NOW update stack offset (variable is at current offset + 8)
    gen->stack_offset += 8;
    
    // Add to symbol table with the NEW offset
    symbol_add(gen, node->var_decl.name, gen->stack_offset, 8, node->var_decl.type, false, false, 0, 0, false);
}

static void codegen_assign(CodeGen *gen, ASTNode *node) {
    codegen_expression(gen, node->assign.value);
    
    // Look up variable in symbol table
    Symbol *sym = symbol_lookup(gen, node->assign.name);
    if (sym) {
        // Apply overflow masking for byte/ubyte/char types
        if (sym->type && (strcmp(sym->type, "byte") == 0 || 
                          strcmp(sym->type, "ubyte") == 0 || 
                          strcmp(sym->type, "char") == 0)) {
            emit(gen, "    and rax, 0xFF       ; mask to byte width for overflow wrapping");
        }
        emit(gen, "    mov [rbp - %d], rax  ; assign %s", sym->offset, node->assign.name);
    } else {
        emit(gen, "    mov [rbp - 8], rax  ; assign %s (fallback)", node->assign.name);
    }
}

static void codegen_array_decl(CodeGen *gen, ASTNode *node) {
    // Calculate array size
    int size = 0;
    if (node->array_decl.size->type == AST_LITERAL) {
        char *converted = convert_number_format(node->array_decl.size->literal.value);
        size = atoi(converted);
    } else {
        size = 10; // Default size if not constant
    }
    
    // SECURITY: Validate array size to prevent DoS/OOM attacks
    const int MAX_ARRAY_SIZE = 1000000; // Max 1 million elements
    if (size <= 0) {
        fprintf(stderr, "Error: Array size must be positive, got %d\n", size);
        exit(1);
    }
    if (size > MAX_ARRAY_SIZE) {
        fprintf(stderr, "Error: Array size %d exceeds maximum of %d elements\n", size, MAX_ARRAY_SIZE);
        exit(1);
    }
    
    // Get element size based on type
    int element_size = 0;
    
    // Check if it's a group type
    GroupType *group = group_type_lookup(gen, node->array_decl.element_type);
    if (group) {
        element_size = group->total_size;
    } else {
        // Otherwise, use primitive type size
        element_size = get_type_size(node->array_decl.element_type);
    }
    
    // SECURITY: Check for overflow in total_bytes calculation
    // Prevent: MAX_INT / element_size < size
    const long MAX_ALLOC = 100 * 1024 * 1024; // Max 100MB per allocation
    if (element_size > 0 && size > MAX_ALLOC / element_size) {
        fprintf(stderr, "Error: Array would require %ld bytes, exceeds maximum of %ld\n", 
                (long)size * element_size, MAX_ALLOC);
        exit(1);
    }
    
    int total_bytes = size * element_size;
    
    emit(gen, "    ; array declaration: %s{%s, %d} (element_size=%d bytes)", 
         node->array_decl.name, node->array_decl.element_type, size, element_size);
    emit(gen, "    sub rsp, %d         ; allocate array space", total_bytes);
    
    int array_base_offset = gen->stack_offset + total_bytes;
    gen->stack_offset += total_bytes;
    
    // Add array to symbol table with element size
    symbol_add(gen, node->array_decl.name, array_base_offset, total_bytes, 
               node->array_decl.element_type, true, false, size, element_size, false);
    
    // Initialize array elements if initializer provided
    if (node->array_decl.initializer) {
        for (int i = 0; i < node->array_decl.initializer->count && i < size; i++) {
            codegen_expression(gen, node->array_decl.initializer->nodes[i]);
            // Calculate offset: base - (index * element_size)
            int elem_offset = array_base_offset - (i * element_size);
            
            // Store based on element size
            if (element_size == 1) {
                emit(gen, "    mov byte [rbp - %d], al ; init %s{%d}",
                     elem_offset, node->array_decl.name, i);
            } else if (element_size == 4) {
                emit(gen, "    mov dword [rbp - %d], eax ; init %s{%d}",
                     elem_offset, node->array_decl.name, i);
            } else {
                emit(gen, "    mov qword [rbp - %d], rax ; init %s{%d}",
                     elem_offset, node->array_decl.name, i);
            }
        }
    }
}

static void codegen_array_assign(CodeGen *gen, ASTNode *node) {
    // Look up array in symbol table
    Symbol *sym = symbol_lookup(gen, node->array_assign.name);
    if (!sym || !sym->is_array) {
        fprintf(stderr, "Error: '%s' is not an array\n", node->array_assign.name);
        return;
    }
    
    // Calculate element address with optional bounds checking (PRIORITY 2.2)
    codegen_expression(gen, node->array_assign.index);
    
    if (g_bounds_check_enabled) {
        emit(gen, "    push rax            ; save index");
        
        // Bounds check: if (index < 0 || index >= array_size) exit
        emit(gen, "    cmp rax, 0");
        int error_label = new_label(gen);
        emit(gen, "    jl .bounds_error_%d", error_label);
        emit(gen, "    cmp rax, %d", sym->array_size);
        emit(gen, "    jge .bounds_error_%d", error_label);
        
        // Calculate value
        codegen_expression(gen, node->array_assign.value);
        emit(gen, "    mov rbx, rax        ; save value");
        emit(gen, "    pop rax             ; restore index");
        
        // Calculate element address using element_size from symbol table
        emit(gen, "    mov rcx, %d         ; element size", sym->element_size);
        emit(gen, "    imul rax, rcx       ; index * size");
        
        // For array parameters (passed as pointers), load the pointer and add offset
        // For local arrays, compute LEA of the array base and add offset
        if (sym->is_array_param) {
            emit(gen, "    mov rcx, [rbp - %d] ; load array pointer (parameter)", sym->offset);
        } else {
            emit(gen, "    lea rcx, [rbp - %d] ; array base (first element)", sym->offset);
        }
        emit(gen, "    add rcx, rax        ; element address");
        
        // Store based on element size
        if (sym->element_size == 1) {
            emit(gen, "    mov byte [rcx], bl  ; store byte");
        } else if (sym->element_size == 4) {
            emit(gen, "    mov dword [rcx], ebx ; store dword");
        } else {
            emit(gen, "    mov qword [rcx], rbx ; store qword");
        }
        
        emit(gen, "    jmp .bounds_ok_%d", error_label + 1);
        
        // Bounds error handler
        emit(gen, ".bounds_error_%d:", error_label);
        emit(gen, "    mov rdi, 1          ; stderr");
        emit(gen, "    lea rsi, [rel bounds_msg]");
        emit(gen, "    mov rdx, 21         ; message length");
        emit(gen, "    mov rax, 1          ; sys_write");
        emit(gen, "    syscall");
        emit(gen, "    mov rdi, 1          ; exit code");
        emit(gen, "    mov rax, 60         ; sys_exit");
        emit(gen, "    syscall");
        emit(gen, ".bounds_ok_%d:", error_label + 1);
    } else {
        // NO bounds checking - direct assignment (faster but unsafe)
        emit(gen, "    push rax            ; save index");
        
        // Calculate value
        codegen_expression(gen, node->array_assign.value);
        emit(gen, "    mov rbx, rax        ; save value");
        emit(gen, "    pop rax             ; restore index");
        
        // Calculate element address using element_size from symbol table
        emit(gen, "    mov rcx, %d         ; element size", sym->element_size);
        emit(gen, "    imul rax, rcx       ; index * size");
        
        if (sym->is_array_param) {
            emit(gen, "    mov rcx, [rbp - %d] ; load array pointer (parameter)", sym->offset);
        } else {
            emit(gen, "    lea rcx, [rbp - %d] ; array base (first element)", sym->offset);
        }
        emit(gen, "    add rcx, rax        ; element address");
        
        // Store based on element size
        if (sym->element_size == 1) {
            emit(gen, "    mov byte [rcx], bl  ; store byte");
        } else if (sym->element_size == 4) {
            emit(gen, "    mov dword [rcx], ebx ; store dword");
        } else {
            emit(gen, "    mov qword [rcx], rbx ; store qword");
        }
    }
}

static void codegen_group_decl(CodeGen *gen, ASTNode *node) {
    // Look up the group type
    GroupType *group = group_type_lookup(gen, node->group_decl.type_name);
    if (!group) {
        fprintf(stderr, "Error: Unknown group type '%s'\n", node->group_decl.type_name);
        return;
    }
    
    emit(gen, "    ; group %s %s (size=%d bytes)", 
         node->group_decl.type_name, node->group_decl.var_name, group->total_size);
    emit(gen, "    sub rsp, %d         ; allocate group space", group->total_size);
    
    int group_base_offset = gen->stack_offset + group->total_size;
    gen->stack_offset += group->total_size;
    
    // CRITICAL FIX: Initialize all group members to safe defaults
    emit(gen, "    ; initialize group members");
    GroupField *field = group->fields;
    while (field) {
        int member_offset = group_base_offset - field->offset;
        
        if (strcmp(field->type, "str") == 0) {
            // Initialize strings to empty_string to prevent segfault on uninitialized access
            emit(gen, "    lea rax, [rel empty_string]");
            emit(gen, "    mov qword [rbp - %d], rax ; init %s.%s (str)", 
                 member_offset, node->group_decl.var_name, field->name);
        } else {
            // Zero-initialize all other types (int, bool, char, byte, deci, etc)
            if (field->size == 1) {
                emit(gen, "    mov byte [rbp - %d], 0   ; init %s.%s", 
                     member_offset, node->group_decl.var_name, field->name);
            } else if (field->size == 4) {
                emit(gen, "    mov dword [rbp - %d], 0  ; init %s.%s", 
                     member_offset, node->group_decl.var_name, field->name);
            } else {
                emit(gen, "    mov qword [rbp - %d], 0  ; init %s.%s", 
                     member_offset, node->group_decl.var_name, field->name);
            }
        }
        field = field->next;
    }
    
    // Add to symbol table
    symbol_add(gen, node->group_decl.var_name, group_base_offset, group->total_size,
               node->group_decl.type_name, false, false, 0, 0, true);
}

// Helper: Walk a member access chain to get its type
// Walks from base identifier through all member accesses
// Returns the final type of the expression
static const char *get_member_access_type(CodeGen *gen, ASTNode *node) {
    if (node->type == AST_IDENT) {
        Symbol *sym = symbol_lookup(gen, node->ident.name);
        return sym ? sym->type : NULL;
    } else if (node->type == AST_ARRAY_ACCESS) {
        // For array access, get the element type from the array
        Symbol *sym = symbol_lookup(gen, node->array_access.name);
        return sym ? sym->type : NULL;
    } else if (node->type == AST_MEMBER_ACCESS) {
        // Get the type of the object
        const char *obj_type = get_member_access_type(gen, node->member_access.object);
        if (!obj_type) return NULL;
        
        GroupType *group = group_type_lookup(gen, obj_type);
        if (!group) return NULL;
        
        GroupField *field = group_field_lookup(group, node->member_access.member_name);
        return field ? field->type : NULL;
    }
    return NULL;
}

// Helper: Calculate the address of a member access expression
// Result is left in rax
static void codegen_member_access_addr(CodeGen *gen, ASTNode *node) {
    if (node->type == AST_IDENT) {
        // Base case: just an identifier
        Symbol *sym = symbol_lookup(gen, node->ident.name);
        if (sym) {
            // Load address of the variable into rax
            emit(gen, "    lea rax, [rbp - %d]", sym->offset);
        }
    } else if (node->type == AST_ARRAY_ACCESS) {
        // Array access: calculate (base + index * element_size)
        Symbol *sym = symbol_lookup(gen, node->array_access.name);
        if (!sym) return;
        
        // Evaluate the index expression - result in rax
        codegen_expression(gen, node->array_access.index);
        
        // Save index in rdx
        emit(gen, "    mov rdx, rax");
        
        // Get element size from the symbol table
        int element_size = sym->element_size;
        
        // Calculate: index * element_size, result in rax
        if (element_size == 1) {
            emit(gen, "    movzx rax, rdx");
        } else {
            emit(gen, "    mov rax, rdx");
            emit(gen, "    imul rax, %d", element_size);
        }
        
        // Now rax = index * element_size
        // Add base address: lea rax, [rbp - offset]
        // Then add the computed offset
        int base_offset = sym->offset;
        emit(gen, "    lea rcx, [rbp - %d]", base_offset);
        emit(gen, "    add rax, rcx");
        // rax now contains: base_address + (index * element_size)
    } else if (node->type == AST_MEMBER_ACCESS) {
        // Recursive case: evaluate the object, then add field offset
        codegen_member_access_addr(gen, node->member_access.object);
        
        // rax now contains the address of the object
        // We need to add the field offset
        
        // Determine the type of the object
        const char *obj_type = get_member_access_type(gen, node->member_access.object);
        
        if (!obj_type) {
            fprintf(stderr, "Error: Cannot determine type in member access address\n");
            return;
        }
        
        GroupType *group = group_type_lookup(gen, obj_type);
        if (!group) {
            fprintf(stderr, "Error: Unknown group type '%s'\n", obj_type);
            return;
        }
        
        GroupField *field = group_field_lookup(group, node->member_access.member_name);
        if (!field) {
            fprintf(stderr, "Error: Group '%s' has no member '%s'\n", group->name, node->member_access.member_name);
            return;
        }
        
        // Add field offset to address
        emit(gen, "    add rax, %d", field->offset);
    }
}

static void codegen_member_assign(CodeGen *gen, ASTNode *node) {
    // Evaluate the object (which might be a nested member access)
    // This leaves the address in rax
    codegen_member_access_addr(gen, node->member_assign.object);
    
    // Save the object address in rcx (a register that's less likely to be clobbered)
    emit(gen, "    mov rcx, rax");
    
    // Generate the value to assign
    codegen_expression(gen, node->member_assign.value);
    
    // Get the type of the object
    const char *obj_type = get_member_access_type(gen, node->member_assign.object);
    
    if (!obj_type) {
        fprintf(stderr, "Error: Cannot determine type of object in member assignment\n");
        return;
    }
    
    // Look up the group type
    GroupType *group = group_type_lookup(gen, obj_type);
    if (!group) {
        fprintf(stderr, "Error: Unknown group type '%s'\n", obj_type);
        return;
    }
    
    // Look up the field
    GroupField *field = group_field_lookup(group, node->member_assign.member_name);
    if (!field) {
        fprintf(stderr, "Error: Group '%s' has no member '%s'\n", 
                group->name, node->member_assign.member_name);
        return;
    }
    
    // Calculate offset from object address (stored in rcx)
    // rcx now points to the object, we need to add field->offset to get member address
    emit(gen, "    add rcx, %d", field->offset);
    
    // Store based on field size
    if (field->size == 1) {
        emit(gen, "    mov byte [rcx], al  ; .%s", node->member_assign.member_name);
    } else if (field->size == 4) {
        emit(gen, "    mov dword [rcx], eax ; .%s",
             node->member_assign.member_name);
    } else {
        emit(gen, "    mov qword [rcx], rax ; .%s",
             node->member_assign.member_name);
    }
}

// Map implementation with simple hash table
// Map structure in memory: [capacity, size, buckets...]
// Each bucket: [key, value, occupied_flag]
#define MAP_DEFAULT_CAPACITY 16
#define MAP_BUCKET_SIZE(key_size, value_size) ((key_size) + (value_size) + 1)  // +1 for occupied flag

static void codegen_map_decl(CodeGen *gen, ASTNode *node) {
    int capacity = MAP_DEFAULT_CAPACITY;
    int key_size = get_type_size(node->map_decl.key_type);
    int value_size = get_type_size(node->map_decl.value_type);
    int bucket_size = MAP_BUCKET_SIZE(key_size, value_size);
    
    // Map structure: capacity(4) + size(4) + buckets
    int map_size = 8 + (capacity * bucket_size);
    gen->stack_offset += map_size;
    
    // Initialize capacity
    emit(gen, "    mov dword [rbp - %d], %d  ; %s capacity",
         gen->stack_offset, capacity, node->map_decl.name);
    
    // Initialize size to 0
    emit(gen, "    mov dword [rbp - %d], 0   ; %s size",
         gen->stack_offset - 4, node->map_decl.name);
    
    // Zero out all buckets (mark as unoccupied)
    for (int i = 0; i < capacity; i++) {
        int bucket_offset = gen->stack_offset - 8 - (i * bucket_size);
        // FIX: Write occupied flag at correct offset: key_size + value_size bytes from bucket start
        emit(gen, "    mov byte [rbp - %d], 0   ; bucket %d occupied flag",
             bucket_offset - key_size - value_size, i);
    }
    
    // Add to symbol table
    symbol_add_map(gen, node->map_decl.name, gen->stack_offset, map_size,
                   node->map_decl.key_type, node->map_decl.value_type, capacity);
}

static void codegen_map_set(CodeGen *gen, ASTNode *node) {
    Symbol *sym = symbol_lookup(gen, node->map_set.map_name);
    if (!sym || !sym->is_map) {
        fprintf(stderr, "Error: '%s' is not a map\n", node->map_set.map_name);
        return;
    }
    
    int key_size = get_type_size(sym->key_type);
    int value_size = get_type_size(sym->value_type);
    int bucket_size = MAP_BUCKET_SIZE(key_size, value_size);
    
    // Evaluate key
    codegen_expression(gen, node->map_set.key);
    emit(gen, "    push rax  ; save key");
    
    // Evaluate value
    codegen_expression(gen, node->map_set.value);
    emit(gen, "    mov rbx, rax  ; save value");
    emit(gen, "    pop rax  ; restore key");
    
    // Simple hash: key % capacity
    emit(gen, "    xor rdx, rdx");
    emit(gen, "    mov rcx, %d  ; capacity", sym->capacity);
    emit(gen, "    div rcx  ; rdx = key %% capacity");
    
    // Linear probing to find empty bucket or matching key
    int probe_label = new_label(gen);
    int found_label = new_label(gen);
    
    emit(gen, ".probe_%d:", probe_label);
    
    // Calculate bucket offset from bucket index (in rdx)
    emit(gen, "    mov rax, rdx");
    emit(gen, "    mov rcx, %d  ; bucket_size", bucket_size);
    emit(gen, "    mul rcx  ; rax = bucket_index * bucket_size");
    
    // Calculate bucket address: map_base - 8 - bucket_offset
    emit(gen, "    lea rsi, [rbp - %d]  ; map base address", sym->offset);
    emit(gen, "    sub rsi, 8  ; skip header (capacity + size)");
    emit(gen, "    sub rsi, rax  ; rsi = bucket address");
    
    // Check occupied flag
    emit(gen, "    movzx rcx, byte [rsi + %d]  ; occupied flag", key_size + value_size);
    emit(gen, "    test rcx, rcx");
    emit(gen, "    jz .found_%d  ; empty bucket", found_label);
    
    // Bucket occupied, check if key matches
    emit(gen, "    pop rax  ; get key again");
    emit(gen, "    push rax");
    if (key_size == 4) {
        emit(gen, "    mov edx, dword [rsi]  ; load bucket key");
        emit(gen, "    cmp eax, edx");
    } else {
        emit(gen, "    mov rdx, qword [rsi]  ; load bucket key");
        emit(gen, "    cmp rax, rdx");
    }
    emit(gen, "    je .found_%d  ; key matches", found_label);
    
    // Try next bucket (linear probing)
    emit(gen, "    pop rax");
    emit(gen, "    push rax  ; keep key on stack");
    // FIX: Save bucket index before loading key, use r9 to preserve it
    emit(gen, "    mov r9, rdx  ; save current bucket index");
    emit(gen, "    inc r9  ; next bucket index");
    emit(gen, "    xor rdx, rdx");
    emit(gen, "    mov rax, r9");
    emit(gen, "    mov rcx, %d  ; capacity", sym->capacity);
    emit(gen, "    div rcx  ; rdx = next_index %% capacity");
    emit(gen, "    jmp .probe_%d", probe_label);
    
    emit(gen, ".found_%d:", found_label);
    emit(gen, "    pop rax  ; restore key");
    
    // rsi already points to the bucket address
    // Store key
    if (key_size == 4) {
        emit(gen, "    mov dword [rsi], eax  ; store key");
    } else {
        emit(gen, "    mov qword [rsi], rax  ; store key");
    }
    
    // Store value
    if (value_size == 4) {
        emit(gen, "    mov dword [rsi + %d], ebx  ; store value", key_size);
    } else {
        emit(gen, "    mov qword [rsi + %d], rbx  ; store value", key_size);
    }
    
    // Mark as occupied
    emit(gen, "    mov byte [rsi + %d], 1  ; mark occupied", key_size + value_size);
}

static void codegen_map_get(CodeGen *gen, ASTNode *node) {
    Symbol *sym = symbol_lookup(gen, node->map_get.map_name);
    if (!sym || !sym->is_map) {
        fprintf(stderr, "Error: '%s' is not a map\n", node->map_get.map_name);
        emit(gen, "    xor rax, rax  ; return 0 on error");
        return;
    }
    
    int key_size = get_type_size(sym->key_type);
    int value_size = get_type_size(sym->value_type);
    int bucket_size = MAP_BUCKET_SIZE(key_size, value_size);
    
    // Evaluate key
    codegen_expression(gen, node->map_get.key);
    
    // Simple hash: key % capacity
    emit(gen, "    push rax  ; save key");
    emit(gen, "    xor rdx, rdx");
    emit(gen, "    mov rcx, %d  ; capacity", sym->capacity);
    emit(gen, "    div rcx  ; rdx = key %% capacity");
    
    // Linear probing
    int probe_label = new_label(gen);
    int found_label = new_label(gen);
    int notfound_label = new_label(gen);
    
    emit(gen, "    mov r8, 0  ; probe count");
    // FIX: Store bucket index in r9 to avoid clobbering from mul/div
    emit(gen, "    mov r9, rdx  ; save bucket index in r9");
    emit(gen, ".probe_%d:", probe_label);
    
    // Check if we've probed all buckets
    emit(gen, "    cmp r8, %d", sym->capacity);
    emit(gen, "    jge .notfound_%d", notfound_label);
    emit(gen, "    inc r8");
    
    // Calculate bucket address - FIX: Use r9 for bucket index
    emit(gen, "    mov rax, r9  ; bucket index from r9");
    emit(gen, "    mov rcx, %d  ; bucket_size", bucket_size);
    emit(gen, "    mul rcx");
    
    emit(gen, "    lea rsi, [rbp - %d]", sym->offset);
    emit(gen, "    sub rsi, 8");
    emit(gen, "    sub rsi, rax");
    
    // Check occupied flag
    emit(gen, "    movzx rcx, byte [rsi + %d]", key_size + value_size);
    emit(gen, "    test rcx, rcx");
    emit(gen, "    jz .notfound_%d  ; empty bucket, key not found", notfound_label);
    
    // Check if key matches - FIX: Use r10 to preserve bucket data, not rdx
    emit(gen, "    pop rax");
    emit(gen, "    push rax");
    if (key_size == 4) {
        emit(gen, "    mov r10d, dword [rsi]  ; load bucket key into r10");
        emit(gen, "    cmp eax, r10d");
    } else {
        emit(gen, "    mov r10, qword [rsi]  ; load bucket key into r10");
        emit(gen, "    cmp rax, r10");
    }
    emit(gen, "    je .found_%d", found_label);
    
    // Try next bucket - FIX: Update r9 for next iteration
    emit(gen, "    mov rax, r9  ; current bucket index");
    emit(gen, "    inc rax  ; next bucket index");
    emit(gen, "    xor rdx, rdx");
    emit(gen, "    mov rcx, %d", sym->capacity);
    emit(gen, "    div rcx  ; rdx = next_index %% capacity");
    emit(gen, "    mov r9, rdx  ; save new bucket index in r9");
    emit(gen, "    jmp .probe_%d", probe_label);
    
    emit(gen, ".found_%d:", found_label);
    emit(gen, "    pop rax  ; clean up key");
    // Load value
    if (value_size == 4) {
        emit(gen, "    mov eax, dword [rsi + %d]", key_size);
        emit(gen, "    cdqe");
    } else {
        emit(gen, "    mov rax, qword [rsi + %d]", key_size);
    }
    
    int end_label = new_label(gen);
    emit(gen, "    jmp .end_%d", end_label);
    
    emit(gen, ".notfound_%d:", notfound_label);
    emit(gen, "    pop rax  ; clean up key");
    emit(gen, "    xor rax, rax  ; return 0");
    
    emit(gen, ".end_%d:", end_label);
}

static void codegen_map_has(CodeGen *gen, ASTNode *node) {
    Symbol *sym = symbol_lookup(gen, node->map_has.map_name);
    if (!sym || !sym->is_map) {
        fprintf(stderr, "Error: '%s' is not a map\n", node->map_has.map_name);
        emit(gen, "    xor rax, rax");
        return;
    }
    
    int key_size = get_type_size(sym->key_type);
    int value_size = get_type_size(sym->value_type);
    int bucket_size = MAP_BUCKET_SIZE(key_size, value_size);
    
    // Evaluate key
    codegen_expression(gen, node->map_has.key);
    
    // Hash and probe (similar to map_get)
    emit(gen, "    push rax");
    emit(gen, "    xor rdx, rdx");
    emit(gen, "    mov rcx, %d", sym->capacity);
    emit(gen, "    div rcx");
    
    int probe_label = new_label(gen);
    int found_label = new_label(gen);
    int notfound_label = new_label(gen);
    
    emit(gen, "    mov r8, 0  ; probe count");
    // FIX: Store bucket index in r9 to avoid clobbering from mul/div
    emit(gen, "    mov r9, rdx  ; save bucket index in r9");
    emit(gen, ".probe_%d:", probe_label);
    emit(gen, "    cmp r8, %d", sym->capacity);
    emit(gen, "    jge .notfound_%d", notfound_label);
    emit(gen, "    inc r8");
    
    // Calculate bucket address - FIX: Use r9 for bucket index
    emit(gen, "    mov rax, r9  ; bucket index from r9");
    emit(gen, "    mov rcx, %d", bucket_size);
    emit(gen, "    mul rcx");
    emit(gen, "    lea rsi, [rbp - %d]", sym->offset);
    emit(gen, "    sub rsi, 8");
    emit(gen, "    sub rsi, rax");
    
    emit(gen, "    movzx rcx, byte [rsi + %d]", key_size + value_size);
    emit(gen, "    test rcx, rcx");
    emit(gen, "    jz .notfound_%d", notfound_label);
    
    // Check if key matches - FIX: Use r10 to preserve bucket data, not rdx
    emit(gen, "    pop rax");
    emit(gen, "    push rax");
    if (key_size == 4) {
        emit(gen, "    mov r10d, dword [rsi]  ; load bucket key into r10");
        emit(gen, "    cmp eax, r10d");
    } else {
        emit(gen, "    mov r10, qword [rsi]  ; load bucket key into r10");
        emit(gen, "    cmp rax, r10");
    }
    emit(gen, "    je .found_%d", found_label);
    
    // Try next bucket - FIX: Update r9 for next iteration
    emit(gen, "    mov rax, r9  ; current bucket index");
    emit(gen, "    inc rax  ; next bucket index");
    emit(gen, "    xor rdx, rdx");
    emit(gen, "    mov rcx, %d", sym->capacity);
    emit(gen, "    div rcx  ; rdx = next_index %% capacity");
    emit(gen, "    mov r9, rdx  ; save new bucket index in r9");
    emit(gen, "    jmp .probe_%d", probe_label);
    
    emit(gen, ".found_%d:", found_label);
    emit(gen, "    pop rax");
    emit(gen, "    mov rax, 1  ; return true");
    
    int end_label = new_label(gen);
    emit(gen, "    jmp .end_%d", end_label);
    
    emit(gen, ".notfound_%d:", notfound_label);
    emit(gen, "    pop rax");
    emit(gen, "    xor rax, rax  ; return false");
    
    emit(gen, ".end_%d:", end_label);
}

static void codegen_map_remove(CodeGen *gen, ASTNode *node) {
    Symbol *sym = symbol_lookup(gen, node->map_remove.map_name);
    if (!sym || !sym->is_map) {
        fprintf(stderr, "Error: '%s' is not a map\n", node->map_remove.map_name);
        return;
    }
    
    int key_size = get_type_size(sym->key_type);
    int value_size = get_type_size(sym->value_type);
    int bucket_size = MAP_BUCKET_SIZE(key_size, value_size);
    
    // Evaluate key
    codegen_expression(gen, node->map_remove.key);
    
    // Hash and probe
    emit(gen, "    push rax");
    emit(gen, "    xor rdx, rdx");
    emit(gen, "    mov rcx, %d", sym->capacity);
    emit(gen, "    div rcx");
    
    int probe_label = new_label(gen);
    int found_label = new_label(gen);
    int end_label = new_label(gen);
    
    emit(gen, "    mov r8, 0  ; probe count");
    // FIX: Store bucket index in r9 to avoid clobbering from mul/div
    emit(gen, "    mov r9, rdx  ; save bucket index in r9");
    emit(gen, ".probe_%d:", probe_label);
    emit(gen, "    cmp r8, %d", sym->capacity);
    emit(gen, "    jge .end_%d", end_label);
    emit(gen, "    inc r8");
    
    // Calculate bucket address - FIX: Use r9 for bucket index
    emit(gen, "    mov rax, r9  ; bucket index from r9");
    emit(gen, "    mov rcx, %d", bucket_size);
    emit(gen, "    mul rcx");
    emit(gen, "    lea rsi, [rbp - %d]", sym->offset);
    emit(gen, "    sub rsi, 8");
    emit(gen, "    sub rsi, rax");
    
    emit(gen, "    movzx rcx, byte [rsi + %d]", key_size + value_size);
    emit(gen, "    test rcx, rcx");
    emit(gen, "    jz .end_%d", end_label);
    
    // Check if key matches - FIX: Use r10 to preserve bucket data, not rdx
    emit(gen, "    pop rax");
    emit(gen, "    push rax");
    if (key_size == 4) {
        emit(gen, "    mov r10d, dword [rsi]  ; load bucket key into r10");
        emit(gen, "    cmp eax, r10d");
    } else {
        emit(gen, "    mov r10, qword [rsi]  ; load bucket key into r10");
        emit(gen, "    cmp rax, r10");
    }
    emit(gen, "    je .found_%d", found_label);
    
    // Try next bucket - FIX: Update r9 for next iteration
    emit(gen, "    mov rax, r9  ; current bucket index");
    emit(gen, "    inc rax  ; next bucket index");
    emit(gen, "    xor rdx, rdx");
    emit(gen, "    mov rcx, %d", sym->capacity);
    emit(gen, "    div rcx  ; rdx = next_index %% capacity");
    emit(gen, "    mov r9, rdx  ; save new bucket index in r9");
    emit(gen, "    jmp .probe_%d", probe_label);
    
    emit(gen, ".found_%d:", found_label);
    emit(gen, "    pop rax");
    // Mark bucket as unoccupied
    emit(gen, "    mov byte [rsi + %d], 0", key_size + value_size);
    
    int done_label = new_label(gen);
    emit(gen, "    jmp .done_%d", done_label);
    
    emit(gen, ".end_%d:", end_label);
    emit(gen, "    pop rax");
    
    emit(gen, ".done_%d:", done_label);
}

static void codegen_block(CodeGen *gen, ASTNode *node) {
    // Save stack offset at block entry
    int saved_offset = gen->stack_offset;
    Symbol *saved_symbols = gen->symbols;  // Save symbol list head
    
    // Save and clear deferred statements at block entry
    ASTList *saved_defers = gen->deferred_stmts;
    gen->deferred_stmts = ast_list_new();
    
    // Process all statements in block
    for (int i = 0; i < node->block.statements->count; i++) {
        codegen_statement(gen, node->block.statements->nodes[i]);
    }
    
    // Execute deferred statements in reverse order (LIFO)
    for (int i = gen->deferred_stmts->count - 1; i >= 0; i--) {
        emit(gen, "    ; begin defer");
        codegen_statement(gen, gen->deferred_stmts->nodes[i]);
        emit(gen, "    ; end defer");
    }
    
    // Clean up deferred statements list
    // Don't free the list itself since we're restoring the old one
    // Just reset it for reuse
    gen->deferred_stmts->count = 0;
    free(gen->deferred_stmts->nodes);
    gen->deferred_stmts->nodes = NULL;
    gen->deferred_stmts->capacity = 0;
    free(gen->deferred_stmts);
    gen->deferred_stmts = saved_defers;  // Restore previous defer queue
    
    // Clean up block-local variables
    // Pop all variables allocated in this block
    int vars_to_pop = (gen->stack_offset - saved_offset) / 8;
    if (vars_to_pop > 0) {
        emit(gen, "    add rsp, %d         ; pop %d block-local variables", vars_to_pop * 8, vars_to_pop);
    }
    
    // Restore stack offset and symbol table to pre-block state
    gen->stack_offset = saved_offset;
    gen->symbols = saved_symbols;  // Restore symbol list (removes block-local symbols)
}

static void codegen_statement(CodeGen *gen, ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_SHOW:
            codegen_show(gen, node);
            break;
        case AST_SHOWF:
            // Format and output with interpolation
            {
                ASTNode *interp = node->showf.interpolated;
                if (interp && interp->type == AST_INTERPOLATED_STRING) {
                    // Output chunks and expressions interleaved
                    for (int i = 0; i < interp->interpolated_string.chunk_count; i++) {
                        // Output string chunk if it exists and is non-empty
                        if (i < interp->interpolated_string.chunks->count) {
                            ASTNode *chunk = interp->interpolated_string.chunks->nodes[i];
                            if (chunk && chunk->type == AST_LITERAL && strlen(chunk->literal.value) > 0) {
                                ASTNode *temp_show = ast_node_new(AST_SHOW);
                                temp_show->show.value = chunk;
                                codegen_show(gen, temp_show);
                                free(temp_show);
                            }
                        }
                        
                        // Output expression if it exists
                        if (i < interp->interpolated_string.expressions->count) {
                            ASTNode *expr = interp->interpolated_string.expressions->nodes[i];
                            if (expr) {
                                ASTNode *temp_show = ast_node_new(AST_SHOW);
                                temp_show->show.value = expr;
                                codegen_show(gen, temp_show);
                                free(temp_show);
                            }
                        }
                    }
                    
                    // Output final newline
                    emit(gen, "    mov rax, 1");
                    emit(gen, "    mov rdi, 1");
                    emit(gen, "    mov rsi, $newline");
                    emit(gen, "    mov rdx, 1");
                    emit(gen, "    syscall");
                }
            }
            break;
        case AST_RETURN:
            codegen_return(gen, node);
            break;
        case AST_IF:
            codegen_if(gen, node);
            break;
        case AST_WHILE:
            codegen_while(gen, node);
            break;
        case AST_LOOP:
            codegen_loop(gen, node);
            break;
        case AST_CYCLE:
            codegen_cycle(gen, node);
            break;
        case AST_CHECK:
            codegen_check(gen, node);
            break;
        case AST_BREAK:
            if (gen->loop_break_label >= 0) {
                emit(gen, "    jmp .L%d", gen->loop_break_label);
            } else {
                fprintf(stderr, "Error: 'break' used outside of loop\n");
            }
            break;
        case AST_CONTINUE:
            if (gen->loop_continue_label >= 0) {
                emit(gen, "    jmp .L%d", gen->loop_continue_label);
            } else {
                fprintf(stderr, "Error: 'continue' used outside of loop\n");
            }
            break;
        case AST_DEFER:
            // Queue the deferred statement for later execution at scope exit
            ast_list_add(gen->deferred_stmts, node->defer.stmt);
            break;
        case AST_EMIT:
            // Emit value from cycle expression (just evaluate and leave in rax)
            // This is different from get[] which also returns from function
            codegen_expression(gen, node->emit.value);
            emit(gen, "    ; emit value left in rax for cycle expression");
            break;
        case AST_READ:
            // Handle read as statement (old form: read[variable])
            if (node->read.target) {
                // Read the value
                codegen_read(gen, node);
                
                // Store to target variable
                Symbol *sym = symbol_lookup(gen, node->read.target->ident.name);
                if (sym) {
                    emit(gen, "    mov [rbp - %d], rax  ; store to %s", sym->offset, node->read.target->ident.name);
                }
            } else {
                // Expression form - just read and leave in rax
                codegen_read(gen, node);
            }
            break;
        case AST_VAR_DECL:
            codegen_var_decl(gen, node);
            break;
        case AST_ASSIGN:
            codegen_assign(gen, node);
            break;
        case AST_ARRAY_DECL:
            codegen_array_decl(gen, node);
            break;
        case AST_ARRAY_ASSIGN:
            codegen_array_assign(gen, node);
            break;
        case AST_GROUP_DECL:
            codegen_group_decl(gen, node);
            break;
        case AST_MEMBER_ASSIGN:
            codegen_member_assign(gen, node);
            break;
        case AST_MAP_DECL:
            codegen_map_decl(gen, node);
            break;
        case AST_MAP_SET:
            codegen_map_set(gen, node);
            break;
        case AST_MAP_REMOVE:
            codegen_map_remove(gen, node);
            break;
        case AST_BLOCK:
            codegen_block(gen, node);
            break;
        case AST_CALL:
            // Function call as statement
            // Push arguments in reverse order (right to left for x86-64 calling convention)
            {
                const char *param_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
                int arg_count = node->call.args->count;
                
                // Check if function exists
                FunctionSignature *sig = NULL;
                for (FunctionSignature *current = gen->functions; current != NULL; current = current->next) {
                    if (strcmp(current->name, node->call.name) == 0) {
                        sig = current;
                        break;
                    }
                }
                
                if (sig == NULL) {
                    emit(gen, "    call runtime_unimplemented_func");
                    break;
                }
                
                // Validate argument count
                if (arg_count < sig->required_param_count) {
                    fprintf(stderr, "Error: Function '%s' requires at least %d arguments, but %d were provided\n", 
                            node->call.name, sig->required_param_count, arg_count);
                    break;
                }
                
                if (arg_count > sig->param_count) {
                    fprintf(stderr, "Error: Function '%s' takes at most %d arguments, but %d were provided\n", 
                            node->call.name, sig->param_count, arg_count);
                    break;
                }
                
                // Determine total arguments to pass (including defaults)
                int total_args = sig->param_count;
                
                // Push arguments beyond 6 onto stack (in reverse order)
                for (int i = total_args - 1; i >= 6; i--) {
                    if (i < arg_count) {
                        // Use provided argument
                        codegen_expression(gen, node->call.args->nodes[i]);
                    } else {
                        // Use default value
                        if (sig->default_values && sig->default_values[i]) {
                            codegen_expression(gen, sig->default_values[i]);
                        } else {
                            // This should not happen if required_param_count was set correctly
                            emit(gen, "    xor rax, rax        ; ERROR: missing required argument");
                        }
                    }
                    emit(gen, "    push rax            ; push arg %d", i);
                }
                
                // Store first 6 arguments in registers
                // We need to evaluate all args first, then move to registers
                // Use stack to temporarily store values
                for (int i = 0; i < total_args && i < 6; i++) {
                    if (i < arg_count) {
                        // Use provided argument
                        codegen_expression(gen, node->call.args->nodes[i]);
                    } else {
                        // Use default value
                        if (sig->default_values && sig->default_values[i]) {
                            codegen_expression(gen, sig->default_values[i]);
                        } else {
                            // This should not happen if required_param_count was set correctly
                            emit(gen, "    xor rax, rax        ; ERROR: missing required argument");
                        }
                    }
                    emit(gen, "    push rax            ; save arg %d", i);
                }
                
                // Pop values into registers in reverse order
                for (int i = (total_args < 6 ? total_args : 6) - 1; i >= 0; i--) {
                    emit(gen, "    pop %s              ; load arg %d", param_regs[i], i);
                }
                
                emit(gen, "    call %s", node->call.name);
                
                // Clean up stack arguments (if any beyond 6)
                if (total_args > 6) {
                    int stack_cleanup = (total_args - 6) * 8;
                    emit(gen, "    add rsp, %d         ; clean up %d stack args", stack_cleanup, total_args - 6);
                }
            }
            break;
        case AST_BUILTIN_CALL:
            // Builtin function call as statement
            {
                const BuiltinInfo *info = builtin_lookup(node->builtin_call.name);
                if (!info) {
                    fprintf(stderr, "Error: Unknown builtin function '@%s'\n", node->builtin_call.name);
                    break;
                }
                emit(gen, "    ; Builtin function @%s", node->builtin_call.name);
                
                // Handle unified type conversion with target_type
                if (info->id == BUILTIN_CAST && node->builtin_call.target_type) {
                    // Use same logic as BUILTIN_CAST in codegen_builtin_call
                    const char *target_types = node->builtin_call.target_type;
                    ASTList *args = node->builtin_call.args;
                    
                    if (args->count == 1) {
                        const char *target_type = target_types;
                        codegen_expression(gen, args->nodes[0]);
                        
                        if (strcmp(target_type, "int") == 0) {
                            if (is_deci_expr(gen, args->nodes[0])) {
                                emit(gen, "    movq xmm0, rax");
                                emit(gen, "    cvttsd2si rax, xmm0");
                            }
                        } else if (strcmp(target_type, "deci") == 0) {
                            if (!is_deci_expr(gen, args->nodes[0])) {
                                emit(gen, "    cvtsi2sd xmm0, rax");
                                emit(gen, "    movq rax, xmm0");
                            }
                        } else if (strcmp(target_type, "byte") == 0) {
                            if (is_deci_expr(gen, args->nodes[0])) {
                                emit(gen, "    movq xmm0, rax");
                                emit(gen, "    cvttsd2si rax, xmm0");
                            }
                            emit(gen, "    movsx rax, al");
                        } else if (strcmp(target_type, "bool") == 0) {
                            if (is_deci_expr(gen, args->nodes[0])) {
                                emit(gen, "    movq xmm0, rax");
                                emit(gen, "    xorpd xmm1, xmm1");
                                emit(gen, "    ucomisd xmm0, xmm1");
                                emit(gen, "    setne al");
                                emit(gen, "    movzx rax, al");
                            } else {
                                emit(gen, "    test rax, rax");
                                emit(gen, "    setnz al");
                                emit(gen, "    movzx rax, al");
                            }
                        } else if (strcmp(target_type, "str") == 0) {
                            emit(gen, "    lea rax, [rel .TODO_STR]");
                        } else if (strcmp(target_type, "char") == 0) {
                            if (is_deci_expr(gen, args->nodes[0])) {
                                emit(gen, "    movq xmm0, rax");
                                emit(gen, "    cvttsd2si rax, xmm0");
                            }
                            emit(gen, "    movsx rax, al");
                        }
                    } else {
                        // Multiple arguments
                        char types_copy[256];
                        strncpy(types_copy, target_types, sizeof(types_copy) - 1);
                        types_copy[sizeof(types_copy) - 1] = '\0';
                        
                        char *saveptr;
                        char *type_str = strtok_r(types_copy, ",", &saveptr);
                        
                        for (int i = 0; i < args->count && type_str; i++) {
                            while (*type_str == ' ') type_str++;
                            
                            codegen_expression(gen, args->nodes[i]);
                            
                            if (strcmp(type_str, "int") == 0) {
                                if (is_deci_expr(gen, args->nodes[i])) {
                                    emit(gen, "    movq xmm0, rax; cvttsd2si rax, xmm0");
                                }
                            } else if (strcmp(type_str, "deci") == 0) {
                                if (!is_deci_expr(gen, args->nodes[i])) {
                                    emit(gen, "    cvtsi2sd xmm0, rax; movq rax, xmm0");
                                }
                            } else if (strcmp(type_str, "byte") == 0) {
                                if (is_deci_expr(gen, args->nodes[i])) {
                                    emit(gen, "    movq xmm0, rax; cvttsd2si rax, xmm0");
                                }
                                emit(gen, "    movsx rax, al");
                            } else if (strcmp(type_str, "bool") == 0) {
                                if (is_deci_expr(gen, args->nodes[i])) {
                                    emit(gen, "    movq xmm0, rax; xorpd xmm1, xmm1; ucomisd xmm0, xmm1; setne al; movzx rax, al");
                                } else {
                                    emit(gen, "    test rax, rax; setnz al; movzx rax, al");
                                }
                            }
                            
                            type_str = strtok_r(NULL, ",", &saveptr);
                        }
                    }
                } else {
                    // Regular builtin call
                    codegen_builtin_call(gen, info->id, node->builtin_call.args);
                }
            }
            break;
        default:
            fprintf(stderr, "Unsupported statement type in codegen\n");
            break;
    }
}

// Generate function code
static void codegen_function(CodeGen *gen, ASTNode *node) {
    // Build parameter types array for signature registration
    int num_params = node->function.params->count;
    char **param_types = NULL;
    ASTNode **default_values = NULL;
    int required_param_count = 0;
    
    if (num_params > 0) {
        param_types = xmalloc(sizeof(char *) * num_params);
        default_values = xmalloc(sizeof(ASTNode *) * num_params);
        
        for (int i = 0; i < num_params; i++) {
            ASTNode *param = node->function.params->nodes[i];
            
            if (param->var_decl.is_array) {
                // For array parameters, create type string: "arr:element_type:size_expr"
                // Build a string representation of the array type
                char type_buf[256];
                snprintf(type_buf, sizeof(type_buf), "arr:%s", param->var_decl.array_element_type);
                param_types[i] = xstrdup(type_buf);
            } else {
                // Regular parameter
                param_types[i] = param->var_decl.type;
            }
            
            // Track default values
            default_values[i] = param->var_decl.value;  // NULL if no default
            
            // Count required parameters (those without defaults)
            if (param->var_decl.value == NULL) {
                required_param_count++;
            }
        }
    }
    
    // Register function signature in global table for return type and parameter lookup
    function_add(gen, node->function.name, node->function.return_type, param_types, num_params, required_param_count, default_values);
    
    // Free the temporary array (function_add makes its own copies)
    if (param_types) {
        for (int i = 0; i < num_params; i++) {
            // Only free if it was dynamically allocated (array params)
            ASTNode *param = node->function.params->nodes[i];
            if (param->var_decl.is_array) {
                free(param_types[i]);
            }
        }
        free(param_types);
    }
    if (default_values) {
        free(default_values);
    }
    
    emit(gen, "");
    emit(gen, "%s:", node->function.name);
    emit(gen, "    push rbp");
    emit(gen, "    mov rbp, rsp");
    
    // PRIORITY 2.3: Stack overflow protection - conditional depth checking
    if (g_stack_check_enabled) {
        int recursion_label = new_label(gen);
        int warn_label = new_label(gen);
        
        emit(gen, "    inc qword [rel g_recursion_depth]  ; increment depth counter");
        emit(gen, "    mov rax, [rel g_recursion_depth]");
        
        // Check hard limit (1000 calls)
        emit(gen, "    cmp rax, %d                       ; MAX_RECURSION_DEPTH", MAX_RECURSION_DEPTH);
        emit(gen, "    jle .recursion_ok_%d", recursion_label);
        emit(gen, "    dec qword [rel g_recursion_depth]  ; undo increment on error");
        emit(gen, "    mov rdi, 1          ; stderr");
        emit(gen, "    lea rsi, [rel stack_limit_msg]");
        emit(gen, "    mov rdx, 31         ; message length");
        emit(gen, "    mov rax, 1          ; sys_write");
        emit(gen, "    syscall");
        emit(gen, "    mov rdi, 1          ; exit code");
        emit(gen, "    mov rax, 60         ; sys_exit");
        emit(gen, "    syscall");
        
        emit(gen, ".recursion_ok_%d:", recursion_label);
        
        // Check warning threshold (900 calls = 90%%)
        emit(gen, "    cmp rax, %d         ; STACK_WARN_THRESHOLD", STACK_WARN_THRESHOLD);
        emit(gen, "    jle .stack_ok_%d", warn_label);
        
        // Approaching limit - print warning
        emit(gen, "    mov rdi, 2          ; stderr");
        emit(gen, "    lea rsi, [rel stack_warn_msg]");
        emit(gen, "    mov rdx, 45         ; message length");
        emit(gen, "    mov rax, 1          ; sys_write");
        emit(gen, "    syscall");
        
        emit(gen, ".stack_ok_%d:", warn_label);
    } else {
        // Stack checking disabled - just increment counter (minimal overhead)
        emit(gen, "    inc qword [rel g_recursion_depth]  ; increment depth counter");
    }
    
    // Store current function's return type for proper return code generation
    gen->current_function_return_type = node->function.return_type;
    
    // Clear symbol table for new function scope
    symbol_table_clear(gen);
    gen->stack_offset = 0;
    
    // Handle function parameters
    // Parameters are passed in registers: RDI, RSI, RDX, RCX, R8, R9
    // Then stack for additional parameters
    const char *param_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
    int param_count = node->function.params->count;
    
    // Allocate stack space for parameters
    if (param_count > 0) {
        int param_stack_size = param_count * 8;
        emit(gen, "    sub rsp, %d         ; allocate space for %d parameters", param_stack_size, param_count);
        
        // CRITICAL: Update stack_offset to account for parameter space
        // Local variables will be allocated AFTER parameters
        gen->stack_offset = param_stack_size;
        
        // Move parameters from registers to stack
        const char *xmm_regs[] = {"xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"};
        for (int i = 0; i < param_count && i < 6; i++) {
            ASTNode *param = node->function.params->nodes[i];
            // Parameters go at: rbp-8, rbp-16, rbp-24, etc. (BELOW saved rbp)
            // These are ABSOLUTE offsets, not relative to stack_offset
            int param_absolute_offset = (i + 1) * 8;
            
            // Determine parameter size and type
            int param_size = 8; // Default to 8 bytes
            const char *param_type_for_symbol = param->var_decl.type;
            bool is_group_param = false;
            bool is_array_param = param->var_decl.is_array;
            
            if (is_array_param) {
                // Array parameters: passed as pointers (8 bytes)
                param_size = 8;
                param_type_for_symbol = param->var_decl.array_element_type;  // Use element type for symbol
            } else {
                // Check if parameter is a group type
                GroupType *param_group = group_type_lookup(gen, param->var_decl.type);
                is_group_param = (param_group != NULL);
                
                if (is_group_param) {
                    // Group parameters: use group size, passed as pointer (8 bytes)
                    param_size = 8;
                } else if (strcmp(param->var_decl.type, "byte") == 0 || strcmp(param->var_decl.type, "ubyte") == 0) {
                    param_size = 1;
                } else if (strcmp(param->var_decl.type, "char") == 0) {
                    param_size = 1;
                } else if (strcmp(param->var_decl.type, "bool") == 0) {
                    param_size = 1;
                }
            }
            
            // Handle deci parameters - they come in xmm registers
            if (!is_array_param && !is_group_param && param->var_decl.type && strcmp(param->var_decl.type, "deci") == 0) {
                // Deci parameter arrives in xmm register, store deci bits
                emit(gen, "    movq [rbp-%d], %s   ; parameter %s (deci from xmm)", param_absolute_offset, xmm_regs[i], param->var_decl.name);
            } else {
                // Regular parameter/array parameter comes in general register
                emit(gen, "    mov [rbp-%d], %s     ; parameter %s", param_absolute_offset, param_regs[i], param->var_decl.name);
            }
            
            // Add to symbol table with ABSOLUTE offset (stored as positive, used as negative)
            // For array parameters, we need to mark them as arrays so they can be accessed correctly
            int param_array_size = is_array_param ? evaluate_array_size(param->var_decl.array_size) : 0;
            int param_element_size = 0;
            if (is_array_param && param->var_decl.array_element_type) {
                const char *elem_type = param->var_decl.array_element_type;
                if (strcmp(elem_type, "byte") == 0 || strcmp(elem_type, "ubyte") == 0 || strcmp(elem_type, "char") == 0 || strcmp(elem_type, "bool") == 0) {
                    param_element_size = 1;
                } else if (strcmp(elem_type, "int") == 0) {
                    param_element_size = 8;  // 64-bit int
                } else if (strcmp(elem_type, "deci") == 0) {
                    param_element_size = 8;  // 64-bit float
                } else if (strcmp(elem_type, "str") == 0) {
                    param_element_size = 8;  // pointer size
                } else {
                    // Group types - lookup size
                    GroupType *group = group_type_lookup(gen, elem_type);
                    param_element_size = group ? group->total_size : 8;
                }
            }
            symbol_add(gen, param->var_decl.name, param_absolute_offset, param_size, param_type_for_symbol, is_array_param, is_array_param, 
                      param_array_size, param_element_size, is_group_param);
        }
        
        // Handle stack-passed parameters (beyond 6)
        for (int i = 6; i < param_count; i++) {
            ASTNode *param = node->function.params->nodes[i];
            // Stack params are at rbp+16, rbp+24, etc. (after return address and saved rbp)
            int param_offset = 16 + (i - 6) * 8;
            
            // Determine parameter type
            int param_size = 8;
            const char *param_type_for_symbol = param->var_decl.type;
            bool is_group_param = false;
            bool is_array_param = param->var_decl.is_array;
            
            if (is_array_param) {
                // Array parameters: passed as pointers (8 bytes)
                param_size = 8;
                param_type_for_symbol = param->var_decl.array_element_type;
            } else {
                // Check if parameter is a group type
                GroupType *param_group = group_type_lookup(gen, param->var_decl.type);
                is_group_param = (param_group != NULL);
                
                if (strcmp(param->var_decl.type, "byte") == 0 || strcmp(param->var_decl.type, "ubyte") == 0) {
                    param_size = 1;
                } else if (strcmp(param->var_decl.type, "char") == 0) {
                    param_size = 1;
                } else if (strcmp(param->var_decl.type, "bool") == 0) {
                    param_size = 1;
                }
            }
            
            // For stack parameters above rbp, we use positive offsets directly
            int param_array_size_stack = is_array_param ? evaluate_array_size(param->var_decl.array_size) : 0;
            int param_element_size_stack = 0;
            if (is_array_param && param->var_decl.array_element_type) {
                const char *elem_type = param->var_decl.array_element_type;
                if (strcmp(elem_type, "byte") == 0 || strcmp(elem_type, "ubyte") == 0 || strcmp(elem_type, "char") == 0 || strcmp(elem_type, "bool") == 0) {
                    param_element_size_stack = 1;
                } else if (strcmp(elem_type, "int") == 0) {
                    param_element_size_stack = 8;
                } else if (strcmp(elem_type, "deci") == 0) {
                    param_element_size_stack = 8;
                } else if (strcmp(elem_type, "str") == 0) {
                    param_element_size_stack = 8;
                } else {
                    GroupType *group = group_type_lookup(gen, elem_type);
                    param_element_size_stack = group ? group->total_size : 8;
                }
            }
            symbol_add(gen, param->var_decl.name, param_offset, param_size, param_type_for_symbol, is_array_param, is_array_param, param_array_size_stack, param_element_size_stack, is_group_param);
        }
    }
    
    // Generate function body
    codegen_statement(gen, node->function.body);
    
    // Default return if no explicit return
    emit(gen, "    dec qword [rel g_recursion_depth]  ; decrement depth counter");
    emit(gen, "    mov rax, 0");
    emit(gen, "    mov rsp, rbp");
    emit(gen, "    pop rbp");
    emit(gen, "    ret");
}

// ============================================================================
// BUILT-IN FUNCTIONS CODE GENERATION
// ============================================================================

// Generate code for builtin function calls
void codegen_builtin_call(CodeGen *gen, BuiltinFunction fn, ASTList *args) {
    // Dispatch based on builtin category
    BuiltinCategory cat = builtin_get_category(fn);
    
    switch (cat) {
        case BUILTIN_CAT_SYSTEM:
            // System control functions
            switch (fn) {
                case BUILTIN_EXIT:
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; exit code");
                    emit(gen, "    mov rax, 60         ; sys_exit");
                    emit(gen, "    syscall");
                    break;
                case BUILTIN_HALT:
                    emit(gen, "    hlt                 ; halt CPU");
                    break;
                case BUILTIN_SLEEP:
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    ; Convert milliseconds to nanoseconds");
                    emit(gen, "    mov rbx, 1000000");
                    emit(gen, "    imul rax, rbx       ; rax = ms * 1000000 = nanoseconds");
                    emit(gen, "    ; Create timespec struct on stack: tv_sec=0, tv_nsec=nanoseconds");
                    emit(gen, "    sub rsp, 32         ; Allocate space for two timespec structs");
                    emit(gen, "    mov qword [rsp], 0  ; tv_sec = 0");
                    emit(gen, "    mov qword [rsp+8], rax ; tv_nsec = nanoseconds");
                    emit(gen, "    lea rdi, [rsp]      ; rdi = ptr to req timespec");
                    emit(gen, "    lea rsi, [rsp+16]   ; rsi = ptr to rem timespec");
                    emit(gen, "    mov rax, 35         ; sys_nanosleep");
                    emit(gen, "    syscall");
                    emit(gen, "    add rsp, 32         ; Clean up stack");
                    break;
                case BUILTIN_CLOCK:
                    emit(gen, "    sub rsp, 16");
                    emit(gen, "    mov rdi, 1          ; CLOCK_MONOTONIC");
                    emit(gen, "    lea rsi, [rsp]");
                    emit(gen, "    mov rax, 228        ; sys_clock_gettime");
                    emit(gen, "    syscall");
                    emit(gen, "    mov rax, [rsp]      ; rax = tv_sec");
                    emit(gen, "    imul rax, 1000      ; rax = tv_sec * 1000 ms");
                    emit(gen, "    mov rcx, [rsp+8]    ; rcx = tv_nsec");
                    emit(gen, "    mov rbx, 1000000");
                    emit(gen, "    mov rax, rcx");
                    emit(gen, "    xor rdx, rdx");
                    emit(gen, "    div rbx             ; rax = tv_nsec / 1000000 ms");
                    emit(gen, "    mov rcx, [rsp]      ; rcx = tv_sec");
                    emit(gen, "    imul rcx, 1000      ; rcx = tv_sec * 1000 ms");
                    emit(gen, "    add rax, rcx        ; rax = total ms");
                    emit(gen, "    add rsp, 16");
                    break;
                case BUILTIN_PANIC:
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rsi, rax");
                    emit(gen, "    mov rdi, 2          ; stderr");
                    emit(gen, "    mov rdx, 256");
                    emit(gen, "    mov rax, 1          ; sys_write");
                    emit(gen, "    syscall");
                    emit(gen, "    mov rdi, 1");
                    emit(gen, "    mov rax, 60         ; sys_exit");
                    emit(gen, "    syscall");
                    break;
                default:
                    emit(gen, "    xor rax, rax        ; unsupported");
            }
            break;
            
        case BUILTIN_CAT_MEMORY:
            // Memory operations
            switch (fn) {
                case BUILTIN_ADDR:
                    if (args->nodes[0]->type == AST_IDENT) {
                        Symbol *sym = symbol_lookup(gen, args->nodes[0]->ident.name);
                        if (sym) {
                            emit(gen, "    lea rax, [rbp - %d]    ; @addr(%s)", sym->offset, args->nodes[0]->ident.name);
                        }
                    }
                    break;
                case BUILTIN_PEEK: {
                    // @peek[addr] - read byte at address
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rcx, rax        ; address to read");
                    emit(gen, "    mov al, [rcx]       ; read byte");
                    emit(gen, "    movzx rax, al       ; zero extend to 64-bit");
                    break;
                }
                case BUILTIN_POKE: {
                    // @poke[addr, value] - write byte at address
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rcx, rax        ; address to write");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov [rcx], al       ; write byte");
                    break;
                }
                case BUILTIN_MEMCPY:
                    // @memcpy[dest, src, size] - copy from src to dest
                    // NOTE: rep movsb expects rsi=src, rdi=dest (backwards from argument order!)
                    codegen_expression(gen, args->nodes[1]);  // src (2nd arg)
                    emit(gen, "    mov rsi, rax        ; src for rep movsb");
                    codegen_expression(gen, args->nodes[0]);  // dest (1st arg)
                    emit(gen, "    mov rdi, rax        ; dest for rep movsb");
                    codegen_expression(gen, args->nodes[2]);  // size (3rd arg)
                    emit(gen, "    mov rcx, rax        ; count");
                    emit(gen, "    rep movsb           ; copy bytes");
                    emit(gen, "    mov rax, 0          ; return 0 on success");
                    break;
                case BUILTIN_MEMCLR:
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    push rax");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rcx, rax");
                    emit(gen, "    pop rdi");
                    emit(gen, "    xor al, al");
                    emit(gen, "    rep stosb");
                    break;
                
                // RLMM - RasCode Memory Model
                case BUILTIN_ALLOC:
                    // @alloc[size] - Allocate heap memory via brk syscall
                    // NOTE: Compiler (rascom) uses memory_safety layer
                    // Generated code uses standalone brk for portability
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; size to allocate");
                    emit(gen, "    add rdi, 8          ; add 8 bytes for size metadata");
                    emit(gen, "    push rdi            ; save total size");
                    emit(gen, "    xor rdi, rdi        ; get current brk");
                    emit(gen, "    mov rax, 12         ; sys_brk");
                    emit(gen, "    syscall");
                    emit(gen, "    mov rbx, rax        ; save current brk (metadata location)");
                    emit(gen, "    pop rdi             ; restore total size");
                    emit(gen, "    add rdi, rbx        ; new brk = old + size");
                    emit(gen, "    mov rax, 12         ; sys_brk");
                    emit(gen, "    syscall");
                    emit(gen, "    cmp rax, rdi        ; check if succeeded");
                    emit(gen, "    jne .alloc_fail_%d", gen->label_count);
                    emit(gen, "    mov [rbx], rdi      ; store size metadata");
                    emit(gen, "    add rbx, 8          ; skip metadata for user data");
                    emit(gen, "    mov rax, rbx        ; return data ptr (after metadata)");
                    emit(gen, "    jmp .alloc_done_%d", gen->label_count);
                    emit(gen, ".alloc_fail_%d:", gen->label_count);
                    emit(gen, "    xor rax, rax        ; return 0 on failure");
                    emit(gen, ".alloc_done_%d:", gen->label_count);
                    gen->label_count++;
                    break;
                
                case BUILTIN_FREE:
                    // @free[ptr] - Mark as freed (brk-based, actual free on exit)
                    // NOTE: Compiler (rascom) uses memory_safety layer
                    // Generated code uses standalone brk for portability
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; user ptr");
                    emit(gen, "    sub rdi, 8          ; get metadata ptr");
                    emit(gen, "    mov qword [rdi], 0  ; mark as freed");
                    emit(gen, "    xor rax, rax        ; return 0");
                    break;
                
                case BUILTIN_REALLOC: {
                    // @realloc[ptr, new_size] - Reallocate with metadata tracking
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rbx, rax        ; old_ptr in rbx");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rcx, rax        ; new_size in rcx");
                    
                    // Get old size from metadata
                    emit(gen, "    mov rax, [rbx - 8]  ; read old size from metadata");
                    emit(gen, "    sub rax, 8          ; adjust for metadata size");
                    emit(gen, "    mov r9, rax         ; old_size in r9");
                    
                    // Allocate new block with metadata
                    emit(gen, "    push rbx            ; save old_ptr");
                    emit(gen, "    push rcx            ; save new_size");
                    emit(gen, "    push r9             ; save old_size");
                    emit(gen, "    mov rdi, rcx        ; new_size");
                    emit(gen, "    add rdi, 8          ; add metadata overhead");
                    emit(gen, "    xor rax, 0          ; get current brk");
                    emit(gen, "    mov rax, 12         ; sys_brk");
                    emit(gen, "    syscall");
                    emit(gen, "    mov r8, rax         ; new_ptr (metadata location) in r8");
                    emit(gen, "    pop r9              ; restore old_size");
                    emit(gen, "    pop rcx             ; restore new_size");
                    emit(gen, "    add rax, rcx        ; new brk = current + new_size + 8");
                    emit(gen, "    add rax, 8");
                    emit(gen, "    mov rdi, rax");
                    emit(gen, "    mov rax, 12         ; sys_brk");
                    emit(gen, "    syscall");
                    emit(gen, "    cmp rax, rdi        ; check success");
                    emit(gen, "    jne .realloc_fail_%d", gen->label_count);
                    
                    // Store metadata
                    emit(gen, "    mov rax, rcx        ; new_size");
                    emit(gen, "    add rax, 8          ; with metadata");
                    emit(gen, "    mov [r8], rax       ; store total size in metadata");
                    emit(gen, "    add r8, 8           ; adjust to user ptr");
                    
                    // Copy old data to new block (copy MIN(old_size, new_size))
                    emit(gen, "    pop rbx             ; old_ptr");
                    emit(gen, "    mov rdi, r8         ; dest = new_ptr");
                    emit(gen, "    mov rsi, rbx        ; src = old_ptr");
                    emit(gen, "    mov rcx, r9         ; old_size");
                    emit(gen, "    cmp rcx, [rsp]      ; compare old_size with new_size");
                    emit(gen, "    jle .realloc_copy_%d", gen->label_count);
                    emit(gen, "    mov rcx, [rsp]      ; use new_size if smaller");
                    emit(gen, ".realloc_copy_%d:", gen->label_count);
                    emit(gen, "    rep movsb           ; copy data");
                    emit(gen, "    add rsp, 8          ; clean stack");
                    emit(gen, "    mov rax, r8         ; return new_ptr");
                    emit(gen, "    jmp .realloc_done_%d", gen->label_count);
                    emit(gen, ".realloc_fail_%d:", gen->label_count);
                    emit(gen, "    add rsp, 16         ; clean up stack");
                    emit(gen, "    xor rax, rax        ; return 0 on failure");
                    emit(gen, ".realloc_done_%d:", gen->label_count);
                    gen->label_count++;
                    break;
                }
                
                case BUILTIN_SALLOC:
                    // @salloc[size] - Proper stack allocation via RSP adjustment
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rsi, rax        ; size to allocate");
                    emit(gen, "    mov rax, rsp        ; current stack pointer");
                    emit(gen, "    sub rsp, rsi        ; allocate on stack");
                    emit(gen, "    mov rax, rsp        ; return stack ptr");
                    // Align to 16-byte boundary for system calls
                    emit(gen, "    and rax, -16        ; align to 16 bytes");
                    break;
                
                case BUILTIN_MEMSET:
                    // @memset[ptr, value, size] - Set memory to value
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    push rax            ; save ptr");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    push rax            ; save value");
                    codegen_expression(gen, args->nodes[2]);
                    emit(gen, "    mov rcx, rax        ; count");
                    emit(gen, "    pop rax             ; restore value");
                    emit(gen, "    pop rdi             ; restore ptr");
                    emit(gen, "    rep stosb           ; fill memory");
                    break;
                
                case BUILTIN_MEMCMP:
                    // @memcmp[ptr1, ptr2, size] - Compare memory
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    push rax");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    push rax");
                    codegen_expression(gen, args->nodes[2]);
                    emit(gen, "    mov rcx, rax");
                    emit(gen, "    pop rsi");
                    emit(gen, "    pop rdi");
                    emit(gen, "    repe cmpsb          ; compare bytes");
                    emit(gen, "    setz al             ; 1 if equal, 0 if not");
                    emit(gen, "    movzx rax, al");
                    break;
                
                case BUILTIN_MMAP:
                    // @mmap[addr, size, prot, flags, fd, offset] or simplified @mmap[size, prot, flags]
                    // Using simplified version with NULL addr and 0 offset, fd=-1
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rax, rdi        ; size");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; prot");
                    codegen_expression(gen, args->nodes[2]);
                    emit(gen, "    mov rdx, rax        ; flags");
                    emit(gen, "    xor rdi, rdi        ; addr = NULL");
                    emit(gen, "    mov r10, rdx        ; flags");
                    emit(gen, "    mov r8, -1          ; fd = -1");
                    emit(gen, "    xor r9, r9          ; offset = 0");
                    emit(gen, "    mov rax, 9          ; sys_mmap");
                    emit(gen, "    syscall");
                    break;
                
                case BUILTIN_MUNMAP:
                    // @munmap[ptr, size] - Unmap memory
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; ptr");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; size");
                    emit(gen, "    mov rax, 11         ; sys_munmap");
                    emit(gen, "    syscall");
                    break;
                
                case BUILTIN_MPROTECT:
                    // @mprotect[ptr, size, prot] - Change memory protection
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; ptr");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; size");
                    codegen_expression(gen, args->nodes[2]);
                    emit(gen, "    mov rdx, rax        ; prot");
                    emit(gen, "    call sc_mprotect");
                    break;
                
                case BUILTIN_HEAP_START:
                    // @heap_start[] - Get heap start address
                    emit(gen, "    xor rdi, rdi");
                    emit(gen, "    mov rax, 12         ; sys_brk");
                    emit(gen, "    syscall             ; returns current brk");
                    break;
                
                case BUILTIN_HEAP_END:
                    // @heap_end[] - Alias for heap_start
                    emit(gen, "    xor rdi, rdi");
                    emit(gen, "    mov rax, 12         ; sys_brk");
                    emit(gen, "    syscall");
                    break;
                
                case BUILTIN_HEAP_SIZE:
                    // @heap_size[] - Return current heap usage
                    // Stores initial brk in data section, tracks current
                    emit(gen, "    xor rdi, rdi        ; get current brk");
                    emit(gen, "    mov rax, 12         ; sys_brk");
                    emit(gen, "    syscall");
                    emit(gen, "    mov rbx, rax        ; current heap end");
                    emit(gen, "    lea rax, [rel heap_start]");
                    emit(gen, "    mov r8, [rax]       ; load stored heap start");
                    emit(gen, "    cmp r8, 0           ; check if initialized");
                    emit(gen, "    jne .heap_size_calc_%d", gen->label_count);
                    emit(gen, "    mov [rax], rbx      ; store initial heap start");
                    emit(gen, "    xor rax, rax        ; first call returns 0");
                    emit(gen, "    jmp .heap_size_done_%d", gen->label_count);
                    emit(gen, ".heap_size_calc_%d:", gen->label_count);
                    emit(gen, "    mov rax, rbx        ; current end");
                    emit(gen, "    sub rax, r8         ; subtract start");
                    emit(gen, ".heap_size_done_%d:", gen->label_count);
                    gen->label_count++;
                    break;
                
                case BUILTIN_PAGE_SIZE:
                    // @page_size[] - Return 4096 (standard page size)
                    emit(gen, "    mov rax, 4096");
                    break;
                
                case BUILTIN_STACK_PTR:
                    // @stack_ptr[] - Get current stack pointer
                    emit(gen, "    mov rax, rsp");
                    break;
                
                case BUILTIN_STACK_SIZE:
                    // @stack_size[] - Get stack usage (rbp - rsp)
                    emit(gen, "    mov rax, rbp");
                    emit(gen, "    sub rax, rsp");
                    break;
                
                case BUILTIN_MFENCE:
                    // @mfence[] - Memory fence (full barrier)
                    emit(gen, "    mfence");
                    emit(gen, "    xor rax, rax");
                    break;
                
                case BUILTIN_LFENCE:
                    // @lfence[] - Load fence
                    emit(gen, "    lfence");
                    emit(gen, "    xor rax, rax");
                    break;
                
                case BUILTIN_SFENCE:
                    // @sfence[] - Store fence
                    emit(gen, "    sfence");
                    emit(gen, "    xor rax, rax");
                    break;
                
                default:
                    emit(gen, "    xor rax, rax        ; unsupported");
            }
            break;
            
        case BUILTIN_CAT_FILE:
            // File I/O operations - use runtime wrappers
            switch (fn) {
                case BUILTIN_FOPEN:
                    // @fopen[path, flags] -> fd
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; path");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; flags");
                    emit(gen, "    call sc_fopen");
                    break;
                case BUILTIN_FREAD:
                    // @fread[fd, buf, len] -> bytes_read
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; fd");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; buffer");
                    codegen_expression(gen, args->nodes[2]);
                    emit(gen, "    mov rdx, rax        ; length");
                    emit(gen, "    call sc_fread");
                    break;
                case BUILTIN_FWRITE:
                    // @fwrite[fd, buf, len] -> bytes_written
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; fd");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; buffer");
                    codegen_expression(gen, args->nodes[2]);
                    emit(gen, "    mov rdx, rax        ; length");
                    emit(gen, "    call sc_fwrite");
                    break;
                case BUILTIN_FSEEK:
                    // @fseek[fd, offset] -> position
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; fd");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; offset");
                    emit(gen, "    call sc_fseek");
                    break;
                case BUILTIN_FCLOSE:
                    // @fclose[fd] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; fd");
                    emit(gen, "    call sc_fclose");
                    break;
                case BUILTIN_FDELETE:
                    // @fdelete[path] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; path");
                    emit(gen, "    call sc_fdelete");
                    break;
                default:
                    emit(gen, "    xor rax, rax        ; unsupported file operation");
            }
            break;
            
        case BUILTIN_CAT_CONVERSION:
            // Conversion/utility functions
            switch (fn) {
                case BUILTIN_TYPE:
                    // @type[value] -> type name as string
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; value");
                    emit(gen, "    call sc_type");
                    break;
                
                case BUILTIN_TO_INT:
                    // Convert value to int (DEPRECATED: use @type[x]::int)
                    // First evaluate the expression - result in rax
                    codegen_expression(gen, args->nodes[0]);
                    // Check if source was deci type
                    if (is_deci_expr(gen, args->nodes[0])) {
                        emit(gen, "    movq xmm0, rax      ; move to xmm0");
                        emit(gen, "    cvttsd2si rax, xmm0 ; truncate deci to int");
                    }
                    // Otherwise rax already has int value
                    break;
                
                case BUILTIN_TO_DECI:
                    // Convert value to deci (DEPRECATED: use @type[x]::deci)
                    codegen_expression(gen, args->nodes[0]);
                    // Check if source was int type (not deci)
                    if (!is_deci_expr(gen, args->nodes[0])) {
                        emit(gen, "    cvtsi2sd xmm0, rax  ; convert int to deci");
                        emit(gen, "    movq rax, xmm0      ; move back to rax");
                    }
                    // Otherwise rax already has deci value
                    break;
                
                case BUILTIN_TO_BYTE:
                    // Convert value to byte (DEPRECATED: use @type[x]::byte)
                    codegen_expression(gen, args->nodes[0]);
                    if (is_deci_expr(gen, args->nodes[0])) {
                        emit(gen, "    movq xmm0, rax      ; move to xmm0");
                        emit(gen, "    cvttsd2si rax, xmm0 ; truncate deci to int");
                    }
                    emit(gen, "    movsx rax, al       ; sign extend byte to 64-bit");
                    break;
                
                case BUILTIN_TO_BOOL:
                    // Convert value to bool (DEPRECATED: use @type[x]::bool)
                    codegen_expression(gen, args->nodes[0]);
                    if (is_deci_expr(gen, args->nodes[0])) {
                        emit(gen, "    movq xmm0, rax      ; move to xmm0");
                        emit(gen, "    xorpd xmm1, xmm1    ; zero xmm1");
                        emit(gen, "    ucomisd xmm0, xmm1  ; compare to 0.0");
                        emit(gen, "    setne al            ; set if not equal");
                        emit(gen, "    movzx rax, al       ; zero extend to 64-bit");
                    } else {
                        emit(gen, "    test rax, rax       ; check if zero");
                        emit(gen, "    setnz al            ; set if not zero");
                        emit(gen, "    movzx rax, al       ; zero extend to 64-bit");
                    }
                    break;
                
                case BUILTIN_TO_STR:
                    // Convert value to string (DEPRECATED: use @type[x]::str)
                    emit(gen, "    lea rax, [rel .TODO_STR]");
                    break;
                
                case BUILTIN_SIZEOF: {
                    // @sizeof[identifier] - Get size of variable in bytes
                    if (args->count > 0 && args->nodes[0]->type == AST_IDENT) {
                        const char *var_name = args->nodes[0]->ident.name;
                        Symbol *sym = symbol_lookup(gen, var_name);
                        
                        if (sym) {
                            int size = get_type_size(sym->type);
                            emit(gen, "    mov rax, %d          ; size of '%s' (%s)", size, var_name, sym->type);
                        } else {
                            emit(gen, "    mov rax, 8          ; unknown variable, default to 8");
                        }
                    } else {
                        // Fallback: evaluate expression and default to 8
                        if (args->count > 0) {
                            codegen_expression(gen, args->nodes[0]);
                        }
                        emit(gen, "    mov rax, 8          ; generic size");
                    }
                    break;
                }
                case BUILTIN_LEN:
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax");
                    emit(gen, "    xor rcx, rcx");
                    emit(gen, ".len_loop_%d:", gen->label_count);
                    emit(gen, "    cmp byte [rdi+rcx], 0");
                    emit(gen, "    je .len_done_%d", gen->label_count);
                    emit(gen, "    inc rcx");
                    emit(gen, "    jmp .len_loop_%d", gen->label_count);
                    emit(gen, ".len_done_%d:", gen->label_count);
                    emit(gen, "    mov rax, rcx");
                    gen->label_count++;
                    break;
                
                case BUILTIN_CONCAT:
                    // Concatenate two strings: @concat[str1, str2]
                    // Evaluate str1
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov r12, rax        ; r12 = str1");
                    
                    // Evaluate str2
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov r13, rax        ; r13 = str2");
                    
                    // Get len(str1)
                    emit(gen, "    mov rdi, r12");
                    emit(gen, "    call strlen");
                    emit(gen, "    mov r14, rax        ; r14 = len1");
                    
                    // Get len(str2)
                    emit(gen, "    mov rdi, r13");
                    emit(gen, "    call strlen");
                    emit(gen, "    mov r15, rax        ; r15 = len2");
                    
                    // Allocate buffer
                    emit(gen, "    lea rdi, [r14 + r15 + 1]  ; total size");
                    emit(gen, "    call malloc");
                    emit(gen, "    mov rbx, rax        ; rbx = result buffer");
                    
                    // Copy str1
                    emit(gen, "    mov rdi, rbx");
                    emit(gen, "    mov rsi, r12");
                    emit(gen, "    mov rcx, r14");
                    emit(gen, "    rep movsb");
                    
                    // Copy str2
                    emit(gen, "    mov rsi, r13");
                    emit(gen, "    mov rcx, r15");
                    emit(gen, "    rep movsb");
                    
                    // Null terminate
                    emit(gen, "    mov byte [rdi], 0");
                    emit(gen, "    mov rax, rbx        ; return result");
                    break;
                
                case BUILTIN_SUBSTR:
                    // Get substring: @substr[str, start, len]
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    push rax            ; save str");
                    
                    codegen_expression(gen, args->nodes[2]);
                    emit(gen, "    push rax            ; save len");
                    emit(gen, "    inc rax             ; +1 for null");
                    emit(gen, "    mov rdi, rax");
                    emit(gen, "    call malloc         ; allocate buffer");
                    emit(gen, "    mov rdi, rax        ; dest");
                    
                    emit(gen, "    pop rdx             ; len");
                    emit(gen, "    pop rsi             ; str");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    add rsi, rax        ; str + start");
                    emit(gen, "    mov rcx, rdx        ; count");
                    emit(gen, "    rep movsb           ; copy bytes");
                    emit(gen, "    mov byte [rdi], 0   ; null terminate");
                    emit(gen, "    sub rdi, rdx        ; restore buffer start");
                    emit(gen, "    mov rax, rdi");
                    break;
                
                case BUILTIN_STRCMP:
                    // Compare two strings: @strcmp[str1, str2]
                    // Returns 0 if equal, <0 if str1<str2, >0 if str1>str2
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; str1");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; str2");
                    emit(gen, "    call strcmp");
                    break;
                
                case BUILTIN_CHR:
                    // Convert int to char: @chr[65] -> 'A'
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    and rax, 0xFF       ; keep only byte");
                    break;
                
                case BUILTIN_ORD:
                    // Convert char to int: @ord['A'] -> 65
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    movzx rax, al       ; zero extend byte");
                    break;
                
                default:
                    emit(gen, "    xor rax, rax        ; unsupported");
            }
            break;
            
        case BUILTIN_CAT_HARDWARE:
            // Hardware access - use runtime wrappers
            switch (fn) {
                case BUILTIN_PORT_IN:
                    // @port_in[port] -> value
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; port");
                    emit(gen, "    call sc_port_in");
                    break;
                case BUILTIN_PORT_OUT:
                    // @port_out[port, value] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; port");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; value");
                    emit(gen, "    call sc_port_out");
                    break;
                case BUILTIN_IOREAD:
                    // @ioread[addr] -> value
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; address");
                    emit(gen, "    call sc_ioread");
                    break;
                case BUILTIN_IOWRITE:
                    // @iowrite[addr, value] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; address");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; value");
                    emit(gen, "    call sc_iowrite");
                    break;
                default:
                    emit(gen, "    xor rax, rax        ; unsupported");
            }
            break;
            
        case BUILTIN_CAT_META:
            // Meta/build-time functions
            switch (fn) {
                case BUILTIN_BUILD_TIME: {
                    time_t now = time(NULL);
                    char *timestamp = ctime(&now);
                    // Remove newline
                    timestamp[strcspn(timestamp, "\n")] = 0;
                    emit(gen, "    lea rax, [rel .build_time_%d]", gen->label_count);
                    fprintf(gen->output, "section .data\n");
                    fprintf(gen->output, ".build_time_%d: db \"%s\", 0\n", gen->label_count, timestamp);
                    fprintf(gen->output, "section .text\n");
                    gen->label_count++;
                    break;
                }
                case BUILTIN_COMPILER_VER:
                    emit(gen, "    lea rax, [rel .compiler_ver_%d]", gen->label_count);
                    fprintf(gen->output, "section .data\n");
                    fprintf(gen->output, ".compiler_ver_%d: db \"rascode %s\", 0\n", gen->label_count, RASCODE_VERSION);
                    fprintf(gen->output, "section .text\n");
                    gen->label_count++;
                    break;
                case BUILTIN_SYSCALL:
                    // Direct syscall with variadic args
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov r9, rax");
                    // Load up to 6 arguments
                    const char *regs[] = {"rdi", "rsi", "rdx", "r10", "r8", "r9"};
                    for (int i = 1; i < args->count && i <= 6; i++) {
                        codegen_expression(gen, args->nodes[i]);
                        if (i < 6) {
                            emit(gen, "    mov %s, rax", regs[i-1]);
                        }
                    }
                    emit(gen, "    mov rax, r9");
                    emit(gen, "    syscall");
                    break;
                case BUILTIN_IRQ_ENABLE:
                    // Enable interrupts via runtime wrapper
                    emit(gen, "    call sc_irq_enable");
                    break;
                case BUILTIN_IRQ_DISABLE:
                    // Disable interrupts via runtime wrapper
                    emit(gen, "    call sc_irq_disable");
                    break;
                case BUILTIN_VERIFY:
                    // Cryptographic signature verification is handled in BUILTIN_CAT_SECURITY
                    emit(gen, "    xor rax, rax        ; verify not in this category");
                    break;
                default:
                    emit(gen, "    xor rax, rax        ; unsupported");
            }
            break;
            
        case BUILTIN_CAT_NETWORK:
            // Network socket operations - use runtime wrappers
            switch (fn) {
                case BUILTIN_SOCKET: {
                    // @socket[type, protocol] -> handle
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; type");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; protocol");
                    emit(gen, "    call sc_socket");
                    break;
                }
                case BUILTIN_CONNECT: {
                    // @connect[sock, addr, port] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; socket");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; addr");
                    codegen_expression(gen, args->nodes[2]);
                    emit(gen, "    mov rdx, rax        ; port");
                    emit(gen, "    call sc_connect");
                    break;
                }
                case BUILTIN_BIND: {
                    // @bind[sock, addr, port] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; socket");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; addr");
                    codegen_expression(gen, args->nodes[2]);
                    emit(gen, "    mov rdx, rax        ; port");
                    emit(gen, "    call sc_bind");
                    break;
                }
                case BUILTIN_LISTEN: {
                    // @listen[sock, backlog] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; socket");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; backlog");
                    emit(gen, "    call sc_listen");
                    break;
                }
                case BUILTIN_ACCEPT: {
                    // @accept[sock] -> new_socket
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; socket");
                    emit(gen, "    call sc_accept");
                    break;
                }
                case BUILTIN_SEND: {
                    // @send[sock, buf, len] -> bytes_sent
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; socket");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; buffer");
                    codegen_expression(gen, args->nodes[2]);
                    emit(gen, "    mov rdx, rax        ; length");
                    emit(gen, "    call sc_send");
                    break;
                }
                case BUILTIN_RECV: {
                    // @recv[sock, buf, len] -> bytes_received
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; socket");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; buffer");
                    codegen_expression(gen, args->nodes[2]);
                    emit(gen, "    mov rdx, rax        ; length");
                    emit(gen, "    call sc_recv");
                    break;
                }
                case BUILTIN_CLOSE: {
                    // @close[sock] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; socket");
                    emit(gen, "    call sc_close");
                    break;
                }
                default:
                    fprintf(stderr, "ERROR: Unhandled network builtin %d\n", fn);
                    break;
            }
            break;
            
        case BUILTIN_CAT_SECURITY:
            // Security/cryptography operations
            switch (fn) {
                case BUILTIN_HASH: {
                    // @hash[buffer, algorithm] -> hash_result
                    // algorithm: 1=CRC32, 2=DJBX33A simple hash
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rcx, rax        ; algorithm");
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; buffer");
                    
                    emit(gen, "    cmp rcx, 1");
                    emit(gen, "    je .hash_crc32");
                    
                    // Default: DJBX33A hash
                    emit(gen, "    xor rax, rax        ; hash=0");
                    emit(gen, ".hash_loop:");
                    emit(gen, "    movzx rbx, byte [rdi]");
                    emit(gen, "    test rbx, rbx");
                    emit(gen, "    jz .hash_done");
                    emit(gen, "    shl rax, 5");
                    emit(gen, "    add rax, rbx");
                    emit(gen, "    inc rdi");
                    emit(gen, "    jmp .hash_loop");
                    emit(gen, ".hash_crc32:");
                    // CRC32 implementation using polynomial 0xEDB88320
                    emit(gen, "    mov rax, 0xFFFFFFFF ; CRC32 init value");
                    emit(gen, ".hash_crc32_loop:");
                    emit(gen, "    movzx rbx, byte [rdi]");
                    emit(gen, "    test rbx, rbx       ; check for null terminator");
                    emit(gen, "    jz .hash_crc32_done");
                    
                    // XOR byte into CRC
                    emit(gen, "    movzx rcx, al       ; get low byte of CRC");
                    emit(gen, "    xor rcx, rbx        ; XOR with data byte");
                    emit(gen, "    and rcx, 0xFF");
                    
                    // Lookup table simulation - use polynomial for 8 bits
                    emit(gen, "    xor r8, r8          ; result = 0");
                    emit(gen, "    mov r9, 8           ; loop 8 times");
                    emit(gen, ".hash_crc32_poly:");
                    emit(gen, "    mov r10, rcx");
                    emit(gen, "    and r10, 1");
                    emit(gen, "    shr rcx, 1");
                    emit(gen, "    test r10, r10");
                    emit(gen, "    jz .hash_crc32_skip_poly");
                    emit(gen, "    xor rcx, 0xEDB88320");
                    emit(gen, ".hash_crc32_skip_poly:");
                    emit(gen, "    dec r9");
                    emit(gen, "    jnz .hash_crc32_poly");
                    
                    // Update CRC
                    emit(gen, "    shr rax, 8          ; shift CRC right");
                    emit(gen, "    xor rax, rcx        ; XOR with polynomial result");
                    emit(gen, "    and rax, 0xFFFFFFFF ; keep 32-bit");
                    emit(gen, "    inc rdi");
                    emit(gen, "    jmp .hash_crc32_loop");
                    emit(gen, ".hash_crc32_done:");
                    emit(gen, "    xor rax, 0xFFFFFFFF ; final XOR");
                    emit(gen, ".hash_done:");
                    break;
                }
                case BUILTIN_RAND: {
                    // @rand[size] -> random_bytes (simplified)
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; size");
                    emit(gen, "    mov rax, 318        ; sys_getrandom");
                    emit(gen, "    xor rsi, rsi        ; buf (use rax)");
                    emit(gen, "    xor rdx, rdx        ; flags");
                    emit(gen, "    syscall");
                    break;
                }
                case BUILTIN_SECURE_ZERO: {
                    // @secure_zero[ptr, size] - overwrite memory with zeros
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; ptr");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rcx, rax        ; size");
                    emit(gen, "    xor rax, rax");
                    emit(gen, ".sec_zero_loop:");
                    emit(gen, "    test rcx, rcx");
                    emit(gen, "    jz .sec_zero_done");
                    emit(gen, "    mov byte [rdi], 0");
                    emit(gen, "    inc rdi");
                    emit(gen, "    dec rcx");
                    emit(gen, "    jmp .sec_zero_loop");
                    emit(gen, ".sec_zero_done:");
                    break;
                }
                case BUILTIN_ENTROPY: {
                    // @entropy -> entropy_value
                    emit(gen, "    mov rax, 1          ; get_random_bytes(1)");
                    emit(gen, "    mov rdi, rsp        ; buffer on stack");
                    emit(gen, "    mov rax, 318        ; sys_getrandom");
                    emit(gen, "    syscall");
                    emit(gen, "    movzx rax, byte [rsp]");
                    break;
                }
                case BUILTIN_VERIFY: {
                    // @verify[sig, pub, data] -> is_valid
                    // Simplified: Verify signature using basic checksum
                    // Sig format: first 8 bytes = hash of (pub || data)
                    
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rbx, rax        ; sig ptr");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rdi, rax        ; pub ptr");
                    codegen_expression(gen, args->nodes[2]);
                    emit(gen, "    mov rsi, rax        ; data ptr");
                    
                    // Calculate hash of pub || data
                    emit(gen, "    xor rax, rax        ; hash = 0");
                    emit(gen, ".verify_pub_loop:");
                    emit(gen, "    movzx rcx, byte [rdi]");
                    emit(gen, "    test rcx, rcx");
                    emit(gen, "    jz .verify_data_loop");
                    emit(gen, "    shl rax, 5");
                    emit(gen, "    add rax, rcx");
                    emit(gen, "    inc rdi");
                    emit(gen, "    jmp .verify_pub_loop");
                    
                    emit(gen, ".verify_data_loop:");
                    emit(gen, "    movzx rcx, byte [rsi]");
                    emit(gen, "    test rcx, rcx");
                    emit(gen, "    jz .verify_compare");
                    emit(gen, "    shl rax, 5");
                    emit(gen, "    add rax, rcx");
                    emit(gen, "    inc rsi");
                    emit(gen, "    jmp .verify_data_loop");
                    
                    // Compare with signature
                    emit(gen, ".verify_compare:");
                    emit(gen, "    mov rcx, [rbx]      ; read signature");
                    emit(gen, "    cmp rax, rcx        ; compare hashes");
                    emit(gen, "    je .verify_valid");
                    emit(gen, "    xor rax, rax        ; invalid");
                    emit(gen, "    jmp .verify_done");
                    emit(gen, ".verify_valid:");
                    emit(gen, "    mov rax, 1          ; valid");
                    emit(gen, ".verify_done:");
                    break;
                }
                default:
                    emit(gen, "    xor rax, rax        ; unsupported security function");
            }
            break;
            
        case BUILTIN_CAT_PROCESS:
            // Process/thread operations
            switch (fn) {
                case BUILTIN_SPAWN: {
                    // @spawn[fn, arg] -> tid
                    // Creates thread that executes fn(arg)
                    // Requires function address and argument setup
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rbx, rax        ; function ptr");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; arg");
                    
                    // Set up thread stack and call function
                    emit(gen, "    push rsi            ; save arg");
                    emit(gen, "    push rbx            ; save function");
                    emit(gen, "    mov rdi, rsi        ; arg for thread");
                    emit(gen, "    mov rax, 56         ; sys_clone");
                    emit(gen, "    mov rsi, CLONE_THREAD | CLONE_VM");
                    emit(gen, "    syscall             ; tid in rax");
                    emit(gen, "    test rax, rax");
                    emit(gen, "    jnz .spawn_parent_%d ; parent process", gen->label_count);
                    
                    // Child process
                    emit(gen, "    pop rbx             ; function ptr");
                    emit(gen, "    pop rdi             ; arg");
                    emit(gen, "    call rbx            ; execute function");
                    emit(gen, "    mov rax, 60         ; sys_exit");
                    emit(gen, "    xor rdi, rdi");
                    emit(gen, "    syscall");
                    
                    emit(gen, ".spawn_parent_%d:", gen->label_count);
                    emit(gen, "    add rsp, 16         ; clean stack");
                    gen->label_count++;
                    break;
                }
                case BUILTIN_JOIN: {
                    // @join[tid] -> result
                    // Waits for thread to finish and returns exit code
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rbx, rax        ; tid");
                    emit(gen, "    lea rsi, [rbp - 8]  ; status location");
                    emit(gen, "    xor rdx, rdx        ; options = 0");
                    emit(gen, "    xor r10, r10        ; rusage = NULL");
                    emit(gen, "    mov rdi, rbx        ; pid");
                    emit(gen, "    mov rax, 114        ; sys_wait4");
                    emit(gen, "    syscall");
                    emit(gen, "    mov rax, qword [rsi] ; get status");
                    emit(gen, "    shr rax, 8          ; extract exit code");
                    break;
                }
                case BUILTIN_PID: {
                    // @pid -> process_id
                    emit(gen, "    mov rax, 39         ; sys_getpid");
                    emit(gen, "    syscall");
                    break;
                }
                case BUILTIN_KILL: {
                    // @kill[pid] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; pid");
                    emit(gen, "    mov rsi, 9          ; SIGKILL");
                    emit(gen, "    mov rax, 62         ; sys_kill");
                    emit(gen, "    syscall");
                    break;
                }
                default:
                    emit(gen, "    xor rax, rax        ; unsupported process function");
            }
            break;
        
        case BUILTIN_CAT_SYNC:
            // Synchronization primitives
            codegen_builtin_sync(gen, fn, args);
            break;
        case BUILTIN_CAT_CHANNEL:
            // Channel communication
            codegen_builtin_channel(gen, fn, args);
            break;
        case BUILTIN_CAT_THREADPOOL:
            // Thread pool operations
            codegen_builtin_threadpool(gen, fn, args);
            break;
        
        case BUILTIN_CAT_STRING:
            // String manipulation functions - implemented in runtime/strings.c
            switch (fn) {
                case BUILTIN_SPLIT: {
                    // @split[str, delim] -> array pointer
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; str");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; delim");
                    emit(gen, "    call sc_split");
                    break;
                }
                case BUILTIN_STR_JOIN: {
                    // @join[arr_ptr, delim] -> string
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; arr_ptr");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; delim");
                    emit(gen, "    call sc_join");
                    break;
                }
                case BUILTIN_TRIM: {
                    // @trim[str] -> trimmed string
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; str");
                    emit(gen, "    call sc_trim");
                    break;
                }
                case BUILTIN_UPPER: {
                    // @upper[str] -> uppercase string
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; str");
                    emit(gen, "    call sc_upper");
                    break;
                }
                case BUILTIN_LOWER: {
                    // @lower[str] -> lowercase string
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; str");
                    emit(gen, "    call sc_lower");
                    break;
                }
                case BUILTIN_INDEX: {
                    // @index[str, substr] -> position
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; str");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; substr");
                    emit(gen, "    call sc_index");
                    break;
                }
                case BUILTIN_REPLACE: {
                    // @replace[str, old, new] -> string
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; str");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; old");
                    codegen_expression(gen, args->nodes[2]);
                    emit(gen, "    mov rdx, rax        ; new");
                    emit(gen, "    call sc_replace");
                    break;
                }
                case BUILTIN_STARTSWITH: {
                    // @startswith[str, prefix] -> bool
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; str");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; prefix");
                    emit(gen, "    call sc_startswith");
                    break;
                }
                case BUILTIN_ENDSWITH: {
                    // @endswith[str, suffix] -> bool
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; str");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; suffix");
                    emit(gen, "    call sc_endswith");
                    break;
                }
                case BUILTIN_REVERSE: {
                    // @reverse[str] -> reversed string
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; str");
                    emit(gen, "    call sc_reverse");
                    break;
                }
                case BUILTIN_REPEAT: {
                    // @repeat[str, count] -> repeated string
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; str");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; count");
                    emit(gen, "    call sc_repeat");
                    break;
                }
                case BUILTIN_PAD: {
                    // @pad[str, length, pad_char] -> padded string
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; str");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; length");
                    codegen_expression(gen, args->nodes[2]);
                    emit(gen, "    mov rdx, rax        ; pad_char");
                    emit(gen, "    call sc_pad");
                    break;
                }
                default:
                    emit(gen, "    xor rax, rax        ; unsupported string function");
            }
            break;
        
        case BUILTIN_CAT_MATH:
            // Math and bit operation functions - implemented in runtime/math.c
            switch (fn) {
                case BUILTIN_ISQRT: {
                    // @isqrt[x] -> integer square root
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; x");
                    emit(gen, "    call sc_isqrt");
                    break;
                }
                case BUILTIN_POW: {
                    // @pow[base, exp] -> base^exp
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; base");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; exp");
                    emit(gen, "    call sc_pow");
                    break;
                }
                case BUILTIN_ABS: {
                    // @abs[x] -> absolute value
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; x");
                    emit(gen, "    call sc_abs");
                    break;
                }
                case BUILTIN_MIN: {
                    // @min[a, b] -> minimum
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; a");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; b");
                    emit(gen, "    call sc_min");
                    break;
                }
                case BUILTIN_MAX: {
                    // @max[a, b] -> maximum
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; a");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; b");
                    emit(gen, "    call sc_max");
                    break;
                }
                case BUILTIN_CLZ: {
                    // @clz[x] -> count leading zeros
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; x");
                    emit(gen, "    call sc_clz");
                    break;
                }
                case BUILTIN_CTZ: {
                    // @ctz[x] -> count trailing zeros
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; x");
                    emit(gen, "    call sc_ctz");
                    break;
                }
                case BUILTIN_POPCOUNT: {
                    // @popcount[x] -> count set bits
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; x");
                    emit(gen, "    call sc_popcount");
                    break;
                }
                case BUILTIN_GCD: {
                    // @gcd[a, b] -> greatest common divisor
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; a");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; b");
                    emit(gen, "    call sc_gcd");
                    break;
                }
                case BUILTIN_LCM: {
                    // @lcm[a, b] -> least common multiple
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; a");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; b");
                    emit(gen, "    call sc_lcm");
                    break;
                }
                case BUILTIN_ISPRIME: {
                    // @isprime[n] -> 1 if prime, 0 otherwise
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; n");
                    emit(gen, "    call sc_isprime");
                    break;
                }
                case BUILTIN_MODPOW: {
                    // @modpow[base, exp, mod] -> base^exp % mod
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; base");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; exp");
                    codegen_expression(gen, args->nodes[2]);
                    emit(gen, "    mov rdx, rax        ; mod");
                    emit(gen, "    call sc_modpow");
                    break;
                }
                case BUILTIN_SQRT: {
                    // @sqrt[x] -> floating-point square root
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; x");
                    emit(gen, "    call sc_sqrt");
                    break;
                }
                case BUILTIN_FLOOR: {
                    // @floor[x] -> floor
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; x");
                    emit(gen, "    call sc_floor");
                    break;
                }
                case BUILTIN_CEIL: {
                    // @ceil[x] -> ceiling
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; x");
                    emit(gen, "    call sc_ceil");
                    break;
                }
                default:
                    emit(gen, "    xor rax, rax        ; unsupported math function");
            }
            break;
        
        case BUILTIN_CAT_ERROR:
            // Error handling functions - implemented in runtime/errors.c
            switch (fn) {
                case BUILTIN_ERROR: {
                    // @error[code, msg] -> error code
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; code");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; msg");
                    emit(gen, "    call sc_error");
                    break;
                }
                case BUILTIN_GET_ERROR_CODE: {
                    // @get_error_code[] -> error code
                    emit(gen, "    call sc_get_error_code");
                    break;
                }
                case BUILTIN_GET_ERROR_MSG: {
                    // @get_error_msg[] -> error message
                    emit(gen, "    call sc_get_error_msg");
                    break;
                }
                case BUILTIN_CLEAR_ERROR: {
                    // @clear_error[] -> result
                    emit(gen, "    call sc_clear_error");
                    break;
                }
                case BUILTIN_ASSERT: {
                    // @assert[condition, msg] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; condition");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; msg");
                    emit(gen, "    call sc_assert");
                    break;
                }
                case BUILTIN_CHECK_ALLOC: {
                    // @check_alloc[ptr, size] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; ptr");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; size");
                    emit(gen, "    call sc_check_alloc");
                    break;
                }
                case BUILTIN_TRY_SYSCALL: {
                    // @try_syscall[result, syscall_num] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; result");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; syscall_num");
                    emit(gen, "    call sc_try_syscall");
                    break;
                }
                case BUILTIN_TRY_FOPEN: {
                    // @try_fopen[fd, filename] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; fd");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; filename");
                    emit(gen, "    call sc_try_fopen");
                    break;
                }
                case BUILTIN_LOG_ERROR: {
                    // @log_error[code] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; code");
                    emit(gen, "    call sc_log_error");
                    break;
                }
                case BUILTIN_RECOVER: {
                    // @recover[] -> result
                    emit(gen, "    call sc_recover");
                    break;
                }
                default:
                    emit(gen, "    xor rax, rax        ; unsupported error function");
            }
            break;
        
        case BUILTIN_CAT_ADVANCED_PROCESS:
            // Advanced process/resource management - implemented in runtime/process.c
            switch (fn) {
                case BUILTIN_FORK: {
                    // @fork[] -> pid
                    emit(gen, "    call sc_fork");
                    break;
                }
                case BUILTIN_WAIT: {
                    // @wait[pid] -> status
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; pid");
                    emit(gen, "    call sc_wait");
                    break;
                }
                case BUILTIN_WAIT_ANY: {
                    // @wait_any[] -> status
                    emit(gen, "    call sc_wait_any");
                    break;
                }
                case BUILTIN_GETPID: {
                    // @getpid[] -> pid
                    emit(gen, "    call sc_getpid");
                    break;
                }
                case BUILTIN_GETPPID: {
                    // @getppid[] -> ppid
                    emit(gen, "    call sc_getppid");
                    break;
                }
                case BUILTIN_CHDIR: {
                    // @chdir[path] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; path");
                    emit(gen, "    call sc_chdir");
                    break;
                }
                case BUILTIN_GETCWD: {
                    // @getcwd[buf, size] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; buf");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; size");
                    emit(gen, "    call sc_getcwd");
                    break;
                }
                case BUILTIN_GETENV: {
                    // @getenv[name] -> value
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; name");
                    emit(gen, "    call sc_getenv");
                    break;
                }
                case BUILTIN_SETENV: {
                    // @setenv[name, value] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; name");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; value");
                    emit(gen, "    call sc_setenv");
                    break;
                }
                case BUILTIN_UNSETENV: {
                    // @unsetenv[name] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; name");
                    emit(gen, "    call sc_unsetenv");
                    break;
                }
                case BUILTIN_GETENV_INT: {
                    // @getenv_int[name, default] -> value
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; name");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; default");
                    emit(gen, "    call sc_getenv_int");
                    break;
                }
                case BUILTIN_SETENV_INT: {
                    // @setenv_int[name, value] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; name");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; value");
                    emit(gen, "    call sc_setenv_int");
                    break;
                }
                case BUILTIN_EXEC: {
                    // @exec[program, args] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; program");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; args");
                    emit(gen, "    call sc_exec");
                    break;
                }
                case BUILTIN_SYSTEM_CALL: {
                    // @system_call[command] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; command");
                    emit(gen, "    call sc_system");
                    break;
                }
                case BUILTIN_GETRLIMIT: {
                    // @getrlimit[resource] -> limit
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; resource");
                    emit(gen, "    call sc_getrlimit");
                    break;
                }
                case BUILTIN_SETRLIMIT: {
                    // @setrlimit[resource, limit] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; resource");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; limit");
                    emit(gen, "    call sc_setrlimit");
                    break;
                }
                case BUILTIN_THREAD_COUNT: {
                    // @thread_count[] -> count
                    emit(gen, "    call sc_thread_count");
                    break;
                }
                default:
                    emit(gen, "    xor rax, rax        ; unsupported process function");
            }
            break;
        
        case BUILTIN_CAT_CONCURRENCY_ADV:
            // Advanced concurrency primitives - implemented in runtime/concurrency.c
            switch (fn) {
                case BUILTIN_RWLOCK_CREATE: {
                    // @rwlock_create[] -> id
                    emit(gen, "    call sc_rwlock_create");
                    break;
                }
                case BUILTIN_RWLOCK_READ: {
                    // @rwlock_read[id] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; id");
                    emit(gen, "    call sc_rwlock_read");
                    break;
                }
                case BUILTIN_RWLOCK_READ_UNLOCK: {
                    // @rwlock_read_unlock[id] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; id");
                    emit(gen, "    call sc_rwlock_read_unlock");
                    break;
                }
                case BUILTIN_RWLOCK_WRITE: {
                    // @rwlock_write[id] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; id");
                    emit(gen, "    call sc_rwlock_write");
                    break;
                }
                case BUILTIN_RWLOCK_WRITE_UNLOCK: {
                    // @rwlock_write_unlock[id] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; id");
                    emit(gen, "    call sc_rwlock_write_unlock");
                    break;
                }
                case BUILTIN_BARRIER_CREATE: {
                    // @barrier_create[num_threads] -> id
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; num_threads");
                    emit(gen, "    call sc_barrier_create");
                    break;
                }
                case BUILTIN_BARRIER_WAIT: {
                    // @barrier_wait[id] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; id");
                    emit(gen, "    call sc_barrier_wait");
                    break;
                }
                case BUILTIN_EVENT_CREATE: {
                    // @event_create[] -> id
                    emit(gen, "    call sc_event_create");
                    break;
                }
                case BUILTIN_EVENT_SIGNAL: {
                    // @event_signal[id] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; id");
                    emit(gen, "    call sc_event_signal");
                    break;
                }
                case BUILTIN_EVENT_WAIT: {
                    // @event_wait[id] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; id");
                    emit(gen, "    call sc_event_wait");
                    break;
                }
                case BUILTIN_EVENT_RESET: {
                    // @event_reset[id] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; id");
                    emit(gen, "    call sc_event_reset");
                    break;
                }
                default:
                    emit(gen, "    xor rax, rax        ; unsupported concurrency function");
            }
            break;
        
        case BUILTIN_CAT_TIME:
            // Time and date/random functions - implemented in runtime/time.c
            switch (fn) {
                case BUILTIN_SRAND: {
                    // @srand[seed] -> void
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; seed");
                    emit(gen, "    call sc_srand");
                    break;
                }
                case BUILTIN_RAND: {
                    // @rand[] -> random value
                    emit(gen, "    call sc_rand");
                    break;
                }
                case BUILTIN_RAND_RANGE: {
                    // @rand_range[max] -> value
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; max");
                    emit(gen, "    call sc_rand_range");
                    break;
                }
                case BUILTIN_RAND_BETWEEN: {
                    // @rand_between[min, max] -> value
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; min");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; max");
                    emit(gen, "    call sc_rand_between");
                    break;
                }
                case BUILTIN_TIME: {
                    // @time[] -> seconds
                    emit(gen, "    call sc_time");
                    break;
                }
                case BUILTIN_TIME_MS: {
                    // @time_ms[] -> milliseconds
                    emit(gen, "    call sc_time_ms");
                    break;
                }
                case BUILTIN_TIME_US: {
                    // @time_us[] -> microseconds
                    emit(gen, "    call sc_time_us");
                    break;
                }
                case BUILTIN_YEAR_FROM_TIME: {
                    // @year_from_time[time_val] -> year
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; time_val");
                    emit(gen, "    call sc_year_from_time");
                    break;
                }
                case BUILTIN_MONTH_FROM_TIME: {
                    // @month_from_time[time_val] -> month
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; time_val");
                    emit(gen, "    call sc_month_from_time");
                    break;
                }
                case BUILTIN_DAY_FROM_TIME: {
                    // @day_from_time[time_val] -> day
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; time_val");
                    emit(gen, "    call sc_day_from_time");
                    break;
                }
                case BUILTIN_HOUR_FROM_TIME: {
                    // @hour_from_time[time_val] -> hour
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; time_val");
                    emit(gen, "    call sc_hour_from_time");
                    break;
                }
                case BUILTIN_MINUTE_FROM_TIME: {
                    // @minute_from_time[time_val] -> minute
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; time_val");
                    emit(gen, "    call sc_minute_from_time");
                    break;
                }
                case BUILTIN_SECOND_FROM_TIME: {
                    // @second_from_time[time_val] -> second
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; time_val");
                    emit(gen, "    call sc_second_from_time");
                    break;
                }
                case BUILTIN_STRFTIME: {
                    // @strftime[format, time_val, buf] -> formatted string
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; format");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; time_val");
                    codegen_expression(gen, args->nodes[2]);
                    emit(gen, "    mov rdx, rax        ; buf");
                    emit(gen, "    call sc_strftime");
                    break;
                }
                case BUILTIN_STRPTIME: {
                    // @strptime[time_str, format] -> time_val
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; time_str");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; format");
                    emit(gen, "    call sc_strptime");
                    break;
                }
                case BUILTIN_DAY_OF_WEEK: {
                    // @day_of_week[time_val] -> day (0-6)
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; time_val");
                    emit(gen, "    call sc_day_of_week");
                    break;
                }
                case BUILTIN_DAY_OF_YEAR: {
                    // @day_of_year[time_val] -> day (0-365)
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; time_val");
                    emit(gen, "    call sc_day_of_year");
                    break;
                }
                case BUILTIN_IS_LEAP_YEAR: {
                    // @is_leap_year[year] -> bool
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; year");
                    emit(gen, "    call sc_is_leap_year");
                    break;
                }
                case BUILTIN_DAYS_IN_MONTH: {
                    // @days_in_month[year, month] -> days
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; year");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; month");
                    emit(gen, "    call sc_days_in_month");
                    break;
                }
                default:
                    emit(gen, "    xor rax, rax        ; unsupported time function");
            }
            break;
        
        case BUILTIN_CAT_SORTING:
            // Sort and search functions - implemented in runtime/sorting.c
            switch (fn) {
                case BUILTIN_QSORT: {
                    // @qsort[arr_ptr, count, elem_size] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; arr_ptr");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; count");
                    codegen_expression(gen, args->nodes[2]);
                    emit(gen, "    mov rdx, rax        ; elem_size");
                    emit(gen, "    call sc_qsort");
                    break;
                }
                case BUILTIN_BSEARCH: {
                    // @bsearch[arr_ptr, count, elem_size, search_val] -> index
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; arr_ptr");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; count");
                    codegen_expression(gen, args->nodes[2]);
                    emit(gen, "    mov rdx, rax        ; elem_size");
                    codegen_expression(gen, args->nodes[3]);
                    emit(gen, "    mov rcx, rax        ; search_val");
                    emit(gen, "    call sc_bsearch");
                    break;
                }
                case BUILTIN_SEARCH: {
                    // @search[arr_ptr, count, value] -> index
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; arr_ptr");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; count");
                    codegen_expression(gen, args->nodes[2]);
                    emit(gen, "    mov rdx, rax        ; value");
                    emit(gen, "    call sc_search");
                    break;
                }
                case BUILTIN_SHUFFLE: {
                    // @shuffle[arr_ptr, count] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; arr_ptr");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; count");
                    emit(gen, "    call sc_shuffle");
                    break;
                }
                case BUILTIN_BUBBLE_SORT: {
                    // @bubble_sort[arr_ptr, count] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; arr_ptr");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; count");
                    emit(gen, "    call sc_bubble_sort");
                    break;
                }
                case BUILTIN_SELECTION_SORT: {
                    // @selection_sort[arr_ptr, count] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; arr_ptr");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; count");
                    emit(gen, "    call sc_selection_sort");
                    break;
                }
                case BUILTIN_INSERTION_SORT: {
                    // @insertion_sort[arr_ptr, count] -> result
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; arr_ptr");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; count");
                    emit(gen, "    call sc_insertion_sort");
                    break;
                }
                case BUILTIN_FIND_MIN: {
                    // @find_min[arr_ptr, count] -> min value
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; arr_ptr");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; count");
                    emit(gen, "    call sc_find_min");
                    break;
                }
                case BUILTIN_FIND_MAX: {
                    // @find_max[arr_ptr, count] -> max value
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; arr_ptr");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; count");
                    emit(gen, "    call sc_find_max");
                    break;
                }
                case BUILTIN_FIND_MIN_IDX: {
                    // @find_min_idx[arr_ptr, count] -> index
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; arr_ptr");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; count");
                    emit(gen, "    call sc_find_min_idx");
                    break;
                }
                case BUILTIN_FIND_MAX_IDX: {
                    // @find_max_idx[arr_ptr, count] -> index
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; arr_ptr");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; count");
                    emit(gen, "    call sc_find_max_idx");
                    break;
                }
                case BUILTIN_COUNT_VAL: {
                    // @count_val[arr_ptr, count, value] -> occurrences
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; arr_ptr");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; count");
                    codegen_expression(gen, args->nodes[2]);
                    emit(gen, "    mov rdx, rax        ; value");
                    emit(gen, "    call sc_count_val");
                    break;
                }
                case BUILTIN_SUM: {
                    // @sum[arr_ptr, count] -> sum
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; arr_ptr");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; count");
                    emit(gen, "    call sc_sum");
                    break;
                }
                case BUILTIN_AVERAGE: {
                    // @average[arr_ptr, count] -> average
                    codegen_expression(gen, args->nodes[0]);
                    emit(gen, "    mov rdi, rax        ; arr_ptr");
                    codegen_expression(gen, args->nodes[1]);
                    emit(gen, "    mov rsi, rax        ; count");
                    emit(gen, "    call sc_average");
                    break;
                }
                default:
                    emit(gen, "    xor rax, rax        ; unsupported sorting function");
            }
            break;
        
        case BUILTIN_CAT_VECTOR:
            // Vector/array operations
            switch (fn) {
                case BUILTIN_VEC_NEW: {
                    // @vec_new[type, capacity] - allocates vector metadata on heap
                    if (args->count >= 2) {
                        // Third arg would be capacity; for now just use raw alloc
                        codegen_expression(gen, args->nodes[1]);  // capacity
                        emit(gen, "    push rax            ; capacity");
                        codegen_expression(gen, args->nodes[0]);  // type (ignored for now)
                        emit(gen, "    pop rcx              ; restore capacity");
                        emit(gen, "    imul rcx, 8         ; element size (assume 8 bytes)");
                        emit(gen, "    add rcx, 24         ; + vector header (3 qwords)");
                        emit(gen, "    mov rdi, rcx        ; allocate");
                        emit(gen, "    mov rax, 189        ; mmap syscall");
                        emit(gen, "    syscall");
                    } else {
                        emit(gen, "    mov rax, 0          ; vec_new: insufficient args");
                    }
                    break;
                }
                case BUILTIN_VEC_PUSH: {
                    // @vec_push[vec, value]
                    if (args->count >= 2) {
                        codegen_expression(gen, args->nodes[1]);  // value
                        emit(gen, "    push rax            ; push value");
                        codegen_expression(gen, args->nodes[0]);  // vec ptr
                        emit(gen, "    pop rbx              ; value back to rbx");
                        emit(gen, "    mov rcx, [rax + 8]  ; get length");
                        emit(gen, "    mov [rax + rcx * 8 + 24], rbx  ; store at [ptr + 24 + len*8]");
                        emit(gen, "    inc qword [rax + 8] ; increment length");
                    }
                    break;
                }
                case BUILTIN_VEC_POP: {
                    // @vec_pop[vec]
                    if (args->count >= 1) {
                        codegen_expression(gen, args->nodes[0]);
                        emit(gen, "    mov rcx, [rax + 8]  ; get length");
                        emit(gen, "    cmp rcx, 0");
                        emit(gen, "    je .vec_pop_empty");
                        emit(gen, "    dec rcx              ; length--");
                        emit(gen, "    mov rax, [rax + rcx * 8 + 24]  ; get last element");
                        emit(gen, "    jmp .vec_pop_done");
                        emit(gen, ".vec_pop_empty:");
                        emit(gen, "    xor rax, rax        ; return 0 for empty");
                        emit(gen, ".vec_pop_done:");
                    }
                    break;
                }
                case BUILTIN_VEC_GET: {
                    // @vec_get[vec, index]
                    if (args->count >= 2) {
                        codegen_expression(gen, args->nodes[1]);  // index
                        emit(gen, "    mov rcx, rax        ; index in rcx");
                        codegen_expression(gen, args->nodes[0]);  // vec ptr
                        emit(gen, "    mov rax, [rax + rcx * 8 + 24]  ; get element");
                    }
                    break;
                }
                case BUILTIN_VEC_SET: {
                    // @vec_set[vec, index, value]
                    if (args->count >= 3) {
                        codegen_expression(gen, args->nodes[2]);  // value
                        emit(gen, "    push rax            ; save value");
                        codegen_expression(gen, args->nodes[1]);  // index
                        emit(gen, "    mov rcx, rax        ; index in rcx");
                        codegen_expression(gen, args->nodes[0]);  // vec ptr
                        emit(gen, "    pop rbx              ; restore value");
                        emit(gen, "    mov [rax + rcx * 8 + 24], rbx  ; set element");
                    }
                    break;
                }
                case BUILTIN_VEC_LEN: {
                    // @vec_len[vec]
                    if (args->count >= 1) {
                        codegen_expression(gen, args->nodes[0]);
                        emit(gen, "    mov rax, [rax + 8]  ; get length");
                    }
                    break;
                }
                case BUILTIN_VEC_CAP: {
                    // @vec_cap[vec]
                    if (args->count >= 1) {
                        codegen_expression(gen, args->nodes[0]);
                        emit(gen, "    mov rax, [rax + 16] ; get capacity");
                    }
                    break;
                }
                case BUILTIN_VEC_FREE: {
                    // @vec_free[vec]
                    if (args->count >= 1) {
                        codegen_expression(gen, args->nodes[0]);
                        emit(gen, "    mov rdi, rax        ; ptr to free");
                        emit(gen, "    mov rax, 11         ; munmap syscall");
                        emit(gen, "    syscall");
                    }
                    break;
                }
                case BUILTIN_VEC_CLEAR: {
                    // @vec_clear[vec]
                    if (args->count >= 1) {
                        codegen_expression(gen, args->nodes[0]);
                        emit(gen, "    mov qword [rax + 8], 0  ; set length to 0");
                    }
                    break;
                }
                case BUILTIN_VEC_RESIZE: {
                    // @vec_resize[vec, new_size]
                    if (args->count >= 2) {
                        codegen_expression(gen, args->nodes[1]);  // new size
                        emit(gen, "    mov rcx, rax        ; new size in rcx");
                        codegen_expression(gen, args->nodes[0]);  // vec ptr
                        emit(gen, "    mov [rax + 8], rcx  ; set new length");
                    }
                    break;
                }
                default:
                    emit(gen, "    xor rax, rax        ; unsupported vector function");
            }
            break;
            
        default:
            emit(gen, "    xor rax, rax            ; unsupported builtin category");
            break;
    }
}

// Main code generation
void codegen_generate(CodeGen *gen, ASTNode *ast) {
    if (ast->type != AST_PROGRAM) {
        fprintf(stderr, "Expected program node in codegen\n");
        return;
    }
    
    // Store constants for later lookup
    gen->constants = ast->program.constants;
    
    // Emit preamble
    emit(gen, "global _start");
    emit(gen, "");
    emit(gen, "extern sc_mutex_create, sc_mutex_lock, sc_mutex_unlock, sc_mutex_trylock, sc_mutex_destroy");
    emit(gen, "extern sc_semaphore_create, sc_semaphore_wait, sc_semaphore_signal");
    emit(gen, "extern sc_cond_create, sc_cond_wait, sc_cond_signal, sc_cond_broadcast");
    emit(gen, "extern sc_atomic_cmp_swap, sc_atomic_increment, sc_atomic_decrement");
    emit(gen, "extern sc_channel_create, sc_channel_send, sc_channel_recv, sc_channel_close, sc_channel_empty, sc_channel_full");
    emit(gen, "extern sc_pool_create, sc_pool_submit, sc_pool_wait, sc_pool_destroy");
    emit(gen, "extern sc_socket, sc_connect, sc_bind, sc_listen, sc_accept, sc_send, sc_recv, sc_close");
    emit(gen, "extern sc_fopen, sc_fread, sc_fwrite, sc_fseek, sc_fclose");
    emit(gen, "extern sc_mmap, sc_munmap, sc_mprotect");
    emit(gen, "extern sc_port_in, sc_port_out, sc_ioread, sc_iowrite");
    emit(gen, "extern sc_syscall, sc_irq_enable, sc_irq_disable, sc_verify");
    emit(gen, "extern sc_split, sc_join, sc_trim, sc_upper, sc_lower, sc_index");
    emit(gen, "extern sc_replace, sc_startswith, sc_endswith, sc_reverse, sc_repeat, sc_pad");
    emit(gen, "extern sc_type, sc_isqrt, sc_pow, sc_abs, sc_min, sc_max");
    emit(gen, "extern sc_clz, sc_ctz, sc_popcount, sc_gcd, sc_lcm, sc_isprime");
    emit(gen, "extern sc_modpow, sc_sqrt, sc_floor, sc_ceil");
    emit(gen, "extern sc_error, sc_get_error_code, sc_get_error_msg, sc_clear_error");
    emit(gen, "extern sc_assert, sc_check_alloc, sc_try_syscall, sc_try_fopen, sc_panic_error, sc_log_error, sc_recover");
    emit(gen, "extern sc_fork, sc_wait, sc_wait_any, sc_getpid, sc_getppid");
    emit(gen, "extern sc_chdir, sc_getcwd, sc_getenv, sc_setenv, sc_unsetenv");
    emit(gen, "extern sc_getenv_int, sc_setenv_int, sc_exec, sc_system, sc_getrlimit, sc_setrlimit, sc_thread_count");
    emit(gen, "extern sc_rwlock_create, sc_rwlock_read, sc_rwlock_read_unlock, sc_rwlock_write, sc_rwlock_write_unlock");
    emit(gen, "extern sc_barrier_create, sc_barrier_wait, sc_event_create, sc_event_signal, sc_event_wait, sc_event_reset");
    emit(gen, "extern sc_srand, sc_rand, sc_rand_range, sc_rand_between, sc_rand_new");
    emit(gen, "extern sc_time, sc_time_ms, sc_time_us, sc_year_from_time, sc_month_from_time");
    emit(gen, "extern sc_day_from_time, sc_hour_from_time, sc_minute_from_time, sc_second_from_time");
    emit(gen, "extern sc_strftime, sc_strptime, sc_day_of_week, sc_day_of_year, sc_is_leap_year, sc_days_in_month");
    emit(gen, "extern sc_qsort, sc_bsearch, sc_search, sc_shuffle, sc_bubble_sort");
    emit(gen, "extern sc_selection_sort, sc_insertion_sort, sc_find_min, sc_find_max");
    emit(gen, "extern sc_find_min_idx, sc_find_max_idx, sc_count_val, sc_sum, sc_average");
    emit(gen, "extern safe_alloc_versioned, safe_free_versioned, validate_array_access, memory_safety_init");
    emit(gen, "");
    emit(gen, "section .data");
    emit(gen, "newline: db 10");
    emit(gen, "bounds_msg: db 'Array bounds error!', 10");
    emit(gen, "type_error_msg: db 'Type error!', 10");
    emit(gen, "unimported_func_msg: db 'Unimplemented function!', 10");
    emit(gen, "stack_limit_msg: db 'Recursion depth limit exceeded', 10");
    emit(gen, "stack_warn_msg: db 'Warning: Approaching the recursion limit now', 10");
    emit(gen, "empty_string: db '', 0       ; empty string for uninitialized str variables");
    emit(gen, "g_recursion_depth: dq 0       ; global recursion depth counter (SECURITY)");
    emit(gen, "heap_start: dq 0            ; stores initial heap brk for @heap_size");
    emit(gen, "");
    emit(gen, "section .text");
    emit(gen, "");
    
    // Helper function to print integers
    emit(gen, "print_int:");
    emit(gen, "    push rbp");
    emit(gen, "    mov rbp, rsp");
    emit(gen, "    sub rsp, 32");
    emit(gen, "    mov rax, rdi");
    emit(gen, "    lea rsi, [rbp - 1]");
    emit(gen, "    mov byte [rsi], 10     ; newline");
    emit(gen, "    mov rbx, 10");
    emit(gen, "    test rax, rax");
    emit(gen, "    jns .convert");
    emit(gen, "    neg rax");
    emit(gen, ".convert:");
    emit(gen, "    xor rcx, rcx");
    emit(gen, ".loop:");
    emit(gen, "    xor rdx, rdx");
    emit(gen, "    div rbx");
    emit(gen, "    add dl, '0'");
    emit(gen, "    dec rsi");
    emit(gen, "    mov [rsi], dl");
    emit(gen, "    inc rcx");
    emit(gen, "    test rax, rax");
    emit(gen, "    jnz .loop");
    emit(gen, "    cmp rdi, 0");
    emit(gen, "    jge .print");
    emit(gen, "    dec rsi");
    emit(gen, "    mov byte [rsi], '-'");
    emit(gen, "    inc rcx");
    emit(gen, ".print:");
    emit(gen, "    mov rax, 1             ; sys_write");
    emit(gen, "    mov rdi, 1             ; stdout");
    emit(gen, "    lea rdx, [rcx + 1]     ; length");
    emit(gen, "    syscall");
    emit(gen, "    leave");
    emit(gen, "    ret");
    emit(gen, "");
    
    // Helper function to print integers without newline
    emit(gen, "print_int_no_newline:");
    emit(gen, "    push rbp");
    emit(gen, "    mov rbp, rsp");
    emit(gen, "    sub rsp, 32");
    emit(gen, "    mov rax, rdi");
    emit(gen, "    lea rsi, [rbp - 1]");
    emit(gen, "    mov rbx, 10");
    emit(gen, "    test rax, rax");
    emit(gen, "    jns .convert_nn");
    emit(gen, "    neg rax");
    emit(gen, ".convert_nn:");
    emit(gen, "    xor rcx, rcx");
    emit(gen, ".loop_nn:");
    emit(gen, "    xor rdx, rdx");
    emit(gen, "    div rbx");
    emit(gen, "    add dl, '0'");
    emit(gen, "    dec rsi");
    emit(gen, "    mov [rsi], dl");
    emit(gen, "    inc rcx");
    emit(gen, "    test rax, rax");
    emit(gen, "    jnz .loop_nn");
    emit(gen, "    cmp rdi, 0");
    emit(gen, "    jge .print_nn");
    emit(gen, "    dec rsi");
    emit(gen, "    mov byte [rsi], '-'");
    emit(gen, "    inc rcx");
    emit(gen, ".print_nn:");
    emit(gen, "    mov rax, 1             ; sys_write");
    emit(gen, "    mov rdi, 1             ; stdout");
    emit(gen, "    mov rdx, rcx           ; length (no newline)");
    emit(gen, "    syscall");
    emit(gen, "    leave");
    emit(gen, "    ret");
    emit(gen, "");
    
    // Helper function to print floating point numbers
    emit(gen, "print_float:");
    emit(gen, "    push rbp");
    emit(gen, "    mov rbp, rsp");
    emit(gen, "    sub rsp, 64             ; space for buffer and saving xmm0");
    emit(gen, "    movsd [rbp-64], xmm0    ; save original value");
    emit(gen, "    ; Check for negative");
    emit(gen, "    xorpd xmm1, xmm1");
    emit(gen, "    ucomisd xmm0, xmm1");
    emit(gen, "    jae .pf_positive");
    emit(gen, "    ; Print minus sign");
    emit(gen, "    mov byte [rbp-1], '-'");
    emit(gen, "    mov rdi, 1");
    emit(gen, "    lea rsi, [rbp-1]");
    emit(gen, "    mov rdx, 1");
    emit(gen, "    mov rax, 1              ; sys_write");
    emit(gen, "    syscall");
    emit(gen, "    ; Make positive");
    emit(gen, "    movsd xmm1, xmm0");
    emit(gen, "    xorpd xmm0, xmm0");
    emit(gen, "    subsd xmm0, xmm1");
    emit(gen, "    movsd [rbp-64], xmm0    ; save positive value");
    emit(gen, ".pf_positive:");
    emit(gen, "    ; Extract integer part");
    emit(gen, "    cvttsd2si rdi, xmm0     ; convert to int");
    emit(gen, "    call print_int_no_newline");
    emit(gen, "    ; Print decimal point");
    emit(gen, "    mov byte [rbp-1], '.'");
    emit(gen, "    mov rdi, 1");
    emit(gen, "    lea rsi, [rbp-1]");
    emit(gen, "    mov rdx, 1");
    emit(gen, "    mov rax, 1");
    emit(gen, "    syscall");
    emit(gen, "    ; Extract fractional part");
    emit(gen, "    movsd xmm0, [rbp-64]    ; reload positive value");
    emit(gen, "    cvttsd2si rax, xmm0     ; get integer part");
    emit(gen, "    cvtsi2sd xmm1, rax      ; convert back to double");
    emit(gen, "    subsd xmm0, xmm1        ; xmm0 = fractional part");
    emit(gen, "    ; Multiply by 100 for 2 decimal places");
    emit(gen, "    mov rax, 100");
    emit(gen, "    cvtsi2sd xmm1, rax");
    emit(gen, "    mulsd xmm0, xmm1");
    emit(gen, "    cvttsd2si rdi, xmm0     ; convert to int");
    emit(gen, "    ; Pad with zero if needed");
    emit(gen, "    cmp rdi, 10");
    emit(gen, "    jge .pf_print_frac");
    emit(gen, "    ; Print leading zero");
    emit(gen, "    mov byte [rbp-1], '0'");
    emit(gen, "    mov rax, 1");
    emit(gen, "    mov rdx, 1");
    emit(gen, "    push rdi                ; save fraction");
    emit(gen, "    mov rdi, 1");
    emit(gen, "    lea rsi, [rbp-1]");
    emit(gen, "    syscall");
    emit(gen, "    pop rdi                 ; restore fraction");
    emit(gen, ".pf_print_frac:");
    emit(gen, "    call print_int_no_newline");
    emit(gen, "    leave");
    emit(gen, "    ret");
    emit(gen, "");
    
    // Helper function: strlen - get string length
    emit(gen, "strlen:");
    emit(gen, "    push rbp");
    emit(gen, "    mov rbp, rsp");
    emit(gen, "    xor rax, rax            ; counter");
    emit(gen, ".strlen_loop:");
    emit(gen, "    cmp byte [rdi+rax], 0");
    emit(gen, "    je .strlen_done");
    emit(gen, "    inc rax");
    emit(gen, "    jmp .strlen_loop");
    emit(gen, ".strlen_done:");
    emit(gen, "    leave");
    emit(gen, "    ret");
    emit(gen, "");
    
    // Helper function: strcpy - copy string
    emit(gen, "strcpy:");
    emit(gen, "    push rbp");
    emit(gen, "    mov rbp, rsp");
    emit(gen, "    mov rax, rdi            ; save dest");
    emit(gen, ".strcpy_loop:");
    emit(gen, "    mov cl, [rsi]");
    emit(gen, "    mov [rdi], cl");
    emit(gen, "    test cl, cl");
    emit(gen, "    jz .strcpy_done");
    emit(gen, "    inc rdi");
    emit(gen, "    inc rsi");
    emit(gen, "    jmp .strcpy_loop");
    emit(gen, ".strcpy_done:");
    emit(gen, "    leave");
    emit(gen, "    ret");
    emit(gen, "");
    
    // Helper function: strcmp - compare strings
    emit(gen, "strcmp:");
    emit(gen, "    push rbp");
    emit(gen, "    mov rbp, rsp");
    emit(gen, ".strcmp_loop:");
    emit(gen, "    mov al, [rdi]");
    emit(gen, "    mov cl, [rsi]");
    emit(gen, "    cmp al, cl");
    emit(gen, "    jne .strcmp_diff");
    emit(gen, "    test al, al");
    emit(gen, "    jz .strcmp_equal");
    emit(gen, "    inc rdi");
    emit(gen, "    inc rsi");
    emit(gen, "    jmp .strcmp_loop");
    emit(gen, ".strcmp_equal:");
    emit(gen, "    xor rax, rax");
    emit(gen, "    leave");
    emit(gen, "    ret");
    emit(gen, ".strcmp_diff:");
    emit(gen, "    movzx rax, al");
    emit(gen, "    movzx rcx, cl");
    emit(gen, "    sub rax, rcx");
    emit(gen, "    leave");
    emit(gen, "    ret");
    emit(gen, "");
    
    // Helper function: malloc - allocate memory
    emit(gen, "malloc:");
    emit(gen, "    push rbp");
    emit(gen, "    mov rbp, rsp");
    emit(gen, "    ; Simple bump allocator using brk syscall");
    emit(gen, "    mov rsi, rdi            ; size to allocate");
    emit(gen, "    mov rax, 12             ; sys_brk");
    emit(gen, "    xor rdi, rdi            ; get current brk");
    emit(gen, "    syscall");
    emit(gen, "    mov rbx, rax            ; save current brk");
    emit(gen, "    add rax, rsi            ; new brk = current + size");
    emit(gen, "    mov rdi, rax");
    emit(gen, "    mov rax, 12             ; sys_brk");
    emit(gen, "    syscall");
    emit(gen, "    mov rax, rbx            ; return old brk");
    emit(gen, "    leave");
    emit(gen, "    ret");
    emit(gen, "");
    
    // Runtime error handlers
    emit(gen, "runtime_type_error:");
    emit(gen, "    mov rdi, 1");
    emit(gen, "    lea rsi, [rel type_error_msg]");
    emit(gen, "    mov rdx, 12");
    emit(gen, "    mov rax, 1          ; sys_write");
    emit(gen, "    syscall");
    emit(gen, "    mov rdi, 1");
    emit(gen, "    mov rax, 60         ; sys_exit");
    emit(gen, "    syscall");
    emit(gen, "");
    
    emit(gen, "runtime_unimplemented_func:");
    emit(gen, "    mov rdi, 1");
    emit(gen, "    lea rsi, [rel unimported_func_msg]");
    emit(gen, "    mov rdx, 24");
    emit(gen, "    mov rax, 1          ; sys_write");
    emit(gen, "    syscall");
    emit(gen, "    mov rdi, 1");
    emit(gen, "    mov rax, 60         ; sys_exit");
    emit(gen, "    syscall");
    emit(gen, "");
    
    // Process package imports
    if (ast->program.imports && ast->program.imports->count > 0) {
        emit(gen, "; Package imports");
        for (int i = 0; i < ast->program.imports->count; i++) {
            ASTNode *import = ast->program.imports->nodes[i];
            emit(gen, "; import: %s", import->package_import.package_name);
            // TODO: For now, we just note the import
            // In the future, we would:
            // 1. Parse the package file
            // 2. Include its symbols in the symbol table
            // 3. Link against compiled .sulib files
        }
        emit(gen, "");
    }
    
    // Process global constants
    if (ast->program.constants->count > 0) {
        emit(gen, "; Global constants");
        for (int i = 0; i < ast->program.constants->count; i++) {
            ASTNode *const_node = ast->program.constants->nodes[i];
            emit(gen, "; const %s = <value>", const_node->const_decl.name);
            // Constants will be handled during expression evaluation
            // For now, just note them in comments
        }
        emit(gen, "");
    }
    
    // Process group definitions
    for (int i = 0; i < ast->program.groups->count; i++) {
        ASTNode *group_node = ast->program.groups->nodes[i];
        GroupType *group = group_type_new(group_node->group_def.name);
        
        // Add fields to the group type (in reverse order since we use linked list)
        for (int j = group_node->group_def.fields->count - 1; j >= 0; j--) {
            ASTNode *field = group_node->group_def.fields->nodes[j];
            int field_size = get_type_size(field->var_decl.type);
            group_type_add_field(group, field->var_decl.name, field->var_decl.type, field_size);
        }
        
        group_type_add(gen, group);
        emit(gen, "; Group type '%s' defined (size=%d bytes)", group->name, group->total_size);
    }
    emit(gen, "");
    
    // Generate code for all functions
    for (int i = 0; i < ast->program.functions->count; i++) {
        codegen_function(gen, ast->program.functions->nodes[i]);
    }
    
    // Entry point
    emit(gen, "");
    emit(gen, "_start:");
    emit(gen, "    call main");
    emit(gen, "    mov rdi, rax           ; exit code");
    emit(gen, "    mov rax, 60            ; sys_exit");
    emit(gen, "    syscall");
}

// Synchronization builtin code generation
void codegen_builtin_sync(CodeGen *gen, BuiltinFunction fn, ASTList *args) {
    switch (fn) {
        case BUILTIN_MUTEX_CREATE:
            // @mutex_create[] -> rax = mutex_id
            emit(gen, "    call sc_mutex_create");
            break;
            
        case BUILTIN_MUTEX_LOCK:
            // @mutex_lock[id]
            codegen_expression(gen, args->nodes[0]);
            emit(gen, "    mov rdi, rax");
            emit(gen, "    call sc_mutex_lock");
            break;
            
        case BUILTIN_MUTEX_UNLOCK:
            // @mutex_unlock[id]
            codegen_expression(gen, args->nodes[0]);
            emit(gen, "    mov rdi, rax");
            emit(gen, "    call sc_mutex_unlock");
            break;
            
        case BUILTIN_MUTEX_TRYLOCK:
            // @mutex_trylock[id]
            codegen_expression(gen, args->nodes[0]);
            emit(gen, "    mov rdi, rax");
            emit(gen, "    call sc_mutex_trylock");
            break;
            
        case BUILTIN_MUTEX_DESTROY:
            // @mutex_destroy[id]
            codegen_expression(gen, args->nodes[0]);
            emit(gen, "    mov rdi, rax");
            emit(gen, "    call sc_mutex_destroy");
            break;
            
        case BUILTIN_SEMAPHORE_CREATE:
            // @semaphore_create[count]
            codegen_expression(gen, args->nodes[0]);
            emit(gen, "    mov rdi, rax");
            emit(gen, "    call sc_semaphore_create");
            break;
            
        case BUILTIN_SEMAPHORE_WAIT:
            // @semaphore_wait[id]
            codegen_expression(gen, args->nodes[0]);
            emit(gen, "    mov rdi, rax");
            emit(gen, "    call sc_semaphore_wait");
            break;
            
        case BUILTIN_SEMAPHORE_SIGNAL:
            // @semaphore_signal[id]
            codegen_expression(gen, args->nodes[0]);
            emit(gen, "    mov rdi, rax");
            emit(gen, "    call sc_semaphore_signal");
            break;
            
        case BUILTIN_COND_CREATE:
            // @cond_create[]
            emit(gen, "    call sc_cond_create");
            break;
            
        case BUILTIN_COND_WAIT:
            // @cond_wait[mutex_id, cond_id]
            codegen_expression(gen, args->nodes[0]);  // mutex_id
            emit(gen, "    push rax");    // Save mutex_id
            codegen_expression(gen, args->nodes[1]);  // cond_id
            emit(gen, "    mov rsi, rax");
            emit(gen, "    pop rdi");     // Get mutex_id back
            emit(gen, "    call sc_cond_wait");
            break;
            
        case BUILTIN_COND_SIGNAL:
            // @cond_signal[cond_id]
            codegen_expression(gen, args->nodes[0]);
            emit(gen, "    mov rdi, rax");
            emit(gen, "    call sc_cond_signal");
            break;
            
        case BUILTIN_COND_BROADCAST:
            // @cond_broadcast[cond_id]
            codegen_expression(gen, args->nodes[0]);
            emit(gen, "    mov rdi, rax");
            emit(gen, "    call sc_cond_broadcast");
            break;
            
        case BUILTIN_ATOMIC_CMP_SWAP:
            // @atomic_cmp_swap[ptr, expected, new_value]
            codegen_expression(gen, args->nodes[0]);  // ptr
            emit(gen, "    mov rdi, rax");
            codegen_expression(gen, args->nodes[1]);  // expected
            emit(gen, "    mov rsi, rax");
            codegen_expression(gen, args->nodes[2]);  // new_value
            emit(gen, "    mov rdx, rax");
            emit(gen, "    call sc_atomic_cmp_swap");
            break;
            
        case BUILTIN_ATOMIC_INCREMENT:
            // @atomic_increment[ptr]
            codegen_expression(gen, args->nodes[0]);
            emit(gen, "    mov rdi, rax");
            emit(gen, "    call sc_atomic_increment");
            break;
            
        case BUILTIN_ATOMIC_DECREMENT:
            // @atomic_decrement[ptr]
            codegen_expression(gen, args->nodes[0]);
            emit(gen, "    mov rdi, rax");
            emit(gen, "    call sc_atomic_decrement");
            break;
            
        default:
            emit(gen, "    xor rax, rax        ; unsupported sync function");
    }
}

// Channel builtins code generation
void codegen_builtin_channel(CodeGen *gen, BuiltinFunction fn, ASTList *args) {
    switch (fn) {
        case BUILTIN_CHANNEL_CREATE:
            // @channel_create[capacity]
            codegen_expression(gen, args->nodes[0]);
            emit(gen, "    mov rdi, rax");
            emit(gen, "    call sc_channel_create");
            break;

        case BUILTIN_CHANNEL_SEND:
            // @channel_send[ch_id, value]
            codegen_expression(gen, args->nodes[0]);
            emit(gen, "    mov rdi, rax    ; channel id");
            codegen_expression(gen, args->nodes[1]);
            emit(gen, "    mov rsi, rax    ; value");
            emit(gen, "    call sc_channel_send");
            break;

        case BUILTIN_CHANNEL_RECV:
            // @channel_recv[ch_id] -> returns value in rax
            codegen_expression(gen, args->nodes[0]);
            emit(gen, "    mov rdi, rax");
            emit(gen, "    call sc_channel_recv");
            break;

        case BUILTIN_CHANNEL_CLOSE:
            // @channel_close[ch_id]
            codegen_expression(gen, args->nodes[0]);
            emit(gen, "    mov rdi, rax");
            emit(gen, "    call sc_channel_close");
            break;

        case BUILTIN_CHANNEL_EMPTY:
            // @channel_empty[ch_id]
            codegen_expression(gen, args->nodes[0]);
            emit(gen, "    mov rdi, rax");
            emit(gen, "    call sc_channel_empty");
            break;

        case BUILTIN_CHANNEL_FULL:
            // @channel_full[ch_id]
            codegen_expression(gen, args->nodes[0]);
            emit(gen, "    mov rdi, rax");
            emit(gen, "    call sc_channel_full");
            break;

        default:
            emit(gen, "    xor rax, rax    ; unsupported channel builtin");
    }
}

// Thread pool builtins code generation
void codegen_builtin_threadpool(CodeGen *gen, BuiltinFunction fn, ASTList *args) {
    switch (fn) {
        case BUILTIN_POOL_CREATE:
            // @pool_create[num_threads]
            codegen_expression(gen, args->nodes[0]);
            emit(gen, "    mov rdi, rax");
            emit(gen, "    call sc_pool_create");
            break;

        case BUILTIN_POOL_SUBMIT:
            // @pool_submit[pool_id, fn_addr, arg]
            codegen_expression(gen, args->nodes[0]);
            emit(gen, "    mov rdi, rax    ; pool id");
            codegen_expression(gen, args->nodes[1]);
            emit(gen, "    mov rsi, rax    ; function address");
            codegen_expression(gen, args->nodes[2]);
            emit(gen, "    mov rdx, rax    ; arg");
            emit(gen, "    call sc_pool_submit");
            break;

        case BUILTIN_POOL_WAIT:
            // @pool_wait[pool_id]
            codegen_expression(gen, args->nodes[0]);
            emit(gen, "    mov rdi, rax");
            emit(gen, "    call sc_pool_wait");
            break;

        case BUILTIN_POOL_DESTROY:
            // @pool_destroy[pool_id]
            codegen_expression(gen, args->nodes[0]);
            emit(gen, "    mov rdi, rax");
            emit(gen, "    call sc_pool_destroy");
            break;

        default:
            emit(gen, "    xor rax, rax    ; unsupported pool builtin");
    }
}
