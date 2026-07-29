# 18. String Manipulation & Math Operations

This document covers two essential builtin function categories: **String Manipulation** (12 functions) and **Math Operations** (12 functions). These were previously undocumented and represent significant language capabilities.

---

## Part 1: String Manipulation Functions

String manipulation builtins provide powerful text processing capabilities complementing RASLang's native string interpolation and I/O features.

### @split[string, delimiter] :: arr{str, ?}
Splits a string into an array of substrings based on a delimiter.

**Signature:**
```ras
fnc split_demo[]::int {
    str text = "apple,banana,cherry";
    arr{str, 3} parts = @split[text, ","];
    
    show[parts{0}];  // Output: apple
    show[parts{1}];  // Output: banana
    show[parts{2}];  // Output: cherry
    
    get[0];
}
```

**Parameters:**
- `string`: Source string to split
- `delimiter`: Character(s) to split on

**Returns:** Array of substrings (fixed size based on delimiter count + 1)

**Use Cases:** Parsing CSV data, tokenizing input, splitting paths

---

### @str_join[array, separator] :: str
Joins array elements into a single string with a separator.

**Signature:**
```ras
fnc join_demo[]::int {
    arr{str, 3} words = {"hello", "world", "test"};
    str result = @str_join[words, " "];
    
    show[result];  // Output: hello world test
    
    get[0];
}
```

**Parameters:**
- `array`: Array of strings to join
- `separator`: String to insert between elements

**Returns:** Single concatenated string

**Use Cases:** Creating CSV rows, building command arguments, formatting output

---

### @trim[string] :: str
Removes leading and trailing whitespace from a string.

**Signature:**
```ras
fnc trim_demo[]::int {
    str padded = "  hello world  ";
    str result = @trim[padded];
    
    show[result];  // Output: hello world
    
    get[0];
}
```

**Parameters:**
- `string`: String to trim

**Returns:** String without leading/trailing whitespace

**Use Cases:** Cleaning user input, parsing formatted data, preparing for comparison

---

### @upper[string] :: str
Converts string to uppercase.

**Signature:**
```ras
fnc upper_demo[]::int {
    str text = "Hello RASLang";
    str result = @upper[text];
    
    show[result];  // Output: HELLO RASLANG
    
    get[0];
}
```

**Parameters:**
- `string`: String to convert

**Returns:** Uppercase string

**Use Cases:** Case-insensitive comparisons, formatting display text

---

### @lower[string] :: str
Converts string to lowercase.

**Signature:**
```ras
fnc lower_demo[]::int {
    str text = "Hello RASLang";
    str result = @lower[text];
    
    show[result];  // Output: hello raslang
    
    get[0];
}
```

**Parameters:**
- `string`: String to convert

**Returns:** Lowercase string

**Use Cases:** Normalizing input for comparison, case-insensitive searches

---

### @index[string, substring] :: int
Finds first occurrence of substring in string.

**Signature:**
```ras
fnc index_demo[]::int {
    str text = "hello world hello";
    int pos = @index[text, "world"];
    
    show[pos];  // Output: 6
    
    get[0];
}
```

**Parameters:**
- `string`: String to search in
- `substring`: String to search for

**Returns:** Index of first occurrence (or -1 if not found)

**Use Cases:** String parsing, finding delimiters, substring detection

---

### @replace[string, search, replacement] :: str
Replaces all occurrences of search string with replacement.

**Signature:**
```ras
fnc replace_demo[]::int {
    str text = "hello world, hello RASLang";
    str result = @replace[text, "hello", "hi"];
    
    show[result];  // Output: hi world, hi RASLang
    
    get[0];
}
```

**Parameters:**
- `string`: Original string
- `search`: Substring to find
- `replacement`: Substring to replace with

**Returns:** Modified string (all occurrences replaced)

**Use Cases:** Text substitution, template processing, variable replacement

---

### @startswith[string, prefix] :: bool
Checks if string starts with given prefix.

**Signature:**
```ras
fnc startswith_demo[]::int {
    str text = "RASLang compiler";
    bool result = @startswith[text, "RAS"];
    
    if result {
        show["Starts with RAS"];  // This executes
    }
    
    get[0];
}
```

