# RasCode - Actual Capabilities (Source Code Audit)

**Date:** April 10, 2026 (Updated)
**Last Updated:** Added `else` keyword and `emit` keyword with cycle-as-expression feature
**Method:** Direct source code analysis (lexer.c, parser.c, builtins.c, ast.h)  
**No documentation consulted** - only actual implementation

---

## Recent Updates (April 10, 2026)

✨ **New Features Added:**
- `else` keyword: Alias for final unconditional `or` in if-chains (clearer syntax)
- `emit` keyword: Set cycle expression value without exiting function
- **Cycle-as-expression**: Cycles can now be used as value sources with `emit` keyword

---

## 1. Core Language

### Data Types (8 Built-in Types)
- `int` - 64-bit signed integer
- `deci` - Floating-point (double precision)
- `char` - Single character
- `str` - String
- `bool` - Boolean (true/false)
- `byte` - 8-bit signed integer
- `ubyte` - 8-bit unsigned integer  
- `none` - Null/nil value

### Operators

**Arithmetic** (11 total)
- Binary: `+`, `-`, `*`, `/`, `%`
- Unary: `+`, `-`
- Inc/Dec: `++`, `--` (prefix and postfix)
- Ternary: `? :`

**Comparison** (6 total)
- `<`, `>`, `==`, `!=`, `<=`, `>=`

**Bitwise** (6 total)
- Bitwise AND: `&`
- Bitwise OR: `|`
- Bitwise XOR: `^`
- Bitwise NOT: `~`
- Left shift: `<<`
- Right shift: `>>`

**Logical** (5 total)
- Logical AND: `&&` and `and` keyword
- Logical OR: `||` and `or` keyword
- Logical XOR: `xor` keyword
- Logical NOT: `!` and `not` keyword

**Assignment** (11 variants)
- `=` basic assignment
- `+=`, `-=`, `*=`, `/=`, `%=` arithmetic assignment
- `&=`, `|=`, `^=` bitwise assignment
- `<<=`, `>>=` shift assignment

**Operator Precedence** (13 levels, correctly implemented in recursive descent parser)
1. Ternary `?:`
2. Logical OR `||`, `xor`
3. Logical AND `&&`, `and`
4. Bitwise OR `|`
5. Bitwise XOR `^`
6. Bitwise AND `&`
7. Equality `==`, `!=`
8. Comparison `<`, `>`, `<=`, `>=`
9. Shift `<<`, `>>`
10. Additive `+`, `-`
11. Multiplicative `*`, `/`, `%`
12. Unary prefix/postfix `+x`, `-x`, `!x`, `~x`, `++x`, `--x`, `x++`, `x--`
13. Primary (literals, identifiers, members, etc.)

---

## 2. Control Flow Structures

### If Statements  
```ras
if[condition] {
    // then
} or[condition] {
    // else-if (can chain multiple)
} else {
    // else (final, no condition)
}
```

**Note:** Final else can also be written as `or { ... }` (both forms equivalent)

✅ Fully supported with arbitrary nesting

### While Loops
```ras
while[condition] {
    // body
}
```
✅ Standard while implementation

### For Loops (called `loop`)
```ras
loop[init; condition; increment] {
    // body
}
```
✅ C-style with full expression support

### Switch Statements (called `cycle`)

**As Statement:**
```ras
cycle[value] {
    when[case1]: { /* ... */ }
    when[case2]: { /* ... */ }
    fixed: { /* default */ }
}
```

**As Expression (NEW):**
```ras
int grade = cycle[score] {
    when[90]: { emit[1]; }
    when[80]: { emit[2]; }
    fixed: { emit[0]; }
};
```

Each branch must use `emit[value]` to set the cycle's result value (doesn't exit function like `get[]` does).

✅ Multiple cases with default
✅ Can be used in expressions (as of Apr 2026 update)
✅ Cycle-as-expression feature with `emit` keyword

