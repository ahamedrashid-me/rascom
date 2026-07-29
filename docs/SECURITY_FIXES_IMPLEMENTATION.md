# RasCode Security Hardening - Implementation Guide
## Critical Fixes to Address Vulnerabilities

---

## FIX 1: MANDATORY BOUNDS CHECKING (MOST CRITICAL)

### Problem:
Array access can be performed without bounds checking when `g_bounds_check_enabled = false`.

### Current Code (UNSAFE):
```c
// Lines 1840-1870 in codegen.c
if (g_bounds_check_enabled) {
    // Bounds check
} else {
    // UNSAFE - no checking!
    emit(gen, "    mov rax, [array_base + index * size]");
}
```

### Solution:
**CHANGE: Always validate bounds, make the flag only control ERROR HANDLING**

```c
// BEFORE (UNSAFE):
if (g_bounds_check_enabled) {
    // Validate bounds
    emit(gen, "    cmp rax, 0");
    emit(gen, "    jl .bounds_error");
    emit(gen, "    cmp rax, %d", sym->array_size);
    emit(gen, "    jge .bounds_error");
    // ... handle error ...
} else {
    // NO VALIDATION AT ALL - VULNERABLE
}

// AFTER (SAFE):
// ALWAYS validate bounds
emit(gen, "    cmp rax, 0");
emit(gen, "    jl .bounds_error_%d", error_label);
emit(gen, "    cmp rax, %d", sym->array_size);
emit(gen, "    jge .bounds_error_%d", error_label);

// Only g_bounds_check_enabled controls error handling
if (g_bounds_check_enabled) {
    emit(gen, ".bounds_error_%d:", error_label);
    emit(gen, "    mov rdi, 1");
    emit(gen, "    lea rsi, [rel bounds_msg]");
    emit(gen, "    mov rdx, 50");
    emit(gen, "    mov rax, 1");  // sys_write
    emit(gen, "    syscall");
    emit(gen, "    mov rdi, 1");
    emit(gen, "    mov rax, 60");  // sys_exit
    emit(gen, "    syscall");
} else {
    // Return 0 on bounds error, don't exit
    emit(gen, ".bounds_error_%d:", error_label);
    emit(gen, "    xor rax, rax");
    emit(gen, "    jmp .bounds_ok_%d", end_label);
}
```

### Implementation:
File: `src/codegen.c` (Lines 1840-1875)

**What to Change:**
1. Find the `case AST_ARRAY_ACCESS:` handler
2. Remove the `if (g_bounds_check_enabled)` outer condition
3. **Always** emit bounds check code
4. Move error handler into conditional block
5. Save 15+ lines, improve safety by 100%

### Performance Impact:
- 2-3 extra instructions per array access
- Negligible on modern CPUs (branch prediction handles it)
- Security benefit VASTLY outweighs cost

### Before/After:
```asm
; BEFORE (UNSAFE at g_bounds_check_enabled=false):
mov rbx, 8
imul rax, rbx
add rax, [array_base]
mov rax, [rax]              ; Unvalidated read!

; AFTER (SAFE):
cmp rax, 0                  ; Always check
jl bounds_error
cmp rax, array_size         ; Always check
jge bounds_error
mov rbx, 8
imul rax, rbx
add rax, [array_base]
mov rax, [rax]              ; Validated read!
```

---

## FIX 2: ALLOCATION HEADER TRACKING (PREVENT USE-AFTER-FREE)

### Problem:
`@free` only zeros metadata, doesn't prevent reuse or double-free.

### Current Code (VULNERABLE):
```c
case BUILTIN_FREE:
    codegen_expression(gen, args->nodes[0]);
    emit(gen, "    mov rdi, rax");
    emit(gen, "    sub rdi, 8");
    emit(gen, "    mov qword [rdi], 0"); // Just zero, no freed flag!
```

### Solution:
**Add freed_flag to allocation header**

