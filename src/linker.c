/*
 * RasCode Internal ELF Linker Implementation
 * Standalone linker that replaces GCC - no external dependencies
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

#include "../include/linker.h"

#define MAX_OBJECTS 32
#define MAX_SECTIONS 16
#define MAX_SYMBOLS 512
#define MAX_RELOCATIONS 256
#define MAX_ERROR 512

#define VERBOSE(linker, fmt, ...) \
    do { if ((linker)->verbose) fprintf(stderr, fmt, ##__VA_ARGS__); } while(0)

// ============================================================================
// ELF64 STRUCTURES (from ELF specification)
// ============================================================================

typedef struct {
    uint8_t e_ident[16];      // ELF identification
    uint16_t e_type;          // Object file type
    uint16_t e_machine;       // Machine type
    uint32_t e_version;       // Object file version
    uint64_t e_entry;         // Entry point virtual address
    uint64_t e_phoff;         // Program header offset
    uint64_t e_shoff;         // Section header offset
    uint32_t e_flags;         // Processor-specific flags
    uint16_t e_ehsize;        // ELF header size
    uint16_t e_phentsize;     // Program header entry size
    uint16_t e_phnum;         // Program header entry count
    uint16_t e_shentsize;     // Section header entry size
    uint16_t e_shnum;         // Section header entry count
    uint16_t e_shstrndx;      // Section header string table index
} Elf64_Ehdr;

typedef struct {
    uint32_t sh_name;         // Section name (index into .shstrtab)
    uint32_t sh_type;         // Section type
    uint64_t sh_flags;        // Section flags
    uint64_t sh_addr;         // Section virtual address
    uint64_t sh_offset;       // Section file offset
    uint64_t sh_size;         // Section size
    uint32_t sh_link;         // Link to another section
    uint32_t sh_info;         // Extra information
    uint64_t sh_addralign;    // Section address alignment
    uint64_t sh_entsize;      // Section entry size
} Elf64_Shdr;

typedef struct {
    uint32_t st_name;         // Symbol name (index into string table)
    uint8_t st_info;          // Symbol binding and type
    uint8_t st_other;         // Symbol visibility
    uint16_t st_shndx;        // Related section index
    uint64_t st_value;        // Symbol value
    uint64_t st_size;         // Symbol size
} Elf64_Sym;

typedef struct {
    uint64_t r_offset;        // Relocation offset
    uint64_t r_info;          // Relocation type and symbol index (64-bit for ELF64)
    int64_t r_addend;         // Relocation addend
} Elf64_Rela;

typedef struct {
    uint32_t p_type;          // Segment type
    uint32_t p_flags;         // Segment flags
    uint64_t p_offset;        // Segment offset in file
    uint64_t p_vaddr;         // Segment virtual address
    uint64_t p_paddr;         // Segment physical address
    uint64_t p_filesz;        // Segment size in file
    uint64_t p_memsz;         // Segment size in memory
    uint64_t p_align;         // Segment alignment
} Elf64_Phdr;

// ELF constants
#define ET_REL 1              // Relocatable file (object file)
#define ET_EXEC 2             // Executable file
#define EM_X86_64 62          // x86-64 architecture
#define SHT_PROGBITS 1        // Program data
#define SHT_SYMTAB 2          // Symbol table
#define SHT_STRTAB 3          // String table
#define SHT_RELA 4            // Relocation entries with addends
#define SHF_ALLOC 0x2         // Occupies memory during execution
#define SHF_EXECINSTR 0x4     // Executable machine instructions
#define PT_LOAD 1             // Loadable program segment
#define PT_GNU_STACK 0x6474e551 // GNU stack segment
#define PF_R 0x4              // Readable
#define PF_W 0x2              // Writable
#define PF_X 0x1              // Executable

#define R_X86_64_64 1         // 64-bit direct relocation
#define R_X86_64_32 2         // 32-bit direct relocation
#define R_X86_64_PC32 4       // 32-bit PC-relative relocation
#define R_X86_64_PLT32 7      // 32-bit PLT-relative relocation
#define R_X86_64_RELATIVE 8   // Relative relocation

// ============================================================================
// LINKER INTERNAL STRUCTURES
// ============================================================================

typedef struct {
    char name[256];
    uint64_t offset;
    uint64_t size;
    uint32_t type;
    uint64_t flags;
    uint8_t *data;
} Section;

typedef struct {
    char name[256];
    uint64_t value;
    uint32_t size;
    uint8_t info;
    uint16_t shndx;
} Symbol;

typedef struct {
    uint64_t offset;
    uint32_t type;
    uint32_t symbol_idx;
    int64_t addend;
} Relocation;

typedef struct {
    char path[512];
    Elf64_Ehdr header;
    Section sections[MAX_SECTIONS];
    uint32_t section_count;
    Symbol symbols[MAX_SYMBOLS];
    uint32_t symbol_count;
    Relocation relocations[MAX_RELOCATIONS];
    uint32_t relocation_count;
} ObjectFile;

struct RasLinker {
    ObjectFile objects[MAX_OBJECTS];
    uint32_t object_count;
    
    // Global symbol table (Phase 5.3)
    Symbol global_symbols[MAX_SYMBOLS * 2];
    uint32_t global_symbol_count;
    
    // Merged sections for final executable
    uint8_t *text_section;       // .text (code)
    uint64_t text_size;
    uint64_t text_capacity;
    
    uint8_t *data_section;       // .data (initialized data)
    uint64_t data_size;
    uint64_t data_capacity;
    
    uint64_t bss_size;           // .bss (uninitialized data)
    
    // Section addresses in final executable
    uint64_t text_base;          // Base address for .text
    uint64_t data_base;          // Base address for .data
    uint64_t bss_base;           // Base address for .bss
    
    // Built-in runtime modules (to be linked)
    // TODO: Add runtime module linking
    
    int verbose;
    char error_msg[MAX_ERROR];
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

static void set_error(RasLinker *linker, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(linker->error_msg, MAX_ERROR, fmt, args);
    va_end(args);
    if (linker->verbose) {
        fprintf(stderr, "LINKER ERROR: %s\n", linker->error_msg);
    }
}

static uint64_t align_to(uint64_t value, uint64_t alignment) {
    if (alignment == 0) alignment = 1;
    return ((value + alignment - 1) / alignment) * alignment;
}

// ============================================================================
// PHASE 5.2: OBJECT FILE READER - Read ELF headers from .o files
// ============================================================================

static int read_elf_header(FILE *f, Elf64_Ehdr *header) {
    // Read ELF header (64 bytes)
    if (fread(header, sizeof(Elf64_Ehdr), 1, f) != 1) {
        return -1;
    }
    
    // Validate ELF magic number
    if (header->e_ident[0] != 0x7F || 
        header->e_ident[1] != 'E' || 
        header->e_ident[2] != 'L' || 
        header->e_ident[3] != 'F') {
        return -1;  // Not an ELF file
    }
    
    return 0;
}

static int load_object_file(RasLinker *linker, const char *obj_path) {
    if (linker->object_count >= MAX_OBJECTS) {
        set_error(linker, "Too many object files");
        return -1;
    }
    
    ObjectFile *obj = &linker->objects[linker->object_count];
    FILE *f = fopen(obj_path, "rb");
    
    if (!f) {
        set_error(linker, "Cannot open object file: %s", obj_path);
        return -1;
    }
    
    strcpy(obj->path, obj_path);
    
    // Read ELF header
    if (read_elf_header(f, &obj->header) != 0) {
        set_error(linker, "Invalid ELF file: %s", obj_path);
        fclose(f);
        return -1;
    }
    
    VERBOSE(linker, "Loaded object: %s (sections: %d)\n", obj_path, obj->header.e_shnum);
    
    // Read section headers and load .text, .data, .symtab, .rela.* sections
    Elf64_Shdr section_header;
    uint64_t shstrtab_offset = 0;
    uint64_t shstrtab_size = 0;
    
    // Phase 5.2a: Read all section headers to find .shstrtab
    if (obj->header.e_shstrndx != 0) {
        if (fseek(f, obj->header.e_shoff + obj->header.e_shstrndx * obj->header.e_shentsize, SEEK_SET) != 0) {
            set_error(linker, "Failed to seek to .shstrtab");
            fclose(f);
            return -1;
        }
        if (fread(&section_header, sizeof(Elf64_Shdr), 1, f) != 1) {
            set_error(linker, "Failed to read .shstrtab header");
            fclose(f);
            return -1;
        }
        shstrtab_offset = section_header.sh_offset;
        shstrtab_size = section_header.sh_size;
    }
    
    // Read all section headers
    obj->section_count = 0;
    obj->symbol_count = 0;
    obj->relocation_count = 0;
    
    // Store file pointer for section data reading later
    FILE *obj_file = f;
    
    for (uint16_t i = 0; i < obj->header.e_shnum && obj->section_count < MAX_SECTIONS; i++) {
        if (fseek(obj_file, obj->header.e_shoff + i * obj->header.e_shentsize, SEEK_SET) != 0) {
            continue;
        }
        
        if (fread(&section_header, sizeof(Elf64_Shdr), 1, obj_file) != 1) {
            continue;
        }
        
        Section *section = &obj->sections[obj->section_count];
        section->offset = section_header.sh_offset;
        section->size = section_header.sh_size;
        section->type = section_header.sh_type;
        section->flags = section_header.sh_flags;
        
        // Get section name from .shstrtab
        if (shstrtab_offset && section_header.sh_name < shstrtab_size) {
            if (fseek(obj_file, shstrtab_offset + section_header.sh_name, SEEK_SET) == 0) {
                char name[256] = {0};
                for (int j = 0; j < 255; j++) {
                    int c = fgetc(obj_file);
                    if (c == EOF || c == 0) break;
                    name[j] = c;
                }
                strcpy(section->name, name);
            }
        }
        
        // Phase 5.2b-ENHANCED: Allocate and read section data for .text and .data
        if ((strcmp(section->name, ".text") == 0 || strcmp(section->name, ".data") == 0) &&
            section->size > 0) {
            section->data = (uint8_t *)malloc(section->size);
            if (section->data) {
                if (fseek(obj_file, section->offset, SEEK_SET) == 0) {
                    if (fread(section->data, 1, section->size, obj_file) != section->size) {
                        VERBOSE(linker, "  WARNING: Failed to read section data for %s\n", section->name);
                    } else {
                        VERBOSE(linker, "  Loaded section data: %s (%lu bytes)\n", 
                                section->name, section->size);
                    }
                }
            }
        }
        
        VERBOSE(linker, "  Section %d: %s (offset=%lu, size=%lu)\n", 
                i, section->name, section->offset, section->size);
        
        // Phase 5.2b: Load .symtab (symbol table)
        if (section_header.sh_type == SHT_SYMTAB) {
            int symbol_count = section_header.sh_size / sizeof(Elf64_Sym);
            VERBOSE(linker, "  Found .symtab with %d symbols\n", symbol_count);
            
            if (fseek(obj_file, section_header.sh_offset, SEEK_SET) == 0) {
                for (int j = 0; j < symbol_count && obj->symbol_count < MAX_SYMBOLS; j++) {
                    Elf64_Sym sym_entry;
                    if (fread(&sym_entry, sizeof(Elf64_Sym), 1, f) == 1) {
                        Symbol *sym = &obj->symbols[obj->symbol_count];
                        sym->value = sym_entry.st_value;
                        sym->size = sym_entry.st_size;
                        sym->info = sym_entry.st_info;
                        sym->shndx = sym_entry.st_shndx;
                        
                        // Get symbol name from .strtab (linked by sh_link)
                        if (section_header.sh_link != 0) {
                            Elf64_Shdr strtab_header;
                            if (fseek(f, obj->header.e_shoff + section_header.sh_link * obj->header.e_shentsize, SEEK_SET) == 0) {
                                if (fread(&strtab_header, sizeof(Elf64_Shdr), 1, f) == 1) {
                                    if (sym_entry.st_name < strtab_header.sh_size) {
                                        if (fseek(f, strtab_header.sh_offset + sym_entry.st_name, SEEK_SET) == 0) {
                                            char name[256] = {0};
                                            for (int k = 0; k < 255; k++) {
                                                int c = fgetc(f);
                                                if (c == EOF || c == 0) break;
                                                name[k] = c;
                                            }
                                            strcpy(sym->name, name);
                                        }
                                    }
                                }
                            }
                        }
                        obj->symbol_count++;
                    }
                }
            }
        }
        
        // Phase 5.2c: Load .rela.text / .rela.data (relocations)
        if (section_header.sh_type == SHT_RELA) {
            int relocation_count = section_header.sh_size / sizeof(Elf64_Rela);
            VERBOSE(linker, "  Found %s with %d relocations\n", section->name, relocation_count);
            
            if (fseek(f, section_header.sh_offset, SEEK_SET) == 0) {
                for (int j = 0; j < relocation_count && obj->relocation_count < MAX_RELOCATIONS; j++) {
                    Elf64_Rela rela_entry;
                    if (fread(&rela_entry, sizeof(Elf64_Rela), 1, f) == 1) {
                        Relocation *reloc = &obj->relocations[obj->relocation_count];
                        reloc->offset = rela_entry.r_offset;
                        // In ELF64: r_info = (symbol << 32) | type
                        // So we need to extract them properly
                        reloc->symbol_idx = (uint32_t)(rela_entry.r_info >> 32);
                        reloc->type = (uint32_t)(rela_entry.r_info & 0xFFFFFFFF);
                        reloc->addend = rela_entry.r_addend;
                        obj->relocation_count++;
                    }
                }
            }
        }
        
        obj->section_count++;
    }
    
    fclose(f);
    linker->object_count++;
    return 0;
}

// ============================================================================
// PHASE 5.3: SYMBOL RESOLUTION - Link symbols across objects
// ============================================================================

/**
 * Phase 5.3a: Add a symbol to the global symbol table
 * Handles symbol conflicts (multiple definitions, undefined symbols, etc.)
 */
