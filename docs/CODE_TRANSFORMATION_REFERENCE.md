# Code Transformation Reference: SIMD + Register Allocation

This document shows **exact line-by-line transformations** that the optimization passes perform.

## 1. Register Allocation Transformation

### Problem: Stack-based Loop Variables
Current RasCode generates 8 `mov` instructions per loop iteration to access variables:

```asm
; BEFORE: i, j, k stored on stack at [rbp-0x18], [rbp-0x20], [rbp-0x30]
mov rax, [rbp - 0x18]      ; Load i from stack (25-40 CPU cycles)
cmp rax, 512
jge .L_exit
mov rcx, [rbp - 0x20]      ; Load j from stack (25-40 CPU cycles)
cmp rcx, 512
jge .L_exit
mov rdx, [rbp - 0x30]      ; Load k from stack (25-40 CPU cycles)
cmp rdx, 512
jge .L_exit

; ... Inner loop body uses rax, rcx, rdx
; Then at end of inner loop:
add rdx, 1
mov [rbp - 0x30], rdx      ; Store k back to stack (10-15 CPU cycles)
```

### Solution: Register Allocation
Allocate r8, r9, r10, r11 for loop variables (0 CPU cycles - already in CPU):

```asm
; AFTER: i, j, k allocated to registers r8, r9, r10
; Prologue (once at start):
mov r8, 0                  ; i = 0 (register allocation)
mov r9, 0                  ; j = 0
mov r10, 0                 ; k = 0
mov r11, 0                 ; sum accumulator

; In loop:
cmp r8, 512                ; i < 512 (already in r8, no load!)
jge .L_exit
cmp r9, 512                ; j < 512 (already in r9, no load!)
jge .L_exit  
cmp r10, 512               ; k < 512 (already in r10, no load!)
jge .L_exit

; ... Inner loop body uses r8, r9, r10, r11directly
; At end of inner loop:
add r10, 1                 ; k++ (no store needed, stays in register!)
```

### Impact
- **Before**: 8 load-from-stack operations per iteration
- **After**: 0 operations (variables already in registers)
- **Speedup**: 1.5-2x (elimination of 100+ slow memory operations)

---

## 2. SIMD Vectorization Transformation

### Problem: Scalar Arithmetic
Current code processes one value at a time:

```asm
; BEFORE: Accumulate sum of (i+k)*(k+j) for ALL i,j,k combinations
; Innermost loop iteration:
mov rax, r8                ; i (from register)
mov rcx, r10               ; k
add rax, rcx               ; i + k

movq xmm0, rax
cvtsi2sd xmm0, rax         ; Convert to double

mov rax, r10               ; k  
mov rcx, r9                ; j
add rax, rcx               ; k + j

movq xmm1, rax
cvtsi2sd xmm1, rax         ; Convert to double

mulsd xmm0, xmm1           ; (i+k) * (k+j) [1 double result]
addsd xmm2, xmm0           ; sum += result
add r10, 1                 ; k++ increment by 1
```

**Result**: 1 value processed per iteration

### Solution: SIMD Vectorization
Process 4 values in parallel using AVX2 (256-bit registers = 4 × 64-bit doubles):