**Parameters:**
- `string`: String to check
- `prefix`: Prefix to match

**Returns:** true if starts with prefix, false otherwise

**Use Cases:** File extension checking, command prefix detection, path validation

---

### @endswith[string, suffix] :: bool
Checks if string ends with given suffix.

**Signature:**
```ras
fnc endswith_demo[]::int {
    str filename = "program.ras";
    bool result = @endswith[filename, ".ras"];
    
    if result {
        show["Valid RASLang file"];  // This executes
    }
    
    get[0];
}
```

**Parameters:**
- `string`: String to check
- `suffix`: Suffix to match

**Returns:** true if ends with suffix, false otherwise

**Use Cases:** File type validation, version checking, status verification

---

### @reverse[string] :: str
Reverses a string.

**Signature:**
```ras
fnc reverse_demo[]::int {
    str text = "RASLang";
    str result = @reverse[text];
    
    show[result];  // Output: gnaLSAR
    
    get[0];
}
```

**Parameters:**
- `string`: String to reverse

**Returns:** Reversed string

**Use Cases:** String validation (palindromes), reversing buffers, encoding/decoding

---

### @repeat[string, count] :: str
Repeats a string N times.

**Signature:**
```ras
fnc repeat_demo[]::int {
    str pattern = "=";
    str line = @repeat[pattern, 20];
    
    show[line];  // Output: ====================
    
    get[0];
}
```

**Parameters:**
- `string`: String to repeat
- `count`: Number of repetitions

**Returns:** Repeated string

**Use Cases:** Creating separators, generating padding, pattern filling

---

### @pad[string, length, padchar] :: str
Pads string to specified length using padchar on the right.

**Signature:**
```ras
fnc pad_demo[]::int {
    str text = "hi";
    str padded = @pad[text, 5, "."];
    
    show[padded];  // Output: hi...
    
    get[0];
}
```

**Parameters:**
- `string`: String to pad
- `length`: Target length
- `padchar`: Character to pad with (single char)

**Returns:** Padded string

**Use Cases:** Formatting columns, table alignment, fixed-width fields

---

## Part 2: Math Operations Functions

Math builtins provide bit manipulation, arithmetic, and computational capabilities essential for systems programming and numerical algorithms.

### @isqrt[number] :: int
Integer square root (floor of sqrt).

**Signature:**
```ras
fnc isqrt_demo[]::int {
    int value = 16;
    int result = @isqrt[value];
    
    show[result];  // Output: 4
    
    get[0];
}
```

**Parameters:**
- `number`: Integer to take sqrt of

**Returns:** Floor of square root

**Use Cases:** Geometric calculations, algorithm optimization, matrix operations

---

### @pow[base, exponent] :: int
Base raised to integer exponent.

**Signature:**
```ras
fnc pow_demo[]::int {
    int base = 2;
    int exp = 8;
    int result = @pow[base, exp];
    
    show[result];  // Output: 256
    
    get[0];
}
```

**Parameters:**
- `base`: Base number
- `exponent`: Exponent (non-negative)

**Returns:** Base^exponent

**Use Cases:** Computing powers, calculating bit positions, modular exponentiation

---

### @abs[number] :: int
Absolute value (magnitude without sign).

**Signature:**
```ras
fnc abs_demo[]::int {
    int negative = -42;
    int result = @abs[negative];
    
    show[result];  // Output: 42
    
    get[0];
}
```

**Parameters:**
- `number`: Integer value

**Returns:** Absolute value

**Use Cases:** Distance calculations, error margins, numeric comparison

---

### @min[a, b, ...] :: int
Minimum of two or more integers (variadic).

**Signature:**
```ras
// Two arguments (required)
int result = @min[15, 42];      // = 15

// Multiple arguments
int lowest = @min[100, 50, 75, 25, 90];  // = 25

fnc min_demo[]::int {
    int x = 15;
    int y = 42;
    int z = 8;
    
    int two_min = @min[x, y];       // 15
    int three_min = @min[x, y, z];  // 8
    
    loop[int i = 0; i < 5; i++] {
        show[@min[i, i+1, i+2]];    // Min of three values
    }
    
    get[0];
}
```