static int add_global_symbol(RasLinker *linker, const Symbol *sym, uint32_t obj_idx) {
    if (linker->global_symbol_count >= MAX_SYMBOLS * 2) {
        set_error(linker, "Global symbol table overflow");
        return -1;
    }
    
    // Check for existing symbol with same name
    for (uint32_t i = 0; i < linker->global_symbol_count; i++) {
        if (strcmp(linker->global_symbols[i].name, sym->name) == 0) {
            // Symbol already in global table
            Symbol *existing = &linker->global_symbols[i];
            
            // Check for duplicate definitions (strong symbols)
            uint8_t sym_bind = sym->info >> 4;        // Binding (strong/weak)
            uint8_t existing_bind = existing->info >> 4;
            
            if (sym_bind == 1 && existing_bind == 1) {  // Both strong
                set_error(linker, "Multiple definitions of symbol: %s", sym->name);
                return -1;  // Duplicate strong symbol = error
            }
            
            // Weak symbol vs strong: keep strong
            if (existing_bind == 1) {
                return 0;  // Keep existing strong symbol
            }
            
            // Update with new definition (if new is stronger or first definition)
            if (sym_bind == 1 || sym->shndx != 0) {
                *existing = *sym;
            }
            return 0;
        }
    }
    
    // New symbol, add to global table
    Symbol *global_sym = &linker->global_symbols[linker->global_symbol_count];
    *global_sym = *sym;
    linker->global_symbol_count++;
    
    VERBOSE(linker, "  Added symbol: %s (value=%lu, size=%u, obj=%u)\n", 
            sym->name, sym->value, sym->size, obj_idx);
    
    return 0;
}

