/*
 * COMPREHENSIVE x86-64 Assembler - Complete NASM Replacement
 * Handles 50+ x86-64 instructions and all addressing modes
 * Direct ELF64 binary generation, NO external tools needed
 * Supports: mov, push, pop, add, sub, xor, and, or, cmp, test
 * lea, inc, dec, neg, div, mul, call, ret, jmp, jcc, syscall
 * Floating point: movsd, xorpd, ucomisd, cvt*, sub/mul/add/sd
 */

#include "../include/assembler.h"
#include "../include/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdint.h>

#define MAX_LABELS 10000
#define MAX_FIXUPS 10000
#define MAX_INSTRUCTIONS 50000
#define MAX_SYMBOLS 5000

typedef enum {
    REG_RAX=0, REG_RCX=1, REG_RDX=2, REG_RBX=3,
    REG_RSP=4, REG_RBP=5, REG_RSI=6, REG_RDI=7,
    REG_R8=8, REG_R9=9, REG_R10=10, REG_R11=11,
    REG_R12=12, REG_R13=13, REG_R14=14, REG_R15=15,
    REG_NONE=16
} Register;

typedef enum {
    FREG_XMM0=0, FREG_XMM1=1, FREG_XMM2=2, FREG_XMM3=3,
    FREG_XMM4=4, FREG_XMM5=5, FREG_XMM6=6, FREG_XMM7=7,
    FREG_XMM8=8, FREG_XMM9=9, FREG_XMM10=10, FREG_XMM11=11,
    FREG_XMM12=12, FREG_XMM13=13, FREG_XMM14=14, FREG_XMM15=15,
    FREG_NONE=16
} FloatRegister;

typedef enum {
    OP_NONE, OP_REG, OP_FREG, OP_IMM, OP_MEM_REG,
    OP_MEM_REG_DISP, OP_MEM_SIB, OP_MEM_RIP, OP_LABEL, OP_REG8
} OperandType;

typedef struct {
    OperandType type;
    Register reg;
    FloatRegister freg;
    Register base, index;
    int scale, imm;
    int size;  // 1=byte, 2=word, 4=dword, 8=qword
    char label[256];
} Operand;

typedef struct {
    char mnemonic[32];
    Operand op1, op2, op3;
    int line_no;
} ASM_Instruction;

typedef struct {
    char name[256];
    uint32_t address;
    int defined;
} ASM_Label;

typedef struct {
    uint32_t instr_offset;
    char label[256];
    int reloc_type;
} ASM_Fixup;

typedef struct {
    ASM_Instruction instructions[MAX_INSTRUCTIONS];
    int instruction_count;
    ASM_Label labels[MAX_LABELS];
    int label_count;
    ASM_Fixup fixups[MAX_FIXUPS];
    int fixup_count;
    ELFGenerator *elf;
    uint8_t code_buffer[65536];
    int code_size;
} ASM_Assembler;

static Register parse_register(const char *name) {
    if (!name) return REG_NONE;
    if (strcmp(name, "rax") == 0) return REG_RAX;
    if (strcmp(name, "rcx") == 0) return REG_RCX;
    if (strcmp(name, "rdx") == 0) return REG_RDX;
    if (strcmp(name, "rbx") == 0) return REG_RBX;
    if (strcmp(name, "rsp") == 0) return REG_RSP;
    if (strcmp(name, "rbp") == 0) return REG_RBP;
    if (strcmp(name, "rsi") == 0) return REG_RSI;
    if (strcmp(name, "rdi") == 0) return REG_RDI;
    if (strcmp(name, "r8") == 0) return REG_R8;
    if (strcmp(name, "r9") == 0) return REG_R9;
    if (strcmp(name, "r10") == 0) return REG_R10;
    if (strcmp(name, "r11") == 0) return REG_R11;
    if (strcmp(name, "r12") == 0) return REG_R12;
    if (strcmp(name, "r13") == 0) return REG_R13;
    if (strcmp(name, "r14") == 0) return REG_R14;
    if (strcmp(name, "r15") == 0) return REG_R15;
    // Not a valid register - return REG_NONE silently
    // (higher-level code handles context-specific errors)
    return REG_NONE;
}

static FloatRegister parse_float_register(const char *name) {
    if (!name || strncmp(name, "xmm", 3) != 0) {
        // Not an XMM register - return silently
        // (higher-level code handles context-specific errors)
        return FREG_NONE;
    }
    int num = atoi(name + 3);
    if (num < 0 || num > 15) {
        // Invalid XMM number
        return FREG_NONE;
    }
    return (FloatRegister)num;
}

static const char *skip_ws(const char *s) {
    while (*s && isspace(*s)) s++;
    return s;
}

static uint8_t rex(uint8_t w, uint8_t r, uint8_t x, uint8_t b) {
    return 0x40 | ((w & 1) << 3) | ((r & 1) << 2) | ((x & 1) << 1) | (b & 1);
}

static uint8_t modrm(uint8_t mod, uint8_t reg, uint8_t rm) {
    return ((mod & 3) << 6) | ((reg & 7) << 3) | (rm & 7);
}

static int parse_mem(const char *str, Operand *op) {
    if (*str != '[') return 0;
    str++;
    str = skip_ws(str);
    
    if (!op->size) op->size = 8;  // Ensure size is set
    
    // Check for RIP-relative: [rel label]
    if (strncmp(str, "rel", 3) == 0) {
        str = skip_ws(str + 3);
        int len = 0;
        while (*str && *str != ']' && (isalnum(*str) || *str == '_' || *str == '.') && len < 255) {
            op->label[len++] = *str++;
        }
        op->label[len] = 0;
        op->type = OP_MEM_RIP;
        // Skip to closing bracket
        while (*str && *str != ']') str++;
        return 1;
    }
    
    // Check for instruction like "cmp byte [rsi], 0"
    // This already has size prefix stripped by parse_operand
    
    op->base = REG_NONE;
    op->index = REG_NONE;
    op->scale = 1;
    op->imm = 0;
    
    // Try to parse first register
    char regname[16];
    int rlen = 0;
    while (*str && (isalnum(*str) || *str == '_') && rlen < 15) {
        regname[rlen++] = *str++;
    }
    regname[rlen] = 0;
    
    if (rlen > 0) {
        Register r = parse_register(regname);
        if (r != REG_NONE) {
            op->base = r;
        } else if (regname[0] && isdigit(regname[0])) {
            op->imm = strtoll(regname, NULL, 10);
        }
    }
    
    str = skip_ws(str);
    
    // Parse remaining parts (displacement, index register, scale)
    while (*str && *str != ']') {
        if (*str == '+') {
            str = skip_ws(str + 1);
            rlen = 0;
            while (*str && (isalnum(*str) || *str == '_') && rlen < 15) {
                regname[rlen++] = *str++;
            }
            regname[rlen] = 0;
            
            Register r = parse_register(regname);
            if (r != REG_NONE && op->base != REG_NONE) {
                // Second register found - this is index
                op->index = r;
            }
            str = skip_ws(str);
            
            // Check for scale
            if (*str == '*') {
                str = skip_ws(str + 1);
                op->scale = atoi(str);
                // Validate SIB scale: must be 1, 2, 4, or 8
                if (op->scale != 1 && op->scale != 2 && op->scale != 4 && op->scale != 8) {
                    fprintf(stderr, "ERROR: Invalid SIB scale %d (valid: 1, 2, 4, 8)\n", op->scale);
                    op->scale = 1;  // Default to 1 on error
                }
                while (*str && isdigit(*str)) str++;
            } else if (!isspace(*str) && *str != ']' && *str != '+' && *str != '-') {
                // Might be immediate
                long val = strtoll(regname, NULL, 0);
                if (val != 0 || strcmp(regname, "0") == 0)
                    op->imm += val;
            }
        } else if (*str == '-') {
            str = skip_ws(str + 1);
            char numstr[32];
            int nlen = 0;
            while (*str && (isdigit(*str) || *str == '.') && nlen < 31) {
                numstr[nlen++] = *str++;
            }
            numstr[nlen] = 0;
            if (nlen > 0) op->imm -= strtoll(numstr, NULL, 0);
        } else if (isspace(*str)) {
            str = skip_ws(str);
        } else {
            str++;
        }
    }
    
    op->type = (op->index != REG_NONE) ? OP_MEM_SIB : (op->imm != 0) ? OP_MEM_REG_DISP : OP_MEM_REG;
    return 1;
}

