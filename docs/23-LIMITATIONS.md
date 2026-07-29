# LIMITATIONS: Features NOT Supported in RASLang

This document explicitly lists language features that are **intentionally not supported** in RASLang (v b-0.2.0 BETA). Understanding these limitations helps prevent confusion and guides design decisions.

---

## Control Flow

### ❌ break Statement
Cannot break out of loops prematurely.

**Not Supported:**
```ras
fnc break_not_supported[]::int {
    // This will not compile:
    loop i from 0 to 10 {
        if i == 5 {
            break;  // ❌ SYNTAX ERROR
        }
        show[i];
    }
    get[0];
}
```

**Workaround:** Use conditional flags or restructure loops
```ras
fnc loop_with_exit[]::int {
    bool continue_loop = true;
    int i = 0;
    
    while continue_loop {
        if i == 5 {
            continue_loop = false;
        }
        show[i];
        i = i + 1;
    }
    
    get[0];
}
```

---

### ❌ continue Statement
Cannot skip to next loop iteration.

**Not Supported:**
```ras
loop i from 0 to 10 {
    if i == 5 {
        continue;  // ❌ SYNTAX ERROR
    }
    show[i];
}
```

**Workaround:** Nest conditionals or use flags
```ras
loop i from 0 to 10 {
    if i not == 5 {
        show[i];
    }
}
```

---

### ❌ goto Statement
No unconditional jumps (label/goto pattern).

**Not Supported:**
```ras
fnc goto_not_supported[]::int {
    show["Start"];
    goto skip;  // ❌ NOT SUPPORTED
    show["Skipped"];
    
    skip:
    show["End"];
    get[0];
}
```

**Workaround:** Use functions and control flow
```ras
fnc skip_section[]::none {
    // This section is "skipped"
}

fnc structured_control[]::int {
    show["Start"];
    skip_section[];
    show["End"];
    get[0];
}
```

---

## Functions & Closures

### ❌ Lambda/Anonymous Functions
Cannot define inline anonymous functions.

**Not Supported:**
```ras
fnc map_demo[]::int {
    arr{int, 3} arr1 = {1, 2, 3};
    
    // Cannot write: arr2 = map(arr1, |x| x * 2)
    
    get[0];
}
```

**Workaround:** Define named functions and pass them
```ras
fnc double_value[x]::int {
    get[x * 2];
}

fnc apply_double[]::int {
    // Apply double_value manually to each element
    get[0];
}
```

---

### ❌ Closures
Functions cannot capture enclosing scope variables.

**Not Supported:**
```ras
fnc closure_demo[]::int {
    int multiplier = 5;
    
    fnc apply[x]::int {
        // Cannot access 'multiplier' from outer scope
        get[x * multiplier];  // ❌ UNDEFINED
    }
    
    get[0];
}
```

**Workaround:** Pass values as function parameters
```ras
fnc apply[x, multiplier]::int {
    get[x * multiplier];
}

fnc closure_workaround[]::int {
    int multiplier = 5;
    int result = apply[3, multiplier];
    get[0];
}
```

---

### ❌ Variadic Functions
Cannot define functions with variable number of arguments.

**Not Supported:**
```ras
fnc sum_all[...args]::int {  // ❌ NOT SUPPORTED
    // Cannot write
    get[0];
}
```

**Workaround:** Use fixed-size arrays or multiple overloads
```ras
fnc sum_array[arr]::int {
    int total = 0;
    // Loop through array
    get[total];
}

fnc sum_two[a, b]::int {
    get[a + b];
}

fnc sum_three[a, b, c]::int {
    get[a + b + c];
}
```

---

## Type System

### ❌ Enums
No enumeration type support.

**Not Supported:**
```ras
enum Color {  // ❌ NOT SUPPORTED
    Red = 0,
    Green = 1,
    Blue = 2
}
```

**Workaround:** Use constants
```ras
const int RED = 0;
const int GREEN = 1;
const int BLUE = 2;

fnc use_colors[]::int {
    int my_color = RED;
    get[0];
}
```

---

### ❌ Generics / Templates
No parametric polymorphism or template instantiation.

**Not Supported:**
```ras
fnc generic_container<T>[value]::T {  // ❌ NOT SUPPORTED
    get[value];
}
```

**Workaround:** Write concrete versions for each type
```ras
fnc container_int[value]::int {
    get[value];
}

fnc container_str[value]::str {
    get[value];
}
```

---