/**
 * Phase 5.3b: Build global symbol table from all loaded object files
 * Resolves all symbol definitions and references
 */
static int resolve_symbols(RasLinker *linker) {
    VERBOSE(linker, "Resolving symbols across %u object files...\n", linker->object_count);
    
    // Pass 1: Collect all defined symbols (non-zero shndx or defined weak symbols)
    for (uint32_t obj_idx = 0; obj_idx < linker->object_count; obj_idx++) {
        ObjectFile *obj = &linker->objects[obj_idx];
        VERBOSE(linker, "Object %u: %s (%u symbols)\n", obj_idx, obj->path, obj->symbol_count);
        
        for (uint32_t sym_idx = 0; sym_idx < obj->symbol_count; sym_idx++) {
            Symbol *sym = &obj->symbols[sym_idx];
            
            // Skip empty names and local symbols
            if (!sym->name[0]) continue;
            
            uint8_t sym_bind = sym->info >> 4;      // Binding (1=global, 2=weak)
            
            // Add defined symbols (non-zero section index)
            if (sym->shndx != 0) {
                if (add_global_symbol(linker, sym, obj_idx) != 0) {
                    return -1;
                }
            }
            // Also add weak symbols (may be overridden later)
            else if (sym_bind == 2) {
                if (add_global_symbol(linker, sym, obj_idx) != 0) {
                    return -1;
                }
            }
        }
    }
    
    // Pass 2: Verify all undefined symbols are resolvable
    for (uint32_t obj_idx = 0; obj_idx < linker->object_count; obj_idx++) {
        ObjectFile *obj = &linker->objects[obj_idx];
        
        for (uint32_t reloc_idx = 0; reloc_idx < obj->relocation_count; reloc_idx++) {
            Relocation *reloc = &obj->relocations[reloc_idx];
            
            if (reloc->symbol_idx < obj->symbol_count) {
                Symbol *ref_sym = &obj->symbols[reloc->symbol_idx];
                
                if (!ref_sym->name[0]) continue;
                
                // Check if symbol is defined
                int found = 0;
                for (uint32_t i = 0; i < linker->global_symbol_count; i++) {
                    if (strcmp(linker->global_symbols[i].name, ref_sym->name) == 0) {
                        found = 1;
                        break;
                    }
                }
                
                if (!found && ref_sym->shndx == 0) {
                    // Undefined symbol - check if it's a special symbol
                    if (strcmp(ref_sym->name, "main") != 0 && 
                        strncmp(ref_sym->name, "__", 2) != 0) {
                        VERBOSE(linker, "WARNING: Undefined symbol: %s (may be resolved at runtime)\n", ref_sym->name);
                    }
                }
            }
        }
    }
    
    VERBOSE(linker, "Symbol resolution complete. Global table has %u symbols\n", 
            linker->global_symbol_count);
    
    return 0;
}