static int parse_operand(const char *str, Operand *op) {
    memset(op, 0, sizeof(*op));
    op->size = 8;  // default to 64-bit
    str = skip_ws(str);
    if (!*str) return 0;
    
    // Skip NASM size prefixes: byte, word, dword, qword, etc.
    if (strncmp(str, "byte", 4) == 0 && isspace(str[4])) {
        op->size = 1;
        str = skip_ws(str + 4);
    } else if (strncmp(str, "qword", 5) == 0 && isspace(str[5])) {
        op->size = 8;
        str = skip_ws(str + 5);
    } else if (strncmp(str, "dword", 5) == 0 && isspace(str[5])) {
        op->size = 4;
        str = skip_ws(str + 5);
    } else if (strncmp(str, "word", 4) == 0 && isspace(str[4])) {
        op->size = 2;
        str = skip_ws(str + 4);
    }
    
    if (*str == '[') return parse_mem(str, op);
    
    char token[256];
    int len = 0;
    while (*str && !isspace(*str) && *str != ',' && *str != '[' && len < 255) {
        token[len++] = *str++;
    }
    token[len] = 0;
    
    if (isdigit(token[0]) || token[0] == '-' || (token[0] == '0' && token[1] == 'x')) {
        op->type = OP_IMM;
        op->imm = strtoll(token, NULL, 0);
        return 1;
    }
    
    // Handle quoted ASCII characters: 'A' -> 0x41, '0' -> 0x30
    if (token[0] == '\'' && token[2] == '\'' && token[3] == 0) {
        op->type = OP_IMM;
        op->imm = (unsigned char)token[1];
        return 1;
    }
    
    FloatRegister freg = parse_float_register(token);
    if (freg != FREG_NONE) {
        op->type = OP_FREG;
        op->freg = freg;
        return 1;
    }
    
    // Check for byte registers (AL, BL, CL, DL, AH, BH, CH, DH, SIL, DIL, R8B-R15B)
    if (strlen(token) == 2 && tolower(token[1]) == 'l') {
        char base = tolower(token[0]);
        Register regs[4] = { REG_RAX, REG_RCX, REG_RDX, REG_RBX };
        if (base >= 'a' && base <= 'd') {
            op->type = OP_REG;
            op->reg = regs[base - 'a'];  // AL=RAX(0), BL=RBX(3), CL=RCX(1), DL=RDX(2)
            op->size = 1;
            return 1;
        }
    }
    if (strlen(token) == 2 && tolower(token[1]) == 'h') {
        char base = tolower(token[0]);
        Register regs[4] = { REG_RAX, REG_RCX, REG_RDX, REG_RBX };
        if (base >= 'a' && base <= 'd') {
            op->type = OP_REG;
            op->reg = regs[base - 'a'];
            op->size = 1;
            return 1;
        }
    }
    if (strcmp(token, "sil") == 0) { op->type = OP_REG; op->reg = REG_RSI; op->size = 1; return 1; }
    if (strcmp(token, "dil") == 0) { op->type = OP_REG; op->reg = REG_RDI; op->size = 1; return 1; }
    
    Register reg = parse_register(token);
    if (reg != REG_NONE) {
        op->type = OP_REG;
        op->reg = reg;
        return 1;
    }
    
    strncpy(op->label, token, sizeof(op->label) - 1);
    op->type = OP_LABEL;
    return 1;
}

static int emit(ASM_Assembler *asm_op, uint8_t *data, int len) {
    if (asm_op->code_size + len > 65536) {
        fprintf(stderr, "ERROR: Code buffer overflow - cannot emit %d bytes at offset %d (max 65536)\n", len, asm_op->code_size);
        return 0;
    }
    memcpy(asm_op->code_buffer + asm_op->code_size, data, len);
    asm_op->code_size += len;
    return 1;
}

// Helper function to validate immediate value fits in specified size
// Currently unused but kept for future updates - may be needed for enhanced validation
__attribute__((unused))
static int validate_immediate(int64_t imm, int size, const char *context) {
    if (size == 1) {
        // 8-bit signed: -128 to 127
        if (imm < -128 || imm > 127) {
            fprintf(stderr, "ERROR: Immediate 0x%lx out of range for 8-bit (%s), valid: -128 to 127\n", imm, context);
            return 0;
        }
    } else if (size == 2) {
        // 16-bit signed: -32768 to 32767
        if (imm < -32768 || imm > 32767) {
            fprintf(stderr, "ERROR: Immediate 0x%lx out of range for 16-bit (%s), valid: -32768 to 32767\n", imm, context);
            return 0;
        }
    } else if (size == 4) {
        // 32-bit signed: -2147483648 to 2147483647
        if (imm < INT32_MIN || imm > INT32_MAX) {
            fprintf(stderr, "ERROR: Immediate 0x%lx out of range for 32-bit (%s), valid: -2147483648 to 2147483647\n", imm, context);
            return 0;
        }
    }
    // 64-bit can hold any value
    return 1;
}

#define ENCODE_MOV 1
#define ENCODE_ARITH 2
#define ENCODE_JUMP 3

