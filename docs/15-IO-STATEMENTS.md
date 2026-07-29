# 15: I/O Statements

## show Statement (Output)

**Syntax** (from parser.c parse_show):

```
show[expression];
```

Outputs text/values to stdout.

**Basic Examples:**
```ras
show["Hello World"];
show[42];
show[3.14];
show[true];
```

**Multiple shows:**
```ras
show["First"];
show["Second"];
show["Third"];
// Output: FirstSecondThird (no spaces)
```

## Output Newlines

By default, `show` does NOT add newlines. To add newlines:

```ras
show["Hello"];
show["\n"];         // Explicit newline

// Or
show["Hello\n"];    // Newline in string
```

From the lexer, `\n` escape sequence exists for newlines.

## String Interpolation in show

See [14-STRING-INTERPOLATION.md](14-STRING-INTERPOLATION.md)

```ras
str name = "Alice";
int age = 30;

show["Name: $name"];            // Simple interpolation
show["Age: ${age + 1}"];        // Expression interpolation
```

## Outputting Variables

**Integers:**
```ras
int x = 42;
show[x];                        // Outputs: 42
```

**Decimals:**
```ras
deci pi = 3.14159;
show[pi];                       // Outputs: 3.14159
```

**Booleans:**
```ras
bool flag = true;
show[flag];                     // Outputs: true
show[false];                    // Outputs: false
```

**Characters:**
```ras
char c = 'A';
show[c];                        // Outputs: A (not 65)
```

**Strings:**
```ras
str msg = "Hello";
show[msg];                      // Outputs: Hello
```

## Array Elements

```ras
arr{int, 5} numbers = {10, 20, 30, 40, 50};

show[numbers{0}];              // Outputs: 10
show[numbers{2}];              // Outputs: 30
```

Loop and display:

```ras
loop[int i = 0; i < 5; i++] {
    show[numbers{i}];
}
```

## Map Values

```ras
map{str, int} scores;
scores->set["Alice", 95];

show[scores->get["Alice"]];     // Outputs: 95
```

## Group Members

```ras
group Point {
    int x;
    int y;
}

fnc main[]::int {
    Point p;
    p.x = 15;
    p.y = 25;
    
    show[p.x];                  // Outputs: 15
    show[p.y];                  // Outputs: 25
    get[0];
}
```

## Function Results

```ras
fnc multiply[a, b]::int {
    get[a * b];
}

show[multiply[6, 7]];           // Outputs: 42
```

## Builtin Functions in show

```ras
show[@len["hello"]];            // Outputs: 5
show[@type[42]::str];           // Outputs: "42"
@concat["hello", " world"];     // Result: "hello world"
```

## showf Statement (Formatted Output with Newline)

**Syntax:**

```
showf[string_with_interpolation];
```

Outputs text with automatic newline appended. Supports PHP-like variable interpolation.

**Key Features:**
- Automatically adds newline `\n` at end (unlike `show[]`)
- No need for explicit `\n` in the string
- Supports `$variable` syntax for simple interpolation
- Variables must be in scope when `showf` is called

**Basic Examples:**
```ras
showf["Hello World"];           // Outputs: Hello World\n

int x = 42;
showf["The answer is $x"];      // Outputs: The answer is 42\n

str name = "Alice";
int age = 30;
showf["Name: $name, Age: $age"]; // Outputs: Name: Alice, Age: 30\n
```

**Difference from show[]:**

| Feature | show[] | showf[] |
|---------|--------|---------|
| Auto newline | No | Yes |
| Syntax | `show["text\n"]` | `showf["text"]` |
| Interpolation | Not supported | `$variable` syntax |
| Variables | Output via comma | `$variable` in string |

**Examples - show vs showf:**

```ras
// Using show (requires explicit newline)
show["Hello"];
show["\n"];                     // Outputs: Hello\n

// Using showf (automatic newline)
showf["Hello"];                 // Outputs: Hello\n

// Multiple variables with show
show["x="];
show[x];
show[", y="];
show[y];
show["\n"];

// Multiple variables with showf
showf["x=$x, y=$y"];            // Cleaner!
```

**Supported Variable Types:**
```ras
int count = 42;
showf["Integer: $count"];       // Outputs: Integer: 42\n

deci ratio = 3.14;
showf["Float: $ratio"];         // Outputs: Float: 3.14\n

str message = "Success";
showf["Status: $message"];      // Outputs: Status: Success\n

bool flag = true;
showf["Active: $flag"];         // Outputs: Active: true\n

char letter = 'A';
showf["Letter: $letter"];       // Outputs: Letter: A\n
```

**Multiple Variables in One showf:**
```ras
int x = 10;
int y = 20;
str operation = "addition";

showf["$operation: $x + $y"];   // Outputs: addition: 10 + 20\n
```

**Note:** Complex expressions like `${x + y}` are now fully supported! ✅

**Examples - Expression Support:**
```ras
int a = 10, b = 5;
showf["Sum: ${a + b}"];          // Outputs: Sum: 15\n
showf["Product: ${a * b}"];       // Outputs: Product: 50\n
showf["Length: ${@len[\"test\"]}"];// Outputs: Length: 4\n
showf["Complex: ${(a + b) * 2}"]; // Outputs: Complex: 30\n
```

