# 06: Operators

## Operator Precedence (Lowest to Highest)

**From parser.c operator precedence functions (14 levels):**

| Level | Operators | Associativity | Example |
|-------|-----------|---------------|---------|
| 1 | `?:` (Ternary) | Right | `cond ? true_val : false_val` |
| 2 | `\|\|`, `xor` (Logical OR) | Left | `a \|\| b`, `a xor b` |
| 3 | `&&`, `and` (Logical AND) | Left | `a && b`, `a and b` |
| 4 | `\|` (Bitwise OR) | Left | `a \| b` |
| 5 | `^` (Bitwise XOR) | Left | `a ^ b` |
| 6 | `&` (Bitwise AND) | Left | `a & b` |
| 7 | `==`, `!=` (Equality) | Left | `a == b`, `a != b` |
| 8 | `<`, `>`, `<=`, `>=` (Comparison) | Left | `a < b`, `a >= b` |
| 9 | `<<`, `>>` (Shift) | Left | `a << 2`, `a >> 1` |
| 10 | `+`, `-` (Additive) | Left | `a + b`, `a - b` |
| 11 | `*`, `/`, `%` (Multiplicative) | Left | `a * b`, `a / b`, `a % b` |
| 12 | Unary Prefix | Right | `+x`, `-x`, `!x`, `~x`, `++x`, `--x` |
| 13 | Unary Postfix | Right | `x++`, `x--` |
| 14 | Primary | N/A | Literals, identifiers, `( )`, function calls |

## Expression Evaluation

Higher precedence = tighter binding:

```ras
int a = 2 + 3 * 4;              // 2 + (3 * 4) = 14, not (2 + 3) * 4
bool b = true && false || true;  // (true && false) || true = true
```

Use parentheses to override precedence:

```ras
int x = (2 + 3) * 4;            // = 20
bool y = true && (false || true); // = true
```

## Operator Details

### Ternary Conditional (Level 1)

```
condition ? value_if_true : value_if_false
```

**Right-associative:** Can chain

```ras
int max = x > y ? x : y;
int grade = score >= 90 ? 'A' : 
            score >= 80 ? 'B' :
            score >= 70 ? 'C' : 'F';
```

Returns type of true/false branch (must match).

### Logical OR (Level 2)

**Operators:** `||` or `xor`

```ras
bool a = true || false;         // true (short-circuit: stops at true)
bool b = true xor false;        // true (XOR: true if different)
bool c = true xor true;         // false (both same)
```

**Short-circuit evaluation:** `||` stops evaluating if left side is true:
```ras
if[x > 0 || expensive_function[]] {  // expensive_function not called if x > 0
    ...
}
```

### Logical AND (Level 3)

**Operators:** `&&` or `and`

```ras
bool a = true && false;         // false (short-circuit: stops at false)
bool b = true and false;        // false (keyword form)
```

**Short-circuit evaluation:** `&&` stops if left side is false:
```ras
if[x > 0 && arr{x} == 5] {      // arr{x} not accessed if x <= 0
    ...
}
```

### Bitwise Operators (Levels 4-6)

**OR:** `a | b` (Sets bit if either operand has it)
```ras
int a = 0b1010;  // 10
int b = 0b0101;  // 5
int c = a | b;   // 0b1111 = 15
```

**XOR:** `a ^ b` (Sets bit if operands differ)
```ras
int a = 0b1010;  // 10
int b = 0b0101;  // 5
int c = a ^ b;   // 0b1111 = 15

int d = 0b1010   // 10
int e = d ^ d;   // 0 (XOR with self)
```

**AND:** `a & b` (Sets bit if both operands have it)
```ras
int a = 0b1010;  // 10
int b = 0b1100;  // 12
int c = a & b;   // 0b1000 = 8
```

**NOT:** `~x` (Inverts all bits)
```ras
int a = 0b0101;  // 5
int b = ~a;      // 0b...11111010 (two's complement)
```

**Uses:**
- Bitwise operations on integer values
- Flag manipulation
- Bit masking

### Equality Operators (Level 7)

**`==`** (Equal):
```ras
int a = 5;
int b = 5;
bool same = a == b;             // true
```

**`!=`** (Not equal):
```ras
str x = "hello";
str y = "world";
bool diff = x != y;             // true
```

Works on all types:
```ras
deci p1 = 3.14;
deci p2 = 3.14;
bool equal = p1 == p2;          // true

bool flag = true;
bool other = false;
bool not_eq = flag != other;    // true
```

### Comparison Operators (Level 8)

**`<`** (Less than):
```ras
int a = 5;
int b = 10;
bool less = a < b;              // true
```

**`>`** (Greater than):
```ras
int x = 20;
int y = 15;
bool greater = x > y;           // true
```

**`<=`** (Less than or equal):
```ras
int m = 5;
int n = 5;
bool lte = m <= n;              // true
```

**`>=`** (Greater than or equal):
```ras
int p = 10;
int q = 5;
bool gte = p >= q;              // true
```