### Try-Catch (called `check`/`when`)
```ras
check {
    // try block
} when[error_type]: {
    // catch handler (can chain multiple)
}
```
✅ Error handling pattern supported

### Loop Control
- `break;` - Exit loop ✅
- `continue;` - Skip to next iteration ✅

### Deferred Execution
```ras
defer statement;  // Executes at scope exit
```
✅ RAII-like cleanup supported

### Cycle Expression Value Setting (NEW - Apr 2026)
```ras
emit[value];  // Sets the value of enclosing cycle expression
```
- Used inside cycle blocks to provide result value
- Alternative to `get[value]` which exits function
- Enables cycle-as-expression feature ✅

---

## 3. Data Structures

### Arrays (Fixed-Size)
```ras
arr{int, 5} numbers;                    // Empty
arr{int, 5} numbers = {1,2,3,4,5};      // Initialized
arr{str, 3} strings = {"a", "b", "c"};  // Any type
```
- Access: `numbers{0}`, `numbers{i}`
- Supports nested: `arr{arr{int, 3}, 2}`
- Supported in groups: group can contain arrays
- Array elements can be groups ✅
- Can pass `arr{type, size}` as function parameter ✅

### Maps (Key-Value)
```ras
map{str, int} scores;
scores->set["Alice", 95];
int val = scores->get["Alice"];
bool has = scores->has["Alice"];
scores->remove["Alice"];
```
- All operations fully supported ✅
- Key types: int, str, deci, char, byte, ubyte, bool
- Value types: same

### Groups (Structs)
```ras
group Person {
    str name;
    int age;
    char initial;
}

Person p;
p.name = "Alice";
str x = p.name;
```
- Member access: `var.member` ✅
- Member assignment: `var.member = value;` ✅
- Nested members: `p.member.submember` (chaining) ✅
- Array of groups: `arr{Group, 10} items;` ✅
- Groups with arrays: `group Data { arr{int, 5} nums; }` ✅
- Access array in group: `p.array{0}` and `p.array{i}.member` ✅

---

## 4. Functions

### Declaration
```ras
fnc name[param1, param2]::returnType {
    get[returnValue];
}
```

### Parameters
- Basic types: `fnc foo[int x, str name]`
- Array parameters: `fnc process[arr{int, 5} data]`
- Mixed: `fnc mixed[int x, arr{str, 3} items, bool flag]`
- Can have any number of parameters

### Return
- `get[value];` returns from function
- Works as early return
- Can return any supported type

### Function Calls
- As expression: `int result = add[2, 3];`
- As statement: `process[data];`
- Argument evaluation works correctly

---

## 5. I/O Operations

### Output - `show[]` (No Auto Newline)
```ras
show["text"];           // Output: text
show[variable];         // Output variable value
show[42 + 8];          // Output expression result
show["\n"];            // Explicit newline required
```

### Output - `showf[]` (Auto Newline)
```ras
showf["simple text"];           // Auto \n at end
showf["value: $x"];             // Variable interpolation
showf["sum: ${x + y}"];         // Expression interpolation
showf["complex: ${(a+b)*2}"];   // Nested expressions
```

**Interpolation Features:**
- Simple variables: `$name`, `$count`
- Expressions: `${expr}`
- Arithmetic in expressions: `${x + y * 2}`
- Comparisons: `${x > 5}`
- Function calls: `${@len[str]}`
- Nested expressions with parentheses: `${(a + b) * (c - d)}`
- Automatic newline added

### Input - `read[]` (Statement Form)
```ras
int x;
str input;
read[x];      // Read into variable (no prompt)
read[input];
```

### Input - `read[]` (Expression Form)
```ras
int num = read["Enter number: "];
str text = read["Enter text: "];
char ch = read["Enter character: "];
bool flag = read["Yes/No (1/0): "];
deci val = read["Decimal: "];
```

