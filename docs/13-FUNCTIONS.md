# 13: Functions

## Function Definition

**Syntax** (from parser.c parse_function):

```
fnc function_name[param1, param2, ...]::return_type {
    statements;
    get[return_value];
}
```

**Components:**
- `fnc` keyword
- Function name (identifier)
- `[...]` parameter list (can be empty)
- `::` double colon separator
- Return type
- `{ ... }` function body

## Simple Function

```ras
fnc greet[]::int {
    show["Hello!"];
    get[0];
}

fnc main[]::int {
    greet[];
    get[0];
}
```

- `greet[]` has no parameters (empty brackets)
- `::int` returns integer
- `get[0]` returns 0 (success)

## Functions with Parameters

```ras
fnc add[a, b]::int {
    get[a + b];
}

fnc main[]::int {
    int result = add[5, 3];     // Returns 8
    show[result];
    get[0];
}
```

**Multiple parameters:**
```ras
fnc multiply[x, y, z]::int {
    get[x * y * z];
}

int product = multiply[2, 3, 4];  // 24
```

**Type annotations on parameters:**
From parser.c, parameter syntax is:

```
type name, type name, ...
```

```ras
fnc divide[num, denom]::deci {
    deci result = @type[num]::deci / @type[denom]::deci;
    get[result];
}

divide[10, 4]  // 2.5
```

## Return Types

**Return int:**
```ras
fnc get_count[]::int {
    get[42];
}
```

**Return string:**
```ras
fnc get_name[]::str {
    get["Alice"];
}
```

**Return bool:**
```ras
fnc is_ready[]::bool {
    get[true];
}
```

**Return group:**
fnc create_point[]::Point {
    Point p;
    p.x = 10;
    p.y = 20;
    get[p];
}
```

**Return none:**
```ras
fnc print_only[msg]::none {
    show[msg];
    // No return (implicitly returns void/none)
}
```

Functions returning `none` cannot be used in expressions.

## Return Statement

**Syntax:**
```ras
get[expression];
```

`get` is the return keyword in RASLang.

```ras
fnc calculate[x]::int {
    if[x < 0] {
        get[-1];            // Early return
    }
    get[x * 2];             // Normal return
}
```

## Function Scope

Functions are global and visible to other functions:

```ras
fnc helper[]::int {
    get[42];
}

fnc caller[]::int {
    int val = helper[];     // Can call helper()
    get[val];
}

fnc main[]::int {
    caller[];               // Can call caller()
    get[0];
}
```

All functions declared at top level. No nested functions.

## Recursion

Functions can call themselves:

```ras
fnc factorial[n]::int {
    if[n <= 1] {
        get[1];
    }
    get[n * factorial[n - 1]];
}

fnc main[]::int {
    show[factorial[5]];     // 120
    get[0];
}
```

**Danger:** Stack overflow if too deep
```ras
fnc infinite_recursion[]::int {
    get[infinite_recursion[]];  // Will crash
}
```

## Pass by Value

Parameters are passed by value (copied):

```ras
fnc modify_int[x]::int {
    x = 100;                // Modifies local copy only
    get[0];
}

fnc main[]::int {
    int value = 5;
    modify_int[value];
    show[value];            // Still 5, not changed
    get[0];
}
```

## Passing Groups

Groups are passed by value:

```ras
group Point {
    int x;
    int y;
}

fnc move_point[p, dx, dy]::int {
    p.x = p.x + dx;         // Modifies local copy
    p.y = p.y + dy;
    get[0];
}

fnc main[]::int {
    Point original;
    original.x = 0;
    original.y = 0;
    
    move_point[original, 10, 20];
    show[original.x];       // Still 0 (copy was modified, not original)
    get[0];
}
```

To pass by reference, use memory addresses (advanced).

## Default Parameters (Optional Parameters)

**✓ SUPPORTED**: Functions can have default parameter values.

**Syntax:**
```ras
fnc function_name[param1, param2 = default_value, param3 = default]::return_type {
    // body
}
```

**Rules:**
- Parameters with defaults must come after parameters without defaults
- Default values are evaluated in the caller's context
- Both provided and default arguments are passed to the function

**Examples:**

```ras
fnc greet[str name, str greeting = "Hi"]::none {
    show[greeting];
    show[" "];
    show[name];
    show["\n"];
}

fnc main[]::int {
    greet["Alice", "Hello"];  // Both args provided
    greet["Bob"];             // Uses default greeting "Hi"
    get[0];
}
```

Output:
```
Hello Alice
Hi Bob
```

**Multiple defaults:**
```ras
fnc add[int a, int b = 10, int c = 5]::int {
    get[a + b + c];
}