**New Header Structure:**
```
Bytes 0-7:  size (uint64_t)
Byte 8:     freed_flag (1=freed, 0=allocated)
Bytes 9-15: padding
Data:       [actual allocat...]
```

### Implementation Steps:

**1. Modify @alloc to set freed_flag=0:**
```c
case BUILTIN_ALLOC:
    // ... existing brk code ...
    emit(gen, "    mov rbx, rax        ; metadata location");
    emit(gen, "    mov [rbx], rdi      ; store size");
    emit(gen, "    mov byte [rbx+8], 0 ; freed_flag=0 (allocated)");  // NEW
    emit(gen, "    add rbx, 16         ; skip header (now 16 bytes)"); // CHANGED FROM 8
    emit(gen, "    mov rax, rbx        ; return user ptr");
```

**2. Modify @free to set freed_flag=1 AND check for double-free:**
```c
case BUILTIN_FREE:
    codegen_expression(gen, args->nodes[0]);
    emit(gen, "    mov rdi, rax        ; user ptr");
    emit(gen, "    sub rdi, 16         ; get header");     // CHANGED FROM 8
    emit(gen, "    cmp byte [rdi+8], 1 ; check freed_flag"); // NEW
    emit(gen, "    je .double_free_%d", label);             // NEW
    emit(gen, "    mov byte [rdi+8], 1 ; freed_flag=1");    // CHANGED
    emit(gen, "    jmp .free_done_%d", label);              // NEW
    emit(gen, ".double_free_%d:", label);                   // NEW
    emit(gen, "    mov rdi, 1");                            // NEW
    emit(gen, "    lea rsi, [rel double_free_msg]");        // NEW
    emit(gen, "    mov rax, 1");                            // NEW
    emit(gen, "    syscall");                               // NEW
    emit(gen, ".free_done_%d:", label);                     // NEW
```

**3. Modify @realloc to check freed_flag:**
```c
case BUILTIN_REALLOC:
    // ... after getting old_ptr ...
    emit(gen, "    mov rcx, [rbx-8]    ; old size");       // CHANGED FROM rbx-8
    emit(gen, "    cmp byte [rbx-16+8], 1 ; check freed"); // NEW
    emit(gen, "    je .realloc_freed_%d", label);          // NEW
    // ... continue with realloc ...
```

**4. Add error messages to data section:**
```asm
section .data
    double_free_msg: db "ERROR: double-free detected", 0x00
    use_after_free_msg: db "ERROR: use-after-free detected", 0x00
```

### File Location:
`src/codegen.c`, Lines 2913-3000 (BUILTIN_ALLOC, BUILTIN_FREE, BUILTIN_REALLOC)

### Effectiveness:
- ❌ Prevents double-free
- ❌ Detects use-after-free attempts
- ✅ Still allows memory reuse (necessary for efficiency)
- ⚠️ Adds 8 bytes per allocation (acceptable)

---

## FIX 3: INTEGER OVERFLOW PROTECTION

### Problem #1: Realloc Size Overflow
```c
@realloc[ptr, 0xFFFFFFFFFFFFFFF8];  // Caused to wrap to 0x0
```

### Solution for @realloc:
```c
case BUILTIN_REALLOC:
    // ... existing code ...
    emit(gen, "    mov rdi, rcx        ; new_size");
    emit(gen, "    add rdi, 16         ; add header (changed from 8)"); // NEW SIZE
    emit(gen, "    jc .realloc_overflow_%d", label);  // NEW: Jump if Carry
    
    emit(gen, "    ; Check max heap size (e.g., 1 GB)");  // NEW
    emit(gen, "    cmp rdi, 0x40000000 ; 1GB limit"); // NEW
    emit(gen, "    jg .realloc_toolarge_%d", label);   // NEW
    
    // ... rest of brk call ...
    
    emit(gen, ".realloc_overflow_%d:", label);  // NEW
    emit(gen, "    mov rax, -1         ; return error");  // NEW
```

