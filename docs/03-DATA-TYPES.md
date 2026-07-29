# 03: Data Types

## Type System Overview

RASLang has **8 primitive data types** (defined in lexer.c):

| Type | Token | Range | Size | Default | Example |
|------|-------|-------|------|---------|---------|
| `int` | TOK_INT | -2^31 to 2^31-1 | 4 bytes | 0 | `int x = 42;` |
| `deci` | TOK_DECI | IEEE 754 double | 8 bytes | 0.0 | `deci pi = 3.14159;` |
| `char` | TOK_CHAR | ASCII 0-255 | 1 byte | 0 | `char c = 'A';` |
| `str` | TOK_STR | UTF-8 string | varies | "" | `str s = "hello";` |
| `bool` | TOK_BOOL | true/false | 1 byte | false | `bool b = true;` |
| `byte` | TOK_BYTE | 0-255 | 1 byte | 0 | `byte b = 255;` |
| `ubyte` | TOK_UBYTE | 0-255 | 1 byte | 0 | `ubyte ub = 127;` |
| `none` | TOK_NONE | void/null | 0 bytes | - | Return type only |

## Primitive Type Details

### int (Integer) - `TOK_INT`
32-bit signed integer, standard C `int`

```ras
int x = 42;
int neg = -100;
int hex = 0xFF;           // 255
int octal = 0755;         // 493
int binary = 0b1010;      // 10
```

Operators:
- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Bitwise: `&`, `|`, `^`, `~`, `<<`, `>>`
- Comparison: `==`, `!=`, `<`, `>`, `<=`, `>=`
- Logical: `&&`, `||`, `!`

### deci (Decimal/Float) - `TOK_DECI`
64-bit IEEE 754 double floating point

```ras
deci pi = 3.14159;
deci neg = -2.71828;
deci small = 0.00001;
deci science = 1.23e-4;     // Scientific notation? Check lexer
```

Operators: Same as int (arithmetic, comparison, logical)

### char (Character) - `TOK_CHAR`
Single ASCII character (8-bit)

```ras
char a = 'A';              // Letter
char digit = '5';
char space = ' ';
char newline = '\n';       // Escape sequence
char tab = '\t';
```

Escape sequences (from lexer):
- `\n` - newline
- `\t` - tab
- `\"` - double quote
- `\\` - backslash
- `\'` - single quote

#### Character-to-Integer Conversion
Character literals are automatically converted to ASCII int in parse_primary:
```ras
char c = 'A';              // Actually stores 65 (ASCII value)
int ascii = @ord['A'];     // Get ASCII int from char
char back = @chr[65];      // Get char from ASCII int
```

### str (String) - `TOK_STR`
UTF-8 encoded text string

```ras
str greeting = "Hello";
str multi = "Line 1\nLine 2";      // With escape sequences
str empty = "";
str interpolated = "Hello $name";
str complex = "Result: ${x + y}";
```

**String Operations:**
- Concatenation: `@concat[s1, s2]`
- Length: `@len[s]`
- Substring: `@substr[s, start, length]`
- Comparison: `@strcmp[s1, s2]`
- Conversion from int: `@type[42]::str`

**Interpolation (Strings Only):**
- Simple: `"Hello $name"` → outputs value of `name`
- Expression: `"Sum: ${a + b}"` → outputs result of `a + b`
- Expands to multiple `show` statements internally

### bool (Boolean) - `TOK_BOOL`
Logical true/false value

```ras
bool flag = true;
bool check = false;
bool result = x > 5;       // Result of comparison
bool logic = true && false; // Result of logical operation
```

Values:
- `true` - boolean true literal
- `false` - boolean false literal

Operators:
- Logical: `&&` (AND), `||` (OR), `!` (NOT), `xor` (XOR)
- Comparison: `==`, `!=`

### byte (Unsigned Byte) - `TOK_BYTE`
8-bit unsigned integer (0-255)

```ras
byte b = 255;
byte hex = 0xFF;
byte small = 0;
```

Use for:
- Memory operations (peek/poke)
- Binary data
- When you need small unsigned values

Difference from `ubyte`:
- `byte` and `ubyte` appear to be equivalent in parser
- Both are 8-bit unsigned
- Distinction unclear from source; use consistently