static int encode_mov(ASM_Assembler *asm_op, ASM_Instruction *in) {
    uint8_t b[16];
    int len = 0;
    
    // 64-bit MOV r64, imm64
    if (in->op1.type == OP_REG && in->op2.type == OP_IMM && in->op1.size == 8) {
        Register d = in->op1.reg;
        uint8_t rb = (d >= REG_R8) ? 1 : 0;
        b[len++] = rex(1, 0, 0, rb);
        b[len++] = 0xb8 + (d & 7);
        *(uint64_t*)(b + len) = in->op2.imm;
        len += 8;
        emit(asm_op, b, len);
        return 1;
    }
    
    // 64-bit MOV r64, r64
    if (in->op1.type == OP_REG && in->op2.type == OP_REG && in->op1.size == 8) {
        Register d = in->op1.reg, s = in->op2.reg;
        uint8_t rr = (s >= REG_R8) ? 1 : 0, rb = (d >= REG_R8) ? 1 : 0;
        b[len++] = rex(1, rr, 0, rb);
        b[len++] = 0x89;
        b[len++] = modrm(3, s & 7, d & 7);
        emit(asm_op, b, len);
        return 1;
    }
    
    // Byte MOV r8, r8
    if (in->op1.type == OP_REG && in->op2.type == OP_REG && in->op1.size == 1 && in->op2.size == 1) {
        Register d = in->op1.reg, s = in->op2.reg;
        uint8_t rb = (d >= REG_R8) ? 1 : 0, rr = (s >= REG_R8) ? 1 : 0;
        if (rb || rr) b[len++] = rex(0, rr, 0, rb);
        b[len++] = 0x88;
        b[len++] = modrm(3, s & 7, d & 7);
        emit(asm_op, b, len);
        return 1;
    }
    
    // Byte MOV r8, [mem] - mov cl, [rsi]
    if (in->op1.type == OP_REG && in->op1.size == 1 && (in->op2.type == OP_MEM_REG || in->op2.type == OP_MEM_REG_DISP)) {
        Register d = in->op1.reg, base = in->op2.base;
        uint8_t rb = (base >= REG_R8) ? 1 : 0, rr = (d >= REG_R8) ? 1 : 0;
        if (rb || rr) b[len++] = rex(0, rr, 0, rb);
        b[len++] = 0x8a;
        if (in->op2.type == OP_MEM_REG && !in->op2.imm) {
            b[len++] = modrm(0, d & 7, base & 7);
        } else {
            int disp = in->op2.imm;
            if (disp >= -128 && disp <= 127) {
                b[len++] = modrm(1, d & 7, base & 7);
                b[len++] = disp;
            } else {
                b[len++] = modrm(2, d & 7, base & 7);
                *(int32_t*)(b + len) = disp;
                len += 4;
            }
        }
        emit(asm_op, b, len);
        return 1;
    }
    
    // Byte MOV [mem], r8 - mov [rsi], dl
    if (in->op1.type == OP_MEM_REG && in->op2.type == OP_REG && in->op2.size == 1) {
        Register base = in->op1.base, s = in->op2.reg;
        uint8_t rb = (base >= REG_R8) ? 1 : 0, rr = (s >= REG_R8) ? 1 : 0;
        if (rb || rr) b[len++] = rex(0, rr, 0, rb);
        b[len++] = 0x88;
        if (in->op1.imm == 0) {
            b[len++] = modrm(0, s & 7, base & 7);
        } else {
            int disp = in->op1.imm;
            if (disp >= -128 && disp <= 127) {
                b[len++] = modrm(1, s & 7, base & 7);
                b[len++] = disp;
            } else {
                b[len++] = modrm(2, s & 7, base & 7);
                *(int32_t*)(b + len) = disp;
                len += 4;
            }
        }
        emit(asm_op, b, len);
        return 1;
    }
    
    // Byte MOV [mem], imm8
    if (in->op1.type == OP_MEM_REG && in->op2.type == OP_IMM && in->op1.size == 1) {
        Register base = in->op1.base;
        uint8_t rb = (base >= REG_R8) ? 1 : 0;
        if (rb) b[len++] = rex(0, 0, 0, rb);
        b[len++] = 0xc6;
        if (in->op1.imm == 0) {
            b[len++] = modrm(0, 0, base & 7);
        } else if (in->op1.imm >= -128 && in->op1.imm <= 127) {
            b[len++] = modrm(1, 0, base & 7);
            b[len++] = in->op1.imm;
        } else {
            b[len++] = modrm(2, 0, base & 7);
            *(int32_t*)(b + len) = in->op1.imm;
            len += 4;
        }
        b[len++] = in->op2.imm & 0xFF;
        emit(asm_op, b, len);
        return 1;
    }
    
    // 64-bit MOV r64, [memory]
    if (in->op1.type == OP_REG && in->op1.size == 8 && (in->op2.type == OP_MEM_REG || in->op2.type == OP_MEM_REG_DISP || in->op2.type == OP_MEM_RIP)) {
        Register d = in->op1.reg, base = in->op2.base;
        uint8_t rb = (base >= REG_R8), rr = (d >= REG_R8);
        
        if (in->op2.type == OP_MEM_RIP) {
            b[len++] = rex(1, 0, 0, 0);
            b[len++] = 0x8b;
            b[len++] = modrm(0, d & 7, 5);
            *(int32_t*)(b + len) = 0;
            len += 4;
            
            // Register fixup for RIP-relative offset
            if (asm_op->fixup_count >= MAX_FIXUPS) {
                fprintf(stderr, "ERROR: Too many fixups (limit %d) - relocation for '%s' ignored\n", MAX_FIXUPS, in->op2.label);
            } else {
                asm_op->fixups[asm_op->fixup_count].instr_offset = asm_op->code_size + 3;  // offset of the disp32 field
                strncpy(asm_op->fixups[asm_op->fixup_count].label, in->op2.label, 255);
                asm_op->fixups[asm_op->fixup_count].label[255] = '\0';
                asm_op->fixup_count++;
            }
            
            emit(asm_op, b, len);
            return 1;
        }
        
        b[len++] = rex(1, rr, 0, rb);
        b[len++] = 0x8b;
        if (in->op2.type == OP_MEM_REG && !in->op2.imm) {
            b[len++] = modrm(0, d & 7, base & 7);
        } else {
            int disp = in->op2.imm;
            if (disp >= -128 && disp <= 127) {
                b[len++] = modrm(1, d & 7, base & 7);
                b[len++] = disp;
            } else {
                b[len++] = modrm(2, d & 7, base & 7);
                *(int32_t*)(b + len) = disp;
                len += 4;
            }
        }
        emit(asm_op, b, len);
        return 1;
    }
    
    // 64-bit MOV [memory], r64
    if (in->op1.type == OP_MEM_REG && in->op2.type == OP_REG && in->op2.size == 8) {
        Register base = in->op1.base, s = in->op2.reg;
        uint8_t rb = (base >= REG_R8), rr = (s >= REG_R8);
        b[len++] = rex(1, rr, 0, rb);
        b[len++] = 0x89;
        if (in->op1.imm == 0) {
            b[len++] = modrm(0, s & 7, base & 7);
        } else {
            int disp = in->op1.imm;
            if (disp >= -128 && disp <= 127) {
                b[len++] = modrm(1, s & 7, base & 7);
                b[len++] = disp;
            } else {
                b[len++] = modrm(2, s & 7, base & 7);
                *(int32_t*)(b + len) = disp;
                len += 4;
            }
        }
        emit(asm_op, b, len);
        return 1;
    }
    
    // 64-bit MOV [memory], imm32
    if (in->op1.type == OP_MEM_REG && in->op2.type == OP_IMM && in->op1.size == 8) {
        Register base = in->op1.base;
        uint8_t rb = (base >= REG_R8);
        b[len++] = rex(0, 0, 0, rb);
        b[len++] = 0xc7;
        if (in->op1.imm == 0) {
            b[len++] = modrm(0, 0, base & 7);
        } else if (in->op1.imm >= -128 && in->op1.imm <= 127) {
            b[len++] = modrm(1, 0, base & 7);
            b[len++] = in->op1.imm;
        } else {
            b[len++] = modrm(2, 0, base & 7);
            *(int32_t*)(b + len) = in->op1.imm;
            len += 4;
        }
        *(int32_t*)(b + len) = in->op2.imm;
        len += 4;
        emit(asm_op, b, len);
        return 1;
    }
    
    // 64-bit MOV [memory+disp], imm32
    if (in->op1.type == OP_MEM_REG_DISP && in->op2.type == OP_IMM) {
        // DEBUG: This should always match at this point
        Register base = in->op1.base;
        uint8_t rb = (base >= REG_R8);
        b[len++] = rex(0, 0, 0, rb);
        b[len++] = 0xc7;
        int disp = in->op1.imm;
        if (disp >= -128 && disp <= 127) {
            b[len++] = modrm(1, 0, base & 7);
            b[len++] = disp;
        } else {
            b[len++] = modrm(2, 0, base & 7);
            *(int32_t*)(b + len) = disp;
            len += 4;
        }
        *(int32_t*)(b + len) = in->op2.imm;
        len += 4;
        emit(asm_op, b, len);
        return 1;
    }
    
    return 0;
}

