# 01: Program Structure

## Overall Program Layout

Every RASLang program has this structure (from parser.c parse phase):

```
[Package Imports]    ← Optional
[Global Constants]   ← Optional
[Group Definitions]  ← Optional
[Function Definitions] ← Required (at least main)
```

### Program Example

```ras
pkg:stdio;                  // Package import
const MAX_SIZE = 100;       // Global constant
const PI = 3.14159;

group Point {               // Custom struct type
    int x;
    int y;
}

fnc add[a, b]::int {        // Function definition
    get[a + b];
}

fnc main[]::int {          // Entry point (implicit)
    show["Program start"];
    get[0];
}
```

## Package Imports

**Syntax** (from parser.c parse_package_import):
```
pkg:package_name;
pkg:package_name as alias;
pkg:<package_name>;
```

**Details:**
- Imports are processed BEFORE constants and groups
- Packages contain precompiled libraries (`.sulib` files)
- Optional `as` clause creates an alias for the imported module
- Multiple imports allowed

**Example:**
```ras
pkg:stdio;              // Import standard I/O package
pkg:math as math_lib;   // Import with alias
```

## Global Constants

**Syntax** (from parser.c parse_const_decl):
```
const NAME = literal_or_expression;
```

**Rules:**
- Must appear after imports, before groups/functions
- Values are determined at compile time
- Can reference other constants
- Used with `@` prefix when accessing from expressions? NO — Use by name directly

**Examples:**
```ras
const PI = 3.14159;
const MAX_ARRAY_SIZE = 1000;
const NEGATIVE = -42;
const STR_CONST = "immutable string";
```

## Group Definitions (Structs)

**Syntax** (from parser.c parse_group):
```
group GroupName {
    type field1;
    type field2;
    grouptype field3;
}
```

**Rules:**
- Defined at top-level (after constants, before functions)
- Fields must have explicit types
- Field types can be primitives OR other group names (nested structs)
- No methods inside groups (functions are separate)
- No default values for fields

**Example:**
```ras
group Address {
    str street;
    str city;
    int zipcode;
}

group Person {
    str name;
    int age;
    Address home;    // Nested group
}
```

**Field Access:**
```ras
Person p;           // Declare variable of group type
p.name = "Alice";
p.age = 30;
p.home.city = "NYC";
```

## Function Definitions

**Syntax** (from parser.c parse_function):
```
fnc function_name[param1, param2, ...]::return_type {
    statement;
    statement;
    get[return_value];
}
```

**Rules:**
- All functions are top-level (no nested functions)
- Parameter list ALWAYS required (even if empty: `[]`)
- Parameters: `type name, type name, ...`
- Return type: ANY type keyword (int, str, etc.) or group name
- Body: block of statements ending with `get[expr];`
- If no `get` at end, returns default value for type

**Examples:**
```ras
// No parameters, returns int
fnc count[]::int {
    get[42];
}

// Multiple parameters, returns string
fnc greet[name, age]::str {
    str msg = @concat["Hello ", name];
    get[msg];
}

// Nested group parameter
fnc get_age[person]::int {
    get[person.age];
}

// No explicit return statement (not recommended)
fnc void_like[]::int {
    show["Done"];
}
```

**Special Function: main**

The `main[]::int` function is the entry point. It should return an exit code:
```ras
fnc main[]::int {
    show["Program running"];
    get[0];  
}
```

## Import Resolution Order (Compiler Phase)

From parser.c:

1. Process imports (`pkg:name;`)
2. Collect constants (`const NAME = value;`)
3. Collect groups (`group Name { ... }`)
4. Collect functions (`fnc name[...]::[...]::returntype { ... }`)
5. Codegen processes in order: imports → constants → groups → functions

This means:
- Functions can call other functions (forward declarations work)
- Groups can reference previous groups but not forward references
- Constants must be defined before use in other constants

## Complete Program Example

```ras
pkg:stdio;          // Import I/O package

const VERSION = "1.0";
const AUTHOR = "Anonymous";

group Config {
    str version;
    str author;
}

fnc print_info[cfg]::int {
    show["Version: "];
    show[cfg.version];
    show["Author: "];
    show[cfg.author];
    get[0];
}

fnc main[]::int {
    Config cfg;
    cfg.version = VERSION;
    cfg.author = AUTHOR;
    print_info[cfg];
    get[0];
}
```

## Compilation Output

RASLang compiler (rascom) generates:
- `.ras.asm` file (x86-64 assembly - intermediate)
- Executable binary (platform-dependent)
- Object files (intermediate)

Example:
```bash
rascom program.ras
# Produces executable: a.out

rascom program.ras -o myapp
# Produces executable: myapp

# Assembly file (.ras.asm) is automatically cleaned up
```
