# RasCode Syntax Analysis - Quick Reference

**Generated**: April 4, 2026  
**Status**: ✅ COMPLETE

## Key Findings

### Syntax Feature Count
- **33 Keywords** (control flow, data, I/O, types, logic)
- **79 Token Types** (operators, delimiters, literals)  
- **37 AST Node Types** (statements and expressions)
- **81 Builtin Functions** (system, memory, math, concurrency, etc.)
- **8 Primitive Data Types** (int, deci, char, str, bool, byte, ubyte, none)
- **36 Binary Operators** + **8 Unary Operators**
- **13 Operator Precedence Levels**

### Distinctive RasCode Syntax Elements

1. **Bracket notation** `[...]` for:
   - Function calls: `name[args]`
   - Control structures: `if[cond]`, `while[cond]`, `loop[init;cond;incr]`
   - I/O: `show[expr]`, `read[var]`
   - Builtin calls: `@function[args]`

2. **Curly braces** `{...}` for:
   - Blocks & scopes
   - Array initialization: `arr{int, 10} x = {1,2,3}`
   - Type specifications: `arr{type, size}`, `map{k_type, v_type}`
   - Array indexing: `array{index}` (not `array[index]`)

3. **Dot operator** `.` for struct/group member access
   - Chaining: `object.member1.member2`
   - Array member access: `array{i}.member`

4. **Arrow operator** `->` for map operations
   - `map->set[key, value]`
   - `map->get[key]`
   - `map->has[key]`
   - `map->remove[key]`

5. **String interpolation** with `$`
   - Simple: `"Hello $name"`
   - Expression: `"Result: ${x + y}"`

6. **Builtin functions** prefixed with `@`
   - Syntax: `@functionName[args...]`
   - Unified type conversion: `@type[value]::int`
   - 81 functions across 15 categories

### Major Feature Categories

#### Control Flow
- `if/or` (if-else), `while`, `loop` (for-style)
- `cycle/when/fixed` (switch-case)
- `check/when` (try-catch)
- `get` (return)

#### Data Structures  
- Primitive types: int, deci, char, str, bool, byte, ubyte, none
- Arrays: `arr{type, size} name`
- Maps: `map{keyType, valueType} name`
- Groups/Structs: `group Name { fields... }`

#### I/O
- `show[expr]` (output, no newline)
- `read[var]` or `read["prompt"]` (input)

#### Advanced Features
- **81 Builtin Functions**: System, Memory, Crypto, Concurrency, Hardware, Networking, File I/O, Math, String operations
- **Safety Features**: Array bounds checking (default on), overflow detection (configurable)
- **Concurrency**: Mutexes, semaphores, condition variables, atomic ops, channels, thread pools
- **Hardware**: Port I/O, IRQ control, memory-mapped I/O
- **Meta**: Compiler version, build time, syscall access

### Number Literal Support
- Decimal: `123`
- Hexadecimal: `0xFF`, `0x1A`
- Binary: `0b1010`, `0B0011`
- Octal: `077`, `0123`

### Comment Styles
- Single-line: `//`
- Multi-line: `//>` ... `<//`

### Escape Sequences in Strings
`\n`, `\t`, `\r`, `\b`, `\v`, `\f`, `\a`, `\\`, `\"`, `\$`

### Security & Performance
- **Bounds checking**: `-fbounds-check` (default on) / `-fno-bounds-check`
- **Overflow checking**: `-foverflow-check` / `-fno-overflow-check` (default off)
- **Recursion limit**: 1000 nesting levels
- **String limit**: 1 MB per literal

### Complete Documentation
See [SYNTAX_ANALYSIS.md](SYNTAX_ANALYSIS.md) for:
- Full keyword list with descriptions
- All 79 token types
- All 37 AST node types with parser locations
- All 81 builtin functions organized by category
- Detailed syntax features with examples
- Line-by-line source code references
- Complete operator precedence table

---

## Files Analyzed
- `src/lexer.c` - Tokenization (1000+ lines)
- `src/parser.c` - Parsing (2000+ lines)
- `src/ast.c` - AST management
- `src/codegen.c` - Code generation
- `src/builtins.c` - Builtin function definitions
- `include/lexer.h` - Token type definitions
- `include/ast.h` - AST node type definitions
- `include/codegen.h` - Codegen structures

## Verification Status
✅ All keywords extracted and cross-referenced
✅ All token types documented
✅ All AST node types catalogued  
✅ All 81 builtin functions listed with signatures
✅ All operators documented with precedence
✅ All string/number literal formats identified
✅ All special syntax features explained
✅ Undocumented/recent features noted
✅ Line number references verified

---

**Ready for**: Documentation validation, feature completeness verification, developer reference