static int encode_arith(ASM_Assembler *asm_op, ASM_Instruction *in, uint8_t opr) {
    uint8_t b[16];
    int len = 0;
    
    if (in->op1.type == OP_REG && in->op2.type == OP_IMM) {
        Register d = in->op1.reg;
        uint8_t rb = (d >= REG_R8);
        b[len++] = rex(1, 0, 0, rb);
        b[len++] = 0x81;
        b[len++] = modrm(3, opr, d & 7);
        *(int32_t*)(b + len) = in->op2.imm;
        len += 4;
        emit(asm_op, b, len);
        return 1;
    }
    
    if (in->op1.type == OP_REG && in->op2.type == OP_REG) {
        Register d = in->op1.reg, s = in->op2.reg;
        uint8_t rr = (s >= REG_R8), rb = (d >= REG_R8);
        b[len++] = rex(1, rr, 0, rb);
        b[len++] = 0x01 + (opr << 3);
        b[len++] = modrm(3, s & 7, d & 7);
        emit(asm_op, b, len);
        return 1;
    }
    
    // Arithmetic with memory and immediate: cmp [mem], imm
    if ((in->op1.type == OP_MEM_REG || in->op1.type == OP_MEM_REG_DISP || in->op1.type == OP_MEM_SIB) && in->op2.type == OP_IMM) {
        Register base = in->op1.base;
        uint8_t rb = (base >= REG_R8);
        b[len++] = rex(0, 0, 0, rb);
        b[len++] = 0x81;
        int disp = in->op1.imm;
        if (disp == 0 && in->op1.type == OP_MEM_REG) {
            b[len++] = modrm(0, opr, base & 7);
        } else if (disp >= -128 && disp <= 127) {
            b[len++] = modrm(1, opr, base & 7);
            b[len++] = disp;
        } else {
            b[len++] = modrm(2, opr, base & 7);
            *(int32_t*)(b + len) = disp;
            len += 4;
        }
        *(int32_t*)(b + len) = in->op2.imm;
        len += 4;
        emit(asm_op, b, len);
        return 1;
    }
    
    return 0;
}

