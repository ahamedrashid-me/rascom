# 14: String Interpolation

## Overview

String interpolation allows embedding expressions directly in string literals using `$variable` or `${expression}` syntax.

Internally, the parser converts interpolated strings into multiple `show` statements (from parser.c parse_interpolated_string).

## Simple Interpolation: $variable

**Syntax:**
```
"... $variable_name ..."
```

**Example:**
```ras
str name = "Alice";
show["Hello $name"];        // Outputs: "Hello Alice"
```

**Multiple variables:**
```ras
str first = "Alice";
str last = "Smith";
show["$first $last"];       // Outputs: "Alice Smith"
```

**Numbers:**
```ras
int age = 30;
show["Age: $age"];          // Outputs: "Age: 30"
```

## Complex Interpolation: ${expression}

**Syntax:**
```
"... ${expression} ..."
```

Allows arbitrary expressions, not just variables:

```ras
int x = 5;
int y = 3;
show["Sum: ${x + y}"];      // Outputs: "Sum: 8"
```

**Method calls (builtins):**
```ras
str text = "hello";
show["Length: ${@len[text]}"];  // Outputs: "Length: 5"
```

**Comparisons:**
```ras
int score = 85;
show["Pass: ${score >= 70}"];  // Outputs: "Pass: true"
```

**Complex expressions:**
```ras
int radius = 10;
deci pi = 3.14;
show["Area: ${pi * 10 * 10}"];  // Outputs: "Area: 314.00" (approx)
```

## Mixing Literal and Interpolation

```ras
str name = "Bob";
int score = 92;

show["Hello $name, your score is ${score}"];
// Outputs: "Hello Bob, your score is 92"
```

Mixed with literal text:
```ras
show["The value of 2+2 is ${2+2}"];  // Outputs: "The value of 2+2 is 4"
```

## How Interpolation Works

The lexer/parser recognizes interpolation markers and expands them into separate `show` statements:

**Input:**
```ras
show["Hello $name"];
```

**Expanded to (internally):**
```ras
show["Hello "];
show[name];
```

Or:

**Input:**
```ras
show["Hello $name, score: ${score}"];
```

**Expanded to (internally):**
```ras
show["Hello "];
show[name];
show[", score: "];
show[score];
```

This means:
- No special show syntax needed
- Interpolation works with any `show` statement only
- Cannot use in regular string assignments

## Interpolation Only in show[]

Interpolation ONLY works in `show[...]` statements:

```ras
show["Hello $name"];           // ✓ Works - interpolated
str greeting = "Hello $name";  // Might NOT interpolate - just string literal
```

If you need interpolation in regular strings, build with `@concat`:

```ras
str name = "Alice";
str greeting = @concat["Hello ", name];  // Explicit concatenation
```

## Variables in Interpolation

The `$variable_name` form:
- Simple variable names only
- No array indexing: `$arr{0}` probably doesn't work
- No member access: `$person.name` probably doesn't work

```ras
str name = "Alice";
show[$name];                // ✓ Simple variable

int x = 5;
show[arr{x}];               // Need full syntax show[arr{x}]
show["Value: ${arr{x}}"];   // Use ${} for expressions
```

## Escape Sequences

Strings support standard escape sequences (from lexer.c):

```ras
show["Line 1\nLine 2"];        // \n = newline
show["Tab\there"];             // \t = tab
show["Quote: \"Hello\""];      // \" = quote
show["Backslash: \\"];         // \\ = backslash
```

Combining with interpolation:

```ras
str name = "Alice";
show["Name:\t$name\n"];        // Tab and newline with interpolation
```

## Performance

Each interpolated string becomes multiple `show` statements. Performance is similar to:

```ras
show["Hello "];
show[name];
show["!"];
```

No hidden overhead beyond outputting multiple times.

## Limitations

**No nested interpolation:**
```ras
str var = "test";
show["Value: ${\"nested $var\"}"];  // Doesn't work
```

**No interpolation in non-show strings:**
```ras
str greeting = "Hello $name";  // Probably just literal string
```

Use functions in non-show contexts:

```ras
str name = "Alice";
str greeting;

show["Hello $name"];           // Interpolated output
greeting = @concat["Hello ", name];  // Explicit concat
```

**Escaping $ in strings:**
```ras
show["Price: $100"];           // Might try to interpolate $100 (error)
show["Price: \\$100"];         // Escape the $ ?
show["Price: "];               // Workaround: split into parts
show["$"];
show["100"];
```

To output literal `$`:
```ras
show["Literal dollar: "];
show["$"];
```

## Use Cases

**Debug output:**
```ras
int debug_level = 2;
show["Debug: Level ${debug_level}"];
```

**Formatted messages:**
```ras
str user = "bob";
int count = 42;
show["User $user has $count items"];
```

**Calculations in output:**
```ras
int width = 10;
int height = 20;
show["Area: ${width * height}"];
```

**Status displays:**
```ras
bool running = true;
show["Server running: ${running}"];
```

## showf[] - Interpolation with Automatic Newline & Expression Support

For convenience, use `showf[...]` for interpolated strings with automatic newlines and full expression support:

```ras
str name = "Alice";
int age = 30;

// Simple variables
showf["Name: $name"];

// Expressions
showf["Next year age: ${age + 1}"];

// Mix of both
showf["$name will be ${age + 1} next year"];
```

**Expression Support in showf:**
- Arithmetic: `${x + y}`, `${a * b}`, `${x / y}`
- Comparisons: `${x < 10}`, `${x == y}`
- Functions: `${@len[text]}`, `${@strlen[s]}`
- Complex: `${(x + y) * 2}`, `${@len[name] + 5}`

See [15-IO-STATEMENTS.md](15-IO-STATEMENTS.md) for full `showf[]` documentation.

## Related Documentation
- [15-IO-STATEMENTS.md](15-IO-STATEMENTS.md) — show[], showf[], and read[] statements
- [02-SYNTAX-RULES.md](02-SYNTAX-RULES.md) — String operations
- [03-DATA-TYPES.md](03-DATA-TYPES.md) — Primitive types in strings
