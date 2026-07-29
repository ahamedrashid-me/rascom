# 02: Syntax Rules

## Fundamental Syntax Rules (from lexer.c and parser.c)

### 1. Statement Termination

**RULE: Every statement ends with semicolon (`;`)**

From parser.c, ALL parse functions expect `TOK_SEMICOLON` at end:
- Variable declarations: `int x;`
- Assignments: `x = 5;`
- Function calls: `print[x];`
- Builtin calls: `@len["string"];`
- Return statements: `get[5];`
- Array operations: `arr{0} = 1;`
- Map operations: `map->set["key", 1];`

**Exception:** Block statements (`if`, `while`, `loop`, etc.) have `{ }` instead of `;`

```ras
int x = 5;          // ✓ Semicolon
x = 10;             // ✓ Semicolon
show["Hello"];      // ✓ Semicolon

if[x > 0] {         // ✗ No semicolon after }
    show["Positive"];
}                   // ✓ No semicolon after }
```

### 2. Brackets vs Braces vs Parentheses

There are THREE different bracket types with specific meanings:

#### Square Brackets `[ ]` (TOK_LBRACKET / TOK_RBRACKET)
- Function parameters/arguments: `fnc[a, b]`, `print[x]`
- Statement keywords with conditions: `if[cond]`, `while[cond]`, `loop[...]`
- Array initialization values: `arr{...} = {1, 2, 3}`
- Builtin function calls: `@len[string]`

```ras
fnc add[a, b]::int { get[a + b]; }  // Parameters in []
show[42];                            // Argument in []
if[x > 0] { }                        // Condition in []
```

#### Curly Braces `{ }` (TOK_LBRACE / TOK_RBRACE)
- Code blocks: function bodies, if bodies, loop bodies
- Array type specification: `arr{int, 10}` means array of int, size 10
- Array indexing: `arr{0}`, `arr{i}`, `name{index}`
- Array literals/initializers: `{1, 2, 3}`
- Map type specification: `map{str, int}` means map from str to int

```ras
fnc test[]::int {                   // Braces for code block
    arr{int, 10} nums;              // Braces for array type
    nums{0} = 5;                    // Braces for indexing
}
```

#### Parentheses `( )` (TOK_LPAREN / TOK_RPAREN)
- Grouping expressions: `(a + b) * c`
- No special language meaning otherwise

```ras
int y = (5 + 3) * 2;               // Parentheses for grouping
```

### 3. Delimiters and Their Meanings

| Delimiter | Name | Usage | Example |
|-----------|------|-------|---------|
| `[ ]` | Square brackets | Function params, keyword args | `fnc[p1, p2]`, `if[x > 0]` |
| `{ }` | Curly braces | Code blocks, array/map types, indexing | `{ code }`, `arr{type, size}`, `arr{index}` |
| `( )` | Parentheses | Expression grouping | `(a + b) * c` |
| `.` | Dot/Period | Member access | `person.name`, `config.age` |
| `->` | Arrow | Map operations | `map->get[key]`, `map->set[k, v]` |
| `:` | Colon | Map case labels | `when[val]: { body }` |
| `::` | Double colon | Return types in functions, type spec | `fnc f[]::int`, `@type[x]::str` |
| `;` | Semicolon | Statement terminator | `x = 5;`, `show[y];` |
| `,` | Comma | Parameter/argument separator | `fnc[a, b, c]`, `@concat["a", "b"]` |
| `@` | At sign | Builtin function prefix | `@len["text"]`, `@alloc[1024]` |
| `$` | Dollar | String interpolation | `"Hello $name"`, `"Value: ${expr}"` |

### 4. Comments

From lexer.c:

**Line Comments:**
```ras
// This is a line comment
show["Hello"];  // Comment at end of line
```

**Multiline Comments:**
```ras
//> This is a
    multiline comment
    spanning many lines
<//
```

