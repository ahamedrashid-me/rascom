# 05: Constants

## Global Constants

**Syntax** (from parser.c parse_const_decl):

```
const CONSTANT_NAME = value;
```

**Rules:**
- Must appear at top-level (after imports, before groups/functions)
- Name is typically UPPERCASE (convention)
- Value must be a compile-time constant expression
- Semicolon required
- Cannot be reassigned after declaration

## Declaration Examples

```ras
const MAX_PLAYERS = 100;
const PI = 3.14159;
const VERSION = "1.0";
const NEGATIVE = -42;
const HEX_VALUE = 0xFF;
const EMPTY_STRING = "";
const TRUE_VAL = true;
```

## Usage

Reference by name in expressions and statements:

```ras
const MAX_SIZE = 1000;

fnc process[]::int {
    arr{int, MAX_SIZE} buffer;  // Use in array type
    
    if[size > MAX_SIZE] {       // Use in condition
        show["Too large"];
    }
    
    int limit = MAX_SIZE;       // Use in assignment
    get[0];
}
```

## Expression in Constants

Constants can reference previously declared constants:

```ras
const WIDTH = 800;
const HEIGHT = 600;
const AREA = WIDTH * HEIGHT;   // = 480000
```

Order matters:
```ras
const AREA = WIDTH * HEIGHT;   // ERROR: WIDTH not yet defined
const WIDTH = 800;
const HEIGHT = 600;
```

## Compile-Time vs Runtime

Constants are evaluated at **compile time**:

```ras
const CURRENT_TIME = 0;        // Hardcoded at build time
```

For runtime values, use variables in main():

```ras
const BUFFER_SIZE = 4096;

fnc main[]::int {
    int buf_addr = @alloc[BUFFER_SIZE];  // Allocate at runtime
    @free[buf_addr];
    get[0];
}
```

## Type Inference for Constants

Types are inferred from the literal value:

```ras
const I = 42;               // int
const F = 3.14;             // deci
const S = "text";           // str
const B = true;             // bool
const C = 'X';              // char (ASCII value)
const H = 0xFF;             // int (hex)
```

No explicit type annotation in parser.

## Constants Cannot Be Arrays or Maps

Constants don't support complex types:

```ras
const NUMS = {1, 2, 3};     // ERROR: Array constant not allowed
const MAP = {};             // ERROR: Map constant not allowed
```

For array/map initialization, do it in code:
```ras
fnc main[]::int {
    arr{int, 3} nums = {1, 2, 3};    // Initializer in code
    get[0];
}
```

## Namespace

Constants are global and can be accessed from any function:

```ras
const SHARED = 100;

fnc func_a[]::int {
    show[SHARED];           // ✓ Accessible
    get[0];
}

fnc func_b[]::int {
    int x = SHARED * 2;     // ✓ Accessible
    get[x];
}
```

No namespacing mechanism for constants.

## String Constants with Interpolation

String constants **cannot use interpolation** (static content only):

```ras
const MESSAGE = "Hello";              // ✓ OK
const GREETING = "Hello $name";       // ❓ Probably won't interpolate
```

For interpolation, build strings at runtime:
```ras
fnc greet[name]::str {
    get[@concat["Hello ", name]];
}
```

## No Constant Functions

You cannot declare constants that are function pointers:

```ras
const FUNC = add;           // ERROR: Not supported
```

To reuse functions, call them directly or store results:
```ras
int precomputed = add[5, 3];      // Pre-compute if needed
```

## Preprocessor-Like Constants

RASLang constants work like C preprocessor defines for literals:

```ras
const DEBUG = false;

fnc log[msg]::int {
    if[DEBUG] {
        show["DEBUG: "];
        show[msg];
    }
    get[0];
}
```

## Memory for Constants

Constants are embedded in the compiled code (readonly data section).

```ras
const READONLY = 42;        // Embedded in binary
const STR_CONST = "text";   // Embedded in binary
```

They don't consume stack or heap memory at runtime (roughly).

## Limitations

**No enumerations:**
```ras
const MONDAY = 1;           // Just constants, not an enum type
const TUESDAY = 2;
```

**No bitflags language support:**
```ras
const READ = 4;
const WRITE = 2;
const EXECUTE = 1;
// Must manually combine: READ | WRITE
```

**No constant expressions with function calls:**
```ras
const SIZE = @len["hello"];  // Probably ERROR: Not compile-time
```

## Related Docs
- [01-PROGRAM-STRUCTURE.md](01-PROGRAM-STRUCTURE.md) — Where constants appear
- [04-VARIABLES.md](04-VARIABLES.md) — Variables vs constants
- [03-DATA-TYPES.md](03-DATA-TYPES.md) — Constant types
