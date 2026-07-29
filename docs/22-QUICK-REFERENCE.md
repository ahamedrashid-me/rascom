# Quick Reference Card — Common RasCode Patterns

One-page guide to essential RasCode patterns for quick lookup.

---

## Variable Declaration & Types

```ras
// Primitives
int x = 42;           // 64-bit signed integer
deci pi = 3.14159;    // Floating-point number
char c = 'A';         // Single character
str s = "hello";      // String
bool flag = true;     // Boolean
byte b = 255;         // Unsigned 8-bit
ubyte u = 0xFF;       // Unsigned 8-bit (alternative)
none nil = none;      // Null value

// Collections
arr{int, 5} array = {1, 2, 3, 4, 5};     // Fixed-size array
map{str, int} config = {                 // Key-value pairs
    "timeout"   : 30,
    "retries"   : 3
};
group Point {         // Struct-like
    int x;
    int y;
}
Point p = {100, 200};
```

---

## Control Flow

### Conditionals

```ras
// If/else
if[x > 10] {
    show["x is large\n"];
} or {
    show["x is small\n"];
}

// Cycle (switch statement)
cycle[user_input] {
    when["help"] : { show["Showing help...\n"]; }
    when["exit"] : { @exit[0]; }
    fixed : { show["Unknown command\n"]; }
}
```

### Loops

```ras
// While loop
int i = 0;
while[i < 10] {
    show[i];
    i = i + 1;
}

// Loop (for loop)
loop[int j = 0; j < 5; j++] {
    show[j];
}

// Array iteration (pattern - use map or foreach would be nice)
arr{int, 3} nums = {1, 2, 3};
loop[int idx = 0; idx < 3; idx++] {
    show[nums{idx}];
}
```

### Error Handling

```ras
// Try-catch pattern
check {
    int fd = @fopen["data.txt", "r"];
    if[fd == 0] {
        @panic["Cannot open file"];
    }
} when: {
    show["Error occurred\n"];
}

// Syscall error handling
int result = @try_syscall[1, "output\n"];
if[result < 0] {
    int err = @get_error_code[];
    show["Syscall failed: "];
    show[@get_error_msg[]];
}
```

---

## Functions

```ras
// Define function
fnc greet[name]::str {
    get[@concat["Hello, ", name]];
}

// Call function
str greeting = greet["Alice"];

// No parameters
fnc get_time[]::int {
    get[@clock[]];
}

// Multiple returns (return early)
fnc search[arr{int, 5}, target]::int {
    loop[int i = 0; i < 5; i++] {
        if[arr{i} == target] {
            get[i];              // Return early
        }
    }
    get[-1];                      // Not found
}
```

---

## Memory Management

```ras
// Allocate memory
int ptr = @alloc[256];           // 256 bytes
@poke[ptr, 42];                  // Write value
int val = @peek[ptr];            // Read value (= 42)

// Deallocate
@free[ptr];

// Resize allocation
int new_ptr = @realloc[ptr, 512];  // Grow to 512 bytes

// Stack variable address (temporary!)
int x = 100;
int addr_x = @addr[x];
int copy = @peek[addr_x];        // Works in same scope
// ❌ Don't save @addr for later use!

// Batch memory operations
int dest = @alloc[100];
int src = @alloc[100];
@memcpy[dest, src, 100];         // Fast copy
@memset[dest, 0, 100];           // Clear memory
```

---

## String Operations

```ras
// String properties
int len = @len["hello"];         // = 5
str upper = @upper["hello"];     // = "HELLO"
str lower = @lower["HELLO"];     // = "hello"

// String manipulation
str combined = @concat["Hello", " ", "World"];
str part = @substr["hello", 1, 3];  // = "ell"
int pos = @indexOf["hello", "ll"];  // = 2

// String splitting
arr{str, 3} parts = @split["a,b,c", ","];
str joined = @join[parts, " "];  // = "a b c"

// String searching & comparison
int cmp = @strcmp["abc", "abc"]; // = 0 (equal)
bool starts = @startsWith["hello", "he"];  // = true
bool ends = @endsWith["hello", "lo"];      // = true

// String replace
str result = @replace["hello world", "world", "RasCode"];
```

---

## Arrays

