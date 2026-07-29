/*
 * RasCode Internal ELF Linker
 * Replaces external GCC linker - links object files into executables
 * No dependency on GCC, LD, or external tools
 * 
 * Purpose: Read .o files from NASM/assembler, link them together,
 * and produce standalone ELF64 executables
 */

#ifndef RASCODE_LINKER_H
#define RASCODE_LINKER_H

#include <stdint.h>
#include <stddef.h>

// Forward declaration
typedef struct RasLinker RasLinker;

// ============================================================================
// LINKER API: Main functions to create, configure, and link object files
// ============================================================================

/**
 * Create new linker instance
 * Returns: Opaque linker context or NULL on error
 */
RasLinker *linker_new(void);

/**
 * Free linker and all associated memory
 */
void linker_free(RasLinker *linker);

/**
 * Add an object file (.o file) to be linked
 * Input: path to .o file (from NASM or internal assembler)
 * Returns: 0 on success, -1 on error
 * 
 * Multiple .o files can be added in sequence
 */
int linker_add_object_file(RasLinker *linker, const char *obj_file_path);

/**
 * Link all added object files and runtime modules into executable
 * Input: output_file - path where executable should be created
 * Returns: 0 on success, -1 on error
 * 
 * This performs:
 * 1. Symbol resolution (find symbols across all objects)
 * 2. Relocation processing (fix address references)
 * 3. Section merging (.text, .data, .bss)
 * 4. ELF header generation
 * 5. Final executable creation
 */
int linker_link(RasLinker *linker, const char *output_file);

/**
 * Enable verbose output for debugging
 */
void linker_set_verbose(RasLinker *linker, int verbose);

/**
 * Get last error message
 */
const char *linker_get_error(RasLinker *linker);

// ============================================================================
// INTERNAL DATA STRUCTURES (opaque to users)
// ============================================================================

// These are defined in linker.c and hidden from users
// Users only access them through the API functions above

#endif // RASCODE_LINKER_H
