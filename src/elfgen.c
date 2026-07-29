/*
 * Pure RASLang ELF64 Generator Implementation
 * Generates x86-64 ELF executables directly
 */

#include "elfgen.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

// ELF header constants
#define EI_NIDENT 16
#define ET_EXEC 2
#define EM_X86_64 62
#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define ELFOSABI_SYSV 0
#define EV_CURRENT 1

// Program header types
#define PT_LOAD 1

// Segment flags
#define PF_X 1
#define PF_W 2
#define PF_R 4

typedef struct {
    uint8_t e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} ELFHeader64;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} ProgramHeader64;

// Helper: write uint16_t in little-endian
static void write_u16(uint8_t *buf, uint16_t val) {
    buf[0] = val & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
}

// Helper: write uint32_t in little-endian
static void write_u32(uint8_t *buf, uint32_t val) {
    buf[0] = val & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
    buf[2] = (val >> 16) & 0xFF;
    buf[3] = (val >> 24) & 0xFF;
}

// Helper: write uint64_t in little-endian
static void write_u64(uint8_t *buf, uint64_t val) {
    buf[0] = val & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
    buf[2] = (val >> 16) & 0xFF;
    buf[3] = (val >> 24) & 0xFF;
    buf[4] = (val >> 32) & 0xFF;
    buf[5] = (val >> 40) & 0xFF;
    buf[6] = (val >> 48) & 0xFF;
    buf[7] = (val >> 56) & 0xFF;
}

ELFGenerator *elfgen_new(void) {
    ELFGenerator *gen = xmalloc(sizeof(ELFGenerator));
    gen->code_cap = 65536;  // 64KB initial buffer
    gen->code = xmalloc(gen->code_cap);
    gen->code_size = 0;
    gen->segment_count = 0;
    return gen;
}

void elfgen_add_code(ELFGenerator *gen, const uint8_t *bytes, uint32_t len) {
    if (gen->code_size + len > gen->code_cap) {
        gen->code_cap *= 2;
        gen->code = xrealloc(gen->code, gen->code_cap);
    }
    memcpy(gen->code + gen->code_size, bytes, len);
    gen->code_size += len;
}

void elfgen_emit_mov_rax_imm64(ELFGenerator *gen, uint64_t value) {
    // MOV rax, imm64: 48 b8 <8 bytes>
    uint8_t bytes[10];
    bytes[0] = 0x48;
    bytes[1] = 0xb8;
    write_u64(bytes + 2, value);
    elfgen_add_code(gen, bytes, 10);
}

void elfgen_emit_syscall(ELFGenerator *gen) {
    // SYSCALL instruction: 0f 05
    uint8_t bytes[2] = { 0x0f, 0x05 };
    elfgen_add_code(gen, bytes, 2);
}

void elfgen_emit_ret(ELFGenerator *gen) {
    // RET instruction: c3
    uint8_t bytes[1] = { 0xc3 };
    elfgen_add_code(gen, bytes, 1);
}