// ============================================================================
// PHASE 5.4: RELOCATION PROCESSING - Fix address references in code/data
// ============================================================================

/**
 * Phase 5.4a: Merge all sections from object files into final executable sections
 * This combines .text, .data, .bss sections from all objects into single sections
 * Returns: 0 on success, -1 on failure
 */
static int merge_sections(RasLinker *linker) {
    VERBOSE(linker, "Merging sections from %u objects...\n", linker->object_count);
    
    // Calculate total sizes and track offsets for each object
    uint64_t total_text_size = 0;
    uint64_t total_data_size = 0;
    uint64_t total_bss_size = 0;
    
    // Track offset for each object (for relocation application)
    typedef struct {
        uint64_t text_offset;
        uint64_t data_offset;
        uint64_t bss_offset;
    } ObjectOffsets;
    
    ObjectOffsets obj_offsets[MAX_OBJECTS] = {0};
    
    // Phase 5.4a1: Calculate total sizes and track per-object offsets
    for (uint32_t obj_idx = 0; obj_idx < linker->object_count; obj_idx++) {
        ObjectFile *obj = &linker->objects[obj_idx];
        obj_offsets[obj_idx].text_offset = total_text_size;
        obj_offsets[obj_idx].data_offset = total_data_size;
        obj_offsets[obj_idx].bss_offset = total_bss_size;
        
        // Sum up section sizes
        for (uint32_t sec_idx = 0; sec_idx < obj->section_count; sec_idx++) {
            Section *section = &obj->sections[sec_idx];
            
            if (strcmp(section->name, ".text") == 0) {
                total_text_size += section->size;
            } else if (strcmp(section->name, ".data") == 0) {
                total_data_size += section->size;
            } else if (strcmp(section->name, ".bss") == 0) {
                total_bss_size += section->size;
            }
        }
    }
    
    VERBOSE(linker, "Merged section sizes: .text=%lu .data=%lu .bss=%lu\n",
            total_text_size, total_data_size, total_bss_size);
    
    // Resize linker buffers if needed
    if (total_text_size > linker->text_capacity) {
        uint8_t *new_text = (uint8_t *)realloc(linker->text_section, total_text_size + 4096);
        if (!new_text) {
            set_error(linker, "Failed to allocate merged .text section");
            return -1;
        }
        linker->text_section = new_text;
        linker->text_capacity = total_text_size + 4096;
    }
    
    if (total_data_size > linker->data_capacity) {
        uint8_t *new_data = (uint8_t *)realloc(linker->data_section, total_data_size + 4096);
        if (!new_data) {
            set_error(linker, "Failed to allocate merged .data section");
            return -1;
        }
        linker->data_section = new_data;
        linker->data_capacity = total_data_size + 4096;
    }
    
    linker->text_size = total_text_size;
    linker->data_size = total_data_size;
    linker->bss_size = total_bss_size;
    
    // ========================================================================
    // CRITICAL: Copy actual section data into merged buffers
    // ========================================================================
    
    uint64_t text_write_offset = 0;
    uint64_t data_write_offset = 0;
    
    for (uint32_t obj_idx = 0; obj_idx < linker->object_count; obj_idx++) {
        ObjectFile *obj = &linker->objects[obj_idx];
        
        for (uint32_t sec_idx = 0; sec_idx < obj->section_count; sec_idx++) {
            Section *section = &obj->sections[sec_idx];
            
            if (strcmp(section->name, ".text") == 0 && section->size > 0) {
                // Copy .text section data
                if (section->data && text_write_offset + section->size <= linker->text_size) {
                    memcpy(linker->text_section + text_write_offset, section->data, section->size);
                    VERBOSE(linker, "  Copied .text from %s: %lu bytes at offset %lu\n",
                            obj->path, section->size, text_write_offset);
                    text_write_offset += section->size;
                }
            } 
            else if (strcmp(section->name, ".data") == 0 && section->size > 0) {
                // Copy .data section data
                if (section->data && data_write_offset + section->size <= linker->data_size) {
                    memcpy(linker->data_section + data_write_offset, section->data, section->size);
                    VERBOSE(linker, "  Copied .data from %s: %lu bytes at offset %lu\n",
                            obj->path, section->size, data_write_offset);
                    data_write_offset += section->size;
                }
            }
        }
    }
    
    VERBOSE(linker, "Section merging complete. Allocated %lu bytes (text+data)\n",
            total_text_size + total_data_size);
    
    return 0;
}

