# HARDCODE AUDIT REPORT - RASLang Documentation Gaps

**Date:** March 31, 2026  
**Status:** CRITICAL GAPS IDENTIFIED  
**Action:** Documentation needs expansion and corrections

---

## Summary of Findings

### ✅ CORRECT IN CURRENT DOCS
- Compiler name: `rascom` ✓
- Keywords count: 36+ ✓
- Operator precedence: 14 levels ✓
- 8 primitive types ✓
- Entry point requirement: `fnc main[]::int` ✓
- Unique syntax: brackets `[]`, braces `{}` ✓

### ❌ GAPS & INACCURACIES

#### 1. BUILTIN COUNT UNDERESTIMATED
- **Current docs:** "100+ builtins"
- **Audit reality:** 88-96 functions documented + 30+ in Phase 5
- **Issue:** Missing 4 major categories entirely
- **Action:** Add 4 new builtin documentation files

#### 2. MISSING CATEGORIES (NOT DOCUMENTED)
| Category | Count | Status | Examples |
|----------|-------|--------|----------|
| String Manipulation | 12 | ❌ MISSING | @split, @upper, @lower, @trim, @index, @replace, @startswith, @endswith, @reverse, @repeat, @pad, @str_join |
| Math Operations | 12 | ❌ MISSING | @isqrt, @pow, @abs, @min, @max, @clz, @ctz, @popcount, @gcd, @lcm, @isprime, @modpow |
| Error Handling | 10 | ❌ MISSING | @error, @assert, @get_error_code, @get_error_msg, @check_alloc, @try_syscall, @try_fopen, @log_error, @recover, @clear_error |
| Process/Resource | 10 | ❌ MISSING | @fork, @wait, @wait_any, @getpid, @signal, @environ, @argv, etc. |

#### 3. BROKEN BUILTIN FILE ORGANIZATION
- **Current:** Single 16-BUILTINS.md (88 functions crammed into one file)
- **Audit:** Functions span 18+ categories across 30+ include/builtins.h entries
- **Action:** Split into 4 builtin files by category

#### 4. OPERATOR PRECEDENCE COUNT
- **Current docs:** "13-level"
- **Audit found:** 14 levels (unary postfix is separate from unary prefix)
- **Action:** Update precedence table

#### 5. MISSING SYNTAX DOCUMENTATION
- **Unusual array access:** `arr{index}` (braces, not brackets)
- **Unusual function calls:** `func[arg]` (brackets, not parens)
- **Comments:** `//>` multiline comments NOT documented
- **Float type:** "deci" is PHP-influenced, non-standard
- **Action:** Add quirks/unusual section to README

#### 6. LIMITATIONS NOT CLEARLY STATED
- **Current:** No explicit "What's NOT Supported" section
- **Audit:** Found 13+ explicit limitations
- **Missing:**
  - ❌ No break/continue
  - ❌ No lambdas
  - ❌ No enums
  - ❌ No try-throw-catch (uses check/when)
  - ❌ No inheritance
  - ❌ No garbage collection
  - ❌ No range expressions
  - ❌ No macros
  - ❌ No generics
  - ❌ No operator overloading
  - ❌ No pointers/references
  - ❌ No defer/finally
  - ❌ No variadic functions
- **Action:** Create explicit limitations reference

#### 7. RUNTIME MODULES NOT DOCUMENTED
Compiler links these modules (found in src/main.c:141-155):
- sync.o (synchronization)
- channels.o (channels)
- threadpool.o (thread pools)
- network.o (networking)
- fileio.o (file operations)
- memmap.o (memory mapping)
- hardware.o (hardware I/O)
- advanced.o (advanced operations)
- strings.o (string manipulation)
- math.o (math functions)
- errors.o (error handling)
- process.o (process control)
- concurrency.o (concurrency primitives)
- time.o (time operations)
- sorting.o (sort/search algorithms)

**Action:** Document that these are pre-compiled and linked

#### 8. AUDIT FINDINGS NOT IN DOCS
- **Max file size:** 10 MB (enforced)
- **Max string size:** 1 MB
- **Max expr depth:** 1000 levels
- **Type casting:** Unified `@type[x]::int` syntax (current; deprecated @to_* functions exist)
- **Compound assignments:** `+=`, `-=`, etc. exist
- **Bitwise compound:** `&=`, `|=`, `^=`, `<<=`, `>>=` exist
- **Error handling:** `@error`, `@assert`, `@panic`, etc.