### ubyte (Unsigned Byte) - `TOK_UBYTE`
8-bit unsigned integer (0-255) - same as byte

```ras
ubyte b = 255;
ubyte b2 = 0x80;
```

### none (Void) - `TOK_NONE`
Special type for functions with no meaningful return value

```ras
fnc print_only[msg]::none {
    show[msg];
}
```

Functions returning `none`:
- Do not use `get[value];`
- Implicitly return without a value
- Cannot be used in expressions that need a value

## Composite Types

### Arrays
See [07-ARRAYS.md](07-ARRAYS.md) for details

**Declaration:**
```ras
arr{int, 10} numbers;         // Array of 10 ints
arr{str, 5} names;
arr{deci, 100} values;
```

### Maps
See [08-MAPS.md](08-MAPS.md) for details

**Declaration:**
```ras
map{str, int} ages;           // Map: str keys → int values
map{int, deci} lookup;        // Map: int keys → deci values
```

### Groups (Custom Types)
See [09-GROUPS.md](09-GROUPS.md) for details

**Definition:**
```ras
group Point {
    int x;
    int y;
}
```

**Usage:**
```ras
Point p;                      // Declare variable of custom type
p.x = 10;
p.y = 20;
```

## Type Conversions

### Implicit (Automatic)

The parser doesn't show implicit conversion rules clearly, so assume **NO implicit conversions** — use explicit conversion with `@type` builtin.

### Explicit Conversion

**Builtin Function: `@type`** (from builtins.c, category BUILTIN_CAT_CONVERSION)

```
@type[value]::target_type
@type[value, additional_args]::type1,type2
```

Examples:
```ras
int x = 42;
deci f = @type[x]::deci;      // int → deci
str s = @type[x]::str;         // int → str
int i = @type[3.14]::int;      // deci → int (truncates)
bool b = @type[1]::bool;       // int → bool
```

**Deprecated Conversion Functions** (still in BUILTIN_REGISTRY):
- `@to_int[x]`
- `@to_deci[x]`
- `@to_byte[x]`
- `@to_bool[x]`
- `@to_str[x]`

Recommendation: Use `@type[x]::targettype` instead.

### Character-Integer Conversion

**`@ord` builtin:** Character → Integer (ASCII value)
```ras
char c = 'A';
int ascii = @ord[c];           // Returns 65
```

**`@chr` builtin:** Integer → Character
```ras
int ascii = 65;
char c = @chr[ascii];          // Returns 'A'
```

## Type Checking and Inference

RASLang is **statically typed** — all variables must have explicit types.

```ras
int x = 5;                     // ✓ Type explicit
x = 10;                        // ✓ Matches declared type
x = "string";                  // ✗ Type mismatch (should error)

str s = @type[42]::str;        // ✓ Explicit conversion
```

**No type inference** for variable declarations:
```ras
x = 5;                         // ✗ Error: x not declared
int x = y + z;                 // ✗ Error if y,z not declared before use
```

## Special Values

### Default Values

When a variable is declared without initialization, it gets a default:

```ras
int x;                         // x = 0
deci d;                        // d = 0.0
char c;                        // c = '\0' (null char, int 0)
str s;                         // s = "" (empty string)
bool b;                        // b = false
byte by;                       // by = 0
```

### Null and Undefined

- **Null pointers**: Addresses are just `int` values, no special null type
- **Undefined**: Variables automatically zero-initialized if not set

## Size Information

**`@sizeof` builtin** — Get type size in bytes

```ras
int int_size = @sizeof[int];      // Returns 4
int deci_size = @sizeof[deci];    // Returns 8
int byte_size = @sizeof[byte];    // Returns 1
int str_size = @sizeof[str];      // Returns ??? (dynamic)
```

## Related Documentation
- [04-VARIABLES.md](04-VARIABLES.md) — Variable declaration and scope
- [06-OPERATORS.md](06-OPERATORS.md) — Type-specific operations
- [16-BUILTINS.md](16-BUILTINS.md) — All conversion functions


## Need to improve the types 