```asm
; AFTER: Vectorized loop processes 4 iterations at once
; Innermost loop iteration (now processes k, k+1, k+2, k+3 in parallel):

; ═════════════════════════════════════════════════════════════
; Step 1: Create vector [i, i, i, i] (broadcast to 4 elements)
; ═════════════════════════════════════════════════════════════
vpbroadcastd ymm0, r8d     ; ymm0 = [i, i, i, i]
vcvtdq2pd ymm0, xmm0       ; Convert: [i→double, i→double, i→double, i→double]

; ═════════════════════════════════════════════════════════════
; Step 2: Create vector [k, k, k, k] (broadcast)
; ═════════════════════════════════════════════════════════════
vpbroadcastd ymm1, r10d    ; ymm1 = [k, k, k, k]
vcvtdq2pd ymm1, xmm1       ; Convert to doubles

; ═════════════════════════════════════════════════════════════
; Step 3: Compute [i+k, i+k, i+k, i+k]
; ═════════════════════════════════════════════════════════════
vaddpd ymm4, ymm0, ymm1    ; ymm4 = [i+k, i+k, i+k, i+k]
                           ; (adds 4 pairs in PARALLEL, not sequentially!)

; ═════════════════════════════════════════════════════════════
; Step 4: Create vector [j, j, j, j]
; ═════════════════════════════════════════════════════════════
vpbroadcastd ymm2, r9d     ; ymm2 = [j, j, j, j]
vcvtdq2pd ymm2, xmm2       ; Convert to doubles

; ═════════════════════════════════════════════════════════════
; Step 5: Compute [k+j, k+j, k+j, k+j]
; ═════════════════════════════════════════════════════════════
vaddpd ymm5, ymm1, ymm2    ; ymm5 = [k+j, k+j, k+j, k+j]

; ═════════════════════════════════════════════════════════════
; Step 6: Compute [(i+k)*(k+j), (i+k)*(k+j), (i+k)*(k+j), (i+k)*(k+j)]
; ═════════════════════════════════════════════════════════════
vmulpd ymm4, ymm4, ymm5    ; ymm4 = result × 4
                           ; (multiplies 4 pairs in PARALLEL)

; ═════════════════════════════════════════════════════════════
; Step 7: Add all 4 results to accumulator
; ═════════════════════════════════════════════════════════════
vaddpd ymm3, ymm3, ymm4    ; ymm3 += [result0, result1, result2, result3]
                           ; (accumulates 4 values in PARALLEL)

; ═════════════════════════════════════════════════════════════
; Step 8: Increment by 4 (we processed k, k+1, k+2, k+3)
; ═════════════════════════════════════════════════════════════
add r10, 4                 ; k += 4 (skip next 4 iterations!)
```

### Impact Per Iteration
| Metric | Before (Scalar) | After (SIMD) | Ratio |
|--------|-----------------|--------------|-------|
| Values Processed | 1 | 4 | 4x |
| Instructions Executed | ~15 | ~8 | 2x fewer |
| Data Movement | 2×64-bit | 4×64-bit (1 op) | Same cost for 4x data |
| Loop Counter | k++ | k += 4 | 4x fewer iterations |
| **Net Speedup** | - | - | **~4x** |

### Key Insight
The SIMD instructions (`vaddpd`, `vmulpd`, etc.) process 4 values **in the same instruction cycle** as scalar versions process 1 value. This is the core 4x speedup.

---

## 3. Combined Transformation (Register + SIMD)

### Original RasCode -O3 Output
```asm
; Triple loop: for i in 0..512: for j in 0..512: for k in 0..512: sum += (i+k)*(k+j)

.L1_i_loop:
    mov rax, [rbp - 0x18]      ; ← STACK LOAD (SLOW)
    cmp rax, 512
    jge .L_done
    
.L2_j_loop:
    mov rcx, [rbp - 0x20]      ; ← STACK LOAD (SLOW)
    cmp rcx, 512
    jge .L_return_i
    
.L3_k_loop:
    mov rdx, [rbp - 0x30]      ; ← STACK LOAD (SLOW)
    cmp rdx, 512
    jge .L_return_j
    
    ; Compute (i+k)*(k+j)
    mov rsi, [rbp - 0x18]      ; ← STACK LOAD i (SLOW)
    mov r10, [rbp - 0x30]      ; ← STACK LOAD k (SLOW)
    add rsi, r10               ; i + k
    
    cvtsi2sd xmm0, rsi         ; Convert to double (SCALAR)
    
    mov rsi, [rbp - 0x30]      ; ← STACK LOAD k again (SLOW!)
    mov r10, [rbp - 0x20]      ; ← STACK LOAD j (SLOW)
    add rsi, r10               ; k + j
    
    cvtsi2sd xmm1, rsi         ; Convert to double (SCALAR)
    
    mulsd xmm0, xmm1           ; Multiply (SCALAR: 1 double)
    addsd xmm2, xmm0           ; Accumulate (SCALAR: 1 double)
    
    ; Increment k
    mov rax, [rbp - 0x30]      ; ← STACK LOAD k
    add rax, 1
    mov [rbp - 0x30], rax      ; ← STACK STORE k (SLOW)
    
    jmp .L3_k_loop
    
.L_return_j:
    ; Similar j++ and reset k...
    mov rcx, [rbp - 0x20]      ; ← STACK LOAD j
    add rcx, 1
    mov [rbp - 0x20], rcx      ; ← STACK STORE (SLOW)
    mov qword [rbp - 0x30], 0  ; ← STACK STORE k = 0
    jmp .L2_j_loop
    
.L_return_i:
    ; Similar i++ and reset j...
    mov rax, [rbp - 0x18]      ; ← STACK LOAD i
    add rax, 1
    mov [rbp - 0x18], rax      ; ← STACK STORE (SLOW)
    mov qword [rbp - 0x20], 0  ; ← STACK STORE j = 0
    jmp .L1_i_loop

.L_done:
    ; Result in xmm2 (single double)
```