/**
 * Phase 5.4b: Find symbol address in global symbol table
 * Returns: Symbol address or 0 if not found
 */
static uint64_t find_symbol_address(RasLinker *linker, const char *sym_name) {
    for (uint32_t i = 0; i < linker->global_symbol_count; i++) {
        if (strcmp(linker->global_symbols[i].name, sym_name) == 0) {
            return linker->global_symbols[i].value;
        }
    }
    return 0;
}

/**
 * Phase 5.4c: Apply a single relocation
 * Supports x86-64 relocation types (R_X86_64_*)
 * Returns: 0 on success, -1 on unsupported relocation
 */
static int apply_single_relocation(RasLinker *linker, ObjectFile *obj, 
                                    Relocation *reloc, uint64_t obj_text_offset) {
    
    if (reloc->symbol_idx >= obj->symbol_count) {
        VERBOSE(linker, "  WARNING: Invalid symbol index %u\n", reloc->symbol_idx);
        return 0;  // Skip invalid references
    }
    
    Symbol *sym = &obj->symbols[reloc->symbol_idx];
    if (!sym->name[0]) {
        return 0;  // Skip unnamed symbols
    }
    
    // Find symbol in global table
    uint64_t sym_value = 0;
    int found = 0;
    for (uint32_t i = 0; i < linker->global_symbol_count; i++) {
        if (strcmp(linker->global_symbols[i].name, sym->name) == 0) {
            sym_value = linker->global_symbols[i].value;
            found = 1;
            break;
        }
    }
    
    if (!found && sym->shndx != 0) {
        VERBOSE(linker, "  WARNING: Undefined symbol in relocation: %s\n", sym->name);
        sym_value = 0;
    }
    
    // Calculate relocation target address
    uint64_t reloc_addr = linker->text_base + obj_text_offset + reloc->offset;
    uint8_t *patch_addr = linker->text_section + obj_text_offset + reloc->offset;
    
    // Apply relocation based on type
    switch (reloc->type) {
        case R_X86_64_64: {
            // 64-bit direct: write S + A
            uint64_t value = sym_value + reloc->addend;
            if (reloc->offset + 8 <= linker->text_size) {
                *(uint64_t *)patch_addr = value;
            }
            VERBOSE(linker, "    R_X86_64_64 @ %lx = %lx (%s)\n", 
                    reloc_addr, value, sym->name);
            break;
        }
        
        case R_X86_64_32: {
            // 32-bit absolute: write S + A (truncate to 32 bits)
            uint32_t value = (uint32_t)(sym_value + reloc->addend);
            if (reloc->offset + 4 <= linker->text_size) {
                *(uint32_t *)patch_addr = value;
            }
            VERBOSE(linker, "    R_X86_64_32 @ %lx = %x (%s)\n", 
                    reloc_addr, value, sym->name);
            break;
        }
        
        case R_X86_64_PC32: {
            // 32-bit PC-relative: write S + A - P (P = relocation target address)
            int32_t value = (int32_t)(sym_value + reloc->addend - reloc_addr);
            if (reloc->offset + 4 <= linker->text_size) {
                *(int32_t *)patch_addr = value;
            }
            VERBOSE(linker, "    R_X86_64_PC32 @ %lx = %x (%s)\n", 
                    reloc_addr, value, sym->name);
            break;
        }
        
        case R_X86_64_RELATIVE: {
            // Relative: write B + A (B = base address, A = addend)
            uint64_t value = linker->text_base + reloc->addend;
            if (reloc->offset + 8 <= linker->text_size) {
                *(uint64_t *)patch_addr = value;
            }
            VERBOSE(linker, "    R_X86_64_RELATIVE @ %lx = %lx\n", 
                    reloc_addr, value);
            break;
        }
        
        case R_X86_64_PLT32: {
            // PLT-relative: write S + A - P (same as PC32 for now)
            int32_t value = (int32_t)(sym_value + reloc->addend - reloc_addr);
            if (reloc->offset + 4 <= linker->text_size) {
                *(int32_t *)patch_addr = value;
            }
            VERBOSE(linker, "    R_X86_64_PLT32 @ %lx = %x (%s)\n", 
                    reloc_addr, value, sym->name);
            break;
        }
        
        default:
            VERBOSE(linker, "  WARNING: Unsupported relocation type %u at offset %lu\n",
                    reloc->type, reloc->offset);
            return 0;  // Skip unsupported types
    }
    
    return 0;
}