**Type Support in Expression Form:**
- `int` - Parse ASCII digits with optional sign ✅
- `str` - Read line until newline (max 255 chars) ✅
- `char` - Single byte read ✅
- `bool` - Parse "true", "false", "1", "0" (case-insensitive first char) ✅
- `deci` - Parse as int, convert to double ✅
- `byte` - Parse int, mask to 8-bit ✅
- `ubyte` - Parse int, mask to 8-bit unsigned ✅

---

## 6. Built-in Functions (200+)

Functions are called with `@name[args]` syntax.

### System Control (5 functions)
- `@exit[code]` - Exit with status code
- `@halt[]` - Halt CPU
- `@sleep[ms]` - Sleep milliseconds
- `@clock[]` - Get system uptime in ms
- `@panic[msg]` - Force termination with error

### Memory Operations (20+ functions)
- Address: `@addr[var]`, `@peek[addr]`, `@poke[addr, byte]`
- Allocation: `@alloc[size]`, `@free[ptr]`, `@realloc[ptr, newsize]`, `@salloc[size]`
- Bulk: `@memcpy[dst, src, size]`, `@memset[dst, val, size]`, `@memcmp[p1, p2, size]`, `@memclr[ptr, size]`
- Memory mapping: `@mmap[size, prot, flags]`, `@munmap[ptr, size]`, `@mprotect[ptr, size, prot]`
- Info: `@heap_start[]`, `@heap_end[]`, `@heap_size[]`, `@page_size[]`, `@stack_ptr[]`, `@stack_size[]`
- Alignment: `@align[ptr, boundary]`
- Memory fences: `@mfence[]`, `@lfence[]`, `@sfence[]`

### Type Conversion (6 functions)
- `@type[val]::type` - Unified type conversion
- `@type[val1, val2]::int` - Concatenate/convert multiple values
- `@type[str]::int` - String to int
- Deprecated but available: `@to_int[]`, `@to_deci[]`, `@to_str[]`, `@to_byte[]`, `@to_bool[]`

### File I/O (6 functions)
- `@fopen[path, mode]` - Open file (returns file descriptor)
- `@fread[fd, buffer, size]` - Read bytes
- `@fwrite[fd, buffer, size]` - Write bytes
- `@fseek[fd, offset]` - Seek position
- `@fclose[fd]` - Close file
- `@fdelete[path]` - Delete file

### Network (8 functions)
- `@socket[type, protocol]` - Create socket
- `@connect[socket, addr, port]` - Connect to server
- `@send[socket, buffer, length]` - Send data
- `@recv[socket, buffer, length]` - Receive data
- `@bind[socket, addr, port]` - Bind socket
- `@listen[socket, backlog]` - Listen for connections
- `@accept[socket]` - Accept connection
- `@close[socket]` - Close socket

### String Manipulation (12 functions)
- `@len[str]` - String length
- `@concat[str1, str2]` - Concatenate
- `@substr[str, start, len]` - Substring
- `@strcmp[str1, str2]` - Compare (0=equal)
- `@split[str, delim]` - Split by delimiter
- `@join[array, delim]` - Join strings
- `@trim[str]` - Remove whitespace
- `@upper[str]` - Uppercase
- `@lower[str]` - Lowercase
- `@indexOf[str, substring]` - Find index
- `@replace[str, old, new]` - Replace substring
- `@startsWith[str, prefix]` - Check prefix
- `@endsWith[str, suffix]` - Check suffix
- `@reverse[str]` - Reverse
- `@repeat[str, n]` - Repeat n times
- `@pad[str, len, char]` - Pad string
- `@chr[int]` - Int to char
- `@ord[char]` - Char to int