### ❌ Inheritance / Polymorphism
No class hierarchy or virtual methods (RASLang has no OOP).

**Not Supported:**
```ras
group Animal {
    ...
}

group Dog : Animal {  // ❌ NOT SUPPORTED - no inheritance
    ...
}
```

**Workaround:** Use composition or separate types
```ras
group Base {
    int value;
}

group Derived {
    Base base_part;
    int extra;
}

fnc work_with_derived[]::int {
    Derived d;
    get[0];
}
```

---

### ❌ Operator Overloading
Cannot redefine built-in operators for custom types.

**Not Supported:**
```ras
group Vector {
    int x;
    int y;
}

fnc Vector::+(other)::Vector {  // ❌ NOT SUPPORTED
    // Cannot overload +
}
```

**Workaround:** Create named functions
```ras
fnc vector_add[v1, v2]::Vector {
    Vector result;
    result.x = v1.x + v2.x;
    result.y = v1.y + v2.y;
    get[result];
}
```

---

## Exception Handling

### ❌ try-throw-catch
No exception throwing or traditional try-catch blocks.

**Not Supported:**
```ras
fnc try_catch_demo[]::int {
    try {
        // Cannot throw exceptions
        throw Exception("Error");  // ❌ NOT SUPPORTED
    } catch Exception e {
        show[e.message];
    }
    
    get[0];
}
```

**RASLang Alternative:** Use check-when
```ras
fnc check_when_demo[]::int {
    check {
        @error[1, "Error occurred"];
    } when {
        str msg = @get_error_msg[];
        show[msg];
    }
    
    get[0];
}
```

---

### ❌ finally Block
No finally clause for guaranteed cleanup.

**Not Supported:**
```ras
fnc finally_not_supported[]::int {
    check {
        show["Trying"];
    } when {
        show["Caught"];
    } finally {  // ❌ NOT SUPPORTED
        show["Cleanup"];
    }
    
    get[0];
}
```

**Workaround:** Use recovery handlers
```ras
fnc cleanup[]::int {
    show["Cleanup running"];
    get[0];
}

fnc with_recovery[]::int {
    @recover[cleanup];
    show["Main logic"];
    get[0];
}
```

---

### ❌ defer Statement
No defer/defer-like statement for cleanup scheduling.

**Not Supported:**
```ras
fnc defer_demo[]::int {
    int fd = @fopen["/tmp/file", "r"];
    
    defer @fclose[fd];  // ❌ NOT SUPPORTED - would clean up before exit
    
    // Use fd
    
    get[0];
}
```

**Workaround:** Explicit cleanup
```ras
fnc explicit_cleanup[]::int {
    int fd = @fopen["/tmp/file", "r"];
    
    // Use fd
    
    @fclose[fd];  // Manual cleanup
    get[0];
}
```

---

## Memory Management

### ❌ Garbage Collection
No automatic garbage collection; manual memory management only.

**Limitation:** Allocations via @alloc must be manually freed with @free.

**Workaround:** Track allocations carefully
```ras
fnc manual_memory[]::int {
    byte memory = @alloc[1024];
    
    // Use memory
    
    @free[memory];  // Must manually free
    
    get[0];
}
```

---

### ❌ Reference Type / Pointers
No pointer or reference semantics (unsafe references).

**Limitation:** Cannot take address of or dereference variables.

**Workaround:** Pass by value or use memory operations
```ras
// Cannot do: int* ptr = &variable;
// Instead: use @addr for address operations

int addr = @addr[variable];  // Get address
byte value = @peek[addr];     // Read from address
@poke[addr, 42];              // Write to address
```

---

## Iteration & Ranges

### ❌ Range Expressions
No range literal syntax (e.g., 1..10).

**Not Supported:**
```ras
loop i in 0..10 {  // ❌ NO RANGE LITERAL
    show[i];
}
```

**Workaround:** Use traditional loop
```ras
loop i from 0 to 10 {
    show[i];
}
```

---

### ❌ for-in Loop
No iterator-based for-in loops.

**Not Supported:**
```ras
arr{int, 5} values = {1, 2, 3, 4, 5};

for value in values {  // ❌ NOT SUPPORTED
    show[value];
}
```

**Workaround:** Traditional index loop
```ras
arr{int, 5} values = {1, 2, 3, 4, 5};

loop i from 0 to 5 {
    show[values{i}];
}
```

---

## Advanced Features

### ❌ Macros / Preprocessor
No preprocessor or macro expansion.

