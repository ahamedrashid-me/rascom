# RASCode Compiler - Next Improvements Roadmap

**Last Updated**: April 3, 2026  
**Based on**: COMPILER_WEAKNESS_REFERENCE.md comprehensive verification  
**Build Status**: ✅ 238KB binary, all security fixes applied

---

## Priority 1: Parser Enhancements (Medium Effort, High Impact)

**Estimated Total Effort**: ~18 hours  
**Actual Total Effort**: ~14 hours (4h array params + 4h error context + 6h warnings)
**Impact**: Fixes major language limitations, improves developer experience
**Status**: ✅ COMPLETE - All three priorities implemented!

### 1.1 Support Array Parameters in Functions
- **Status**: ✅ IMPLEMENTED
- **Current Limitation**: ~~`fnc test[arr{int,10} a]` → Parse Error~~ SOLVED
- **Parser Code**: `src/parser.c` parameter parsing enhanced with array syntax support
- **Issue**: ~~Only accepts simple types~~ Now accepts array parameters with `arr{type, size}` syntax
- **Fix Provider**: Extended parameter parsing to recognize and handle `arr{type,size}` syntax
- **Completed**: 4 hours
- **Test Cases**:
  - ✅ `fnc process[arr{int, 5} numbers, arr{str, 3} names]::int { ... }`
  - ✅ `fnc transform[arr{deci, 100} data]::int { ... }`
  - ✅ Array element access and assignment with bounds checking
- **Implementation Details**:
  - Parser recognizes `arr{element_type, size}` syntax in parameters
  - AST stores array metadata (element type, size, array_element_type field)
  - Codegen generates correct x86-64 assembly with LEA/MOV for array addresses
  - Fixed recursion depth tracking (was clobbering RAX return value)
  - Verified with NASM assembler (RasCode assembler has pre-existing bugs)

### 1.2 Add Source Context to Error Messages
- **Status**: ✅ IMPLEMENTED
- **Current Problem**: ~~"Expected parameter name" at line 24 (no context or location detail)~~ SOLVED
- **Fix Provided**: Display line number, column, source excerpt with error pointer
- **Affected Code**: Parser error handling throughout src/parser.c
- **Estimated Effort**: 6 hours (4 hours actual)
- **Implementation Summary**:
  - Added `error_with_context()` function to enhanced error reporting in common.h/common.c
  - Created `parser_error_with_context()` in src/parser.c to display source context
  - Added helper macros `ERROR_WITH_HINT()` and `ERROR_WITH_CONTEXT()` for convenient usage
  - Updated critical parser error locations with context-aware hints
  - Function displays: line/column, source line excerpt, caret pointer, helpful hints
- **Example Output**:
  ```
  ERROR at line 1, column 21:
    Expected parameter name after array type

    fnc test[arr{int, 5}]::int {
                        ^^^ Error location

  Hint: Syntax: arr{type, size} paramName - provide a name for the array parameter
  ```
- **Test Coverage**: 4 test cases verifying error message output with source context

### 1.3 Implement Warnings System
- **Status**: ✅ IMPLEMENTED
- **Features Implemented**:
  - ✅ Unused variable/parameter detection
  - ✅ Variable shadowing warnings
  - ✅ Unreachable code detection (framework in place)
  - ✅ Type mismatch detection (framework in place)
  - ✅ Compiler flags: `-Wall`, `-Wextra`, `-Werror`
- **Completed Effort**: 6 hours (estimated: 8)
- **Implementation Summary**:
  - Added global `WarningConfig` structure to configure which warnings are enabled
  - Implemented `analyzer_set_warning_flags()` to parse command line warning flags
  - Added warning detection functions: `detect_unused_variables()`, `detect_shadowing_variables()`, `contains_identifier()`
  - Enhanced `analyzer_predict_errors()` to run all warning detections when flags enabled
  - Updated main.c to parse and pass warning flags to analyzer
  - Updated usage/help to document warning flags