int elfgen_write_elf(ELFGenerator *gen, const char *output_file) {
    FILE *f = fopen(output_file, "wb");
    if (!f) {
        error("Failed to open output file for ELF generation");
        return -1;
    }

    // ELF header
    ELFHeader64 hdr;
    memset(&hdr, 0, sizeof(hdr));
    
    // e_ident
    hdr.e_ident[0] = 0x7f;  // ELF magic
    hdr.e_ident[1] = 'E';
    hdr.e_ident[2] = 'L';
    hdr.e_ident[3] = 'F';
    hdr.e_ident[4] = ELFCLASS64;
    hdr.e_ident[5] = ELFDATA2LSB;
    hdr.e_ident[6] = EV_CURRENT;
    hdr.e_ident[7] = ELFOSABI_SYSV;
    
    hdr.e_type = ET_EXEC;
    hdr.e_machine = EM_X86_64;
    hdr.e_version = 1;
    hdr.e_entry = 0x401000;  // Entry point (matches p_vaddr)
    hdr.e_phoff = sizeof(ELFHeader64);  // Program header offset
    hdr.e_shoff = 0;  // No section headers for now
    hdr.e_flags = 0;
    hdr.e_ehsize = sizeof(ELFHeader64);
    hdr.e_phentsize = sizeof(ProgramHeader64);
    hdr.e_phnum = 1;  // One loadable segment
    hdr.e_shentsize = 0;
    hdr.e_shnum = 0;
    hdr.e_shstrndx = 0;
    
    // Write ELF header
    fwrite(hdr.e_ident, 1, EI_NIDENT, f);
    write_u16((uint8_t*)&hdr.e_type, hdr.e_type);
    fwrite(&hdr.e_type, 2, 1, f);
    write_u16((uint8_t*)&hdr.e_machine, hdr.e_machine);
    fwrite(&hdr.e_machine, 2, 1, f);
    write_u32((uint8_t*)&hdr.e_version, hdr.e_version);
    fwrite(&hdr.e_version, 4, 1, f);
    write_u64((uint8_t*)&hdr.e_entry, hdr.e_entry);
    fwrite(&hdr.e_entry, 8, 1, f);
    write_u64((uint8_t*)&hdr.e_phoff, hdr.e_phoff);
    fwrite(&hdr.e_phoff, 8, 1, f);
    write_u64((uint8_t*)&hdr.e_shoff, hdr.e_shoff);
    fwrite(&hdr.e_shoff, 8, 1, f);
    write_u32((uint8_t*)&hdr.e_flags, hdr.e_flags);
    fwrite(&hdr.e_flags, 4, 1, f);
    write_u16((uint8_t*)&hdr.e_ehsize, hdr.e_ehsize);
    fwrite(&hdr.e_ehsize, 2, 1, f);
    write_u16((uint8_t*)&hdr.e_phentsize, hdr.e_phentsize);
    fwrite(&hdr.e_phentsize, 2, 1, f);
    write_u16((uint8_t*)&hdr.e_phnum, hdr.e_phnum);
    fwrite(&hdr.e_phnum, 2, 1, f);
    write_u16((uint8_t*)&hdr.e_shentsize, hdr.e_shentsize);
    fwrite(&hdr.e_shentsize, 2, 1, f);
    write_u16((uint8_t*)&hdr.e_shnum, hdr.e_shnum);
    fwrite(&hdr.e_shnum, 2, 1, f);
    write_u16((uint8_t*)&hdr.e_shstrndx, hdr.e_shstrndx);
    fwrite(&hdr.e_shstrndx, 2, 1, f);
    
    // Program header
    ProgramHeader64 phdr;
    phdr.p_type = PT_LOAD;
    phdr.p_flags = PF_R | PF_X | PF_W;  // Read + Execute + Write (for data section)
    phdr.p_offset = 0x1000;  // Code offset in file
    phdr.p_vaddr = 0x400000 + 0x1000;
    phdr.p_paddr = 0x400000 + 0x1000;
    phdr.p_filesz = gen->code_size;
    phdr.p_memsz = gen->code_size;
    phdr.p_align = 0x1000;
    
    write_u32((uint8_t*)&phdr.p_type, phdr.p_type);
    fwrite(&phdr.p_type, 4, 1, f);
    write_u32((uint8_t*)&phdr.p_flags, phdr.p_flags);
    fwrite(&phdr.p_flags, 4, 1, f);
    write_u64((uint8_t*)&phdr.p_offset, phdr.p_offset);
    fwrite(&phdr.p_offset, 8, 1, f);
    write_u64((uint8_t*)&phdr.p_vaddr, phdr.p_vaddr);
    fwrite(&phdr.p_vaddr, 8, 1, f);
    write_u64((uint8_t*)&phdr.p_paddr, phdr.p_paddr);
    fwrite(&phdr.p_paddr, 8, 1, f);
    write_u64((uint8_t*)&phdr.p_filesz, phdr.p_filesz);
    fwrite(&phdr.p_filesz, 8, 1, f);
    write_u64((uint8_t*)&phdr.p_memsz, phdr.p_memsz);
    fwrite(&phdr.p_memsz, 8, 1, f);
    write_u64((uint8_t*)&phdr.p_align, phdr.p_align);
    fwrite(&phdr.p_align, 8, 1, f);
    
    // Padding to code offset
    uint8_t padding[0x1000];
    memset(padding, 0, sizeof(padding));
    fwrite(padding, 1, 0x1000 - ftell(f), f);
    
    // Write code
    fwrite(gen->code, 1, gen->code_size, f);
    
    fclose(f);
    
    // Make executable
    chmod(output_file, 0755);
    
    return 0;
}