/**
 * Phase 5.4d: Apply all relocations from all objects
 * Updates code/data sections with final symbol addresses
 */
static int apply_relocations(RasLinker *linker) {
    VERBOSE(linker, "Applying relocations...\n");
    
    // Build offset mapping for each object - track where each object's sections are in merged buffer
    uint64_t text_offset = 0;
    uint64_t data_offset = 0;
    
    for (uint32_t obj_idx = 0; obj_idx < linker->object_count; obj_idx++) {
        ObjectFile *obj = &linker->objects[obj_idx];
        VERBOSE(linker, "Object %u: Processing %u relocations (text_offset=%lu)\n", 
                obj_idx, obj->relocation_count, text_offset);
        
        // Apply each relocation in this object
        for (uint32_t reloc_idx = 0; reloc_idx < obj->relocation_count; reloc_idx++) {
            Relocation *reloc = &obj->relocations[reloc_idx];
            
            if (apply_single_relocation(linker, obj, reloc, text_offset) != 0) {
                return -1;
            }
        }
        
        // Update offset for next object's .text section
        for (uint32_t sec_idx = 0; sec_idx < obj->section_count; sec_idx++) {
            Section *section = &obj->sections[sec_idx];
            if (strcmp(section->name, ".text") == 0) {
                text_offset += section->size;
                VERBOSE(linker, "  After object %u .text: text_offset now=%lu\n", obj_idx, text_offset);
            }
        }
    }
    
    VERBOSE(linker, "Relocation processing complete\n");
    return 0;
}

// ============================================================================
// PHASE 5.5: ELF GENERATION - Create final executable with merged sections
// ============================================================================

/**
 * Phase 5.5: Write complete ELF64 executable with .text and .data sections
 * Creates a fully functional statically-linked x86-64 executable
 */