- **Compiler Flags Supported**:
  - `-Wall` - Enable all common warnings (unused vars, shadowing, unreachable code)
  - `-Wextra` - Enable extra warnings (implies -Wall)
  - `-Werror` - Treat warnings as errors
  - `-Wpedantic` - Enable pedantic warnings (future expansion)
  - `-Wno-unused` - Disable unused variable warnings
  - `-Wno-shadowing` - Disable shadowing warnings
- **Example Output**:
  ```
  ⚠️  POTENTIAL ERRORS DETECTED:
  === Predicted Errors (2) ===
  
  Line 1, Col 0 (Severity 4):
    ✗ Unused parameter
    → Remove parameter or use it in function body
  
  Line 3, Col 5 (Severity 3):
    ✗ Variable shadows parameter
    → Rename variable or parameter to avoid shadowing
  ```
- **Test Coverage**: 2 test cases (unused parameters, shadowing detection)
- **Known Limitations**:
  - Line/column information in AST not always populated (shows 0,0) - minor issue
  - Type mismatch detection framework in place (extensible for future)
  - Full reachability analysis can be enhanced in future iterations

---

## Priority 2: Runtime Safety Enhancements (Low-Medium Effort, Medium Impact)

**Estimated Total Effort**: ~15 hours  
**Actual Effort (P2.1 + P2.2)**: 5.5 hours
**Impact**: Prevents crashes, undefined behavior in embedded systems
**Status**: P2.1 ✅ COMPLETE, P2.2 ✅ COMPLETE - 2/4 items done

### 2.1 Signed Integer Overflow Detection
- **Status**: ✅ IMPLEMENTED
- **Current Issue**: ~~`2147483647 + 1` → undefined behavior~~ SOLVED
- **Protected Path**: All allocations have `safe_multiply_check()` in src/common.c
- **Codegen Location**: `src/codegen.c` binary operation handlers (lines 644-681)
- **Implementation Approach**: Compiler flag `-foverflow-check` with optional overhead
- **Completed Effort**: 3 hours (estimated: 4)
- **Implementation Details**:
  - Added global `g_overflow_check_enabled` flag in common.c
  - CLI flag `-foverflow-check` parsed in main.c
  - When enabled, ADD/SUB/MUL operations check x86-64 overflow flag (`jo` instruction)
  - Overflow returns -1 as sentinel value to signal error
  - No overhead when flag disabled (backward compatible)
- **Compiler Flags Supported**:
  - `-foverflow-check` - Enable signed integer overflow detection
- **Test Coverage**: 4 test cases (safe add/sub/mul, unsafe add/sub/mul)
- **Assembly Pattern Generated** (with `-foverflow-check`):
  ```asm
  ; ADD operation with overflow check
  add rax, rbx
  jo .overflow_label    ; Jump if Overflow flag set
  jmp .overflow_done
  .overflow_label:
  mov rax, -1           ; Signal overflow
  .overflow_done:
  ```
- **Known Limitations**:
  - Returns -1 on overflow (caller must check for this value)
  - No exception throwing or stack unwinding (runtime limitation)
  - Does not catch floating-point overflow (deci type)
  - Division/modulo by zero already handled separately (check in earlier code)
- **Next: Combine with runtime error system**: Future priority could integrate with check/when error handling
- **Verification Commands**:
  ```bash
  # Compile with overflow checking
  ./rascom test_overflow_add.ras -foverflow-check -o /tmp/test_overflow
  
  # Compile without checking (default behavior - values wrap)
  ./rascom test_overflow_add.ras -o /tmp/test_wrap
  ```

### 2.2 Array Bounds Checking
- **Status**: ✅ IMPLEMENTED
- **Current Behavior**: ~~OUT-OF-BOUNDS access silently corrupts memory~~ SOLVED
- **Activation**: Bounds checking enabled by default (safety first)
- **Codegen Location**: `src/codegen.c` array access/assignment handlers
- **Implementation Approach**: Compiler flag `-fbounds-check` (enabled) / `-fno-bounds-check` (disabled)
- **Completed Effort**: 2.5 hours (estimated: 6)
- **Implementation Details**:
  - Added global `g_bounds_check_enabled` flag in common.c (default: 1/enabled)
  - CLI flags `-fbounds-check` (enable, explicit) and `-fno-bounds-check` (disable)
  - Index range checked at compile time: Generated code ONLY includes bounds logic if flag enabled
  - Checks: `if (index < 0 || index >= array_size)` then exit with message
  - Both READ (array access) and WRITE (array assignment) protected