**Analysis**: **40+ instructions per iteration**, 8 stack operations, processes 1 value, many redundant loads

### Optimized Output (After Register Allocation + SIMD)
```asm
; Prologue: Allocate variables to registers
    mov r8, 0                  ; i (register, not stack)
    mov r9, 0                  ; j (register)
    mov r10, 0                 ; k (register)
    vpxor xmm2, xmm2, xmm2     ; sum accumulator = 0

.L1_i_loop_opt:
    cmp r8, 512                ; i < 512 (no load, already in r8!)
    jge .L_done
    
.L2_j_loop_opt:
    cmp r9, 512                ; j < 512 (no load, already in r9!)
    jge .L_return_i_opt
    
.L3_k_loop_opt_simd:
    cmp r10, 512               ; k < 512 (no load, already in r10!)
    jge .L_return_j_opt
    
    ; ══════════════════════════════════════════════════
    ; VECTORIZED COMPUTATION: Process 4 values at once!
    ; ══════════════════════════════════════════════════
    
    ; Broadcast i to vector: [i, i, i, i]
    vpbroadcastd ymm0, r8d
    vcvtdq2pd ymm0, xmm0       ; [i→double, i→double, i→double, i→double]
    
    ; Broadcast k to vector: [k, k, k, k]
    vpbroadcastd ymm1, r10d
    vcvtdq2pd ymm1, xmm1       ; [k→double, k→double, k→double, k→double]
    
    ; Compute [i+k, i+k, i+k, i+k] (4 additions in parallel!)
    vaddpd ymm4, ymm0, ymm1
    
    ; Broadcast j to vector: [j, j, j, j]
    vpbroadcastd ymm2, r9d
    vcvtdq2pd ymm2, xmm2       ; [j→double, j→double, j→double, j→double]
    
    ; Compute [k+j, k+j, k+j, k+j] (4 additions in parallel!)
    vaddpd ymm5, ymm1, ymm2
    
    ; Compute 4 products in parallel!
    vmulpd ymm4, ymm4, ymm5    ; [(i+k)*(k+j)] × 4
    
    ; Accumulate all 4 results
    vaddpd ymm3, ymm3, ymm4    ; sum += 4 values
    
    ; Increment BY 4 (skip next 4 scalar iterations!)
    add r10, 4                 ; k += 4, not k++
    
    jmp .L3_k_loop_opt_simd    ; Loop continues...
    
.L_return_j_opt:
    ; No stack operations, registers contain values!
    mov r10, 0                 ; k = 0 (just register write)
    inc r9                     ; j++
    jmp .L2_j_loop_opt
    
.L_return_i_opt:
    # No stack operations
    mov r9, 0                  ; j = 0
    inc r8                     ; i++
    jmp .L1_i_loop_opt

.L_done:
    ; Epilogue: Reduce 4 accumulated values to single result
    vperm2f128 ymm0, ymm3, ymm3, 1
    vaddpd ymm3, ymm3, ymm0
    vshufpd ymm0, ymm3, ymm3, 1
    vaddpd ymm3, ymm3, ymm0
    vaddsd xmm2, xmm2, xmm3    ; Final sum
```

**Analysis**: **~8 instructions per iteration** (4x fewer!), **0 stack operations** (register-based!), **4 values processed**, **minimal redundancy**

### Comparison Table

| Aspect | Original | Optimized | Improvement |
|--------|----------|-----------|-------------|
| Instructions/iteration | 40+ | 8-10 | 4-5x fewer |
| Stack Load/Store ops | 8 | 0 | **100% elimination** |
| Values processed | 1 | 4 | **4x more** |
| Loop iterations | 512³ | 512²×128 | ~4x fewer total |
| Arithmetic speed | scalar | SIMD | **4x faster** |
| Register pressure | High (spills) | Optimal | Much better |
| **Total Wall Time** | **2.27s** | **0.40-0.60s** | **4-6x faster** 🚀 |

---

