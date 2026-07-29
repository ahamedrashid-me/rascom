/*
 * Pure RasCode ELF64 Generator
 * Generates x86-64 ELF executables directly without external tools
 * No dependency on gcc, nasm, ld, or libc
 */

#ifndef ELFGEN_H
#define ELFGEN_H

#include <stdint.h>

// ELF header structures for x86-64
typedef struct {
    uint32_t offset;      // Offset in file
    uint32_t vaddr;       // Virtual address
    uint32_t size;        // Size in file
    uint32_t mem_size;    // Size in memory
    uint32_t flags;       // Flags (R/W/X)
    uint32_t align;       // Alignment
} ELFSegment;

typedef struct {
    uint8_t *code;        // Generated machine code
    uint32_t code_size;   // Current code size
    uint32_t code_cap;    // Code buffer capacity
    
    ELFSegment segments[3]; // .text, .data, .bss
    int segment_count;
} ELFGenerator;

// Initialize ELF generator
ELFGenerator *elfgen_new(void);

// Add machine code bytes
void elfgen_add_code(ELFGenerator *gen, const uint8_t *bytes, uint32_t len);

// Emit x86-64 instruction helpers
void elfgen_emit_mov_rax_imm64(ELFGenerator *gen, uint64_t value);
void elfgen_emit_mov_rdi_imm64(ELFGenerator *gen, uint64_t value);
void elfgen_emit_mov_rsi_imm64(ELFGenerator *gen, uint64_t value);
void elfgen_emit_mov_rdx_imm64(ELFGenerator *gen, uint64_t value);
void elfgen_emit_mov_rcx_imm64(ELFGenerator *gen, uint64_t value);
void elfgen_emit_add_rax_imm64(ELFGenerator *gen, int64_t value);
void elfgen_emit_sub_rax_imm64(ELFGenerator *gen, int64_t value);
void elfgen_emit_mov_rax_rbp(ELFGenerator *gen);           // mov rax, [rbp]
void elfgen_emit_mov_rbp_rax(ELFGenerator *gen);           // mov [rbp], rax
void elfgen_emit_syscall(ELFGenerator *gen);
void elfgen_emit_call_rax(ELFGenerator *gen);              // call rax
void elfgen_emit_jmp_rax(ELFGenerator *gen);               // jmp rax
void elfgen_emit_ret(ELFGenerator *gen);
void elfgen_emit_push_rax(ELFGenerator *gen);
void elfgen_emit_pop_rax(ELFGenerator *gen);
void elfgen_emit_nop(ELFGenerator *gen);
void elfgen_emit_int3(ELFGenerator *gen);                  // breakpoint

// Generate ELF file
int elfgen_write_elf(ELFGenerator *gen, const char *output_file);

// Cleanup
void elfgen_free(ELFGenerator *gen);

#endif // ELFGEN_H