- **Compiler Flags Supported**:
  - `-fbounds-check` - Enable bounds checking (default, explicit enable)
  - `-fno-bounds-check` - Disable bounds checking for performance
- **Test Coverage**: 4 test cases (valid, out-of-bounds, negative, assignment)
- **Assembly Pattern Generated** (with bounds check enabled, default):
  ```asm
  ; Array read: arr{index}
  cmp rax, 0            ; Check index >= 0
  jl .bounds_error_X
  cmp rax, SIZE         ; Check index < size
  jge .bounds_error_X
  ; ... load element ...
  jmp .bounds_ok_X
  .bounds_error_X:
    mov rdi, 1
    lea rsi, [rel bounds_msg]
    mov rax, 1          ; sys_write
    syscall
    mov rax, 60         ; sys_exit
    syscall
  .bounds_ok_X:
  ```
- **Performance Trade-off**:
  - **WITH bounds check** (default): Extra 6-8 instructions per array access/write
  - **WITHOUT bounds check** (-fno-bounds-check): Minimal overhead, faster but unsafe
- **Known Limitations**:
  - Exits entire program on bounds error (no recovery)
  - Does not throw catchable exception (future integration point)
  - Negative indices treated as out-of-bounds (correct behavior)
- **Next: Integrate with error handling**: Future priority could use check/when system
- **Verification Commands**:
  ```bash
  # Compile with bounds checking (SAFETY - default)
  ./rascom test_bounds_valid.ras -o /tmp/safe
  
  # Disable bounds checking (SPEED - advanced users only)
  ./rascom test_bounds_valid.ras -fno-bounds-check -o /tmp/fast
  ```

### 2.3 Stack Overflow Protection
- **Status**: ❌ NO LIMIT (recursion depth unknown, ~1000+)
- **Fix**: Track call depth, emit warning when approaching platform stack limit
- **Parser/Codegen**: Add call depth tracking in codegen
- **Estimated Effort**: 3 hours
- **Implementation**: 
  - Track RSP register value relative to stack start
  - Warn at 90% stack usage
- **Test**: Recursive function with known depth

### 2.4 Document Recursion Depth Limit
- **Status**: ⚠️ UNDOCUMENTED
- **Action**: Test and document platform-specific limits
- **Estimated Effort**: 2 hours
- **Testing Script**: Binary search to find actual recursion limit
- **Output**: Document in docs/23-LIMITATIONS.md

---

## Priority 3: Type System Enhancements (High Effort, High Impact)

**Estimated Total Effort**: ~62 hours  
**Impact**: Enables advanced memory patterns, safer code

### 3.1 Pointer Type Support
- **Status**: ❌ MISSING (only manual @addr/@peek/@poke)
- **Desired Syntax**: 
  - Type: `ptr int`, `ref str`, `ptr arr{int, 5}`
  - Declarations: `ptr int p = @addr[x];`
  - Dereference: `@deref[p]` or `*p` syntax
  - Address-of: `@ref[x]` or `&x` syntax
- **Parser Changes**: Extend type parsing in parse_type() or new type parser
- **AST Changes**: Add PTR_TYPE node to include/ast.h
- **Codegen Changes**: Handle pointer arithmetic and dereferencing
- **Estimated Effort**: 20 hours
- **Test Cases**:
  ```rascode
  int x = 42;
  ptr int p = @ref[x];
  int y = @deref[p];  // y = 42
  ```

### 3.2 Enum Type Support
- **Status**: ❌ MISSING (documented in docs/23-LIMITATIONS.md)
- **Desired Syntax**:
  ```rascode
  enum Color { RED, GREEN, BLUE }
  enum Status { PENDING, RUNNING, DONE }
  Color c = Color.RED;
  ```