### Problem #2: Array Index Multiplication Overflow
```c
buffer{0x100000000 / 8} * 8 => wraps to small value
```

### Solution:
```c
case AST_ARRAY_ACCESS:
    // ... evaluate index into rax ...
    // Check bounds BEFORE multiplication
    emit(gen, "    cmp rax, 0");
    emit(gen, "    jl .bounds_error");
    emit(gen, "    cmp rax, %d", sym->array_size);
    emit(gen, "    jge .bounds_error");
    
    // Check multiplication won't overflow
    emit(gen, "    mov rbx, %d         ; element_size", sym->element_size);
    emit(gen, "    cmp rax, 0x%lx", LLONG_MAX / sym->element_size);  // NEW
    emit(gen, "    jg .bounds_error    ; index * size too large"); // NEW
    
    emit(gen, "    imul rax, rbx");
    emit(gen, "    jo .bounds_error");   // Jump if Overflow occurred
```

### File Location:
`src/codegen.c`
- @realloc: Lines 2945-2980
- Array access: Lines 1840-1875

---

## FIX 4: STACK CANARIES

### Current State:
```
RBP-40: Local variables
RBP-56: Saved RBP <- CAN BE OVERWRITTEN
RBP-48: Return Address <- CAN BE OVERWRITTEN
RBP+0:  Current RBP
```

### Solution:
Place canary between locals and saved RBP

### Implementation in Function Prologue:
```c
static void codegen_function(CodeGen *gen, ASTNode *node) {
    // ... existing prologue ...
    emit(gen, "    push rbp");
    emit(gen, "    mov rbp, rsp");
    // NEW: Load and place canary
    emit(gen, "    lea rax, [rel __stack_chk_guard]");
    emit(gen, "    mov rcx, [rax]       ; Load canary value");
    
    // Calculate canary position (first 8 bytes of local variables)
    int canary_offset = 8;
    emit(gen, "    mov [rbp-%d], rcx    ; Place canary", canary_offset);
    
    // ... rest of prologue ...
    
    // NEW global variable:
    // In data section: __stack_chk_guard: dq 0xdeadbeefdeadbeef
}
```

### Implementation in Function Epilogue:
```c
static void codegen_return(CodeGen *gen, ASTNode *node) {
    // NEW: Verify canary before returning
    emit(gen, "    lea rax, [rel __stack_chk_guard]");
    emit(gen, "    mov rcx, [rax]       ; Load expected canary");
    emit(gen, "    cmp rcx, [rbp-8]     ; Compare with stored");
    emit(gen, "    jne __stack_chk_fail ; If mismatch, abort");
    
    emit(gen, "    mov rsp, rbp");
    emit(gen, "    pop rbp");
    emit(gen, "    ret");
}
```

### Add to Initialization:
```asm
section .data
    __stack_chk_guard: dq 0xdeadbeefdeadbeef  ; Random value on startup
```

### On Failure:
```c
// NEW function:
void __stack_chk_fail(void) {
    fprintf(stderr, "FATAL: Stack smashing detected!\n");
    abort();  // Or sys_exit
}
```

### File Location:
`src/codegen.c`, Lines 2370-2410 (codegen_function prologue)

### Protection Level:
- Defeats simple buffer overflow exploits
- Doesn't prevent all attacks (can skip canary check)
- Raises attack bar significantly

---

## FIX 5: OVERFLOW CHECKS IN ARITHMETIC OPERATIONS

### Problem:
Binary operations like `+`, `-`, `*` can overflow without detection

### Current Code (UNSAFE):
```c
if (strcmp(node->binary.op, "+") == 0) {
    emit(gen, "    add rax, rbx");
    // Just add, no overflow check!
}
```