**Parameters:**
- `a`: First integer (required)
- `b`: Second integer (required)
- `...`: Additional integers (optional, 0 to many more)

**Returns:** Smallest of all arguments

**Variadic Support:** ✅ Yes - accepts 2+ arguments

**Use Cases:** Range clipping, boundary checking, array indexing, finding minimum across multiple values

---

### @max[a, b, ...] :: int
Maximum of two or more integers (variadic).

**Signature:**
```ras
// Two arguments (required)
int result = @max[15, 42];      // = 42

// Multiple arguments
int highest = @max[100, 50, 75, 25, 90];  // = 100

fnc max_demo[]::int {
    int x = 15;
    int y = 42;
    int z = 100;
    
    int two_max = @max[x, y];       // 42
    int three_max = @max[x, y, z];  // 100
    
    loop[int i = 0; i < 5; i++] {
        show[@max[i, i+1, i+2]];    // Max of three values
    }
    
    get[0];
}
```

**Parameters:**
- `a`: First integer (required)
- `b`: Second integer (required)
- `...`: Additional integers (optional, 0 to many more)

**Returns:** Largest of all arguments

**Variadic Support:** ✅ Yes - accepts 2+ arguments

**Use Cases:** Range clipping, boundary checking, array indexing, finding maximum across multiple values

**Returns:** Larger of the two

**Use Cases:** Range clamping, peak detection, allocation sizing

---

### @clz[number] :: int
Count leading zeros in binary representation.

**Signature:**
```ras
fnc clz_demo[]::int {
    int value = 1;  // Binary: ...00000001
    int result = @clz[value];  // 63 leading zeros
    
    show[result];
    
    get[0];
}
```

**Parameters:**
- `number`: Integer value

**Returns:** Count of leading zero bits

**Use Cases:** Bit-length calculation, integer logarithm, algorithm tuning

**Note:** CLZ is undefined for 0 (may return machine-dependent value)

---

### @ctz[number] :: int
Count trailing zeros in binary representation.

**Signature:**
```ras
fnc ctz_demo[]::int {
    int value = 8;  // Binary: 1000
    int result = @ctz[value];  // 3 trailing zeros
    
    show[result];
    
    get[0];
}
```

**Parameters:**
- `number`: Integer value

**Returns:** Count of trailing zero bits

**Use Cases:** Finding bit position, power-of-2 detection, bit manipulation

**Note:** CTZ is undefined for 0 (may return machine-dependent value)

---

### @popcount[number] :: int
Count set bits (1s) in binary representation.

**Signature:**
```ras
fnc popcount_demo[]::int {
    int value = 15;  // Binary: 1111 (four 1s)
    int result = @popcount[value];
    
    show[result];  // Output: 4
    
    get[0];
}
```

**Parameters:**
- `number`: Integer value

**Returns:** Number of 1 bits

**Use Cases:** Hamming weight, bit set membership, checksum calculation

---

### @gcd[a, b] :: int
Greatest Common Divisor using Euclidean algorithm.

**Signature:**
```ras
fnc gcd_demo[]::int {
    int num1 = 48;
    int num2 = 18;
    int result = @gcd[num1, num2];
    
    show[result];  // Output: 6
    
    get[0];
}
```

**Parameters:**
- `a`: First integer (non-negative)
- `b`: Second integer (non-negative)

**Returns:** GCD of a and b

**Use Cases:** Fraction reduction, ratio calculations, divisor finding

---

### @lcm[a, b] :: int
Least Common Multiple: lcm(a,b) = (a × b) / gcd(a,b).

**Signature:**
```ras
fnc lcm_demo[]::int {
    int num1 = 12;
    int num2 = 18;
    int result = @lcm[num1, num2];
    
    show[result];  // Output: 36
    
    get[0];
}
```

**Parameters:**
- `a`: First integer (non-negative)
- `b`: Second integer (non-negative)

**Returns:** LCM of a and b

**Use Cases:** Scheduling calculations, periodic events, cycle detection