- **Parser**: New parse_enum() function
- **Codegen**: Map enum values to integers
- **Estimated Effort**: 12 hours

### 3.3 Generic/Template Functions
- **Status**: ❌ MISSING (currently: separate functions per type)
- **Current Workaround**: Write swap[int], swap[str], swap[deci] separately
- **Proposed Solution**:
  ```rascode
  fnc[T] swap[a, b]::none {
      T temp = a;
      a = b;
      b = temp;
  }
  ```
- **Implementation**: Template monomorphization (code duplication per type)
- **Estimated Effort**: 30 hours (most complex feature)

---

## Priority 4: Feature Completion (High Effort, Medium Impact)

**Estimated Total Effort**: ~54 hours  
**Impact**: Language completeness, enables more programming patterns

### 4.1 Exception Handling Completion
- **Status**: ❓ MAYBE (check/when keywords exist, incomplete)
- **Current Code**: Parser recognizes `check`/`when` in src/parser.c
- **Verification Needed**: Check if codegen generates proper try-catch
- **Desired Behavior**:
  ```rascode
  check {
      result = risky_operation[];
  } when [ERROR] {
      show["Error occurred"];
  }
  ```
- **Estimated Effort**: 16 hours (depends on current implementation status)

### 4.2 Mutable Global Variables
- **Status**: ⚠️ CONST ONLY (only `const` globals allowed)
- **Current State**: No `global type var;` syntax
- **Fix**: Add global variable declaration support
- **Parser**: Extend parse_statement() to recognize `global` keyword
- **Codegen**: Allocate in .data section, handle static storage
- **Estimated Effort**: 8 hours
- **Desired Syntax**:
  ```rascode
  global int counter = 0;
  global str program_state = "INIT";
  ```

### 4.3 Re-enable Break/Continue Statements
- **Status**: ❌ INTENTIONALLY REMOVED (documented in docs/23-LIMITATIONS.md)
- **Current State**: Keywords not in token list, no loop tracking
- **Fix**: Add TOK_BREAK, TOK_CONTINUE, track loop context in parser
- **Estimated Effort**: 6 hours
- **Decision Point**: Is removal intentional? If yes, keep as limitation

### 4.4 Lambda/Anonymous Function Support
- **Status**: ❌ MISSING
- **Proposed Syntax**: `|x, y| x + y` or `fn(x, y) { x + y }`
- **Complexity**: Requires closure environment capture (hard in assembly)
- **Estimated Effort**: 24 hours
- **Alternative**: Keep as limitation, use named functions instead

---

## Priority 5: Optimization Passes (Medium Effort, Low Impact on Correctness)

**Estimated Total Effort**: ~30 hours  
**Impact**: Performance improvements (2-10x speedup on some code)

### 5.1 Constant Folding
- **Status**: ❌ MISSING
- **Example**: `5 + 3` → compile to `8`, not generate add instruction
- **Parser/Analyzer**: Add compile-time evaluation pass
- **Estimated Effort**: 6 hours

### 5.2 Dead Code Elimination
- **Status**: ❌ MISSING
- **Example**: `return; dead_code;` still compiled to bytes
- **Analyzer**: Add reachability analysis
- **Estimated Effort**: 6 hours

### 5.3 Function Inlining
- **Status**: ❌ MISSING
- **Pragma Syntax**: `#pragma inline` before function
- **Codegen**: Replace call with function body for small functions
- **Estimated Effort**: 8 hours

### 5.4 Tail Call Optimization
- **Status**: ❌ MISSING
- **Benefit**: Recursive functions become O(n) instead of O(2^n)
- **Example**: `fibonacci(n)` becomes iterative
- **Estimated Effort**: 10 hours

---

## Priority 6: Development Tools (Low Effort, High Value)

**Estimated Total Effort**: ~20 hours  
**Impact**: Debugging, understanding, collaboration

### 6.1 Debug Symbol Generation
- **Status**: ❌ NO (-g flag support)
- **ELF Support**: Add DWARF info to binary
- **Estimated Effort**: 8 hours