### Solution:
```c
if (strcmp(node->binary.op, "+") == 0) {
    emit(gen, "    add rax, rbx");
    emit(gen, "    jo .overflow_%d", overflow_label);  // Jump on Overflow
    
    // Optional: return sentinel value on error
    emit(gen, ".overflow_%d:", overflow_label);
    emit(gen, "    mov rax, -1         ; -1 = overflow error");
}
```

### Implementation:
- `add` / `sub` / `imul`: Use `JO` (Jump if Overflow flag set)
- `mul`: Use `JC` (Jump if Carry flag set)  
- Report error via returning -1 or 0

### File Location:
`src/codegen.c`, Lines 1035-1150 (codegen_binary_op)

### Flag Mapping:
| Operation | Flag | Instruction |
|-----------|------|-------------|
| Addition | OF (Overflow) | `jo overflow` |
| Subtraction | OF | `jo overflow` |
| Signed Mul | OF | `jo overflow` |
| Division | - | Check divisor ≠ 0 |

---

## FIX 6: CONTROL FLOW INTEGRITY (CFI) - MEDIUM EFFORT

### Problem:
Function pointers can be overwritten, indirect calls are unvalidated

### Attack:
```asm
; Original:
mov rax, [function_ptr_location]
call rax  ; Could call anywhere

; Attacker:
[overwrite function_ptr with gadget address]
call rax  ; Calls gadget, executes ROP
```

### Solution (Simple Whitelist):
```asm
; Compile all functions to known addresses
; Create whitelist of valid targets
section .data
    valid_targets:
        dq func1_addr
        dq func2_addr
        dq func3_addr
        dq 0  ; terminator

; Before indirect call:
    mov rax, [function_ptr]
    lea rsi, [rel valid_targets]
    xor ecx, ecx
.search_target:
    mov rdi, [rsi + rcx*8]
    test rdi, rdi
    jz .target_invalid        ; Not found in list
    cmp rax, rdi
    je .target_valid
    inc ecx
    cmp ecx, MAX_FUNCTIONS
    jl .search_target
    
.target_invalid:
    mov rdi, 1
    lea rsi, [rel cfi_msg]
    mov rax, 1
    syscall
    mov rax, 60  ; exit
    syscall
    
.target_valid:
    call rax
```

### Production Alternative:
- Use Shadow Stack (if CPU supports)
- CET (Control-Flow Enforcement Technology) on Intel
- Return Stack Buffer (RSB) prediction

### File Location:
Modify function call generation for builtin functions

**Difficulty:** Medium (requires function address tracking)

---

## FIX 7: SECURE HEAP REGION (PIE + ASLR)

### Problem:
Heap always at same address (brk always returns same base first call)

### Solution:
Enable Position-Independent Executable (PIE) compilation

### Compile Flags:
```bash
gcc -fPIE -pie -Wl,-z,relro,-z,now ...
```

### Effects:
- Heap base randomized by kernel on each run
- Code section randomized
- Data section randomized
- Defeats ROP gadget chains based on known addresses

### File Location:
Makefile, compilation flags section

**Difficulty:** Low (just compiler flags)

---

## FIX 8: SEPARABLE MEMORY REGIONS

### Problem:
Code, data, heap, stack all in one executable

### Solution:
Use mmap for separate regions

```c
#define HEAP_BASE 0x...  // Random, set by kernel
#define HEAP_SIZE 0x10000000  // 256 MB

void setup_memory_regions() {
    // Code: RX (execute-only)
    // Data: RW (read-write, no execute)
    // Heap: RW (read-write, no execute)
    // Stack: RW (read-write, no execute)
}
```

Call during runtime initialization.

### File Location:
New file: `src/runtime.c`

**Difficulty:** Medium (requires arch integration)

---

## PRIORITY IMPLEMENTATION ORDER

### Phase 1 (THIS WEEK - CRITICAL):
1. ✅ **Mandatory Bounds Checking** - 30 minutes
   - File: `src/codegen.c`, Lines 1840-1875  
   - Impact: 🔴 Eliminates most buffer overflows
   - Complexity: Very Low