Rules:
- Line comments: `//` until end of line
- Multiline: `//>` to `<//` (must close properly)

### 5. Identifiers and Keywords

**Valid Identifier Names:**
- Start with letter or underscore: `[a-zA-Z_]`
- Followed by letters, digits, underscores: `[a-zA-Z0-9_]*`

```ras
int myVar;          // ✓
str _private;       // ✓
bool x1;            // ✓
float bad;          // ✗ 'float' is invalid type (only deci)
```

**Reserved Keywords** (from lexer.c):
```
fnc  get  loop  if  or  while  cycle  when  fixed  check
read  show  group  set  arr  map  pkg  use  const
int  deci  char  str  bool  byte  ubyte  none
true  false  and  xor  not
```

Cannot use these as variable names.

### 6. Literals

**Number Literals:**
- Decimal: `42`, `0`, `-5`
- Hexadecimal: `0xFF`, `0x123ABC` (case insensitive)
- Octal: `0755` (digits 0-7)
- Binary: `0b1010`, `0B1111`

**Decimal Literals (Floating Point):**
- `3.14`, `0.5`, `-2.5`, `1.0`

**String Literals:**
- Double quotes: `"Hello"`
- Escape sequences: `\n` (newline), `\t` (tab), `\"` (quote), `\\` (backslash)
- Interpolation: `$var` or `${expr}`

**Character Literals:**
- Single quotes: `'A'`, `'!'`, `' '` (space)
- Converted to ASCII int value automatically

**Boolean Literals:**
- `true` (keyword)
- `false` (keyword)

### 7. Operator Syntax

**Binary Operators:**
```ras
int sum = a + b;              // Infix: operand op operand
bool result = x > 5 && y < 10;
```

**Unary Operators:**
```ras
int neg = -x;                 // Prefix
int post = x++;               // Postfix
```

**Ternary Operator:**
```ras
int val = condition ? true_val : false_val;
```

### 8. Type Annotations

**Explicit Type Declaration:**
```ras
int age;
str name;
bool flag;
deci pi;
```

**Type Conversion (Builtin):**
```ras
int x = 42;
deci f = @type[x]::deci;     // Convert x to deci, using @type builtin with :: notation
```

### 9. Array and Map Type Definition

**Array Type:**
```
arr{element_type, size} name;
arr{int, 100} numbers;        // Array of 100 ints
arr{str, 50} names;           // Array of 50 strings
```

**Map Type:**
```
map{key_type, value_type} name;
map{str, int} ages;           // Map from string to int
map{int, str} lookup;         // Map from int to string
```

### 10. Scope and Nesting

**Function-Level Scope:**
```ras
fnc outer[]::int {
    int x = 1;              // Scope: within function outer
    { 
        int x = 2;          // Shadow: new scope in this block
        show[x];            // Prints 2
    }
    show[x];                // Prints 1
    get[0];
}
```

**Block Scope (from parse_block):**
- Variables declared in blocks are scoped to that block
- Inner blocks can shadow outer variables
- Blocks created by: functions, if/else, loops, check/when, cycles

### 11. Line and Column Tracking

The lexer tracks line and column for error reporting:
- Line starts at 1
- Column starts at 1
- Newlines increment line counter, reset column to 1

Error messages show: `Error at line X, column Y: message`

## Syntax Conventions (Recommended)

While not required by parser, these are recommended:

```ras
// Use consistent indentation (4 spaces or tab)
fnc example[]::int {
    if[condition] {
        show["nested"];
    }
    get[0];
}

// One statement per line
int x = 1;
int y = 2;
// NOT: int x = 1; int y = 2;

// Spaces around operators
int result = a + b;
// NOT: int result=a+b;

// Space after commas
fnc[a, b, c]
// NOT: fnc[a,b,c]
```

## Special Characters and Encoding

- Source files: UTF-8 encoding
- Comments can contain any unicode
- String literals can contain unicode (escaping varies)
- Character literals: single ASCII character only