Work on numeric types (`int`, `deci`):
```ras
deci temp1 = 98.6;
deci temp2 = 99.5;
if[temp1 < temp2] { show["Rising"]; }
```

String comparison: `@strcmp[s1, s2]` builtin instead

### Shift Operators (Level 9)

**`<<`** (Left shift): Multiply by power of 2
```ras
int x = 5;          // 0b0101
int shifted = x << 2; // 0b10100 = 20
```

**`>>`** (Right shift): Divide by power of 2 (arithmetic)
```ras
int y = 20;         // 0b10100
int shifted = y >> 2; // 0b00101 = 5
```

Commonly used for:
- Efficient multiplication/division by 2^n
- Bit field manipulation

### Additive Operators (Level 10)

**`+`** (Addition):
```ras
int sum = 5 + 3;                // 8
deci total = 1.5 + 2.5;         // 4.0
str concat = @concat["ab", "cd"]; // "abcd" (use builtin for strings)
```

**`-`** (Subtraction):
```ras
int diff = 10 - 3;              // 7
deci result = 5.5 - 2.5;        // 3.0
```

### Multiplicative Operators (Level 11)

**`*`** (Multiplication):
```ras
int prod = 3 * 4;               // 12
deci area = 3.5 * 2.0;          // 7.0
```

**`/`** (Division):
```ras
int quot = 10 / 3;              // 3 (integer division)
deci precise = 10.0 / 3.0;      // 3.333...
```

**Beware integer division:**
```ras
int result = 7 / 2;             // 3, not 3.5
deci correct = @type[7]::deci / 2.0;  // 3.5
```

**`%`** (Modulo/Remainder):
```ras
int remainder = 10 % 3;         // 1
int is_even = x % 2 == 0;       // true if x is even
```

### Unary Prefix Operators (Level 12)

**Prefix `+`** (Unary plus - usually no-op):
```ras
int x = +5;                     // 5
```

**Prefix `-`** (Negation):
```ras
int x = 5;
int neg = -x;                   // -5
```

**Prefix `!`** (Logical NOT):
```ras
bool flag = true;
bool opposite = !flag;          // false
```

**Prefix `~`** (Bitwise NOT):
```ras
int x = 0b0101;
int inverted = ~x;              // 0b...11111010
```

**Prefix `++`** (Pre-increment):
```ras
int x = 5;
int y = ++x;                    // x becomes 6, y = 6
```

**Prefix `--`** (Pre-decrement):
```ras
int x = 5;
int y = --x;                    // x becomes 4, y = 4
```

### Unary Postfix Operators (Level 13)

**Postfix `++`** (Post-increment):
```ras
int x = 5;
int y = x++;                    // y = 5, x becomes 6
```

**Postfix `--`** (Post-decrement):
```ras
int x = 5;
int y = x--;                    // y = 5, x becomes 4
```

### Primary Expressions (Level 14)

**Literals:**
```ras
int num = 42;
deci decimal = 3.14;
str text = "hello";
char ch = 'X';
bool flag = true;
```

**Identifiers:**
```ras
int x = value;                  // Variable reference
```

**Function calls:**
```ras
show[message];                  // Call function show
int result = add[5, 3];         // Call function add
```

**Parenthesized expressions:**
```ras
int x = (a + b) * c;
```

**Array access:**
```ras
arr{int, 10} arr;
int first = arr{0};
```

**Member access:**
```ras
Person p;
str name = p.name;
```

**Map operations:**
```ras
map{str, int} ages;
int age = ages->get["Bob"];
```

**Builtin calls:**
```ras
int len = @len["string"];
str upper = @concat["hello", " world"];
```

## Operator Limitations

**No shorthand operators:**
```ras
x += 5;             // ERROR: Use x = x + 5
x -= 3;             // ERROR: Use x = x - 3
x *= 2;             // ERROR: Use x = x * 2
```

**No assignment in expressions:**
```ras
if[(x = 5) > 0] {   // Probably ERROR
    ...
}
```

**No comma operator:**
```ras
for(i = 0, j = 10; ...)  // Not a thing in RASLang
```

**No typeof operator:**
```ras
if[typeof(x) == "int"] { }  // Use @type[x]::str instead
```

## Type Semantics

**Operators preserve types:**
```ras
int x = 5;
int y = 3;
int z = x + y;          // Result is int

deci a = 5.0;
deci b = 3.0;
deci c = a + b;         // Result is deci
```

**Mixed type operations:** Usually require explicit conversion
```ras
int i = 5;
deci d = 3.0;
deci result = @type[i]::deci + d;  // Convert int to deci first
```

## Related Documentation
- [04-VARIABLES.md](04-VARIABLES.md) — Using operators with variables
- [11-LOOPS.md](11-LOOPS.md) — Increment/decrement in loops
- [10-CONDITIONALS.md](10-CONDITIONALS.md) — Logical operators in conditions