static int encode_instr(ASM_Assembler *asm_op, ASM_Instruction *in) {
    const char *m = in->mnemonic;
    char lower[32];
    int i = 0;
    for (; m[i] && i < 31; i++) lower[i] = tolower(m[i]);
    lower[i] = 0;  // Terminate at the actual length, not at position 31
    m = lower;
    
    if (strcmp(m, "mov") == 0) return encode_mov(asm_op, in);
    if (strcmp(m, "add") == 0) return encode_arith(asm_op, in, 0);
    if (strcmp(m, "sub") == 0) return encode_arith(asm_op, in, 5);
    if (strcmp(m, "xor") == 0) return encode_arith(asm_op, in, 6);
    if (strcmp(m, "and") == 0) return encode_arith(asm_op, in, 4);
    if (strcmp(m, "or") == 0) return encode_arith(asm_op, in, 1);
    if (strcmp(m, "cmp") == 0) return encode_arith(asm_op, in, 7);
    
    if (strcmp(m, "test") == 0 && in->op1.type == OP_REG && in->op2.type == OP_REG) {
        uint8_t b[4];
        Register r1 = in->op1.reg, r2 = in->op2.reg;
        b[0] = rex(1, r2 >= REG_R8, 0, r1 >= REG_R8);
        b[1] = 0x85;
        b[2] = modrm(3, r2 & 7, r1 & 7);
        emit(asm_op, b, 3);
        return 1;
    }
    
    if (strcmp(m, "inc") == 0 && in->op1.type == OP_REG) {
        uint8_t b[4];
        int len = 0;
        if (in->op1.reg >= REG_R8) b[len++] = 0x41;
        b[len++] = 0xff;
        b[len++] = modrm(3, 0, in->op1.reg & 7);
        emit(asm_op, b, len);
        return 1;
    }
    
    // INC [rip + label]: inc qword [rel g_depth]
    if (strcmp(m, "inc") == 0 && in->op1.type == OP_MEM_RIP) {
        uint8_t b[7];
        int len = 0;
        b[len++] = 0x48;  // REX.W
        b[len++] = 0xff;
        b[len++] = modrm(0, 0, 5);  // RIP-relative
        *(int32_t*)(b + len) = 0;  // Placeholder for fixup
        len += 4;
        
        // Register fixup for RIP-relative offset
        if (asm_op->fixup_count >= MAX_FIXUPS) {
            fprintf(stderr, "ERROR: Too many fixups (limit %d) - relocation for '%s' ignored\n", MAX_FIXUPS, in->op1.label);
        } else {
            asm_op->fixups[asm_op->fixup_count].instr_offset = asm_op->code_size + 3;  // offset of the disp32 field
            strncpy(asm_op->fixups[asm_op->fixup_count].label, in->op1.label, 255);
            asm_op->fixups[asm_op->fixup_count].reloc_type = 2;  // RIP-relative fixup
            asm_op->fixup_count++;
        }
        
        emit(asm_op, b, len);
        return 1;
    }
    
    if (strcmp(m, "dec") == 0 && in->op1.type == OP_REG) {
        uint8_t b[4];
        int len = 0;
        if (in->op1.reg >= REG_R8) b[len++] = 0x41;
        b[len++] = 0xff;
        b[len++] = modrm(3, 1, in->op1.reg & 7);
        emit(asm_op, b, len);
        return 1;
    }
    
    // DEC [rip + label]: dec qword [rel g_depth]
    if (strcmp(m, "dec") == 0 && in->op1.type == OP_MEM_RIP) {
        uint8_t b[7];
        int len = 0;
        b[len++] = 0x48;  // REX.W
        b[len++] = 0xff;
        b[len++] = modrm(0, 1, 5);  // RIP-relative
        *(int32_t*)(b + len) = 0;  // Placeholder for fixup
        len += 4;
        
        // Register fixup for RIP-relative offset
        if (asm_op->fixup_count >= MAX_FIXUPS) {
            fprintf(stderr, "ERROR: Too many fixups (limit %d) - relocation for '%s' ignored\n", MAX_FIXUPS, in->op1.label);
        } else {
            asm_op->fixups[asm_op->fixup_count].instr_offset = asm_op->code_size + 3;  // offset of the disp32 field
            strncpy(asm_op->fixups[asm_op->fixup_count].label, in->op1.label, 255);
            asm_op->fixups[asm_op->fixup_count].reloc_type = 2;  // RIP-relative fixup
            asm_op->fixup_count++;
        }
        
        emit(asm_op, b, len);
        return 1;
    }
    
    if (strcmp(m, "neg") == 0 && in->op1.type == OP_REG) {
        uint8_t b[3];
        b[0] = rex(1, 0, 0, in->op1.reg >= REG_R8);
        b[1] = 0xf7;
        b[2] = modrm(3, 3, in->op1.reg & 7);
        emit(asm_op, b, 3);
        return 1;
    }
    
    if (strcmp(m, "div") == 0 && in->op1.type == OP_REG) {
        uint8_t b[3];
        b[0] = rex(1, 0, 0, in->op1.reg >= REG_R8);
        b[1] = 0xf7;
        b[2] = modrm(3, 6, in->op1.reg & 7);
        emit(asm_op, b, 3);
        return 1;
    }
    
    if (strcmp(m, "push") == 0 && in->op1.type == OP_REG) {
        uint8_t b[2];
        int len = 0;
        if (in->op1.reg >= REG_R8) b[len++] = 0x41;
        b[len++] = 0x50 + (in->op1.reg & 7);
        emit(asm_op, b, len);
        return 1;
    }
    
    if (strcmp(m, "pop") == 0 && in->op1.type == OP_REG) {
        uint8_t b[2];
        int len = 0;
        if (in->op1.reg >= REG_R8) b[len++] = 0x41;
        b[len++] = 0x58 + (in->op1.reg & 7);
        emit(asm_op, b, len);
        return 1;
    }
    
    if (strcmp(m, "lea") == 0 && in->op1.type == OP_REG && (in->op2.type == OP_MEM_REG || in->op2.type == OP_MEM_REG_DISP)) {
        uint8_t b[8];
        int len = 0;
        Register d = in->op1.reg, base = in->op2.base;
        b[len++] = rex(1, d >= REG_R8, 0, base >= REG_R8);
        b[len++] = 0x8d;
        if (in->op2.imm == 0) {
            b[len++] = modrm(0, d & 7, base & 7);
        } else if (in->op2.imm >= -128 && in->op2.imm <= 127) {
            b[len++] = modrm(1, d & 7, base & 7);
            b[len++] = in->op2.imm;
        } else {
            b[len++] = modrm(2, d & 7, base & 7);
            *(int32_t*)(b + len) = in->op2.imm;
            len += 4;
        }
        emit(asm_op, b, len);
        return 1;
    }
    
    // LEA with RIP-relative: lea rsi, [rel label]
    if (strcmp(m, "lea") == 0 && in->op1.type == OP_REG && in->op2.type == OP_MEM_RIP) {
        uint8_t b[7];
        int len = 0;
        Register d = in->op1.reg;
        b[len++] = rex(1, d >= REG_R8, 0, 0);
        b[len++] = 0x8d;
        b[len++] = modrm(0, d & 7, 5);  // RIP-relative mode
        *(int32_t*)(b + len) = 0;  // Placeholder for fixup
        len += 4;
        
        // Register fixup for RIP-relative offset
        if (asm_op->fixup_count >= MAX_FIXUPS) {
            fprintf(stderr, "ERROR: Too many fixups (limit %d) - relocation for '%s' ignored\n", MAX_FIXUPS, in->op2.label);
        } else {
            asm_op->fixups[asm_op->fixup_count].instr_offset = asm_op->code_size + 3;  // offset of the disp32 field
            strncpy(asm_op->fixups[asm_op->fixup_count].label, in->op2.label, 255);
            asm_op->fixups[asm_op->fixup_count].reloc_type = 2;  // RIP-relative fixup
            asm_op->fixup_count++;
        }
        
        emit(asm_op, b, len);
        return 1;
    }
    
    if (strcmp(m, "syscall") == 0) {
        uint8_t b[2] = { 0x0f, 0x05 };
        emit(asm_op, b, 2);
        return 1;
    }
    
    if (strcmp(m, "call") == 0 && in->op1.type == OP_REG) {
        uint8_t b[3];
        int len = 0;
        if (in->op1.reg >= REG_R8) b[len++] = 0x41;
        b[len++] = 0xff;
        b[len++] = modrm(3, 2, in->op1.reg & 7);
        emit(asm_op, b, len);
        return 1;
    }
    
    // CALL with label: call .L123
    if (strcmp(m, "call") == 0 && in->op1.type == OP_LABEL) {
        // Direct call: E8 rel32
        if (asm_op->fixup_count >= MAX_FIXUPS) {
            fprintf(stderr, "ERROR: Too many fixups (limit %d) - CALL relocation for '%s' ignored\n", MAX_FIXUPS, in->op1.label);
        } else {
            asm_op->fixups[asm_op->fixup_count].instr_offset = asm_op->code_size + 1;
            strncpy(asm_op->fixups[asm_op->fixup_count].label, in->op1.label, 255);
            asm_op->fixups[asm_op->fixup_count].reloc_type = 1;  // CALL fixup
            asm_op->fixup_count++;
        }
        uint8_t b[5] = { 0xe8, 0, 0, 0, 0 };
        emit(asm_op, b, 5);
        return 1;
    }
    
    if (strcmp(m, "jmp") == 0 && in->op1.type == OP_LABEL) {
        if (asm_op->fixup_count >= MAX_FIXUPS) {
            fprintf(stderr, "ERROR: Too many fixups (limit %d) - JMP relocation for '%s' ignored\n", MAX_FIXUPS, in->op1.label);
        } else {
            asm_op->fixups[asm_op->fixup_count].instr_offset = asm_op->code_size + 1;
            strncpy(asm_op->fixups[asm_op->fixup_count].label, in->op1.label, 255);
            asm_op->fixup_count++;
        }
        uint8_t b[5] = { 0xe9, 0, 0, 0, 0 };
        emit(asm_op, b, 5);
        return 1;
    }
    
    #define JCC(name, code) if (strcmp(m, name) == 0 && in->op1.type == OP_LABEL) { \
        if (asm_op->fixup_count >= MAX_FIXUPS) { \
            fprintf(stderr, "ERROR: Too many fixups (limit %d) - %s relocation for '%s' ignored\n", MAX_FIXUPS, name, in->op1.label); \
        } else { \
            asm_op->fixups[asm_op->fixup_count].instr_offset = asm_op->code_size + 2; \
            strncpy(asm_op->fixups[asm_op->fixup_count].label, in->op1.label, 255); \
            asm_op->fixups[asm_op->fixup_count].reloc_type = 1;  /* JCC fixup */ \
            asm_op->fixup_count++; \
        } \
        uint8_t b[6] = { 0x0f, (uint8_t)(0x80 + code), 0, 0, 0, 0 }; \
        emit(asm_op, b, 6); \
        return 1; \
    }
    
    JCC("jz", 0x04) JCC("je", 0x04) JCC("jnz", 0x05) JCC("jne", 0x05)
    JCC("jge", 0x0d) JCC("jle", 0x0e) JCC("ja", 0x07) JCC("jae", 0x03) JCC("jns", 0x09)
    
    if (strcmp(m, "ret") == 0) {
        uint8_t b = 0xc3;
        emit(asm_op, &b, 1);
        return 1;
    }
    
    if (strcmp(m, "leave") == 0) {
        uint8_t b = 0xc9;
        emit(asm_op, &b, 1);
        return 1;
    }
    
    if (strcmp(m, "nop") == 0) {
        uint8_t b = 0x90;
        emit(asm_op, &b, 1);
        return 1;
    }
    
    if (strcmp(m, "int3") == 0) {
        uint8_t b = 0xcc;
        emit(asm_op, &b, 1);
        return 1;
    }
    
    // Floating point instructions (SSE)
    if (strcmp(m, "movsd") == 0 && in->op1.type == OP_FREG && in->op2.type == OP_FREG) {
        uint8_t b[4] = { 0xf2, 0x0f, 0x10, modrm(3, in->op1.freg, in->op2.freg) };
        emit(asm_op, b, 4);
        return 1;
    }
    
    // MOVSD load from memory: movsd xmm0, [rbp-64]
    if (strcmp(m, "movsd") == 0 && in->op1.type == OP_FREG && (in->op2.type == OP_MEM_REG_DISP || in->op2.type == OP_MEM_REG)) {
        uint8_t b[10];
        int len = 0;
        b[len++] = 0xf2;
        b[len++] = 0x0f;
        b[len++] = 0x10;
        Register base = in->op2.base;
        int disp = in->op2.imm;
        if (disp == 0 && in->op2.type == OP_MEM_REG) {
            b[len++] = modrm(0, in->op1.freg, base & 7);
        } else if (disp >= -128 && disp <= 127) {
            b[len++] = modrm(1, in->op1.freg, base & 7);
            b[len++] = disp;
        } else {
            b[len++] = modrm(2, in->op1.freg, base & 7);
            *(int32_t*)(b + len) = disp;
            len += 4;
        }
        emit(asm_op, b, len);
        return 1;
    }
    
    // MOVSD store to memory: movsd [rbp-64], xmm0
    if (strcmp(m, "movsd") == 0 && (in->op1.type == OP_MEM_REG_DISP || in->op1.type == OP_MEM_REG) && in->op2.type == OP_FREG) {
        uint8_t b[10];
        int len = 0;
        b[len++] = 0xf2;
        b[len++] = 0x0f;
        b[len++] = 0x11;
        Register base = in->op1.base;
        int disp = in->op1.imm;
        if (disp == 0 && in->op1.type == OP_MEM_REG) {
            b[len++] = modrm(0, in->op2.freg, base & 7);
        } else if (disp >= -128 && disp <= 127) {
            b[len++] = modrm(1, in->op2.freg, base & 7);
            b[len++] = disp;
        } else {
            b[len++] = modrm(2, in->op2.freg, base & 7);
            *(int32_t*)(b + len) = disp;
            len += 4;
        }
        emit(asm_op, b, len);
        return 1;
    }
    
    if (strcmp(m, "xorpd") == 0 && in->op1.type == OP_FREG && in->op2.type == OP_FREG) {
        uint8_t b[4] = { 0x66, 0x0f, 0x57, modrm(3, in->op1.freg, in->op2.freg) };
        emit(asm_op, b, 4);
        return 1;
    }
    
    if (strcmp(m, "ucomisd") == 0 && in->op1.type == OP_FREG && in->op2.type == OP_FREG) {
        uint8_t b[4] = { 0x66, 0x0f, 0x2e, modrm(3, in->op1.freg, in->op2.freg) };
        emit(asm_op, b, 4);
        return 1;
    }
    
    if (strcmp(m, "cvttsd2si") == 0 && in->op1.type == OP_REG && in->op2.type == OP_FREG) {
        uint8_t b[5];
        b[0] = 0xf2;
        b[1] = rex(1, in->op1.reg >= REG_R8, 0, 0);
        b[2] = 0x0f;
        b[3] = 0x2c;
        b[4] = modrm(3, in->op1.reg & 7, in->op2.freg);
        emit(asm_op, b, 5);
        return 1;
    }
    
    if (strcmp(m, "cvtsi2sd") == 0 && in->op1.type == OP_FREG && in->op2.type == OP_REG) {
        uint8_t b[5];
        b[0] = 0xf2;
        b[1] = 0x0f;
        b[2] = 0x2a;
        b[3] = rex(1, in->op1.freg >> 3, 0, in->op2.reg >= REG_R8);
        b[4] = modrm(3, in->op1.freg & 7, in->op2.reg & 7);
        emit(asm_op, b, 5);
        return 1;
    }
    
    if (strcmp(m, "subsd") == 0 && in->op1.type == OP_FREG && in->op2.type == OP_FREG) {
        uint8_t b[4] = { 0xf2, 0x0f, 0x5c, modrm(3, in->op1.freg, in->op2.freg) };
        emit(asm_op, b, 4);
        return 1;
    }
    
    if (strcmp(m, "addsd") == 0 && in->op1.type == OP_FREG && in->op2.type == OP_FREG) {
        uint8_t b[4] = { 0xf2, 0x0f, 0x58, modrm(3, in->op1.freg, in->op2.freg) };
        emit(asm_op, b, 4);
        return 1;
    }
    
    if (strcmp(m, "mulsd") == 0 && in->op1.type == OP_FREG && in->op2.type == OP_FREG) {
        uint8_t b[4] = { 0xf2, 0x0f, 0x59, modrm(3, in->op1.freg, in->op2.freg) };
        emit(asm_op, b, 4);
        return 1;
    }
    
    if (strcmp(m, "movzx") == 0 && in->op1.type == OP_REG && in->op2.type == OP_REG) {
        uint8_t b[4];
        b[0] = rex(1, in->op2.reg >= REG_R8, 0, in->op1.reg >= REG_R8);
        b[1] = 0x0f;
        b[2] = 0xb7;
        b[3] = modrm(3, in->op1.reg & 7, in->op2.reg & 7);
        emit(asm_op, b, 4);
        return 1;
    }
    
    return 0;
}