---

### @isprime[number] :: bool
Primality test (deterministic for 64-bit integers).

**Signature:**
```ras
fnc isprime_demo[]::int {
    int test1 = 17;
    int test2 = 20;
    
    if @isprime[test1] {
        show["17 is prime"];  // This executes
    }
    
    if @isprime[test2] {
        show["20 is prime"];  // This does not execute
    }
    
    get[0];
}
```

**Parameters:**
- `number`: Integer to test

**Returns:** true if prime, false otherwise

**Use Cases:** Number theory, cryptography, algorithm selection

---

### @modpow[base, exponent, modulus] :: int
Modular exponentiation: (base^exponent) mod modulus (efficient computation).

**Signature:**
```ras
fnc modpow_demo[]::int {
    int base = 2;
    int exp = 10;
    int mod = 1000;
    int result = @modpow[base, exp, mod];
    
    show[result];  // Output: 24 (1024 mod 1000)
    
    get[0];
}
```

**Parameters:**
- `base`: Base value
- `exponent`: Exponent (non-negative)
- `modulus`: Modulus divisor

**Returns:** (base^exponent) % modulus

**Use Cases:** Cryptography (RSA), large number computation, hash calculation

**Efficiency:** Computes without computing full power first (prevents overflow)

---

## Mixed String & Math Examples

### Parsing Numeric Strings
```ras
fnc parse_numbers[]::int {
    str data = "10,20,30,40,50";
    arr{str, 5} parts = @split[data, ","];
    
    // Note: Would need @to_int (deprecated) or type casting
    // to convert strings to numbers
    show[parts{0}];  // Output: 10
    
    get[0];
}
```

### String-based Math Formatting
```ras
fnc format_power[]::int {
    int base = 3;
    int exp = 4;
    int result = @pow[base, exp];  // 81
    
    str formatted = @concat[@to_str[base], "^", @to_str[exp], "=", @to_str[result]];
    show[formatted];  // Output: 3^4=81
    
    get[0];
}
```

### Bit-string Representation
```ras
fnc binary_info[]::int {
    int value = 42;
    
    int ones = @popcount[value];
    int leading = @clz[value];
    int trailing = @ctz[value];
    
    show[@concat["Value: ", @to_str[value]]];
    show[@concat["Set bits (popcount): ", @to_str[ones]]];
    show[@concat["Leading zeros: ", @to_str[leading]]];
    show[@concat["Trailing zeros: ", @to_str[trailing]]];
    
    get[0];
}
```

### @indexOf[string, substring] :: int
Find index of substring occurrence in string.

```ras
int idx = @indexOf["hello world", "world"];
// Returns: 6
```

### @startsWith[string, prefix] :: bool
Check if string starts with given prefix.

```ras
bool result = @startsWith["hello world", "hello"];
// Returns: 1 (true)
```

### @endsWith[string, suffix] :: bool
Check if string ends with given suffix.

```ras
bool result = @endsWith["hello world", "world"];
// Returns: 1 (true)
```

---

## Part 2: Advanced Math Operations

### @sqrt[value] :: deci
Floating-point square root.

```ras
deci result = @sqrt[16];  // Returns: 4.0
```

### @floor[value] :: deci
Floor function (round down).

```ras
deci result = @floor[3.7];  // Returns: 3.0
```

### @ceil[value] :: deci
Ceiling function (round up).

```ras
deci result = @ceil[3.2];  // Returns: 4.0
```

---

## Part 3: Sorting & Array Operations

### @qsort[array_ptr, size, comparator_fn] :: none
Quicksort array in-place using comparator function.

```ras
fnc compare[a, b]::int {
    if[a < b] { get[-1]; }
    if[a > b] { get[1]; }
    get[0];
}

fnc main[]::int {
    int[5] arr = [5, 2, 8, 1, 9];
    @qsort[@addr[arr], 5, @addr[compare]];
    // arr now: [1, 2, 5, 8, 9]
    get[0];
}
```

### @bsearch[array_ptr, key, comparator_fn] :: int
Binary search in sorted array using comparator.