Based on your current specification in `03-DATA-TYPES.md`, Rascode has a great, highly lean foundation with **8 primitive types**: `int`, `deci`, `char`, `str`, `bool`, `byte`, `ubyte`, and `none`.

But if you are building the **BOSS OF ADA**, yes, you absolutely need a few specific data types added to your type system. Ada’s legendary safety and Rust/Zig's unmatched system control don't just come from memory allocation; they come from **Type Strictness**.

Since Rascode is targeting **system-level tasks** with explicit memory control, here are the critical gaps in your current data types that you must fill to beat the competition.

---

### 1. Pointer Types (`ptr`) or Native Unsigned Memory Addresses

Right now, your documentation states:

> *"Null pointers: Addresses are just `int` values, no special null type"*

**Why this is a vulnerability:** Your `int` type is a standard 32-bit signed integer. If Rascode is compiled on a modern 64-bit machine, memory addresses require 64 bits (8 bytes). Packing a 64-bit pointer into a 32-bit `int` will cause integer truncation, completely breaking your memory model on 64-bit systems. Furthermore, using a *signed* integer means memory addresses could theoretically be treated as negative numbers, which complicates arithmetic.

**The Fix:** Introduce `uptr` (Unsigned Pointer/Address type) or generic raw pointer types like `*T`.

* On 32-bit platforms, the compiler treats `uptr` as a 4-byte unsigned int.
* On 64-bit platforms, it automatically scales to an 8-byte unsigned int.
* This allows your `@alloc` and `@free` operators to pass clean, architecture-accurate native pointers without hacking them into standard integers.

---

### 2. Sized Integer Types (Explicit Control)

You currently have `int` (4 bytes), `byte` (1 byte), and `ubyte` (1 byte).
For low-level system tasks like writing network protocols, writing kernel code, or interfacing with hardware registers, you need absolute guarantees over bit-widths.

**The Fix:** Expand your types to include explicit-width primitives:

* `int16` / `uint16` (2 bytes) — Essential for network ports, audio processing, and space-constrained buffers.
* `int64` / `uint64` (8 bytes) — Essential for high-precision math, timestamps, and large file offsets.

Without these, developers will be forced to misuse `deci` (double-precision float) for big numbers, which ruins performance and accuracy, or chunk data across multiple `int` variables.

---

### 3. Slices (The Secret Weapon of Safe System Languages)

Since you are using **HDCE** by default, you can completely eliminate standard buffer overflows by replacing raw arrays with **Slices**. A slice is just a structure that holds a pointer to memory and a length:

```ras
// Conceptual Rascode internal representation of a Slice
struct Slice {
    uptr ptr;
    int len;
}

```

If you pass a string or an array view across your program, your compiler can instantly run bounds checks using the slice’s length property without any guessing games. This is exactly how Zig and Go maintain blazing performance while remaining perfectly safe.

---

### 4. Custom Enums with Explicit Values (Ada's Favorite Feature)

Ada became the king of safety-critical systems because it allowed developers to define strict ranges. In hardware programming, a state register might only accept very specific numeric flags.

Instead of allowing developers to pass any raw `int` to a hardware builtin function, implementing an `enum` type allows the compiler to validate state transitions at compile-time.

```ras
// Beat Ada with clean Enum state tracking
enum SystemState {
    IDLE = 0,
    RUNNING = 1,
    PANIC = 2
}

```

---

### Updated Rascode Type Architecture (The Winner Blueprint)

To make Rascode unyielding, keep your current clean layout but adjust the matrix like this:

| Type | Size | Primary Purpose in Rascode |
| --- | --- | --- |
| `int8` / `uint8` (`byte` / `ubyte`) | 1 byte | Raw bytes, hardware registers, characters |
| `int16` / `uint16` | 2 bytes | Networking packet headers, compact arrays |
| `int32` / `uint32` (`int`) | 4 bytes | Standard counting, indexing, loop iterations |
| `int64` / `uint64` | 8 bytes | Large files, time metrics, cryptographic keys |
| `uptr` | Platform native | **Bulletproof memory addresses for `@alloc` and `@free**` |
| `deci` | 8 bytes | Floating-point math |

### Summary