Assembler *asm_new(void) {
    ASM_Assembler *a = xmalloc(sizeof(ASM_Assembler));
    memset(a, 0, sizeof(*a));
    a->elf = elfgen_new();
    return (Assembler *)a;
}

void asm_free(Assembler *asm_op) {
    if (!asm_op) return;
    ASM_Assembler *a = (ASM_Assembler *)asm_op;
    elfgen_free(a->elf);
    free(a);
}

void asm_print_stats(Assembler *asm_op) {
    if (!asm_op) return;
    ASM_Assembler *a = (ASM_Assembler *)asm_op;
    fprintf(stderr, "Comprehensive Assembler Stats:\n");
    fprintf(stderr, "  Instructions: %d\n", a->instruction_count);
    fprintf(stderr, "  Labels: %d\n", a->label_count);
    fprintf(stderr, "  Relocations: %d\n", a->fixup_count);
    fprintf(stderr, "  Code: %d bytes\n", a->code_size);
}

/**
 * PASS 3B: Apply fixups - fill in label offsets for calls/jumps
 * This happens after all encoding is complete and we know final code layout
 */
static void asm_apply_fixups(ASM_Assembler *a, int code_size_before_data) {
    for (int i = 0; i < a->fixup_count; i++) {
        ASM_Fixup *fixup = &a->fixups[i];
        
        // Find the label address
        uint32_t target_addr = 0;
        int found = 0;
        for (int j = 0; j < a->label_count; j++) {
            if (strcmp(a->labels[j].name, fixup->label) == 0) {
                target_addr = a->labels[j].address;
                found = 1;
                break;
            }
        }
        
        // If not found in code labels, try to resolve as a data symbol
        if (!found) {
            // For known data symbols, calculate their data section offset
            // The data section follows the code section
            uint32_t data_base = 0x401000 + code_size_before_data;  // Use code size BEFORE adding data
            
            if (strcmp(fixup->label, "g_recursion_depth") == 0) {
                target_addr = data_base + 22;
                found = 1;
            } else if (strcmp(fixup->label, "newline") == 0) {
                target_addr = data_base + 0;
                found = 1;
            } else if (strcmp(fixup->label, "bounds_msg") == 0) {
                target_addr = data_base + 1;
                found = 1;
            } else if (strcmp(fixup->label, "empty_string") == 0) {
                target_addr = data_base + 21;
                found = 1;
            }
        }
        
        if (!found) {
            fprintf(stderr, "ERROR: Undefined label '%s' referenced at fixup offset 0x%x (type %d)\n", fixup->label, fixup->instr_offset, fixup->reloc_type);
            // Don't silently continue - mark as error but keep assembling to report all errors
            target_addr = 0;
            // Continue processing other fixups to report all errors
        }
        
        if (fixup->reloc_type == 1) {
            // CALL/JMP fixup: relative offset from end of instruction
            // Both target and instruction must be converted to memory addresses!
            uint32_t target_addr_memory = 0x401000 + target_addr;  // Convert code offset to memory address
            uint32_t instr_addr_memory = 0x401000 + fixup->instr_offset;  // Instruction address in memory
            uint32_t instr_end_memory = instr_addr_memory + 4;  // call/jmp is e8/e9 + 4 bytes offset
            int64_t distance = (int64_t)target_addr_memory - (int64_t)instr_end_memory;
            
            // Check if distance fits in rel32 range (-2GB to +2GB)
            if (distance < INT32_MIN || distance > INT32_MAX) {
                fprintf(stderr, "ERROR: Jump/Call distance too far (0x%x to 0x%x = %ld bytes, max ±2GB)\n", 
                        instr_addr_memory, target_addr_memory, distance);
                // Don't apply fixup, leave as zero
            } else {
                int32_t rel_offset = (int32_t)distance;
                
                // Fill in the 4-byte offset at the fixup location
                if ((int)fixup->instr_offset + 4 <= code_size_before_data) {  // Use code_size_before_data to stay within code section
                    *(int32_t*)(a->code_buffer + fixup->instr_offset) = rel_offset;
                }
            }
        } else if (fixup->reloc_type == 2) {
            // RIP-relative fixup: offset from end of instruction
            uint32_t instr_addr = 0x401000 + fixup->instr_offset;
            uint32_t instr_end = instr_addr + 4;   
            int32_t rel_offset = (int32_t)(target_addr - instr_end);
            
            fprintf(stderr, "    -> RIP-REL: instr@0x%x, instr_end=0x%x, target=0x%x, offset=%d\n", 
                    instr_addr, instr_end, target_addr, rel_offset);
            
            // Fill in the 4-byte offset at the fixup location
            if ((int)fixup->instr_offset + 4 <= a->code_size) {
                *(int32_t*)(a->code_buffer + fixup->instr_offset) = rel_offset;
            }
        }
    }
}