```ras
int idx = @bsearch[@addr[sorted_array], search_key, @addr[compare]];
// Returns: index if found, negative if not found
```

### @bubble_sort[array_ptr, size, comparator_fn] :: none
Bubble sort (stable, slow but simple).

### @selection_sort[array_ptr, size, comparator_fn] :: none
Selection sort (in-place, unstable).

### @insertion_sort[array_ptr, size, comparator_fn] :: none
Insertion sort (stable, good for small arrays).

### @shuffle[array_ptr, size] :: none
Randomly shuffle array in-place.

```ras
@shuffle[@addr[arr], 10];  // Shuffle 10-element array
```

### @search[array_ptr, value] :: int
Linear search through array.

```ras
int idx = @search[@addr[arr], search_value];
// Returns: index if found, -1 if not
```

### @find_min[array_ptr, size] :: int
Find minimum value in array.

```ras
int min_val = @find_min[@addr[arr], 5];
```

### @find_max[array_ptr, size] :: int
Find maximum value in array.

```ras
int max_val = @find_max[@addr[arr], 5];
```

### @find_min_idx[array_ptr, size] :: int
Find index of minimum value in array.

```ras
int min_idx = @find_min_idx[@addr[arr], 5];
```

### @find_max_idx[array_ptr, size] :: int
Find index of maximum value in array.

```ras
int max_idx = @find_max_idx[@addr[arr], 5];
```

### @count_val[array_ptr, value] :: int
Count occurrences of value in array.

```ras
int count = @count_val[@addr[arr], 42];
// Returns: number of times 42 appears in arr
```

### @sum[array_ptr, size] :: int
Sum all values in integer array.

```ras
int total = @sum[@addr[arr], 10];
```

### @average[array_ptr, size] :: deci
Calculate average of array values.

```ras
deci avg = @average[@addr[arr], 10];
```

---

## Summary Table

| Function | Input | Output | Category |
|----------|-------|--------|----------|
| @split | string, delim | arr{str} | String |
| @str_join | arr{str}, sep | str | String |
| @trim | str | str | String |
| @upper | str | str | String |
| @lower | str | str | String |
| @indexOf | str, substr | int | String |
| @replace | str, search, repl | str | String |
| @startsWith | str, prefix | bool | String |
| @endsWith | str, suffix | bool | String |
| @reverse | str | str | String |
| @repeat | str, count | str | String |
| @pad | str, len, char | str | String |
| @sqrt | deci | deci | Math |
| @floor | deci | deci | Math |
| @ceil | deci | deci | Math |
| @isqrt | int | int | Math |
| @pow | int, int | int | Math |
| @abs | int | int | Math |
| @min | int, int | int | Math |
| @max | int, int | int | Math |
| @clz | int | int | Math |
| @ctz | int | int | Math |
| @popcount | int | int | Math |
| @gcd | int, int | int | Math |
| @lcm | int, int | int | Math |
| @isprime | int | bool | Math |
| @modpow | int, int, int | int | Math |
| @qsort | ptr, size, cmp | none | Sorting |
| @bsearch | ptr, key, cmp | int | Sorting |
| @bubble_sort | ptr, size, cmp | none | Sorting |
| @selection_sort | ptr, size, cmp | none | Sorting |
| @insertion_sort | ptr, size, cmp | none | Sorting |
| @shuffle | ptr, size | none | Sorting |
| @search | ptr, value | int | Sorting |
| @find_min | ptr, size | int | Sorting |
| @find_max | ptr, size | int | Sorting |
| @find_min_idx | ptr, size | int | Sorting |
| @find_max_idx | ptr, size | int | Sorting |
| @count_val | ptr, value | int | Sorting |
| @sum | ptr, size | int | Sorting |
| @average | ptr, size | deci | Sorting |

---

## Compilation & Testing

These functions are pre-compiled and linked into every RASLang program:

```bash
rascom string_math_demo.ras -o string_math_demo
./string_math_demo
# Output shows processed strings and calculated values
```

---

**Next File:** See [19-BUILTINS-ADVANCED.md](19-BUILTINS-ADVANCED.md) for Error Handling and Process/Resource operations.