---

## Recommended Documentation Structure

Reorganize from **20 files** → **22 files** with proper builtin categorization:

### Core (unchanged)
1. README.md
2. 01-PROGRAM-STRUCTURE.md
3. 02-SYNTAX-RULES.md (ADD: unusual syntax, comments)
4. 03-DATA-TYPES.md
5. 04-VARIABLES.md
6. 05-CONSTANTS.md
7. 06-OPERATORS.md (UPDATE: 14 levels, add compound assignments)
8. 07-ARRAYS.md
9. 08-MAPS.md
10. 09-GROUPS.md
11. 10-CONDITIONALS.md
12. 11-LOOPS.md
13. 12-TRY-CATCH.md
14. 13-FUNCTIONS.md
15. 14-STRING-INTERPOLATION.md
16. 15-IO-STATEMENTS.md

### Builtins (REORGANIZE: split into 4 files)
17. **16-BUILTINS-CORE.md** (System, Memory, Type, File, Network, Security, Hardware) = ~48 functions
18. **17-BUILTINS-SYNC.md** (Mutex, Semaphore, Cond, Atomic, Channel, ThreadPool) = ~30 functions
19. **18-BUILTINS-STRING-MATH.md** (String Manip 12 + Math 12) = 24 functions
20. **19-BUILTINS-ADVANCED.md** (Error Handling 10 + Process 10 + Time/Sort) = 20+ functions

### Advanced (unchanged but enhanced)
21. 20-MEMORY-OPERATIONS.md
22. 21-CONCURRENCY.md
23. 22-COMPLETE-EXAMPLES.md (ADD: examples using new builtins)

### New Sections
- **LIMITATIONS.md** — Explicit "not supported" list
- **QUIRKS.md** — Unusual syntax/features compared to other languages

---

## Immediate Corrections Needed

### 1. Update Builtin Count in README
- Change: "100+ builtins"  
- To: "88-96 documented functions + Phase 5 extensions"

### 2. Add Limitations Section
Include explicit list of ❌ not supported features

### 3. Document Unusual Syntax
- Array access: `arr{i}` not `arr[i]`
- Function calls: `fn[x]` not `fn(x)`
- Multiline comments: `//>` `<//>` not `/* */`

### 4. Split Builtins Documentation  
Current single 16-BUILTINS.md is too large; split into category-based files

### 5. Update Precedence Level
- Change from "13 levels" to "14 levels"
- Add postfix operators as level 13

### 6. Add Quirks Reference
- Float type called "deci" (not standard)
- Keyword "or" for else (unusual)
- "else" keyword not used
- cycle/when/fixed for switch (non-standard names)

---

## Audit Verification Checklist

- [x] Keywords verified (36+ found in lexer.c)
- [x] Operators verified (14 levels found in parser.c)
- [x] Data types verified (8 primitives)
- [x] Builtins verified (88-96 in builtins.c + header)  
- [x] Compilation pipeline verified (4 stages)
- [x] Entry point verified (fn main[]::int)
- [x] Runtime modules verified (15 linked)
- [x] Limitations verified (13+ explicit)
- [x] Syntax verified (all unusual patterns confirmed)

---

## Files to Update/Create

1. **UPDATE:** README.md — Add limitations, correct builtin count
2. **UPDATE:** 02-SYNTAX-RULES.md — Add unusual syntax section
3. **UPDATE:** 06-OPERATORS.md — Change from 13 to 14 levels
4. **DELETE:** 16-BUILTINS.md (too large)
5. **CREATE:** 16-BUILTINS-CORE.md
6. **CREATE:** 17-BUILTINS-SYNC.md
7. **CREATE:** 18-BUILTINS-STRING-MATH.md
8. **CREATE:** 19-BUILTINS-ADVANCED.md
9. **CREATE:** 23-LIMITATIONS.md
10. **CREATE:** 24-QUIRKS-AND-UNUSUAL.md (optional)

---

## Next Steps

1. ✅ Audit complete - proceed to corrections
2. ⏳ Update existing docs for accuracy
3. ⏳ Split builtins into 4 categor files
4. ⏳ Add new limitation/quirks sections
5. ⏳ Verify all examples use correct syntax
6. ⏳ Final cross-check against source code