```ras
// Declaration and initialization
arr{int, 5} numbers = {10, 20, 30, 40, 50};

// Access elements
int first = numbers{0};         // = 10
numbers{2} = 99;                // Modify element

// Array length
int size = @len[numbers];       // = 5

// Sorting and searching
arr{int, 5} sorted = @qsort[numbers];      // Sort array
int idx = @bsearch[sorted, 30];            // Binary search
int idx2 = @search[numbers, 30];           // Linear search
int min = @search[numbers, @min[numbers]]; // Min value index
```

---

## Higher-Order Math

```ras
// Basic
int abs_val = @abs[-42];        // = 42
deci sq = @sqrt[16.0];          // = 4.0
int pow_val = @pow[2, 8];       // = 256

// Min/Max
int smallest = @min[3, 7, 2, 9];     // = 2 (variadic)
int largest = @max[3, 7, 2, 9];      // = 9 (variadic)

// Bit operations
int bits = @popcount[15];       // = 4 (4 bits set in 0b1111)
int shift = 4 << 2;             // = 16 (bitshift left)
int right = 16 >> 2;            // = 4 (bitshift right)

// Number theory
int g = @gcd[24, 36];           // = 12
bool prime = @isprime[17];       // = true
int sqrt_int = @isqrt[17];       // = 4 (integer square root)
```

---

## File I/O

```ras
// Reading files
int fd = @fopen["data.txt", "r"];
if[fd > 0] {
    str line = "";
    // Note: fread typically reads fixed chunks
    // Full implementation depends on wrapper functions
    @fclose[fd];
}

// Writing files
int out = @fopen["output.txt", "w"];
@fwrite[out, "Hello\n", 6];
@fclose[out];

// Seeking within file
@fseek[fd, 0, 0];               // Seek to start (0 = SEEK_SET)
@fseek[fd, -100, 1];            // 100 bytes back (1 = SEEK_CUR)

// File operations
int deleted = @fdelete["temp.txt"];  // Delete file
```

---

## Standard I/O (Console Output)

```ras
// Basic output (no automatic newline)
show["Hello"];                  // Output: Hello
show[42];                       // Output: 42
show["Multiple"];
show[" "];
show["words"];                  // Output: Multiple words

// Output with newline
show["Line 1\n"];
show["Line 2\n"];

// Formatted output with automatic newline (showf)
int x = 42;
showf["The answer is $x"];      // Output: The answer is 42\n

str name = "Alice";
int age = 30;
showf["$name is $age years old"];  // Output: Alice is 30 years old\n

// Multiple variables
int a = 10, b = 20;
showf["a=$a, b=$b"];            // Output: a=10, b=20\n

// Expression interpolation with operators
int x = 5, y = 3;
showf["$x + $y = ${x + y}"];    // Output: 5 + 3 = 8\n
showf["$x > $y: ${x > y}"];     // Output: 5 > 3: true\n
showf["Length: ${@len[\"test\"]}"];  // Output: Length: 4\n
```

---

## Standard Input (Console Input)

```ras
// Expression form with prompt (recommended)
str name = read["Enter your name: "];
int age = read["Enter your age: "];
char grade = read["Enter grade: "];
bool confirmed = read["Confirm? (true/false): "];

showf["Name: $name, Age: $age, Grade: $grade"];

// Statement form without prompt (older style)
str response;
read[response];    // Reads from stdin directly

// Examples by type
deci temperature = read["Temperature (C): "];
byte value = read["Byte value (0-255): "];
ubyte unsigned_val = read["Unsigned value: "];

// Interactive menu
showf["1. Add"];
showf["2. Subtract"];
int choice = read["Select (1-2): "];
```

---

## Threading & Synchronization

```ras
// Simple thread creation
fnc worker[id]::int {
    show["Worker "];
    show[id];
    show[" done\n"];
    get[0];
}

@spawn[worker, 1];              // Create background thread
@spawn[worker, 2];

// Thread synchronization
int mutex = @mutex_create[];
@mutex_lock[mutex];
// Critical section
@mutex_unlock[mutex];

// Atomic operations
int counter = 0;
int old = @atomic_cmp_swap[counter, 0, 1];  // Compare-and-swap

// Semaphore for rate limiting
int sem = @semaphore_create[3];  // Allow 3 concurrent
@semaphore_wait[sem];            // Acquire
// Do work...
@semaphore_signal[sem];          // Release
```

---

## Channels (Go-style Concurrency)

