/*
 * x86-64 Assembler - Pure RasCode Implementation
 * Converts NASM-syntax assembly to machine code and ELF64 binaries
 * No external tools required (no NASM, LD, etc.)
 */

#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdint.h>
#include <stddef.h>
#include "elfgen.h"

// Opaque pointer to internal assembler state
typedef void Assembler;

// Create new assembler instance
Assembler *asm_new(void);

// Parse and assemble from file
int asm_assemble_file(Assembler *asm_op, const char *input_file, 
                      const char *output_file);

// Parse and assemble from string
int asm_assemble_string(Assembler *asm_op, const char *source, 
                        const char *output_file);

// Cleanup
void asm_free(Assembler *asm_op);

// Debug: print assembly stats
void asm_print_stats(Assembler *asm_op);

#endif // ASSEMBLER_H