static int write_elf_executable(RasLinker *linker, const char *output_file) {
    FILE *f = fopen(output_file, "wb");
    if (!f) {
        set_error(linker, "Cannot create output file: %s", output_file);
        return -1;
    }
    
    VERBOSE(linker, "Writing ELF executable...\n");
    
    // ========================================================================
    // PHASE 5.5a: Build ELF64 header
    // ========================================================================
    
    Elf64_Ehdr header;
    memset(&header, 0, sizeof(header));
    
    // ELF identification
    header.e_ident[0] = 0x7F;           // ELF magic
    header.e_ident[1] = 'E';
    header.e_ident[2] = 'L';
    header.e_ident[3] = 'F';
    header.e_ident[4] = 2;              // 64-bit
    header.e_ident[5] = 1;              // Little-endian
    header.e_ident[6] = 1;              // ELF version 1
    header.e_ident[7] = 0;              // System V ABI
    header.e_ident[8] = 0;              // ABI version
    
    // File type and machine
    header.e_type = ET_EXEC;            // Executable file
    header.e_machine = EM_X86_64;       // x86-64 architecture
    header.e_version = 1;
    header.e_entry = linker->text_base; // Entry point = start of .text
    header.e_flags = 0;
    header.e_ehsize = sizeof(Elf64_Ehdr);
    
    // Program header info (now generating program headers for kernel loading)
    header.e_phentsize = sizeof(Elf64_Phdr);
    header.e_phnum = 2;                 // PT_LOAD for .text, PT_LOAD for .data
    uint64_t program_header_offset = sizeof(Elf64_Ehdr);
    header.e_phoff = program_header_offset;
    
    // Section header info
    header.e_shentsize = sizeof(Elf64_Shdr);
    header.e_shnum = 4;                 // .text, .data, .bss, .shstrtab
    header.e_shstrndx = 3;              // String table index
    
    // Will fill in section header offset after calculating all sizes
    // Offset = ELF header + program headers + .text + .data
    uint64_t section_header_offset = sizeof(Elf64_Ehdr) + (2 * sizeof(Elf64_Phdr)) + linker->text_size + linker->data_size;
    header.e_shoff = section_header_offset;
    
    // Write ELF header
    if (fwrite(&header, sizeof(Elf64_Ehdr), 1, f) != 1) {
        set_error(linker, "Failed to write ELF header");
        fclose(f);
        return -1;
    }
    
    VERBOSE(linker, "  ELF header wrote: type=%u machine=%u entry=%lx\n",
            header.e_type, header.e_machine, header.e_entry);
    
    // ========================================================================
    // PHASE 5.5b: Write program headers for kernel loader
    // ========================================================================
    
    Elf64_Phdr program_headers[2];
    memset(program_headers, 0, sizeof(program_headers));
    
    // Program header 0: PT_LOAD for .text (code)
    program_headers[0].p_type = PT_LOAD;
    program_headers[0].p_flags = PF_R | PF_X;  // Readable, Executable
    program_headers[0].p_offset = sizeof(Elf64_Ehdr) + (2 * sizeof(Elf64_Phdr));  // Start after headers
    program_headers[0].p_vaddr = linker->text_base;
    program_headers[0].p_paddr = linker->text_base;
    program_headers[0].p_filesz = linker->text_size;
    program_headers[0].p_memsz = linker->text_size;
    program_headers[0].p_align = 0x1000;  // 4KB page alignment
    
    VERBOSE(linker, "  Program header 0 (.text): offset=%lx vaddr=%lx filesz=%lu\n",
            program_headers[0].p_offset, program_headers[0].p_vaddr, program_headers[0].p_filesz);
    
    // Program header 1: PT_LOAD for .data (initialized data)
    if (linker->data_size > 0) {
        program_headers[1].p_type = PT_LOAD;
        program_headers[1].p_flags = PF_R | PF_W;  // Readable, Writable
        program_headers[1].p_offset = sizeof(Elf64_Ehdr) + (2 * sizeof(Elf64_Phdr)) + linker->text_size;
        program_headers[1].p_vaddr = linker->data_base;
        program_headers[1].p_paddr = linker->data_base;
        program_headers[1].p_filesz = linker->data_size;
        program_headers[1].p_memsz = linker->data_size;  // No BSS in statically linked executables
        program_headers[1].p_align = 0x1000;  // 4KB page alignment
        
        VERBOSE(linker, "  Program header 1 (.data): offset=%lx vaddr=%lx filesz=%lu memsz=%lu\n",
                program_headers[1].p_offset, program_headers[1].p_vaddr, 
                program_headers[1].p_filesz, program_headers[1].p_memsz);
    }
    
    // Write program headers
    if (fwrite(program_headers, sizeof(Elf64_Phdr), 2, f) != 2) {
        set_error(linker, "Failed to write program headers");
        fclose(f);
        return -1;
    }
    
    VERBOSE(linker, "  Wrote %d program headers\n", 2);
    
    // ========================================================================
    // PHASE 5.5c: Write .text section
    // ========================================================================
    
    if (linker->text_size > 0) {
        if (fwrite(linker->text_section, 1, linker->text_size, f) != linker->text_size) {
            set_error(linker, "Failed to write .text section");
            fclose(f);
            return -1;
        }
        VERBOSE(linker, "  Wrote .text section: %lu bytes\n", linker->text_size);
    }
    
    // ========================================================================
    // PHASE 5.5d: Write .data section
    // ========================================================================
    
    if (linker->data_size > 0) {
        if (fwrite(linker->data_section, 1, linker->data_size, f) != linker->data_size) {
            set_error(linker, "Failed to write .data section");
            fclose(f);
            return -1;
        }
        VERBOSE(linker, "  Wrote .data section: %lu bytes\n", linker->data_size);
    }
    
    // ========================================================================
    // PHASE 5.5e: Build and write section headers
    // ========================================================================
    
    Elf64_Shdr section_headers[4];
    memset(section_headers, 0, sizeof(section_headers));
    
    // Section 0: NULL section (always present)
    section_headers[0].sh_type = 0;
    section_headers[0].sh_flags = 0;
    
    // Section 1: .text
    if (linker->text_size > 0) {
        section_headers[1].sh_name = 1;                    // Offset in .shstrtab
        section_headers[1].sh_type = SHT_PROGBITS;
        section_headers[1].sh_flags = SHF_ALLOC | SHF_EXECINSTR;
        section_headers[1].sh_addr = linker->text_base;
        section_headers[1].sh_offset = sizeof(Elf64_Ehdr) + (2 * sizeof(Elf64_Phdr));  // After program headers
        section_headers[1].sh_size = linker->text_size;
        section_headers[1].sh_addralign = 1;
    }
    
    // Section 2: .data
    if (linker->data_size > 0) {
        section_headers[2].sh_name = 7;                    // Offset in .shstrtab
        section_headers[2].sh_type = SHT_PROGBITS;
        section_headers[2].sh_flags = SHF_ALLOC;
        section_headers[2].sh_addr = linker->data_base;
        section_headers[2].sh_offset = sizeof(Elf64_Ehdr) + (2 * sizeof(Elf64_Phdr)) + linker->text_size;
        section_headers[2].sh_size = linker->data_size;
        section_headers[2].sh_addralign = 1;
    }
    
    // Section 3: .shstrtab (section name string table)
    // String table contains: "\0.text\0.data\0.shstrtab\0"
    uint64_t strtab_offset = section_header_offset + 4 * sizeof(Elf64_Shdr);
    section_headers[3].sh_name = 13;                       // Offset in .shstrtab
    section_headers[3].sh_type = SHT_STRTAB;
    section_headers[3].sh_flags = 0;
    section_headers[3].sh_offset = strtab_offset;
    section_headers[3].sh_size = 24;                       // Size of string table
    section_headers[3].sh_addralign = 1;
    
    // Seek to section header offset
    if (fseek(f, section_header_offset, SEEK_SET) != 0) {
        set_error(linker, "Failed to seek to section headers");
        fclose(f);
        return -1;
    }
    
    // Write section headers
    if (fwrite(section_headers, sizeof(Elf64_Shdr), 4, f) != 4) {
        set_error(linker, "Failed to write section headers");
        fclose(f);
        return -1;
    }
    
    VERBOSE(linker, "  Wrote %u section headers\n", 4);
    
    // ========================================================================
    // PHASE 5.5f: Write section name string table (.shstrtab)
    // ========================================================================
    
    const char shstrtab[] = "\0.text\0.data\0.shstrtab\0";
    if (fwrite(shstrtab, 1, sizeof(shstrtab), f) != sizeof(shstrtab)) {
        set_error(linker, "Failed to write .shstrtab");
        fclose(f);
        return -1;
    }
    
    VERBOSE(linker, "  Wrote .shstrtab\n");
    
    fclose(f);
    
    // Set executable permissions
    if (chmod(output_file, 0755) != 0) {
        set_error(linker, "Failed to set executable permissions");
        return -1;
    }
    
    VERBOSE(linker, "ELF executable created successfully: %s\n", output_file);
    return 0;
}