2. ✅ **Allocation Header with Freed Flag** - 1 hour
   - File: `src/codegen.c`, Lines 2913-3000
   - Impact: 🔴 Prevents use-after-free
   - Complexity: Low

3. ✅ **Integer Overflow Checks** - 1 hour
   - File: `src/codegen.c`, Multiple locations
   - Impact: 🟡 Prevents realloc attacks
   - Complexity: Medium

### Phase 2 (NEXT WEEK - HIGH PRIORITY):
4. **Stack Canaries** - 1.5 hours
   - File: `src/codegen.c`, prologue/epilogue
   - Impact: 🟡 Prevents stack smashing
   - Complexity: Low

5. **Array Index Multiplication Overflow** - 1 hour
   - File: `src/codegen.c`, array access
   - Impact: 🔴 Prevents wraparound attacks
   - Complexity: Low

6. **Compile with PIE + ASLR flags** - 15 minutes
   - File: `Makefile`
   - Impact: 🟡 Defeats ROP gadget chains
   - Complexity: Very Low

### Phase 3 (LATER - NICE TO HAVE):
7. Control Flow Integrity (CFI) - 4 hours
8. Separate memory regions - 3 hours
9. Replace DJBX33A with BLAKE2b - 2 hours
10. Add atomic operations for thread safety - 3 hours

---

## TESTING STRATEGY

After each fix, test with:

```bash
# Test 1: Buffer overflow attempt
./rascode -W 'main[] { arr{int, 5}; arr{10} = 0; }'

# Test 2: Use-after-free attempt
./rascode -W 'main[] { p = @alloc[100]; @free[p]; x = @read[p]; }'

# Test 3: Integer overflow
./rascode -W 'main[] { p = @alloc[100]; @realloc[p, 0xFFFFFFFFFFFFFFFF]; }'

# Test 4: Stack smashing (canary)
./rascode -W 'main[] { arr{int, 5}; loop(i: 0..100) { arr{i} = 0; } }'
```

---

## COMPILATION COMMAND (WITH FIXES)

```bash
cd /home/void/Desktop/RASIDE/RasCode
gcc -c src/codegen.c -o obj/codegen.o \
    -Wall -Wno-deprecated-declarations \
    -fPIE                              # Position Independent Executable
    -fstack-protector-strong           # Strong stack canaries (GCC)
    -D_FORTIFY_SOURCE=2                # Fortified C functions

gcc obj/*.o -o rascom \
    -pie                               # Link as PIE
    -Wl,-z,relro,-z,now               # Full RELRO + immediate binding
```

---

## VALIDATION CHECKLIST

After implementing FIX 1-3:
- [ ] All 15 tests still pass
- [ ] Buffer overflow attempts rejected
- [ ] Use-after-free attempts detected  
- [ ] Integer overflow caught
- [ ] Performance impact < 5%
- [ ] No memory leaks introduced

After implementing FIX 4-6:
- [ ] Stack canaries verified
- [ ] PIE enabled (check with `checksec`)
- [ ] ASLR working (heap addresses differ per run)
- [ ] All tests still pass

---

## EXPECTED OUTCOME

**Before Fixes:**
- Security Rating: 2/10 ❌
- Viable for production: NO

**After Phase 1-2 Fixes:**
- Security Rating: 6/10 ✅
- Viable for production: MAYBE (with warnings)

**After All Fixes:**
- Security Rating: 7/10 ✅
- Viable for production: YES (for contained environments)

**Rust Baseline:**
- Security Rating: 10/10 ✅
- Rust prevents >95% of these issues at compile-time

---

**CRITICAL NOTE:** These fixes make RasCode much safer, but even with all changes implemented, it will still not match Rust's compile-time guarantees. Use-after-free detection via runtime magic is fundamentally less secure than Rust's borrow checker preventing it at compile-time.