**Not Supported:**
```ras
#define MAX_SIZE 100  // ❌ NO MACROS

const int MAX_SIZE = 100;  // ✅ Use constants instead
```

---

### ❌ Inline Assembly
No inline assembly blocks.

**Not Supported:**
```ras
fnc asm_not_supported[]::int {
    asm {  // ❌ NOT SUPPORTED
        mov rax, 42
    }
    
    get[0];
}
```

**Workaround:** Write separate .asm file or use system calls
```bash
# Compile to .asm, edit, reassemble
rascom program.ras
# program.ras.asm is now available
# Edit and reassemble if needed
```

---

### ❌ Reflection / Introspection
No runtime type information or reflection capabilities.

**Not Supported:**
```ras
// Cannot inspect type at runtime
// No typeof(), no reflection API
```

**Workaround:** Use explicit type information at compile time
```ras
fnc type_aware[value, type_id]::int {
    // Pass explicit type information
    get[0];
}
```

---

### ❌ Module System / Packages
No import of external modules or packages.

**Limitation:** RASLang is self-contained; no external dependencies.

**Current Approach:** All builtins are pre-linked; no user packages

---

### ❌ Concurrency Primitives (Limited)
While RASLang has threads, mutexes, channels, it does NOT have:
- async/await
- Goroutine-style lightweight threads
- Select statement (for channels)
- Go defer() function

**Supported:** @spawn, @join, @mutex_*, @channel_*

**Not Supported:**
```ras
async {  // ❌ NO ASYNC
    // Cannot use async/await
}

go spawn_task[];  // Similar to @spawn, but syntax different
```

---

### ❌ REPL / Interactive Mode
No interactive REPL; compile-only.

**Requirement:** Must write .ras file and compile with rascom

```bash
rascom program.ras  # Compile and run
# No interactive mode: rascom -i
```

---

### ❌ Conditional Compilation
No #ifdef or conditional compilation directives.

**Not Supported:**
```ras
#ifdef DEBUG  // ❌ NOT SUPPORTED
    show["Debug mode"];
#endif
```

**Workaround:** Use runtime conditionals or build separate programs
```ras
const bool DEBUG = true;

if DEBUG {
    show["Debug mode"];
}
```

---

## Platform & Environment

### ❌ Cross-Platform Compilation
Compiler only targets Linux x86-64.

**Limitation:** Cannot compile for Windows, macOS, ARM, etc.

**Target:** Linux x86-64 exclusively

---

### ❌ Dynamic Loading
Cannot load .so/.dll libraries at runtime.

**Limitation:** All functionality must be built-in or statically linked

---

## Syntax & Parsing

### ❌ Optional Semicolons
Semicolons are REQUIRED (not optional like in JS/Go).

**Not Supported:**
```ras
show[42]  // ❌ Missing semicolon - syntax error
```

**Required:**
```ras
show[42];  // ✅ Semicolon required
```

---

### ❌ Implicit Type Conversion
Some conversions must be explicit.

**Limitation:** Automatic int→str conversions not always available

**Workaround:** Use @to_str[], @to_int[], etc.

---

## Summary: Intentional Design Decisions

| Feature | Status | Reason |
|---------|--------|--------|
| break/continue | ❌ | Simplified control flow |
| Lambdas | ❌ | Explicit function definitions |
| Generics | ❌ | Minimal type system |
| Inheritance | ❌ | No OOP complexity |
| try-throw | ❌ | check-when alternative |
| GC | ❌ | Systems language design |
| Macros | ❌ | Simplicity |
| Async/await | ❌ | Thread-based concurrency |
| Reflection | ❌ | Compile-time focus |
| Cross-platform | ❌ | x86-64 Linux focus |

---

## Implications for Developers

**When porting from other languages:**

1. **No break/continue** → Use flags or restructure loops
2. **No exceptions** → Use check-when error handling
3. **No OOP** → Use groups (structs) + standalone functions
4. **No generics** → Write separate functions per type
5. **No macros** → Use constants or code generation
6. **Linear execution** → No async; use @spawn for parallelism

**RASLang favors:**
- Explicit over implicit
- Function-oriented over object-oriented
- Manual memory management (systems language)
- Compile-time checks over runtime behavior
- Linux x86-64 systems programming

---

**For more information:** See [README.md](README.md) for supported features, or [02-SYNTAX-RULES.md](02-SYNTAX-RULES.md) for syntax overview.