fnc main[]::int {
    show[add[10, 20, 30]];    // 60 (all provided)
    show[add[10, 20]];        // 35 (c=5 default)
    show[add[10]];            // 25 (b=10, c=5 defaults)
    get[0];
}
```

**Type-specific defaults:**
```ras
fnc multiply[int x, int y = 2, int z = 3]::int {
    get[x * y * z];
}

fnc calculate[deci pi = 3.14, deci e = 2.71]::deci {
    get[pi + e];
}
```

**Comparison with manual defaults:**

Before (manual approach):
```ras
fnc greet_manual[name]::int {
    if[@len[name] == 0] {
        name = "World";     // Awkward default inside function
    }
    show["Hello "];
    show[name];
    get[0];
}
```

After (default parameters):
```ras
fnc greet[str name = "World"]::none {
    show["Hello "];
    show[name];
}
```

**Error handling:**
- Calling with too few required arguments → Compile-time error
- Calling with too many arguments → Compile-time error

## Variadic Functions (Not Supported)

Cannot accept variable number of arguments:

```ras
fnc sum_all[values...]::int {  // ERROR: Not supported
    get[0];
}
```

Workaround: Pass array or map

```ras
fnc sum_array[arr_ptr, size]::int {
    int total = 0;
    loop[int i = 0; i < size; i++] {
        total = total + @peek[arr_ptr + i];
    }
    get[total];
}
```

## Anonymous Functions/Lambdas (Not Supported)

No lambda expressions:

```ras
map->filter[(x) => x > 5]   // ERROR: Not supported in RASLang
```

Define separate function instead:

```ras
fnc is_greater_than_five[x]::bool {
    get[x > 5];
}

// Use separately
```

## Function Pointers

No first-class function values:

```ras
fnc my_func[]::int { get[0]; }

fnc apply[f]::int {
    get[f[]];               // ERROR: Can't pass functions
}
```

Workaround: Use addresses manually (advanced)

```ras
fnc apply_at_address[func_addr]::int {
    // Call function at memory address
    get[0];
}
```

## Function Organization

**No modules/namespaces officially**, but conventions:

```ras
// Math module (naming convention)
fnc math_add[a, b]::int { get[a + b]; }
fnc math_multiply[a, b]::int { get[a * b]; }

// String module
fnc str_uppercase[s]::str { get[s]; }  // Not implemented
fnc str_length[s]::int { get[@len[s]]; }
```

## Entry Point

`fnc main[]::int` is the entry point:

```ras
fnc main[]::int {
    show["Program start"];
    get[0];  // Exit code
}
```

Return code:
- `0` = success
- Non-zero = error/failure

## Common Patterns

**Guard clause:**
```ras
fnc safe_divide[a, b]::deci {
    if[b == 0] {
        get[0.0];           // Return zero on division by zero
    }
    get[@type[a]::deci / @type[b]::deci];
}
```

**Early return:**
```ras
fnc find_in_array[arr_addr, target]::int {
    loop[int i = 0; i < 10; i++] {
        if[@peek[arr_addr + i] == target] {
            get[i];         // Early return with result
        }
    }
    get[-1];                // Not found
}
```

**Multiple returns based on condition:**
```ras
fnc classify[value]::str {
    if[value < 0] {
        get["negative"];
    }
    if[value == 0] {
        get["zero"];
    }
    get["positive"];
}
```

## Performance

Functions are zero-cost abstractions (compiled to direct code):

```ras
fnc add[a, b]::int {
    get[a + b];
}
```

Compiles to direct addition instruction, not function call overhead (if inlined).

Recursion has call stack overhead — use loops when possible.

## Debugging Functions

No built-in debugging, but can add debug output:

```ras
const DEBUG = true;

fnc process[x]::int {
    if[DEBUG] {
        show["Processing: "];
        show[x];
    }
    int result = x * 2;
    if[DEBUG] {
        show["Result: "];
        show[result];
    }
    get[result];
}
```

## Related Documentation
- [01-PROGRAM-STRUCTURE.md](01-PROGRAM-STRUCTURE.md) — Function definitions
- [04-VARIABLES.md](04-VARIABLES.md) — Function parameters
- [03-DATA-TYPES.md](03-DATA-TYPES.md) — Return types
- [09-GROUPS.md](09-GROUPS.md) — Groups as parameters
