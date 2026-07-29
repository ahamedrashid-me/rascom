# 04: Variables

## Variable Declaration

**Syntax** (from parser.c parse_var_decl):

```
type variable_name;
type variable_name = initial_value;
```

**Rules:**
- Type must be one of: `int`, `deci`, `char`, `str`, `bool`, `byte`, `ubyte`, or group name
- Variable name: valid identifier (letter/underscore + alphanumeric/underscores)
- Initialization optional (defaults to zero/false/"")
- Semicolon required

**Examples:**
```ras
int age;
str name = "Alice";
bool active = true;
deci price = 9.99;
```

## Variable Scope

Variables are scoped to their enclosing block (function, if, loop, etc.):

```ras
fnc example[]::int {
    int outer = 1;
    
    if[true] {
        int inner = 2;      // Scoped to if block
        show[outer];        // Can access outer scope
        show[inner];
    }
    
    show[inner];            // ERROR: inner out of scope
    get[0];
}
```

**Shadowing:** Inner scopes can redeclare names:

```ras
int x = 1;
{
    int x = 2;              // Shadow: different variable
    show[x];                // Prints 2
}
show[x];                    // Prints 1
```

## Variable Assignment

**Syntax:**
```ras
variable_name = new_value;
```

**Rules:**
- Type must match declared type (no implicit conversion)
- Right-hand side must be expression of correct type
- Semicolon required

**Examples:**
```ras
int x;
x = 42;
x = x + 1;                  // ✓ Right side evaluates to int

str name = "Bob";
name = @concat[name, " Smith"];

deci pi = 3.14;
```

## Compound Assignment Operators

**From parser.c parse_statement:**

The parser recognizes `++` and `--` as standalone statements:

```ras
i++;                        // Increment: i = i + 1
i--;                        // Decrement: i = i - 1
```

NOT as operators in expressions (they work as unary prefix/postfix in expressions though).

As statement:
```ras
int i = 5;
i++;                        // i is now 6
i--;                        // i is now 5
show[i];                    // Shows "5"
```

As expression:
```ras
int x = 5;
int a = x++;                // a = 5, x = 6 (postfix)
int b = ++x;                // x = 7, b = 7 (prefix)
```

**No other compound operators:** (`+=`, `-=`, `*=`, `/=`, etc. NOT supported)

```ras
x += 5;                     // ERROR: Not supported
x *= 2;                     // ERROR: Not supported
```

Instead, use full assignment:
```ras
x = x + 5;
x = x * 2;
```

## Local Variables

All variables declared inside functions are local:

```ras
fnc process[input]::int {
    int result = input * 2;     // Local to process()
    str temp = "working";       // Local to process()
    result = result + 1;
    get[result];
}

// result and temp don't exist here
fnc main[]::int {
    int r = process[10];        // r is local to main()
    get[0];
}
```

## Parameter Variables

Function parameters are treated as local variables:

```ras
fnc add[a, b]::int {
    a = 10;                     // OK: modify parameter
    b = 20;
    get[a + b];
}
```

Parameters are **passed by value** (not by reference):
```ras
fnc modify[x]::int {
    x = 100;
    get[0];
}

fnc main[]::int {
    int value = 5;
    modify[value];              // Pass value (copy)
    show[value];                // Shows "5" not "100"
    get[0];
}
```

For pass-by-reference behavior, pass an address (using `@addr`) and manually dereference (`@peek`/`@poke`).

## Array and Map Variables

See [07-ARRAYS.md](07-ARRAYS.md) and [08-MAPS.md](08-MAPS.md)

```ras
arr{int, 10} numbers;           // Array variable
numbers{0} = 5;
numbers{1} = 10;

map{str, int} ages;             // Map variable
ages->set["Alice", 30];
int alice_age = ages->get["Alice"];
```

## Group Variables (Struct Instances)

**Declaration:**
```ras
GroupName variable_name;
```

**Example:**
```ras
group Person {
    str name;
    int age;
}

fnc main[]::int {
    Person p;                   // Create instance
    p.name = "Bob";
    p.age = 25;
    
    show[p.name];
    show[p.age];
    get[0];
}
```

**No constructor syntax** — just direct field assignment.

## Global Constants vs Variables

**Global Constants** (see [05-CONSTANTS.md](05-CONSTANTS.md)):
```ras
const MAX_SIZE = 100;           // Compile-time constant
```

**No global mutable variables** — only functions have local variables.

```ras
int global_var;                 // ERROR: Can only be const
```

If you need mutable global state, use static memory allocation:
```ras
const BUFFER_ADDR = @alloc[1024];   // Allocate at compile time
```

Or allocate at runtime in main:
```ras
fnc main[]::int {
    int buf_ptr = @alloc[1024];
    @poke[buf_ptr, 65];         // Write 'A'
    show[@peek[buf_ptr]];       // Read back
    @free[buf_ptr];
    get[0];
}
```

## Memory Layout of Variables

From codegen.c symbol management:

```ras
fnc test[]::int {
    int x;                      // offset 0 (4 bytes)
    int y;                      // offset 4 (4 bytes)
    str name;                   // offset 8 (varies)
    arr{int, 5} nums;           // offset ??? (5 * 4 = 20 bytes)
}
```

Variables are allocated on the stack in order of declaration. Arrays and maps take additional space for their elements.

## Initialized vs Uninitialized Variables

**Initialized:**
```ras
int x = 42;                     // x starts with value 42
```

**Uninitialized (default values):**
```ras
int x;                          // x = 0
deci d;                         // d = 0.0
bool b;                         // b = false
str s;                          // s = ""
```

Difference doesn't affect program correctness, but initialized is more explicit and readable.

## Limitations

**No dynamic typing:**
```ras
var x = 5;                      // ERROR: var keyword not supported
```

**No type re-declaration:**
```ras
int x = 5;
int x = 10;                     // ERROR: Redeclaration (shadowing OK in different scope)
```

**No list/tuple unpacking:**
```ras
a, b = 1, 2;                    // ERROR: Not supported
```

**No destructuring:**
```ras
Person {name, age} = p;         // ERROR: Not supported
```

## Related Docs
- [03-DATA-TYPES.md](03-DATA-TYPES.md) — Type system
- [05-CONSTANTS.md](05-CONSTANTS.md) — Global constants
- [06-OPERATORS.md](06-OPERATORS.md) — Operations on variables
- [07-ARRAYS.md](07-ARRAYS.md) — Array variables
- [09-GROUPS.md](09-GROUPS.md) — Struct instance variables
