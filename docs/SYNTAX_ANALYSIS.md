# RasCode Compiler - Complete Syntax Analysis

**Analysis Date**: April 4, 2026  
**Source Files Analyzed**: `lexer.c`, `parser.c`, `ast.c`, `codegen.c`, `builtins.c`  
**Analysis Level**: Comprehensive (Lexical, Parse, AST, Codegen layers)

---

## 1. KEYWORDS (33 total)

All keywords defined in [src/lexer.c](src/lexer.c#L4-L38):

### Control Flow Keywords
| Keyword | Token | Purpose | Line |
|---------|-------|---------|------|
| `fnc` | TOK_FNC | Function declaration | lexer.c:6 |
| `get` | TOK_GET | Return statement | lexer.c:7 |
| `if` | TOK_IF | Conditional statement | lexer.c:9 |
| `or` | TOK_OR | Else/elif clause | lexer.c:10 |
| `while` | TOK_WHILE | While loop | lexer.c:11 |
| `loop` | TOK_LOOP | For-style loop | lexer.c:8 |
| `cycle` | TOK_CYCLE | Switch/case statement | lexer.c:12 |
| `when` | TOK_WHEN | Case clause in cycle/check | lexer.c:13 |
| `fixed` | TOK_FIXED | Default case in cycle | lexer.c:14 |
| `check` | TOK_CHECK | Try-catch exception handling | lexer.c:15 |

### Data Declaration Keywords
| Keyword | Token | Purpose | Line |
|---------|-------|---------|------|
| `arr` | TOK_ARR | Array declaration | lexer.c:22 |
| `map` | TOK_MAP | Map/dictionary declaration | lexer.c:23 |
| `group` | TOK_GROUP | Struct/record declaration | lexer.c:20 |
| `set` | TOK_SET | Map operations keyword | lexer.c:21 |
| `const` | TOK_CONST | Global constant declaration | lexer.c:26 |

### I/O Keywords
| Keyword | Token | Purpose | Line |
|---------|-------|---------|------|
| `show` | TOK_SHOW | Output without newline | lexer.c:19 |
| `read` | TOK_READ | Input from user | lexer.c:18 |

### Module/Package Keywords
| Keyword | Token | Purpose | Line |
|---------|-------|---------|------|
| `pkg` | TOK_PKG | Package import/naming | lexer.c:24 |
| `use` | TOK_USE | Package usage (reserved) | lexer.c:25 |

### Type Keywords (8 total)
| Keyword | Token | Purpose | Line |
|---------|-------|---------|------|
| `int` | TOK_INT | Integer type | lexer.c:28 |
| `deci` | TOK_DECI | Decimal/floating-point type | lexer.c:29 |
| `char` | TOK_CHAR | Character type | lexer.c:30 |
| `str` | TOK_STR | String type | lexer.c:31 |
| `bool` | TOK_BOOL | Boolean type | lexer.c:32 |
| `byte` | TOK_BYTE | Unsigned byte type | lexer.c:33 |
| `ubyte` | TOK_UBYTE | Explicit unsigned byte | lexer.c:34 |
| `none` | TOK_NONE | Void/no-return type | lexer.c:35 |

### Boolean/Logic Keywords
| Keyword | Token | Purpose | Line |
|---------|-------|---------|------|
| `true` | TOK_TRUE | Boolean literal true | lexer.c:37 |
| `false` | TOK_FALSE | Boolean literal false | lexer.c:38 |
| `and` | TOK_AND_KW | Logical AND operator (keyword form) | lexer.c:40 |
| `xor` | TOK_XOR | Logical XOR operator (keyword form) | lexer.c:41 |
| `not` | TOK_NOT | Logical NOT operator (keyword form) | lexer.c:42 |

---

## 2. TOKEN TYPES (79 total)

Defined in [include/lexer.h](include/lexer.h#L6-L105)

### Keyword Tokens (33)
See Section 1 above.

### Literal Token Types
| Token Type | Symbol | Example | Line |
|------------|--------|---------|------|
| TOK_NUMBER | — | `123`, `0xFF`, `0b1010`, `077` | lexer.h:43 |
| TOK_DECIMAL | — | `3.14`, `2.5` | lexer.h:44 |
| TOK_STRING | `"..."` | `"hello world"` | lexer.h:45 |
| TOK_CHAR_LIT | `'...'` | `'a'`, `'\n'` | lexer.h:46 |

**Special Lexer Features** ([src/lexer.c](src/lexer.c#L150-200)):
- **Number formats supported**: 
  - Decimal: `123`
  - Hexadecimal: `0xFF` or `0Xff`
  - Binary: `0b1010` or `0B1010`
  - Octal: `077` (leading zero followed by 0-7 digits)
- **String escape sequences** ([src/lexer.c](src/lexer.c#L285-315)):
  - `\n` (newline), `\t` (tab), `\r` (carriage return)
  - `\b` (backspace), `\v` (vertical tab), `\f` (form feed)
  - `\a` (bell), `\\` (backslash), `\"` (quote)
  - `\$` (escaped dollar for interpolation)
- **String interpolation marker**: `$` in strings (see Section 6)
- **Comments**:
  - Single-line: `//`
  - Multi-line: `//>` ... `<//` (nested support)

### Identifier Token
| Token Type | Purpose | Line |
|------------|---------|------|
| TOK_IDENT | Variable/function/type names | lexer.h:49 |

### Operator Tokens (44)

#### Arithmetic Operators
| Token | Operator | Line |
|-------|----------|------|
| TOK_PLUS | `+` | lexer.h:52 |
| TOK_MINUS | `-` | lexer.h:53 |
| TOK_STAR | `*` | lexer.h:54 |
| TOK_SLASH | `/` | lexer.h:55 |
| TOK_PERCENT | `%` (modulo) | lexer.h:56 |

#### Comparison Operators
| Token | Operator | Line |
|-------|----------|------|
| TOK_EQ | `==` | lexer.h:60 |
| TOK_NEQ | `!=` | lexer.h:61 |
| TOK_LT | `<` | lexer.h:62 |
| TOK_GT | `>` | lexer.h:63 |
| TOK_LTE | `<=` | lexer.h:64 |
| TOK_GTE | `>=` | lexer.h:65 |

#### Logical Operators
| Token | Operator | Line |
|-------|----------|------|
| TOK_AND | `&&` (symbol form) | lexer.h:68 |
| TOK_LOG_OR | `\|\|` (symbol form) | lexer.h:69 |
| TOK_NOT | `!` (prefix unary) | lexer.h:70 |
| TOK_AND_KW | `and` (keyword form of AND) | lexer.h:71 |
| TOK_XOR | `xor` (keyword form) | lexer.h:72 |

#### Bitwise Operators
| Token | Operator | Line |
|-------|----------|------|
| TOK_BIT_AND | `&` | lexer.h:75 |
| TOK_BIT_OR | `\|` | lexer.h:76 |
| TOK_BIT_XOR | `^` | lexer.h:77 |
| TOK_BIT_NOT | `~` | lexer.h:78 |
| TOK_LSHIFT | `<<` | lexer.h:79 |
| TOK_RSHIFT | `>>` | lexer.h:80 |

#### Assignment Operators (10)
| Token | Operator | Line |
|-------|----------|------|
| TOK_ASSIGN | `=` | lexer.h:83 |
| TOK_PLUS_ASSIGN | `+=` | lexer.h:84 |
| TOK_MINUS_ASSIGN | `-=` | lexer.h:85 |
| TOK_STAR_ASSIGN | `*=` | lexer.h:86 |
| TOK_SLASH_ASSIGN | `/=` | lexer.h:87 |
| TOK_PERCENT_ASSIGN | `%=` | lexer.h:88 |
| TOK_AND_ASSIGN | `&=` | lexer.h:89 |
| TOK_OR_ASSIGN | `\|=` | lexer.h:90 |
| TOK_XOR_ASSIGN | `^=` | lexer.h:91 |
| TOK_LSHIFT_ASSIGN | `<<=` | lexer.h:92 |
| TOK_RSHIFT_ASSIGN | `>>=` (3-char token) | lexer.h:93 |

#### Increment/Decrement Operators
| Token | Operator | Line |
|-------|----------|------|
| TOK_PLUSPLUS | `++` | lexer.h:96 |
| TOK_MINUSMINUS | `--` | lexer.h:97 |

#### Ternary Operator
| Token | Operator | Line |
|-------|----------|------|
| TOK_QUESTION | `?` | lexer.h:100 |

### Delimiter Tokens (11)
| Token | Symbol | Line |
|-------|--------|------|
| TOK_LPAREN | `(` | lexer.h:103 |
| TOK_RPAREN | `)` | lexer.h:104 |
| TOK_LBRACKET | `[` | lexer.h:105 |
| TOK_RBRACKET | `]` | lexer.h:106 |
| TOK_LBRACE | `{` | lexer.h:107 |
| TOK_RBRACE | `}` | lexer.h:108 |
| TOK_SEMICOLON | `;` | lexer.h:109 |
| TOK_COLON | `:` | lexer.h:110 |
| TOK_COLONCOLON | `::` (type specification) | lexer.h:111 |
| TOK_COMMA | `,` | lexer.h:112 |
| TOK_DOT | `.` | lexer.h:113 |

### Special Operator Tokens
| Token | Symbol | Purpose | Line |
|-------|--------|---------|------|
| TOK_ARROW | `->` | Map operation prefix | lexer.h:114 |
| TOK_AT | `@` | Builtin function prefix | lexer.h:115 |

### Special Tokens
| Token | Purpose | Line |
|-------|---------|------|
| TOK_EOF | End of file | lexer.h:118 |
| TOK_ERROR | Lexical error | lexer.h:119 |

---

## 3. STATEMENT TYPES (27 AST node types)

Defined in [include/ast.h](include/ast.h#L6-L38)

### Program-Level Statements
| AST Type | Description | Parser | Line |
|----------|-------------|--------|------|
| AST_PROGRAM | Root node containing imports, constants, groups, functions | parser_parse() | ast.h:7 |
| AST_CONST_DECL | Global constant declaration: `const name = value;` | parse_const_decl() | ast.h:8, parser.c:1906 |
| AST_PACKAGE_IMPORT | Package import: `pkg:<name>;` | parse_package_import() | ast.h:37, parser.c:1880 |
| AST_GROUP_DEF | Struct/aggregate type: `group Name { ... }` | parse_group() | ast.h:25, parser.c:1697 |

### Function-Related
| AST Type | Description | Parser | Line |
|----------|-------------|--------|------|
| AST_FUNCTION | Function definition: `fnc name[params] type { ... }` | parse_function() | ast.h:9, parser.c:1754 |
| AST_RETURN | Return statement: `get[expr];` | parse_return() | ast.h:11, parser.c:1295 |

### Block/Scope
| AST Type | Description | Parser | Line |
|----------|-------------|--------|------|
| AST_BLOCK | Block of statements: `{ stmt1; stmt2; ... }` | parse_block() | ast.h:10, parser.c:750 |

### Control Flow Statements
| AST Type | Description | Parser | Line |
|----------|-------------|--------|------|
| AST_IF | If-else statement: `if[cond] { ... } or[...] { ... }` | parse_if() | ast.h:12, parser.c:1308 |
| AST_WHILE | While loop: `while[cond] { ... }` | parse_while() | ast.h:14, parser.c:1313 |
| AST_LOOP | For-style loop: `loop[init; cond; incr] { ... }` | parse_loop() | ast.h:13, parser.c:1318 |
| AST_CYCLE | Switch statement: `cycle[expr] { when[...]: {...} fixed {...} }` | parse_cycle() | ast.h:15, parser.c:1323 |
| AST_WHEN | Case clause: `when[value]: { ... }` | (in parse_cycle/check) | ast.h:16, parser.c:1063 |
| AST_CHECK | Try-catch: `check { ... } when[...]: { ... }` | parse_check() | ast.h:17, parser.c:1328 |

### Variable Management
| AST Type | Description | Parser | Line |
|----------|-------------|--------|------|
| AST_VAR_DECL | Variable declaration: `type name [= init];` | parse_var_decl() | ast.h:18, parser.c:1349 |
| AST_ASSIGN | Variable assignment: `name = value;` | (in parse_statement) | ast.h:19, parser.c:1517 |

### Array Operations
| AST Type | Description | Parser | Line |
|----------|-------------|--------|------|
| AST_ARRAY_DECL | Array declaration: `arr{type, size} name [= {...}];` | (in parse_statement) | ast.h:21, parser.c:1363 |
| AST_ARRAY_ACCESS | Array element access: `name{index}` | parse_primary() | ast.h:23, parser.c:376 |
| AST_ARRAY_ASSIGN | Array element assignment: `name{index} = value;` | (in parse_statement) | ast.h:20, parser.c:1623 |

### Map/Dictionary Operations (4)
| AST Type | Description | Parser | Line |
|----------|-------------|--------|------|
| AST_MAP_DECL | Map declaration: `map{keyType, valueType} name;` | (in parse_statement) | ast.h:27, parser.c:1419 |
| AST_MAP_SET | Map set operation: `map->set[key, value];` | (in parse_statement) | ast.h:29, parser.c:1579 |
| AST_MAP_GET | Map get operation: `map->get[key]` (expression) | parse_primary() | ast.h:30, parser.c:347 |
| AST_MAP_HAS | Map has operation: `map->has[key]` (expression) | parse_primary() | ast.h:31, parser.c:359 |
| AST_MAP_REMOVE | Map remove operation: `map->remove[key];` | (in parse_statement) | ast.h:32, parser.c:1593 |

### Struct/Group Operations
| AST Type | Description | Parser | Line |
|----------|-------------|--------|------|
| AST_GROUP_DECL | Group variable declaration: `GroupType varName;` | (in parse_statement) | ast.h:26, parser.c:1343 |
| AST_MEMBER_ACCESS | Struct member access: `object.member` | parse_primary() | ast.h:24, parser.c:303 |
| AST_MEMBER_ASSIGN | Member assignment: `object.member = value;` | (in parse_statement) | ast.h:25, parser.c:1506 |

### I/O Statements
| AST Type | Description | Parser | Line |
|----------|-------------|--------|------|
| AST_SHOW | Output statement: `show[expr];` | parse_show() | ast.h:8, parser.c:1298 |
| AST_READ | Input statement: `read[varname];` or `read["prompt"]` | parse_read() | ast.h:9, parser.c:1333 |

### Expression-Level Statements
| AST Type | Description | Parser | Line |
|----------|-------------|--------|------|
| AST_CALL | Function call: `name[args...]` | parse_primary() | ast.h:22, parser.c:405 |
| AST_BUILTIN_CALL | Builtin call: `@name[args...];` or `@type[val]::int` | (in parse_statement) | ast.h:36, parser.c:1224 |

### Expressions (6 types)
| AST Type | Description | Parser | Line |
|----------|-------------|--------|------|
| AST_BINARY_OP | Binary operation: `left op right` | parse_multiplicative(), etc. | ast.h:19 |
| AST_UNARY_OP | Unary operation: `op operand` or `operand op` | parse_unary() | ast.h:20, parser.c:534 |
| AST_TERNARY_OP | Ternary conditional: `cond ? true_expr : false_expr` | parse_ternary() | ast.h:21, parser.c:706 |
| AST_LITERAL | Literal value: `123`, `"string"`, `true`, etc. | parse_primary() | ast.h:34, parser.c:163 |
| AST_IDENT | Identifier reference | parse_primary() | ast.h:35, parser.c:280 |

---

## 4. DATA TYPES & OPERATORS SUPPORTED

### Primitive Data Types (8)
Defined in [include/lexer.h](include/lexer.h#L28-L35):

| Type | Keyword | Size | Range | Purpose | Line |
|------|---------|------|-------|---------|------|
| Integer | `int` | 64-bit | -2^63 to 2^63-1 | General integer arithmetic | lexer.h:30 |
| Decimal | `deci` | 64-bit float | ±1.8e±308 | Floating-point arithmetic | lexer.h:29 |
| Character | `char` | 8-bit | 0-255 ASCII | Single character | lexer.h:30 |
| String | `str` | Dynamic | — | Text (null-terminated) | lexer.h:31 |
| Boolean | `bool` | 1-bit | 0 or 1 | Logical values (true/false) | lexer.h:32 |
| Byte | `byte` | 8-bit | -128 to 127 | Signed byte | lexer.h:33 |
| Unsigned Byte | `ubyte` | 8-bit | 0-255 | Unsigned byte | lexer.h:34 |
| Void | `none` | 0-bit | — | No return value | lexer.h:35 |

### Composite Type Constructors
| Construct | Syntax | Example | Parser | Line |
|-----------|--------|---------|--------|------|
| Array | `arr{type, size} name` | `arr{int, 10} scores;` | parse_statement() | parser.c:1363 |
| Map | `map{keyType, valueType} name` | `map{str, int} cache;` | parse_statement() | parser.c:1419 |
| Struct/Group | `group Name { type field; ... }` | See Section 6 example | parse_group() | parser.c:1697 |

### Operator Precedence (13 levels, lowest to highest)

Defined in [src/parser.c](src/parser.c#L650-673) with parser functions:

| Precedence | Operators | Associativity | Parser Function | Line |
|------------|-----------|----------------|-----------------|------|
| 1 (lowest) | `?:` (ternary) | Right | parse_ternary() | parser.c:706 |
| 2 | `\|\|` (logical OR), `xor` | Left | parse_logical_or() | parser.c:692 |
| 3 | `&&` (logical AND), `and` | Left | parse_logical_and() | parser.c:677 |
| 4 | `\|` (bitwise OR) | Left | parse_bitwise_or() | parser.c:658 |
| 5 | `^` (bitwise XOR) | Left | parse_bitwise_xor() | parser.c:645 |
| 6 | `&` (bitwise AND) | Left | parse_bitwise_and() | parser.c:632 |
| 7 | `==`, `!=` | Left | parse_equality() | parser.c:617 |
| 8 | `<`, `>`, `<=`, `>=` | Left | parse_comparison() | parser.c:602 |
| 9 | `<<`, `>>` (shifts) | Left | parse_shift() | parser.c:587 |
| 10 | `+`, `-` (additive) | Left | parse_additive() | parser.c:572 |
| 11 | `*`, `/`, `%` (multiplicative) | Left | parse_multiplicative() | parser.c:557 |
| 12 | `+x`, `-x`, `!x`, `~x`, `++x`, `--x` (prefix unary) | Right | parse_unary() | parser.c:520 |
| 13 (highest) | `++x`, `--x` (postfix), `f[...]`, `x[y]`, `.member` | Left | parse_postfix() | parser.c:497 |

### Binary Operators (36 total)

#### Arithmetic (5)
| Operator | Token | Line | Precedence |
|----------|-------|------|------------|
| `+` | TOK_PLUS | lexer.h:52 | 10 |
| `-` | TOK_MINUS | lexer.h:53 | 10 |
| `*` | TOK_STAR | lexer.h:54 | 11 |
| `/` | TOK_SLASH | lexer.h:55 | 11 |
| `%` | TOK_PERCENT | lexer.h:56 | 11 |

#### Comparison (6)
| Operator | Token | Line | Precedence |
|----------|-------|------|------------|
| `==` | TOK_EQ | lexer.h:60 | 7 |
| `!=` | TOK_NEQ | lexer.h:61 | 7 |
| `<` | TOK_LT | lexer.h:62 | 8 |
| `>` | TOK_GT | lexer.h:63 | 8 |
| `<=` | TOK_LTE | lexer.h:64 | 8 |
| `>=` | TOK_GTE | lexer.h:65 | 8 |

#### Logical (3)
| Operator | Token | Line | Precedence |
|----------|-------|------|------------|
| `&&` or `and` | TOK_AND / TOK_AND_KW | lexer.h:68,71 | 3 |
| `\|\|` or `xor` | TOK_LOG_OR / TOK_XOR | lexer.h:69,72 | 2 |
| (Note: `!` is prefix unary) | TOK_NOT | lexer.h:70 | 12 |

#### Bitwise (6)
| Operator | Token | Line | Precedence |
|----------|-------|------|------------|
| `&` | TOK_BIT_AND | lexer.h:75 | 6 |
| `\|` | TOK_BIT_OR | lexer.h:76 | 4 |
| `^` | TOK_BIT_XOR | lexer.h:77 | 5 |
| `<<` | TOK_LSHIFT | lexer.h:79 | 9 |
| `>>` | TOK_RSHIFT | lexer.h:80 | 9 |

#### Assignment (11)
| Operator | Token | Line |
|----------|-------|------|
| `=` | TOK_ASSIGN | lexer.h:83 |
| `+=` | TOK_PLUS_ASSIGN | lexer.h:84 |
| `-=` | TOK_MINUS_ASSIGN | lexer.h:85 |
| `*=` | TOK_STAR_ASSIGN | lexer.h:86 |
| `/=` | TOK_SLASH_ASSIGN | lexer.h:87 |
| `%=` | TOK_PERCENT_ASSIGN | lexer.h:88 |
| `&=` | TOK_AND_ASSIGN | lexer.h:89 |
| `\|=` | TOK_OR_ASSIGN | lexer.h:90 |
| `^=` | TOK_XOR_ASSIGN | lexer.h:91 |
| `<<=` | TOK_LSHIFT_ASSIGN | lexer.h:92 |
| `>>=` (3-char) | TOK_RSHIFT_ASSIGN | lexer.h:93 |

### Unary Operators (8)
| Operator | Type | Token | Associativity | Line |
|----------|------|-------|----------------|------|
| `+x` | Prefix arithmetic | TOK_PLUS | Right | parser.c:520 |
| `-x` | Prefix arithmetic | TOK_MINUS | Right | parser.c:520 |
| `!x` | Prefix logical NOT | TOK_NOT | Right | parser.c:520 |
| `~x` | Prefix bitwise NOT | TOK_BIT_NOT | Right | parser.c:520 |
| `++x` | Prefix increment | TOK_PLUSPLUS | Right | parser.c:520 |
| `--x` | Prefix decrement | TOK_MINUSMINUS | Right | parser.c:520 |
| `x++` | Postfix increment | TOK_PLUSPLUS | Left | parser.c:497 |
| `x--` | Postfix decrement | TOK_MINUSMINUS | Left | parser.c:497 |

---

## 5. BUILTIN FUNCTIONS (81 total)

Defined in [src/builtins.c](src/builtins.c#L4-190), organized by category.

### System Control (5)
| Function | Signature | Return Type | Min Args | Max Args | Line |
|----------|-----------|------------|----------|----------|------|
| `exit` | `exit[code]` | `none` | 1 | 1 | builtins.c:5 |
| `halt` | `halt[]` | `none` | 0 | 0 | builtins.c:6 |
| `sleep` | `sleep[ms]` | `none` | 1 | 1 | builtins.c:7 |
| `clock` | `clock[]` | `int` | 0 | 0 | builtins.c:8 |
| `panic` | `panic[msg]` | `none` | 1 | 1 | builtins.c:9 |

### Memory Operations (20)
| Function | Purpose | Return Type | Args | Line |
|----------|---------|------------|------|------|
| `addr[var]` | Get address of variable | `int` | 1 | builtins.c:12 |
| `peek[addr]` | Read byte at address | `byte` | 1 | builtins.c:13 |
| `poke[addr, value]` | Write byte at address | `none` | 2 | builtins.c:14 |
| `memcpy[dst, src, size]` | Copy memory block | `none` | 3 | builtins.c:15 |
| `memclr[addr, size]` | Clear memory block | `none` | 2 | builtins.c:16 |
| `align[ptr, boundary]` | Align pointer to boundary | `int` | 2 | builtins.c:17 |
| `alloc[size]` | Allocate heap memory | `int` | 1 | builtins.c:18 |
| `free[ptr]` | Free allocated memory | `none` | 1 | builtins.c:19 |
| `realloc[ptr, size]` | Resize allocation | `int` | 2 | builtins.c:20 |
| `salloc[size]` | Stack allocate | `int` | 1 | builtins.c:21 |
| `memset[ptr, val, size]` | Set memory to value | `none` | 3 | builtins.c:22 |
| `memcmp[ptr1, ptr2, size]` | Compare memory | `int` | 3 | builtins.c:23 |
| `mmap[size, prot, flags]` | Memory map | `int` | 3 | builtins.c:24 |
| `munmap[ptr, size]` | Unmap memory | `int` | 2 | builtins.c:25 |
| `mprotect[ptr, size, prot]` | Change protection | `int` | 3 | builtins.c:26 |
| `heap_start[]` | Get heap start | `int` | 0 | builtins.c:27 |
| `heap_end[]` | Get heap end | `int` | 0 | builtins.c:28 |
| `heap_size[]` | Get heap size | `int` | 0 | builtins.c:29 |
| `page_size[]` | Get page size | `int` | 0 | builtins.c:30 |
| `stack_ptr[]` | Get stack pointer | `int` | 0 | builtins.c:31 |
| `stack_size[]` | Get stack usage | `int` | 0 | builtins.c:32 |
| `mfence[]` | Memory full barrier | `none` | 0 | builtins.c:33 |
| `lfence[]` | Load fence | `none` | 0 | builtins.c:34 |
| `sfence[]` | Store fence | `none` | 0 | builtins.c:35 |

### Type Conversion (6)
| Function | Purpose | Return Type | Args | Note | Line |
|----------|---------|------------|------|------|------|
| `type[val...]::type` | **Unified** type conversion | varies | 1+ | Preferred method; supports multiple args | builtins.c:38 |
| `to_int[val]` | Convert to int | `int` | 1 | DEPRECATED | builtins.c:39 |
| `to_deci[val]` | Convert to decimal | `deci` | 1 | DEPRECATED | builtins.c:40 |
| `to_byte[val]` | Convert to byte | `byte` | 1 | DEPRECATED | builtins.c:41 |
| `to_bool[val]` | Convert to bool | `bool` | 1 | DEPRECATED | builtins.c:42 |
| `to_str[val]` | Convert to string | `str` | 1 | DEPRECATED | builtins.c:43 |

**Type Conversion Syntax** ([src/parser.c](src/parser.c#L1264-1285)):
```
@type[value]::int        // Convert value to int
@type[value]::deci       // Convert value to decimal
@type[val1, val2]::str   // String concatenation
```

### File I/O (6)
| Function | Purpose | Return Type | Args | Line |
|----------|---------|------------|------|------|
| `fopen[path, mode]` | Open file | `int` (fd) | 2 | builtins.c:46 |
| `fread[fd, buffer, size]` | Read from file | `int` (bytes read) | 3 | builtins.c:47 |
| `fwrite[fd, buffer, size]` | Write to file | `int` (bytes written) | 3 | builtins.c:48 |
| `fseek[fd, offset]` | Seek position | `int` | 2 | builtins.c:49 |
| `fclose[fd]` | Close file | `int` | 1 | builtins.c:50 |
| `fdelete[path]` | Delete file | `int` | 1 | builtins.c:51 |

### Network (8)
| Function | Purpose | Return Type | Args | Line |
|----------|---------|------------|------|------|
| `socket[type, protocol]` | Create socket | `int` (fd) | 2 | builtins.c:54 |
| `connect[socket, addr, port]` | Connect to server | `int` | 3 | builtins.c:55 |
| `send[socket, buffer, length]` | Send data | `int` (bytes sent) | 3 | builtins.c:56 |
| `recv[socket, buffer, length]` | Receive data | `int` (bytes recv) | 3 | builtins.c:57 |
| `bind[socket, addr, port]` | Bind socket | `int` | 3 | builtins.c:58 |
| `listen[socket, backlog]` | Listen for connections | `int` | 2 | builtins.c:59 |
| `accept[socket]` | Accept connection | `int` (new fd) | 1 | builtins.c:60 |
| `close[socket]` | Close socket | `int` | 1 | builtins.c:61 |

### Security & Cryptography (5)
| Function | Purpose | Return Type | Args | Line |
|----------|---------|------------|------|------|
| `hash[buffer, algorithm]` | Hash data | `int` | 2 | builtins.c:64 |
| `rand[size]` | Generate random bytes | `int` | 1 | builtins.c:65 |
| `secure_zero[ptr, size]` | Securely zero memory | `none` | 2 | builtins.c:66 |
| `entropy[]` | Read hardware entropy | `int` | 0 | builtins.c:67 |
| `verify[sig, pub, data]` | Verify signature | `bool` | 3 | builtins.c:68 |

### Utility Functions (8)
| Function | Purpose | Return Type | Args | Line |
|----------|---------|------------|------|------|
| `len[str or arr]` | Get length | `int` | 1 | builtins.c:71 |
| `sizeof[type]` | Get type size in bytes | `int` | 1 | builtins.c:73 |
| `concat[str1, str2]` | Concatenate strings | `str` | 2 | builtins.c:74 |
| `substr[str, start, len]` | Extract substring | `str` | 3 | builtins.c:75 |
| `strcmp[str1, str2]` | Compare strings | `int` (0=equal) | 2 | builtins.c:76 |
| `chr[int]` | Int to char | `char` | 1 | builtins.c:77 |
| `ord[char]` | Char to int | `int` | 1 | builtins.c:78 |

### Hardware I/O (6)
| Function | Purpose | Return Type | Args | Line |
|----------|---------|------------|------|------|
| `port_in[port]` | Read from I/O port | `byte` | 1 | builtins.c:81 |
| `port_out[port, value]` | Write to I/O port | `none` | 2 | builtins.c:82 |
| `irq_enable[irq]` | Enable interrupt | `none` | 1 | builtins.c:83 |
| `irq_disable[irq]` | Disable interrupt | `none` | 1 | builtins.c:84 |
| `ioread[addr]` | Read memory-mapped I/O | `int` | 1 | builtins.c:85 |
| `iowrite[addr, value]` | Write memory-mapped I/O | `none` | 2 | builtins.c:86 |

### Process & Threading (4)
| Function | Purpose | Return Type | Args | Line |
|----------|---------|------------|------|------|
| `spawn[function, arg]` | Create thread/process | `int` (pid) | 2 | builtins.c:89 |
| `join[tid]` | Wait for thread | `int` | 1 | builtins.c:90 |
| `pid[]` | Get process ID | `int` | 0 | builtins.c:91 |
| `kill[pid]` | Terminate process | `int` | 1 | builtins.c:92 |

### Synchronization (15)
#### Mutexes
| Function | Purpose | Return Type | Args | Line |
|----------|---------|------------|------|------|
| `mutex_create[]` | Create mutex | `int` (mutex_id) | 0 | builtins.c:95 |
| `mutex_lock[id]` | Lock mutex (blocking) | `int` | 1 | builtins.c:96 |
| `mutex_unlock[id]` | Unlock mutex | `int` | 1 | builtins.c:97 |
| `mutex_trylock[id]` | Try lock (non-blocking) | `int` (1=locked, 0=already) | 1 | builtins.c:98 |
| `mutex_destroy[id]` | Destroy mutex | `int` | 1 | builtins.c:99 |

#### Semaphores
| Function | Purpose | Return Type | Args | Line |
|----------|---------|------------|------|------|
| `semaphore_create[count]` | Create semaphore | `int` (sem_id) | 1 | builtins.c:101 |
| `semaphore_wait[id]` | Wait/decrement | `int` | 1 | builtins.c:102 |
| `semaphore_signal[id]` | Signal/increment | `int` | 1 | builtins.c:103 |

#### Condition Variables
| Function | Purpose | Return Type | Args | Line |
|----------|---------|------------|------|------|
| `cond_create[]` | Create condition var | `int` (cond_id) | 0 | builtins.c:105 |
| `cond_wait[cond, mutex]` | Wait on condition | `int` | 2 | builtins.c:106 |
| `cond_signal[cond]` | Signal one waiter | `int` | 1 | builtins.c:107 |
| `cond_broadcast[cond]` | Signal all waiters | `int` | 1 | builtins.c:108 |

#### Atomic Operations
| Function | Purpose | Return Type | Args | Line |
|----------|---------|------------|------|------|
| `atomic_cmp_swap[ptr, old, new]` | Compare-and-swap | `int` (1=success) | 3 | builtins.c:110 |
| `atomic_increment[ptr]` | Atomic ++ | `int` (new value) | 1 | builtins.c:111 |
| `atomic_decrement[ptr]` | Atomic -- | `int` (new value) | 1 | builtins.c:112 |

### Channel Communication (6)
| Function | Purpose | Return Type | Args | Line |
|----------|---------|------------|------|------|
| `channel_create[capacity]` | Create channel | `int` (chan_id) | 1 | builtins.c:115 |
| `channel_send[chan, value]` | Send to channel | `int` | 2 | builtins.c:116 |
| `channel_recv[chan]` | Receive from channel | `int` (value) | 1 | builtins.c:117 |
| `channel_close[chan]` | Close channel | `int` | 1 | builtins.c:118 |
| `channel_empty[chan]` | Check if empty | `int` (1=empty) | 1 | builtins.c:119 |
| `channel_full[chan]` | Check if full | `int` (1=full) | 1 | builtins.c:120 |

### Thread Pool (4)
| Function | Purpose | Return Type | Args | Line |
|----------|---------|------------|------|------|
| `pool_create[worker_count]` | Create thread pool | `int` (pool_id) | 1 | builtins.c:123 |
| `pool_submit[pool_id, fn_addr, arg]` | Submit task | `int` (task_id) | 3 | builtins.c:124 |
| `pool_wait[pool_id]` | Wait for all tasks | `int` | 1 | builtins.c:125 |
| `pool_destroy[pool_id]` | Destroy pool | `int` | 1 | builtins.c:126 |

### String Manipulation (12)
| Function | Purpose | Return Type | Args | Line |
|----------|---------|------------|------|------|
| `split[str, delimiter]` | Split string | `str` | 2 | builtins.c:129 |
| `join[str, str]` | Join strings | `str` | 2 | builtins.c:130 |
| `trim[str]` | Trim whitespace | `str` | 1 | builtins.c:131 |
| `upper[str]` | To uppercase | `str` | 1 | builtins.c:132 |
| `lower[str]` | To lowercase | `str` | 1 | builtins.c:133 |
| `indexOf[str, substr]` | Find substring | `int` (index) | 2 | builtins.c:134 |
| `replace[str, old, new]` | Replace substring | `str` | 3 | builtins.c:135 |
| `startsWith[str, prefix]` | Check prefix | `bool` | 2 | builtins.c:136 |
| `endsWith[str, suffix]` | Check suffix | `bool` | 2 | builtins.c:137 |
| `reverse[str]` | Reverse string | `str` | 1 | builtins.c:138 |
| `repeat[str, count]` | Repeat string | `str` | 2 | builtins.c:139 |
| `pad[str, length, padchar]` | Pad string | `str` | 3 | builtins.c:140 |

### Mathematical Functions (15)
| Function | Purpose | Return Type | Args | Line |
|----------|---------|------------|------|------|
| `isqrt[n]` | Integer square root | `int` | 1 | builtins.c:143 |
| `pow[base, exp]` | Power function | `int` | 2 | builtins.c:144 |
| `abs[n]` | Absolute value | `int` | 1 | builtins.c:145 |
| `min[a, b, ...]` | Minimum value | `int` | 2+ | builtins.c:146 |
| `max[a, b, ...]` | Maximum value | `int` | 2+ | builtins.c:147 |
| `clz[n]` | Count leading zeros | `int` | 1 | builtins.c:148 |
| `ctz[n]` | Count trailing zeros | `int` | 1 | builtins.c:149 |
| `popcount[n]` | Population count | `int` (# of 1 bits) | 1 | builtins.c:150 |
| `gcd[a, b]` | Greatest common divisor | `int` | 2 | builtins.c:151 |
| `lcm[a, b]` | Least common multiple | `int` | 2 | builtins.c:152 |
| `isprime[n]` | Check if prime | `int` (1=prime) | 1 | builtins.c:153 |
| `modpow[base, exp, mod]` | Modular exponentiation | `int` | 3 | builtins.c:154 |
| `sqrt[n]` | Floating square root | `deci` | 1 | builtins.c:155 |
| `floor[x]` | Floor function | `deci` | 1 | builtins.c:156 |
| `ceil[x]` | Ceiling function | `deci` | 1 | builtins.c:157 |

### Process Resource Management (3)
| Function | Purpose | Return Type | Args | Line |
|----------|---------|------------|------|------|
| `fork[arg]` | Fork process | `int` (pid) | 1 | builtins.c:160 |
| `wait[pid]` | Wait for process | `int` (status) | 1 | builtins.c:161 |
| `wait_any[timeout]` | Wait for any child | `int` (pid) | 1 | builtins.c:162 |

### Meta/Build Information (3)
| Function | Purpose | Return Type | Args | Line |
|----------|---------|------------|------|------|
| `build_time[]` | Get compile timestamp | `str` | 0 | builtins.c:165 |
| `compiler_ver[]` | Get compiler version | `str` | 0 | builtins.c:166 |
| `syscall[num, args...]` | Direct system call | `int` | 1+ | builtins.c:167 |
| `import[lib]` | Import library (.sulib) | `none` | 1 | builtins.c:168 |

---

## 6. SPECIAL SYNTAX FEATURES

### A. String Interpolation

**Feature**: Dynamic string substitution using `$` marker  
**Defined in**: [src/lexer.c](src/lexer.c#L264-315) and [src/parser.c](src/parser.c#L799-1000)

**Syntax Forms**:
1. Simple variable interpolation: `"Hello $name"` expands to show the value of `name`
2. Expression interpolation: `"Result: ${x + y}"` evaluates the expression inside `{}`

**Implementation**:
- Lexer detects `$` in strings and preserves it (line 336-340)
- String escape sequences supported: `\$` escapes the interpolation marker
- Parser detects interpolation markers ([parse_interpolated_string()](src/parser.c#L799)) and generates multiple `show[]` statements
- Expression inside `${}` is parsed by a temporary lexer/parser instance (lines 860-875)

**Example**:
```ras
int x = 42;
show["The answer is ${x}"];  // Generates: show["The answer is "]; show[x];
```

### B. Map Operations

**Feature**: Hash map/dictionary with key-value storage  
**Syntax**: 
```ras
map{keyType, valueType} mapName;
mapName->set[key, value];      // Set key-value
value = mapName->get[key];     // Get value
exists = mapName->has[key];    // Check key exists
mapName->remove[key];          // Remove key
```

**AST Nodes**: AST_MAP_DECL, AST_MAP_SET, AST_MAP_GET, AST_MAP_HAS, AST_MAP_REMOVE  
**Parser Location**: [src/parser.c](src/parser.c#L1419-1458) and [lines 1559-1601]  
**Type Support**: Keys/values can be `int`, `str`, `deci`, `bool`, `char`, `byte`, `ubyte`

### C. Array Operations

**Feature**: Fixed-size arrays with bounds checking  
**Syntax**:
```ras
arr{type, size} arrayName;                    // Declaration
arr{int, 10} numbers = {1, 2, 3, ...};       // With initializer
element = arrayName{index};                   // Access
arrayName{index} = value;                     // Assignment
```

**Bounds Checking**: Enabled by default; can be disabled with `-fno-bounds-check` flag  
([src/codegen.c](src/codegen.c#L861) and [parser.c](src/parser.c#L1363))  
**Parser Location**: [src/parser.c](src/parser.c#L1363-1410)

### D. Group/Struct Definition

**Feature**: User-defined composite types with named fields  
**Syntax**:
```ras
group Person {
    int age;
    str name;
    bool active;
}

Person p;        // Declaration
p.age = 25;      // Member access
val = p.name;    // Member read
```

**Parser Location**: [parse_group()](src/parser.c#L1697-1747)  
**AST Nodes**: AST_GROUP_DEF, AST_GROUP_DECL, AST_MEMBER_ACCESS, AST_MEMBER_ASSIGN  
**Nested Support**: Members can chain: `p.address.street`

### E. Unified Type Conversion

**Feature**: Single builtin syntax for type conversion  
**Syntax**: `@type[value...]::targetType`  
**Parser Location**: [src/parser.c](src/parser.c#L1264-1285)  

**Examples**:
```ras
int_val = @type["42"]::int;         // String to int
str_val = @type[123]::str;          // Int to string
deci_val = @type[42]::deci;         // Int to decimal
combined = @type["Hello", " ", "World"]::str;  // String concatenation
```

**Deprecated forms** (still supported):
- `@to_int[x]`, `@to_deci[x]`, `@to_str[x]`, `@to_byte[x]`, `@to_bool[x]`

### F. Builtin Function Calls

**Feature**: Direct compiler-generated system functions via `@` prefix  
**Syntax**: `@functionName[args...];` or `result = @functionName[args];`  
**Parser Location**: [src/parser.c](src/parser.c#L1224-1262)  
**Codegen Location**: [src/codegen.c](src/codegen.c#L998-1024)

**Error Checking** (lines 1012-1020):
- Validates argument count against min_args/max_args
- Unknown builtins result in zero return value

### G. Global Constants

**Feature**: Compile-time constant declarations  
**Syntax**: `const NAME = expression;` (at program level)  
**Parser Location**: [parse_const_decl()](src/parser.c#L1906-1932)  
**Scope**: Global (accessible in all functions)  
**Evaluation**: Expression evaluated at compile-time

**Example**:
```ras
const PI = 314;        // Integer constant
const MSG = "Hello";   // String constant
```

### H. Package Import System

**Feature**: Modular code organization  
**Syntax**: `pkg:<package_name>;`  
**Parser Location**: [parse_package_import()](src/parser.c#L1880-1896)  
**AST Node**: AST_PACKAGE_IMPORT  
**Implementation Note**: Package resolution handled at compile-time; uses `::` scope resolution

### I. Exception Handling (Check-When)

**Feature**: Try-catch style error handling  
**Syntax**:
```ras
check {
    // code that may error
} when[ErrorType]: {
    // handle specific error
} when[]: {         // Optional catch-all
    // handle any error
}
```

**Parser Location**: [parse_check()](src/parser.c#L1025-1061)  
**AST Nodes**: AST_CHECK, AST_WHEN (with value = NULL for catch-all)

### J. Switch-Case (Cycle-When)

**Feature**: Value-based dispatch  
**Syntax**:
```ras
cycle[expr] {
    when[value1]: {
        // statements
    }
    when[value2]: {
        // statements
    }
    fixed {
        // default case
    }
}
```

**Parser Location**: [parse_cycle()](src/parser.c#L1001-1077)  
**AST Nodes**: AST_CYCLE, AST_WHEN  
**Design**: Each case ends with `}` (no break needed; fall-through not supported)

### K. Ternary Conditional Expression

**Feature**: Inline if-then-else expression  
**Syntax**: `condition ? trueExpr : falseExpr`  
**Parser Location**: [parse_ternary()](src/parser.c#L706-723)  
**Precedence**: Lowest, right-associative  
**Example**: `min = x < y ? x : y;`

### L. Number Literal Formats

**Supported in Lexer** ([src/lexer.c](src/lexer.c#L150-250)):
- **Decimal**: `123`, `42`
- **Hexadecimal**: `0xFF`, `0x1A`, `0xDEADBEEF`
- **Binary**: `0b1010`, `0B11110000`
- **Octal**: `077`, `0123` (leading zero, digits 0-7 only; `089` would parse as decimal)

### M. Character Literals

**Syntax**: `'c'` where `c` is a single ASCII character  
**Escape Support**: `'\n'`, `'\t'`, `'\\', etc.  
**Lexer Location**: [lexer_read_char()](src/lexer.c#L217-233)

### N. Comment Styles

**Single-line**: `// comment until end of line`  
**Multi-line**: `//>` ... `<//` (supports nesting)  
**Lexer Functions**: [lexer_skip_line_comment()](src/lexer.c#L80-85), [lexer_skip_multiline_comment()](src/lexer.c#L87-108)

### O. Array/Function Call Bracket Syntax

**Brackets `[...]` serve multiple purposes**:
- **Function calls**: `functionName[arg1, arg2]`
- **Show output**: `show[expression]`
- **Read input**: `read[variable]` or `read["prompt"]`
- **If condition**: `if[condition] {...}`
- **While condition**: `while[condition] {...}`
- **Loop**: `loop[init; cond; incr] {...}`
- **Cycle switch**: `cycle[expr] {...}`
- **Builtin calls**: `@function[args]`

This is RasCode's distinctive syntax choice (vs. parentheses in C-like languages).

### P. Curly Braces for Collections

**Curly braces `{...}` serve multiple purposes**:
- **Blocks**: `{statement1; statement2;}`
- **Array initializers**: `arr{int, 10} x = {1, 2, 3, ...};`
- **Map type spec**: `map{keyType, valueType} name;`
- **Array type spec**: `arr{type, size} name;`
- **Group members**: `group Name {type field1; type field2;}`
- **Array indexing**: `array{index}` (RasCode uses curly braces, not square)

### Q. Dot Operator for Member Access

**Syntax**: `object.member`, `object.member1.member2` (chaining)  
**Supports**:
- Struct/group member access: `person.name`
- Nested access: `company.employee.salary`
- Array element member access: `employees{0}.name`

**Parser Location**: [lines 303-329 (parse_primary)](src/parser.c#L303-329)

### R. Arrow Operator for Map Access

**Syntax**: `mapName->get[key]`, `mapName->set[key, value]`, `mapName->has[key]`, `mapName->remove[key]`  
**Parser Location**: [src/parser.c](src/parser.c#L1559-1601)  
**Token**: TOK_ARROW (`->`)

---

## 7. UNDOCUMENTED & RECENTLY ADDED FEATURES

### A. Security Features (Recently Added - P2.x Series)

**Array Bounds Checking** ([IMPLEMENTATION_NOTES_P2.2.md](IMPLEMENTATION_NOTES_P2.2.md)):
- **Default**: Enabled (safety-first)
- **Disable**: `-fno-bounds-check` flag
- **Code**: Check at [src/codegen.c](src/codegen.c#L861) and line 1640

**Overflow Detection** ([IMPLEMENTATION_NOTES_P2.1.md](IMPLEMENTATION_NOTES_P2.1.md)):
- **Default**: Disabled (performance-first)
- **Enable**: `-foverflow-check` flag
- **Checks**: Addition, subtraction, multiplication overflow

### B. Recursion Depth Limit

**Feature**: Prevents stack overflow from deeply nested expressions  
**Limit**: 1000 nesting levels  
**Location**: [src/parser.c](src/parser.c#L740-746)  
**Code**:
```c
const int MAX_DEPTH = 1000;
if (parser->depth > MAX_DEPTH) {
    error_at(..., "Expression nesting too deep - max 1000 levels");
}
```

### C. String Literal Size Limit

**Feature**: Prevents DoS attacks via massive string allocation  
**Limit**: 1 MB (1,048,576 bytes) per string literal  
**Location**: [src/lexer.c](src/lexer.c#L270)  
**Purpose**: Prevents memory exhaustion during tokenization

### D. Compiler Meta Functions

**New in builtins**: Access to compiler information  
- `build_time[]` → compilation timestamp (str)
- `compiler_ver[]` → RasCode version (str)
- Location: [src/builtins.c](src/builtins.c#L165-166)

### E. Direct Syscall Access

**Feature**: Low-level system call interface  
**Syntax**: `@syscall[syscall_number, arg1, arg2, ...]`  
**Return**: `int` (syscall return value)  
**Location**: [src/builtins.c](src/builtins.c#L167)  
**Purpose**: Advanced/systems programming

### F. Advanced Synchronization Primitives

**Recently added concurrent programming features**:
- Atomic compare-and-swap: `@atomic_cmp_swap[ptr, old, new]`
- Atomic increment/decrement
- Condition variables with broadcast
- Channel communication (Go-style)
- Thread pools with work queues
- Location: [src/builtins.c](src/builtins.c#L110-126)

### G. Memory Fence Operations

**Low-level memory barriers**:
- `@mfence[]` - Full memory barrier
- `@lfence[]` - Load fence
- `@sfence[]` - Store fence
- Location: [src/builtins.c](src/builtins.c#L33-35)
- **Purpose**: Multi-threaded code synchronization

### H. Hardware I/O and Port Access

**Embedded systems support**:
- `@port_in[port]` - Read I/O port
- `@port_out[port, value]` - Write I/O port
- `@irq_enable[irq_num]` - Enable interrupt
- `@irq_disable[irq_num]` - Disable interrupt
- `@ioread[addr]` - Read memory-mapped I/O
- `@iowrite[addr, value]` - Write memory-mapped I/O
- Location: [src/builtins.c](src/builtins.c#L81-86)

### I. Stack Allocation

**Dynamic stack memory**:
- `@salloc[size]` - Allocate on stack (faster than heap, freed at function exit)
- Location: [src/builtins.c](src/builtins.c#L21)

### J. Library Import System

**Modular library loading**:
- `@import[".sulib_library_name"]` - Load compiled library at runtime
- **Format**: `.sulib` (RasCode compiled library format)
- Location: [src/builtins.c](src/builtins.c#L168)

---

## SUMMARY STATISTICS

| Category | Count |
|----------|-------|
| **Keywords** | 33 |
| **Token Types** | 79 |
| **AST Node Types** | 37 |
| **Primitive Types** | 8 |
| **Binary Operators** | 36 |
| **Unary Operators** | 8 |
| **Builtin Functions** | 81 |
| **Operator Precedence Levels** | 13 |
| **Supported Number Formats** | 4 (decimal, hex, binary, octal) |
| **Comment Styles** | 2 |
| **String Escape Sequences** | 10 |
| **Delimiter Tokens** | 11 |
| **Compound Assignment Operators** | 11 |

---

## CROSS-REFERENCE: FEATURE → SOURCE CODE LOCATIONS

| Feature | Primary File | Lines | Secondary References |
|---------|-------------|-------|----------------------|
| Keywords | lexer.c | 4-38 | lexer.h:9-42 |
| Tokens | lexer.c | everywhere | lexer.h:6-119 |
| Keywords Recognition | lexer.c | 4-38 | (is_keyword function) |
| Number Parsing | lexer.c | 150-250 | (lexer_read_number) |
| String Parsing | lexer.c | 254-340 | (lexer_read_string) |
| String Interpolation | parser.c | 799-1000 | (parse_interpolated_string) |
| Operator Precedence | parser.c | 557-723 | (Multiple parse_* functions) |
| AST Node Types | ast.h | 6-38 | ast.c (free functions) |
| Builtin Functions | builtins.c | 4-190 | parser.c:1224-1285 |
| Type Conversions | builtins.c | 38-43 | parser.c:1264-1285 |
| Map Operations | parser.c | 1559-1601 | ast.h:26-32 |
| Array Operations | parser.c | 1363-1410 | ast.h:21, 23 |
| Groups/Structs | parser.c | 1697-1747 | ast.h:25-26 |
| Member Access | parser.c | 303-329 | ast.h:24-25 |
| Exception Handling | parser.c | 1025-1061 | ast.h:17 |
| Bounds Checking | codegen.c | 861, 1640 | IMPLEMENTATION_NOTES_P2.2.md |
| Overflow Detection | codegen.c | (conditional) | IMPLEMENTATION_NOTES_P2.1.md |

---

**Document Generated**: April 4, 2026  
**Tool**: Source Code Analysis  
**Status**: Complete & Comprehensive