### Math Operations (15 functions)
- Basic: `@abs[x]`, `@sqrt[x]`, `@pow[x, y]`
- Integer: `@isqrt[x]`, `@floor[x]`, `@ceil[x]`
- Min/Max: `@min[a, b, ...]`, `@max[a, b, ...]` (variadic)
- Bit manipulation: `@popcount[x]`, `@clz[x]`, `@ctz[x]`
- Number theory: `@gcd[a, b]`, `@lcm[a, b]`, `@isprime[x]`, `@modpow[base, exp, mod]`

### Threading & Concurrency (45+ functions)

**Basic Threading:**
- `@spawn[function, arg]` - Create thread
- `@join[tid]` - Wait for thread
- `@pid[]` - Get process ID
- `@kill[pid]` - Terminate process

**Mutexes:**
- `@mutex_create[]` - Create mutex
- `@mutex_lock[mutex_id]` - Lock
- `@mutex_unlock[mutex_id]` - Unlock
- `@mutex_trylock[mutex_id]` - Non-blocking lock
- `@mutex_destroy[mutex_id]` - Destroy

**Semaphores:**
- `@semaphore_create[count]` - Create with initial count
- `@semaphore_wait[id]` - Decrement (wait)
- `@semaphore_signal[id]` - Increment (signal)

**Condition Variables:**
- `@cond_create[]` - Create
- `@cond_wait[cond, mutex]` - Wait (atomic with mutex)
- `@cond_signal[cond]` - Signal one
- `@cond_broadcast[cond]` - Signal all

**Atomics:**
- `@atomic_cmp_swap[ptr, expected, new]` - Lock-free CAS
- `@atomic_increment[ptr]` - Lock-free increment
- `@atomic_decrement[ptr]` - Lock-free decrement

**Channels:**
- `@channel_create[capacity]` - Create buffer
- `@channel_send[channel, value]` - Send (blocking if full)
- `@channel_recv[channel]` - Receive (blocking if empty)
- `@channel_close[channel]` - Close
- `@channel_empty[channel]` - Check empty
- `@channel_full[channel]` - Check full

**Thread Pools:**
- `@pool_create[worker_count]` - Create pool
- `@pool_submit[pool, function, arg]` - Queue task
- `@pool_wait[pool]` - Wait for completion
- `@pool_destroy[pool]` - Cleanup

**Advanced (RWLocks, Barriers, Events):**
- Reader-writer locks: `@rwlock_create[]`, `@rwlock_read[]`, `@rwlock_read_unlock[]`, etc.
- Barriers: `@barrier_create[count]`, `@barrier_wait[barrier]`
- Events: `@event_create[]`, `@event_set[]`, `@event_reset[]`, `@event_wait[]`

### Process Management (15 functions)
- `@fork[function]` - Fork process
- `@wait[pid]` - Wait for specific process
- `@wait_any[ptr]` - Wait for any child
- `@getpid[]` - Get process ID
- `@getppid[]` - Get parent process ID
- `@chdir[path]` - Change directory
- `@getcwd[]` - Get working directory
- `@getenv[name]` - Get environment variable
- `@setenv[name, value]` - Set environment variable
- `@unsetenv[name]` - Unset environment variable
- `@exec[path, args]` - Execute program
- `@system_call[command]` - Execute shell command
- `@getrlimit[resource]` - Get resource limit
- `@setrlimit[resource, limit]` - Set resource limit
- `@thread_count[]` - Get thread count

### Security (5 functions)
- `@hash[data, algorithm]` - Hash data
- `@rand[size]` - Random bytes
- `@secure_zero[ptr, size]` - Securely zero memory
- `@entropy[]` - System entropy
- `@verify[sig, pubkey, data]` - Verify signature

### Hardware Access (6 functions)
- `@port_in[port]` - Read I/O port
- `@port_out[port, value]` - Write I/O port
- `@irq_enable[irq]` - Enable interrupt
- `@irq_disable[irq]` - Disable interrupt
- `@ioread[addr]` - Read memory-mapped I/O
- `@iowrite[addr, value]` - Write memory-mapped I/O