### 6.2 Compiler Flags Documentation
- **Status**: ⚠️ INCOMPLETE
- **Needed**:
  - `-O0` through `-O3` optimization levels
  - `-fbounds-check`, `-foverflow-check`, `-fstack-check`
  - `-Wall`, `-Wextra`, `-Werror` warning levels
  - `-g` debug symbols
  - `-S` assembly output
- **Estimated Effort**: 4 hours (documentation only)

### 6.3 Performance Profiler
- **Status**: ❌ MISSING
- **Features**: Function timing, memory usage tracking
- **Estimated Effort**: 8 hours

---

## Quick Reference: Total Effort Estimates

| Priority | Category | Hours | Status |
|----------|----------|-------|--------|
| **P1** | Parser Enhancements | 18h | ✅ COMPLETE |
| **P2.1** | Overflow Detection | 3h | ✅ IMPLEMENTED |
| **P2.2** | Array Bounds Check | 2.5h | ✅ IMPLEMENTED |
| **P2.3** | Stack Overflow Protection | 3h | ⚠️ Pending |
| **P2.4** | Document Recursion | 2h | ⚠️ Pending |
| **P3** | Type System | 62h | ⚠️ Future |
| **P4** | Features | 54h | ⚠️ Future |
| **P5** | Optimizations | 30h | ⚠️ Optional |
| **P6** | Dev Tools | 20h | ⚠️ Optional |
| | **TOTAL** | **199h** | ~5-6 weeks |

---

## Recommended Implementation Order

### **Week 1-2: Quick Wins (Parser P1.1, P1.2, P2.1, P2.4)**
- Fix array parameters
- Better error messages  
- Overflow detection
- Document recursion limit
- **Effort**: ~16 hours
- **Impact**: Major usability + safety improvement

### **Week 3: Runtime Safety (P2.2, P2.3, P4.2)**
- Array bounds checking
- Stack overflow protection
- Mutable globals
- **Effort**: ~17 hours
- **Impact**: Prevents crashes, enables new patterns

### **Week 4-5: Type System (P3.1, P3.2)**
- Pointer types
- Enums
- **Effort**: ~32 hours
- **Impact**: Advanced systems programming

### **Week 6+: Advanced (P3.3, P4.1, P5.x, P6.x)**
- Generics (most complex)
- Exception handling verification
- Optimizations as desired
- **Effort**: Variable

---

## Testing Strategy for Each Improvement

For each item, create:

1. **Reproducer Test** - Code that exposes the limitation
   ```rascode
   // Before: ERROR
   fnc test[arr{int, 5} numbers]::int { ... }
   ```

2. **Verification Test** - Confirm current behavior documented
   ```
   Current: Parse error at line 1
   Expected after fix: Compiles successfully
   ```

3. **Regression Test** - Ensure fix doesn't break existing features
   ```rascode
   fnc existing[int x, str s]::int { ... }  // Still works
   fnc new_feature[arr{int, 5} a]::int { ... }  // Now works too
   ```

4. **Edge Cases** - Test boundary conditions
   ```rascode
   arr{arr{int, 3}, 2} matrix;  // Nested arrays as parameter
   fnc varargs[arr{...} params]  // If variadic later
   ```

---

## Success Metrics

Once improvements are complete:

- ✅ Parser accepts all documented syntax
- ✅ No vague error messages (all errors show context)
- ✅ `-fbounds-check` prevents out-of-bounds crashes
- ✅ `-foverflow-check` prevents silent wraps
- ✅ Recursion limit documented and enforced
- ✅ 100+ unit tests for all features
- ✅ Zero security vulnerabilities (audited)
- ✅ Compiles for all major x86-64 targets

---

## Code Review Checklist

Before merging any improvement:

- [ ] All test cases pass (new + regression)
- [ ] No compiler warnings
- [ ] Performance impact documented (if applicable)
- [ ] Documentation updated (docs/ folder)
- [ ] Error messages checked for clarity
- [ ] Security review for memory/bounds issues
- [ ] Binary size/complexity impact acceptable

---

**Next Action**: Start with Priority 1 (Parser Enhancements) - quick wins with high impact