Supported in expressions:
- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Comparisons: `<`, `>`, `==`, `!=`, `<=`, `>=`
- Logical: `&&`, `||`, `!`
- Function calls: `@len[]`, `@strlen[]`, etc.
- Nested parentheses: `${(x + y) * (a - b)}`

## read Statement (Input)

**Syntax:**

```
read[variable];                 // Statement form (old style)
type var = read["prompt"];      // Expression form (new style)
```

Read input from stdin into a variable or expression result.

## Statement Form (Traditional)

Read into an existing variable:

```ras
str name;
int age;
char initial;

read[name];                     // Read string input
read[age];                      // Read integer input
read[initial];                  // Read single character
```

No prompt displayed. Input is read from stdin directly.

## Expression Form (Recommended)

Read with inline variable assignment and optional prompt:

```ras
str name = read["Enter your name: "];
int age = read["Enter your age: "];
char first_char = read["Enter a character: "];
```

The prompt string is displayed to stdout before reading. Very useful for interactive programs.

## Supported Data Types in read[]

The `read[]` expression form supports all RasCode data types:

**String:**
```ras
str text = read["Enter text: "];
showf["You entered: $text"];
```
- Reads until newline (newline is stripped)
- Stores result in static buffer
- Max 255 characters

**Integer:**
```ras
int number = read["Enter a number: "];
showf["You got: $number"];
```
- Parses ASCII digits
- Supports negative numbers
- Stops at first non-digit

**Character:**
```ras
char ch = read["Enter one character: "];
showf["Character: $ch"];
```
- Reads single byte from stdin
- Ignores following newline

**Boolean:**
```ras
bool flag = read["Enter true/false (1/0): "];
showf["Flag value: $flag"];
```
- Parses "true", "false", "1", "0"
- Case-insensitive for first char: t/T/1 = true, else false

**Decimal (Float):**
```ras
deci value = read["Enter a decimal: "];
showf["Value: $value"];
```
- Parses as integer, converts to double
- Decimal point input ignored (parsed as int only)

**Byte/Unsigned Byte:**
```ras
byte signed_val = read["Enter byte value: "];
ubyte unsigned_val = read["Enter unsigned byte: "];
showf["Byte: $signed_val, Ubyte: $unsigned_val"];
```
- Parses integer, masked to 8-bit range
- byte: -128 to 127
- ubyte: 0 to 255

## Examples by Use Case

**Simple menu prompt:**
```ras
showf["1. Add"];
showf["2. Subtract"];
showf["3. Exit"];

int choice = read["Select option: "];
```

**Interactive calculator:**
```ras
int x = read["First number: "];
int y = read["Second number: "];
int sum = x + y;
showf["Sum: $sum"];
```

**Data entry form:**
```ras
str name = read["Name: "];
int age = read["Age: "];
char grade = read["Grade: "];

showf["Recorded: $name, age $age, grade $grade"];
```

**Type conversion in chain:**
```ras
str input = read["Enter value: "];
// Now input is a string - no automatic type conversion
// Use it directly or convert with @type[] if needed
```

## Input from Files

For file input, see [19-FILE-NETWORK.md](19-FILE-NETWORK.md):

```ras
int fd = @fopen["input.txt", "r"];
int bytes = @fread[fd, buffer, 100];
@fclose[fd];
```

## Output Formatting

No printf-like formatting built-in. Concatenate strings:

```ras
int value = 42;
str formatted = @concat["Value: ", @type[value]::str];
show[formatted];                // Outputs: Value: 42
```

Or show in parts:

```ras
show["Value: "];
show[value];
```

## Buffering

No explicit flush statement visible in parser. Output is typically line-buffered or fully buffered.

For critical output, might need empty `show["\n"]` to flush:

```ras
show["Progress: "];
show[progress];
show["\n"];
```

## Error Output

No stderr in basic RASLang. All output goes to stdout. For errors, print to stdout or use file operations (see 19-FILE-NETWORK.md).

## Common Patterns

**Line output:**
```ras
show["Line 1\n"];
show["Line 2\n"];
show["Line 3\n"];
```

**Formatted output:**
```ras
str name = "Alice";
int score = 95;

show["Name: $name, Score: $score\n"];
```

**Debug trace:**
```ras
const DEBUG = true;

fnc log[msg]::int {
    if[DEBUG] {
        show["[LOG] $msg\n"];
    }
    get[0];
}

log["Starting process"];
log["Step 1 complete"];
log["Done"];
```

**Progress display:**
```ras
loop[int i = 1; i <= 100; i++] {
    if[i % 10 == 0] {
        show["Progress: $i%\n"];
    }
}
```

## Related Documentation
- [14-STRING-INTERPOLATION.md](14-STRING-INTERPOLATION.md) — String interpolation in show
- [02-SYNTAX-RULES.md](02-SYNTAX-RULES.md) — Escape sequences
- [19-FILE-NETWORK.md](19-FILE-NETWORK.md) — File and network I/O
- [16-BUILTINS.md](16-BUILTINS.md) — I/O related builtins