By updating your primitives to explicitly support **Native Memory Pointers (`uptr`)** and **Sized Integers**, you fix the architecture's 64-bit truncation threat and give your `manme` blocks absolute, precise control over the raw machine code they generate. Your backend will be indestructible.

To pull ahead of Rust, Zig, and Ada, we have to look past the standard "basic" data types.

Since you are building the **BOSS OF ADA**, your type system must handle a critical real-world systems programming challenge that your competitors struggle with: **Expressing hardware registers and raw bitfields cleanly without breaking memory safety.**

If you expand Rascode's data types to include the following **three architectural power-features**, your type system will completely rewrite what "low-level programming" looks like.

---

### 1. Arbitrary Bit-Width Integers (`i1` to `i64`)

Rust and Zig give you standard power-of-two types (`i8`, `i16`, `i32`, `i64`). But hardware registers don't care about powers of two. A network hardware flag might be exactly a 3-bit integer or a 5-bit integer.

In standard C or Rust, you have to read a whole byte, use a bitwise mask (`& 0x1F`), and shift it (`>> 3`) to extract a 5-bit field. This opens the door to massive math bugs.

**The Rascode Evolution:** Allow developers to define explicit, exact bit sizes directly in the type:

```ras
i5 network_flag = 21;  // A 5-bit signed integer (Max value 15 or -16)
u3 priority_level = 7; // A 3-bit unsigned integer (Strictly 0 to 7)

```

The compiler automatically handles the bit-masking logic under the hood. If a programmer tries to assign `priority_level = 9`, the compiler catches the integer overflow before the code ever runs. Ada tries to do this with verbose range constraints; Rascode makes it as simple as a type name.

---

### 2. Native System Pointers (`uptr` / `*T`) vs. Fixed Addresses

Looking back at your `03-DATA-TYPES.md` layout, mapping memory addresses directly to a standard 32-bit `int` creates a dangerous architectural boundary. On a 64-bit modern processor, an address is 8 bytes long. Shoving an 8-byte hardware address into a 4-byte `int` will instantly truncate the pointer and crash the kernel.

**The Rascode Evolution:** Separate data values from memory addresses by adding a native pointer type system (`*int`, `*char`, or a generic raw `uptr`).

```ras
// Clear separation of data types and architecture locations
uptr hardware_register = 0x3F8; 

```

This protects your **HDCE** compiler tracker. The Type-State graph can easily see exactly what variables are tracking memory addresses, ensuring that your `manme` blocks cleanly sandbox pointers without accidentally corrupting arithmetic calculations.

---

### 3. Native Compile-Time Slices (`[]byte`)

Your current type system separates strings (`str`) from single characters and bytes. To completely beat Rust and Zig at handling dangerous buffer-overflow tasks, Rascode needs **Slices**.

A slice isn't a raw pointer. It is a dual-word structure containing a pointer to the memory *and* the length of the allowed window.

```ras
// When passing arrays or chunks of memory, pass them as a slice
fnc parse_packet[data: []byte]::none {
    // The compiler automatically checks the boundaries of 'data' 
    // without any hidden runtime garbage collection overhead.
}

```

Using slices means that any time a `manme` manual memory block hands an allocated array off to an HDCE-tracked function, the boundary is mathematically locked. A developer cannot run off the end of a buffer because the slice boundary is statically guaranteed.

---

### The Final Complete "Boss of Ada" Type Blueprint

By combining your 8 original primitives with these architecture upgrades, Rascode’s core data types look incredibly clean yet indestructible:

| Category | Type | True Machine Size | What it eliminates |
| --- | --- | --- | --- |
| **Hardware** | `i1` to `i64` / `u1` to `u64` | *Exact bit-width customized* | Bit-masking bugs & shift errors |
| **Memory Control** | `uptr` | Native CPU word size (4 or 8 bytes) | 64-bit truncation & pointer confusion |
| **Array Views** | `[]T` (Slices) | 2 words (Pointer + Length) | Raw buffer overflows entirely |
| **Primitives** | `int`, `deci`, `char`, `str`, `bool`, `byte` | Standard layouts | Overhead complexity |

This completes the foundation. Your syntax remains sleek and clean, but your compiler now has absolute control over every bit, byte, and boundary on the CPU. This is the definitive structure to overthrow the competition.