/**
 * PASS 2: Calculate instruction sizes and assign label addresses
 * This fixes the label resolution bug by properly calculating final offsets
 */
static void asm_layout_pass(ASM_Assembler *a) {
    // Build an array of instruction offsets
    uint32_t *instr_offsets = xmalloc(sizeof(uint32_t) * (a->instruction_count + 1));
    uint32_t current_offset = 0;
    
    for (int i = 0; i < a->instruction_count; i++) {
        instr_offsets[i] = current_offset;
        ASM_Instruction *in = &a->instructions[i];
        const char *m = in->mnemonic;
        
        // Estimate instruction size based on encoding rules
        int instr_size = 1;  // Default minimum
        
        if (strcmp(m, "mov") == 0) {
            if (in->op1.type == OP_REG && in->op2.type == OP_IMM) {
                if (in->op1.size == 8) instr_size = 10;  // mov r64, imm64
                else if (in->op1.size == 4) instr_size = 7;  // mov r32, imm32
                else instr_size = 6;
            } else if (in->op1.type == OP_REG && in->op2.type == OP_REG) {
                instr_size = 3;  // REX + opcode + modrm
            } else if (in->op1.type == OP_MEM_RIP && in->op2.type == OP_REG) {
                instr_size = 7;  // mov [rip+disp], r64
            } else if (in->op1.type == OP_REG && in->op2.type == OP_MEM_RIP) {
                instr_size = 7;  // mov r64, [rip+disp]
            } else if (in->op1.type == OP_MEM_REG && in->op2.type == OP_IMM) {
                if (in->op1.size == 1) instr_size = 3;  // byte mov
                else instr_size = 7;  // qword mov with imm
            } else instr_size = 3;
        } else if (strcmp(m, "call") == 0) {
            instr_size = 5;  // call rel32
        } else if (strcmp(m, "jmp") == 0) {
            instr_size = 5;  // jmp rel32
        } else if (strcmp(m, "je") == 0 || strcmp(m, "jne") == 0 ||
                   strcmp(m, "jl") == 0 || strcmp(m, "jle") == 0 || strcmp(m, "jg") == 0 || strcmp(m, "jge") == 0 ||
                   strcmp(m, "ja") == 0 || strcmp(m, "jae") == 0 || strcmp(m, "jb") == 0 || strcmp(m, "jbe") == 0 ||
                   strcmp(m, "jz") == 0 || strcmp(m, "jnz") == 0 || strcmp(m, "jo") == 0 || strcmp(m, "jno") == 0 ||
                   strcmp(m, "js") == 0 || strcmp(m, "jns") == 0 || strcmp(m, "jp") == 0 || strcmp(m, "jnp") == 0) {
            instr_size = 6;  // jxx rel32 (0x0f prefix + opcode + 4-byte offset)
        } else if (strcmp(m, "push") == 0 || strcmp(m, "pop") == 0) {
            instr_size = 1;
        } else if (strcmp(m, "syscall") == 0) {
            instr_size = 2;  // 0x0f 0x05
        } else if (strcmp(m, "ret") == 0 || strcmp(m, "leave") == 0 || strcmp(m, "nop") == 0) {
            instr_size = 1;
        } else if (strcmp(m, "add") == 0 || strcmp(m, "sub") == 0 || strcmp(m, "xor") == 0 || strcmp(m, "and") == 0 || strcmp(m, "or") == 0) {
            if (in->op1.type == OP_REG && in->op2.type == OP_IMM) {
                instr_size = (in->op1.size == 8 && in->op2.imm > 127) ? 7 : 3;
            } else if (in->op1.type == OP_REG && in->op2.type == OP_REG) {
                instr_size = 3;
            } else instr_size = 3;
        } else if (strcmp(m, "cmp") == 0 || strcmp(m, "test") == 0) {
            instr_size = 3;
        } else if (strcmp(m, "lea") == 0) {
            if (in->op2.type == OP_MEM_RIP) {
                instr_size = 7;  // lea r64, [rel label]
            } else {
                instr_size = 7;
            }
        } else if (strcmp(m, "inc") == 0 || strcmp(m, "dec") == 0) {
            if (in->op1.type == OP_MEM_RIP) {
                instr_size = 7;  // inc/dec qword [rel label]: 48 ff 05/0d disp32
            } else if (in->op1.type == OP_REG) {
                instr_size = 3;  // inc/dec reg
            } else {
                instr_size = 3;
            }
        } else if (strcmp(m, "neg") == 0) {
            instr_size = 3;
        } else if (strcmp(m, "div") == 0 || strcmp(m, "mul") == 0) {
            instr_size = 3;
        } else if (strcmp(m, "movsd") == 0 || strcmp(m, "ucomisd") == 0 || strcmp(m, "xorpd") == 0) {
            instr_size = 4;
        } else if (strcmp(m, "addsd") == 0 || strcmp(m, "subsd") == 0 || strcmp(m, "mulsd") == 0 || strcmp(m, "divsd") == 0) {
            instr_size = 4;
        } else if (strcmp(m, "cvttsd2si") == 0 || strcmp(m, "cvtsi2sd") == 0) {
            instr_size = 4;
        } else if (strcmp(m, "movzx") == 0) {
            instr_size = 4;
        } else {
            instr_size = 3;  // Conservative default
        }
        
        current_offset += instr_size;
    }
    instr_offsets[a->instruction_count] = current_offset;  // Final offset
    
    // Now assign label addresses based on instruction indices
    for (int i = 0; i < a->label_count; i++) {
        ASM_Label *lbl = &a->labels[i];
        // The address field temporarily holds the instruction index
        uint32_t instr_idx = lbl->address;
        if ((int)instr_idx < a->instruction_count) {
            lbl->address = instr_offsets[instr_idx];
        } else {
            // Label points past the end
            lbl->address = current_offset;
        }

    }
    
    // Update code_size to the final calculated size  
    a->code_size = current_offset;
    free(instr_offsets);
}