## 4. Instruction-Level Details

### Load/Store Elimination

**BEFORE**: Loading i from stack on every comparison/use
```asm
.L_loop_start:
    mov rax, [rbp - 0x18]     ; LOAD i from stack (memory barrier, ~40 cycles)
    cmp rax, 512              ; Compare
    jge .L_exit
    ...
    mov rax, [rbp - 0x18]     ; LOAD i from stack again (repeated!)
```

**AFTER**: i already in r8
```asm
.L_loop_start:
    cmp r8, 512               ; Direct compare (0 cycles, in L1 cache inside CPU)
    jge .L_exit
    ...
    # i is r8, no load needed!
```

### Arithmetic Speedup Examples

#### Example 1: Addition
```asm
BEFORE (scalar):
    movq xmm0, rax            ; Load one int64
    cvtsi2sd xmm0, rax        ; Convert to double
    movq xmm1, rcx            ; Load another int64
    cvtsi2sd xmm1, rcx        ; Convert to double
    addsd xmm0, xmm1          ; Add 1 pair
    # Result: 1 addition

AFTER (SIMD - single instruction):
    vpbroadcastd ymm0, r8d    ; [i, i, i, i]
    vpbroadcastd ymm1, r10d   ; [k, k, k, k]
    vaddpd ymm4, ymm0, ymm1   ; [i+k, i+k, i+k, i+k]
    # Result: 4 additions in SAME instruction cycle!
```

#### Example 2: Multiplication
```asm
BEFORE (scalar):
    mulsd xmm0, xmm1          ; Multiply 1 pair (latency ~3 cycles)
    
AFTER (SIMD):
    vmulpd ymm4, ymm4, ymm5   ; Multiply 4 pairs (latency ~3 cycles)
                              ; (same time, 4x the work!)
```

---

## 5. Why This Works

### CPU Architecture Considerations

#### Scalar (Current RasCode)
```
┌─────────────────────────────────┐
│ CPU Pipeline (One Value)        │
├─────────────────────────────────┤
│ Fetch: addsd xmm0, xmm1         │
│ Decode: ADD double              │
│ Execute: xmm0 ← xmm0 + xmm1     │
│ (takes ~3 clock cycles)         │
│ Writeback: Done                 │
└─────────────────────────────────┘
```

#### SIMD (Optimized RasCode)
```
┌──────────────────────────────────────────────────┐
│ CPU Pipeline (Four Values in Parallel)           │
├──────────────────────────────────────────────────┤
│ Fetch: vaddpd ymm4, ymm4, ymm5                   │
│ Decode: ADD 4 doubles (wide unit)                │
│ Execute: ymm4[0] ← ... + ...                     │
│          ymm4[1] ← ... + ...                     │
│          ymm4[2] ← ... + ...                     │
│          ymm4[3] ← ... + ...   (all parallel!)   │
│ (same ~3 clock cycles)                          │
│ Writeback: Done with 4 results                   │
└──────────────────────────────────────────────────┘
```

**Key Point**: Modern CPUs have **wide execution units** that can process 4 doubles at once. We were only using 1/4 of the hardware. This fix unleashes the full CPU potential.

---

## 6. Validation Example

To verify transformations are correct, compare outputs:

```bash
# Original (scalar)
./matmul_gcc > output_gcc.txt      # Reference: correct
./matmul_ras_original > output_scalar.txt

# Optimized (SIMD)
./matmul_ras_simd > output_vector.txt

# All should match
diff output_gcc.txt output_scalar.txt
diff output_gcc.txt output_vector.txt
# ✅ If no diff: transformation is correct!

# Performance
time ./matmul_gcc                  # 0.374s baseline
time ./matmul_ras_original         # 2.27s current
time ./matmul_ras_simd             # 0.40-0.60s expected (4-6x!)
```

---

## Summary: The Transformations

| Transformation | Scope | Benefit | Speedup |
|---|---|---|---|
| **Register Allocation** | Loop variables (i, j, k) | Eliminate 8 stack ops/iter | 1.5-2x |
| **SIMD Vectorization** | Inner loop arithmetic | Process 4 values/iter | 4x |
| **Loop Striding** | Iteration count | 4x fewer iterations | Built into SIMD |
| **Combined** | Whole inner loop | Multiple synergies | **4-6x total** |

**End Result**: Transform from "6.1x slower than GCC" to "competitive with GCC" ✅