### Error Handling (10 functions)
- `@error[msg]` - Trigger error
- `@get_error_code[]` - Get last error code
- `@get_error_msg[]` - Get error message
- `@clear_error[]` - Clear error state
- `@assert[condition, message]` - Assert
- `@check_alloc[ptr]` - Check if valid allocation
- `@try_syscall[...]` - Try syscall with error handling
- `@try_fopen[path, mode]` - Try file open
- `@log_error[msg]` - Log to stderr
- `@recover[]` - Recover from error

### Utility Functions
- `@sizeof[type]` - Size in bytes

---

## 7. Advanced Language Features

### Comments
- Single-line: `// comment`
- Multi-line: `//> comment <//`

### Packages (Basic Support)
- Import: `pkg:package_name;`
- Alias: `pkg:external_module as alias;`

### Global Constants
```ras
const MAX_SIZE = 100;
const VERSION = "1.0";
```

### Expression Nesting
- Security feature: max 1000 levels deep to prevent stack overflow
- Expressions as values: ternary `? :`, read[], cycle (with emit)
- All can be nested in assignments and complex expressions

### Variable Declaration
```ras
int x = 10;
str name = "Alice";
bool flag;  // Uninitialized
deci pi = 3.14;
```

### Type Inference
- Assignment automatically infers type in `read[]` expressions
- `x = read["prompt"];` infers type from context

### Member Access Chains
```ras
p.address.city;           // Nested access
arr{i}.member.submember;  // Array element member chains
```

---

## 8. What RasCode is Built For

Based on source code structure:

✅ **Systems Programming**
- Direct memory access (`@addr`, `@peek`, `@poke`)
- Memory management (`@alloc`, `@free`, `@realloc`)
- I/O ports and hardware access

✅ **Concurrent Programming**
- Full threading support
- Mutexes, semaphores, condition variables
- Lock-free atomic operations
- Channels for inter-thread communication
- Thread pools for work queuing

✅ **Network Programming**
- Socket API (TCP/UDP)
- Client-server capabilities

✅ **Systems-Level Languages**
- Bitwise operations with full operator set
- Low-level memory management
- Process control (`@fork`, `@exec`)
- Environment variables, working directory

✅ **Bare Metal / Embedded**
- Hardware I/O ports
- Memory mapping
- Interrupt control
- Direct syscall access

✅ **General Programming**
- Arrays, maps, structs
- Functions with parameters
- High-level constructs (if/while/for/switch)
- String manipulation
- Math operations

---

## 9. What's NOT Directly in Language

**Not Built-in:**
- Generics/Templates
- Exceptions (has try/catch pattern via `check`)
- Default parameters
- Function overloading
- Macros/Preprocessor
- Goto (controlled via break/continue)
- Lambda/Closures (functions are first-class object but not anonymous)
- Class inheritance/OOP (has structs/groups only)
- Operator overloading
- Module system (basic pkg support)
- Garbage collection (manual `@free` required)

---

## 10. Compilation Target

All features compile to **x86-64 Assembly**:
- Output: `.asm` files
- Assembler: NASM
- Linker: GNU ld
- Object format: ELF (Linux)
- Direct syscall interface

---

## Summary

RasCode is a **systems programming language** with:
- **Minimal design**: 8 types, clean operator/statement syntax
- **Maximum capability**: 200+ builtins for extreme system access
- **Thread-first design**: Full concurrency primitives built-in
- **Low-level access**: Direct memory, I/O, interrupts, syscalls
- **Modern conveniences**: String interpolation, maps, structs, error handling, cycle-as-expressions
- **Proven compiler**: Direct x86-64 code gen with validated output
- **Expression-oriented**: Ternary, cycle, read operations can be used as value sources

**Best for:** OS development, embedded systems, systems tools, concurrent server applications, hardware control

**Not ideal for:** Web development, data science, high-level application logic (though possible)