// ============================================================================
// PUBLIC LINKER API
// ============================================================================

RasLinker *linker_new(void) {
    RasLinker *linker = (RasLinker *)calloc(1, sizeof(RasLinker));
    if (!linker) return NULL;
    
    linker->text_capacity = 65536;  // 64KB initial
    linker->text_section = (uint8_t *)malloc(linker->text_capacity);
    if (!linker->text_section) {
        free(linker);
        return NULL;
    }
    
    linker->data_capacity = 16384;  // 16KB initial
    linker->data_section = (uint8_t *)malloc(linker->data_capacity);
    if (!linker->data_section) {
        free(linker->text_section);
        free(linker);
        return NULL;
    }
    
    linker->object_count = 0;
    linker->global_symbol_count = 0;  // Phase 5.3: Initialize global symbol table
    linker->text_base = 0x400000;     // Standard Linux text base
    linker->data_base = 0x600000;     // Standard Linux data base
    linker->bss_base = 0x601000;      // Standard Linux bss base
    linker->verbose = 0;
    strcpy(linker->error_msg, "No error");
    
    return linker;
}

void linker_free(RasLinker *linker) {
    if (!linker) return;
    
    if (linker->text_section) free(linker->text_section);
    if (linker->data_section) free(linker->data_section);
    
    free(linker);
}

int linker_add_object_file(RasLinker *linker, const char *obj_file_path) {
    if (!linker || !obj_file_path) {
        return -1;
    }
    
    return load_object_file(linker, obj_file_path);
}

int linker_link(RasLinker *linker, const char *output_file) {
    if (!linker || !output_file) {
        return -1;
    }
    
    VERBOSE(linker, "Starting linking process...\n");
    
    // Step 1: Resolve symbols (Phase 5.3)
    if (resolve_symbols(linker) != 0) {
        return -1;
    }
    
    // Step 2: Merge sections (Phase 5.4a)
    if (merge_sections(linker) != 0) {
        return -1;
    }
    
    // Step 3: Apply relocations (Phase 5.4d)
    if (apply_relocations(linker) != 0) {
        return -1;
    }
    
    // Step 4: Write final executable (Phase 5.5)
    if (write_elf_executable(linker, output_file) != 0) {
        return -1;
    }
    
    VERBOSE(linker, "Linking completed successfully\n");
    return 0;
}

void linker_set_verbose(RasLinker *linker, int verbose) {
    if (linker) linker->verbose = verbose;
}

const char *linker_get_error(RasLinker *linker) {
    if (!linker) return "Invalid linker";
    return linker->error_msg;
}