void elfgen_emit_mov_rdi_imm64(ELFGenerator *gen, uint64_t value) {
    // MOV rdi, imm64: 48 bf <8 bytes>
    uint8_t bytes[10];
    bytes[0] = 0x48;
    bytes[1] = 0xbf;
    write_u64(bytes + 2, value);
    elfgen_add_code(gen, bytes, 10);
}

void elfgen_emit_mov_rsi_imm64(ELFGenerator *gen, uint64_t value) {
    // MOV rsi, imm64: 48 be <8 bytes>
    uint8_t bytes[10];
    bytes[0] = 0x48;
    bytes[1] = 0xbe;
    write_u64(bytes + 2, value);
    elfgen_add_code(gen, bytes, 10);
}

void elfgen_emit_mov_rdx_imm64(ELFGenerator *gen, uint64_t value) {
    // MOV rdx, imm64: 48 ba <8 bytes>
    uint8_t bytes[10];
    bytes[0] = 0x48;
    bytes[1] = 0xba;
    write_u64(bytes + 2, value);
    elfgen_add_code(gen, bytes, 10);
}

void elfgen_emit_mov_rcx_imm64(ELFGenerator *gen, uint64_t value) {
    // MOV rcx, imm64: 48 b9 <8 bytes>
    uint8_t bytes[10];
    bytes[0] = 0x48;
    bytes[1] = 0xb9;
    write_u64(bytes + 2, value);
    elfgen_add_code(gen, bytes, 10);
}

void elfgen_emit_add_rax_imm64(ELFGenerator *gen, int64_t value) {
    // ADD rax, imm64: 48 05 <4 bytes> (for 32-bit sign-extended imm)
    uint8_t bytes[7];
    bytes[0] = 0x48;
    bytes[1] = 0x05;
    write_u32(bytes + 2, (uint32_t)value);
    elfgen_add_code(gen, bytes, 6);
}

void elfgen_emit_sub_rax_imm64(ELFGenerator *gen, int64_t value) {
    // SUB rax, imm64: 48 2d <4 bytes>
    uint8_t bytes[7];
    bytes[0] = 0x48;
    bytes[1] = 0x2d;
    write_u32(bytes + 2, (uint32_t)value);
    elfgen_add_code(gen, bytes, 6);
}

void elfgen_emit_mov_rax_rbp(ELFGenerator *gen) {
    // MOV rax, [rbp]: 48 8b 45 00
    uint8_t bytes[4] = { 0x48, 0x8b, 0x45, 0x00 };
    elfgen_add_code(gen, bytes, 4);
}

void elfgen_emit_mov_rbp_rax(ELFGenerator *gen) {
    // MOV [rbp], rax: 48 89 45 00
    uint8_t bytes[4] = { 0x48, 0x89, 0x45, 0x00 };
    elfgen_add_code(gen, bytes, 4);
}

void elfgen_emit_call_rax(ELFGenerator *gen) {
    // CALL rax: ff d0
    uint8_t bytes[2] = { 0xff, 0xd0 };
    elfgen_add_code(gen, bytes, 2);
}

void elfgen_emit_jmp_rax(ELFGenerator *gen) {
    // JMP rax: ff e0
    uint8_t bytes[2] = { 0xff, 0xe0 };
    elfgen_add_code(gen, bytes, 2);
}

void elfgen_emit_push_rax(ELFGenerator *gen) {
    // PUSH rax: 50
    uint8_t bytes[1] = { 0x50 };
    elfgen_add_code(gen, bytes, 1);
}

void elfgen_emit_pop_rax(ELFGenerator *gen) {
    // POP rax: 58
    uint8_t bytes[1] = { 0x58 };
    elfgen_add_code(gen, bytes, 1);
}

void elfgen_emit_nop(ELFGenerator *gen) {
    // NOP: 90
    uint8_t bytes[1] = { 0x90 };
    elfgen_add_code(gen, bytes, 1);
}

void elfgen_emit_int3(ELFGenerator *gen) {
    // INT3 (breakpoint): cc
    uint8_t bytes[1] = { 0xcc };
    elfgen_add_code(gen, bytes, 1);
}

void elfgen_free(ELFGenerator *gen) {
    if (gen) {
        free(gen->code);
        free(gen);
    }
}