```ras
// Channel creation and communication
int ch = @channel_create[5];     // Buffered channel (capacity 5)

// Send (from one thread)
@channel_send[ch, 42];

// Receive (from another thread)
int value = @channel_recv[ch];   // Blocks if empty

// Check channel state
bool empty = @channel_empty[ch];
bool full = @channel_full[ch];

// Close channel
@channel_close[ch];
```

---

## Type Conversion

```ras
// Convert to specific types
int to_integer = @to_int["123"];        // Parse string
deci to_decimal = @to_deci["3.14"];     // Parse float
byte to_byte = @to_byte[256];           // Truncate int
bool to_bool = @to_bool[1];             // Any int != 0 → true
str to_string = @to_str[42];            // Convert int to string

// Character conversion
int code = @ord['A'];                   // 'A' → 65
char character = @chr[65];              // 65 → 'A'
```

---

## Common Patterns

### Safe String Building (Avoid O(n²))
```ras
// ❌ SLOW: Repeated concatenation
str result = "";
loop[int i = 0; i < 1000; i++] {
    result = @concat[result, "text\n"];
}

// ✅ FAST: Use array + join
arr{str, 1000} lines = {...};
str result = @join[lines, "\n"];
```

### Efficient Search in Sorted Array
```ras
// Sort array first
arr{int, 100} data = {...};
arr{int, 100} sorted = @qsort[data];

// Then binary search (O(log n) per search)
loop[int i = 0; i < queries; i++] {
    int idx = @bsearch[sorted, query_array{i}];
}
```

### Memory Pool Pattern
```ras
// Allocate large block once
int pool = @alloc[10000];

// Use offsets for "allocations"
int obj1 = pool + 0;      // First object at +0
int obj2 = pool + 1000;   // Second object at +1000
int obj3 = pool + 2000;   // etc.

// Deallocate entire pool at once
@free[pool];
```

### Resource Cleanup (Error-Safe)
```ras
check {
    int fd = @fopen["data", "r"];
    if[fd <= 0] {
        @panic["Open failed"];
    }
    // ... use fd ...
    @fclose[fd];
} when: {
    show["Error during file operation\n"];
    // If error thrown, fd cleanup happens in error handler
}
```

---

## Builtin Function Reference

### Most Commonly Used (80/20 Rule)
```
Memory:    @alloc, @free, @peek, @poke, @memcpy
String:    @len, @concat, @substr, @split, @join
I/O:       @show, @read, @fopen, @fread, @fwrite, @fclose
Math:      @min, @max, @abs, @pow, @sqrt
Control:   @exit, @sleep, @panic
Thread:    @spawn, @join, @mutex_lock, @mutex_unlock, @channel_send/recv
```

### Advanced (10/20 Rule)
```
Memory:    @addr, @memset, @memcmp, @realloc, @mmap
String:    @indexOf, @replace, @upper, @lower, @trim
Array:     @qsort, @bsearch, @search, @shuffle
Bitwise:   @popcount, @clz, @ctz
Numeric:   @gcd, @lcm, @isprime, @isqrt
Threading: @semaphore_*, @atomic_*, @pool_*
```

---

## Performance Gotchas

| Pattern | ❌ Bad | ✅ Good | Speedup |
|---------|--------|---------|---------|
| String build | `= @concat[a, b]` in loop | array + @join | 100-1000x |
| Search many | `@search` each time | @qsort once, @bsearch | 10-50x |
| Copy memory | peek/poke in loop | @memcpy | 10-100x |
| File I/O | byte-by-byte | Large buffers | 100-1000x |
| Mutex contention | Hold during I/O | Short critical section | 1000x+ |

---

## Debugging Tips

```ras
// Debug output (verbose - multiple show statements)
show["Debug: x="];
show[x];
show["  y="];
show[y];
show["\n"];

// Debug output (cleaner - using showf)
showf["Debug: x=$x  y=$y"];

// Assert pattern (verbose)
if[result == expected] {
    show["✓ Test passed\n"];
} or {
    show["✗ Test FAILED: got "];
    show[result];
    @panic["Assertion failed"];
}

// Assert pattern (cleaner with showf)
if[result == expected] {
    showf["✓ Test passed"];
} or {
    showf["✗ Test FAILED: got $result"];
    @panic["Assertion failed"];
}

// Performance timing
int start = @clock[];
// ... do work ...
int elapsed = @clock[] - start;
show["Elapsed: "];
show[elapsed];
show[" ms\n"];
```

---

Generated: April 4, 2026 | RasCode Quick Reference v1.0