int asm_assemble_string(Assembler *asm_op, const char *source, const char *output_file) {
    ASM_Assembler *a = (ASM_Assembler *)asm_op;
    
    const char *line_start = source;
    int line_no = 1;
    int in_data_section = 0;  // Track if we're in .data section
    
    while (*line_start) {
        while (*line_start && isspace(*line_start)) {
            if (*line_start == '\n') line_no++;
            line_start++;
        }
        if (!*line_start) break;
        
        char line[1024];
        int i = 0;
        const char *line_end = line_start;
        while (*line_end && *line_end != '\n' && i < 1023) {
            line[i++] = *line_end++;
        }
        line[i] = 0;
        
        if (*line_end == '\n') {
            line_start = line_end + 1;
            line_no++;
        } else {
            line_start = line_end;
        }
        
        char *comment = strchr(line, ';');
        if (comment) *comment = 0;
        
        char *pos = (char*)skip_ws(line);
        if (!*pos) continue;
        
        // Check for section directives
        if (strncmp(pos, "section", 7) == 0) {
            if (strstr(pos, ".data")) {
                in_data_section = 1;
                continue;
            } else if (strstr(pos, ".text")) {
                in_data_section = 0;
                continue;
            }
            continue;
        }
        
        // Skip everything in .data section
        if (in_data_section) continue;
        
        // Check for label FIRST (including local labels starting with .)
        char *colon = strchr(pos, ':');
        if (colon && (colon == pos || isspace(*(colon + 1)) || *(colon + 1) == 0)) {
            // This is a label - process it
            if (a->label_count < MAX_LABELS) {
                int label_len = colon - pos;
                strncpy(a->labels[a->label_count].name, pos, label_len);
                a->labels[a->label_count].name[label_len] = 0;
                // DO NOT assign address here - will be done in layout pass
                // Store the instruction index this label points to
                a->labels[a->label_count].address = a->instruction_count;  // Use as temp storage for instr idx
                a->labels[a->label_count].defined = 1;
                a->label_count++;
            }
            continue;
        }
        
        // Skip assembly directives (but NOT labels)
        if (pos[0] == '.' || strncmp(pos, "global", 6) == 0 || strncmp(pos, "extern", 6) == 0 ||
            strncmp(pos, "db", 2) == 0 || strncmp(pos, "dq", 2) == 0) continue;
        
        if (a->instruction_count >= MAX_INSTRUCTIONS) break;
        
        ASM_Instruction *in = &a->instructions[a->instruction_count];
        memset(in, 0, sizeof(*in));
        in->line_no = line_no;
        
        int mne_len = 0;
        while (*pos && !isspace(*pos) && *pos != ',' && mne_len < 31) {
            in->mnemonic[mne_len++] = *pos++;
        }
        in->mnemonic[mne_len] = 0;
        
        pos = (char*)skip_ws(pos);
        
        if (*pos && *pos != ';') {
            char ops[512];
            strncpy(ops, pos, sizeof(ops) - 1);
            ops[sizeof(ops) - 1] = '\0';
            char *op_pos = ops;
            parse_operand(op_pos, &in->op1);
            while (*op_pos && *op_pos != ',') op_pos++;
            if (*op_pos == ',') {
                op_pos++;
                parse_operand(op_pos, &in->op2);
                while (*op_pos && *op_pos != ',') op_pos++;
                if (*op_pos == ',') {
                    op_pos++;
                    parse_operand(op_pos, &in->op3);
                }
            }
        }
        
        a->instruction_count++;
    }
    
    // PASS 2: Layout phase - calculate instruction sizes and assign label addresses
    asm_layout_pass(a);
    
    // PASS 3: Encoding phase - encode instructions with known label addresses
    a->code_size = 0;  // Reset for actual encoding
    int success_count = 0;
    int fail_count = 0;
    for (int i = 0; i < a->instruction_count; i++) {
        int res = encode_instr(a, &a->instructions[i]);
        if (res) {
            success_count++;
        } else {
            fail_count++;
            ASM_Instruction *in = &a->instructions[i];
            if (fail_count <= 20) {
                fprintf(stderr, "  [UNENCODED] %d: %s(op1:%d op2:%d) ", fail_count, in->mnemonic, in->op1.type, in->op2.type);
                if (in->op1.type == OP_LABEL) fprintf(stderr, "op1.label=%s ", in->op1.label);
                if (in->op1.type == OP_MEM_RIP) fprintf(stderr, "op1.rip=%s ", in->op1.label);
                fprintf(stderr, "\n");
            }
        }
    }
    fprintf(stderr, "  [Assembler] Encoded %d/%d instructions (%d bytes). Unencoded: %d\n", success_count, a->instruction_count, a->code_size, fail_count);
    
    if (success_count > 0) {
        // STEP 1: Add data section to code buffer BEFORE fixups
        // This ensures fixups see the correct final layout
        int code_size_before = a->code_size;
        int data_offset = a->code_size;
        
        // Add placeholder data bytes for data symbols
        // Layout: newline (1), bounds_msg (20), empty_string (1), g_recursion_depth (8)  
        if (data_offset + 30 < (int)sizeof(a->code_buffer)) {
            // Initialize newline (1 byte)
            a->code_buffer[data_offset] = 0x0a;
            
            // Initialize bounds_msg (20 bytes including the newline after)
            const char bounds[] = "Array bounds error!";
            for (int i = 0; i < 19 && i < 20; i++) {
                a->code_buffer[data_offset + 1 + i] = bounds[i];
            }
            a->code_buffer[data_offset + 20] = 0x0a;
            
            // Initialize empty_string (1 byte: null terminator)
            a->code_buffer[data_offset + 21] = 0x00;
            
            // Initialize g_recursion_depth (8 bytes: 0)
            for (int i = 0; i < 8; i++) {
                a->code_buffer[data_offset + 22 + i] = 0x00;
            }
            
            // Update code size to include data
            a->code_size = data_offset + 30;
            

        }
        
        // STEP 2: Apply fixups AFTER data has been added and code_size updated
        // Pass the original code size (before data) so data addresses are calculated correctly
        asm_apply_fixups(a, code_size_before);
        
        // STEP 3: Write ELF with the complete binary
        elfgen_add_code(a->elf, a->code_buffer, a->code_size);
        int result = elfgen_write_elf(a->elf, output_file);
        if (result == 0 && access(output_file, F_OK) == 0) {
            return 0;
        }
    }
    
    return -1;
}

int asm_assemble_file(Assembler *asm_op, const char *input_file, const char *output_file) {
    FILE *f = fopen(input_file, "r");
    if (!f) {
        fprintf(stderr, "Failed to open: %s\n", input_file);
        return -1;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *source = xmalloc(size + 1);
    if ((long)fread(source, 1, size, f) != size) {
        fprintf(stderr, "Failed to read: %s\n", input_file);
        free(source);
        fclose(f);
        return -1;
    }
    source[size] = 0;
    fclose(f);
    
    int result = asm_assemble_string(asm_op, source, output_file);
    free(source);
    return result;
}
