# 07: Arrays

## Array Declaration

**Syntax** (from parser.c parse_statement parse_array_decl):

```
arr{element_type, size} array_name;
arr{element_type, size} array_name = {value1, value2, ...};
```

**Rules:**
- Element type: any primitive type or group name
- Size: compile-time constant or expression resulting in int
- Semicolon required
- Optional initializer with `{...}` syntax

## Declaration Examples

```ras
arr{int, 10} numbers;           // Array of 10 uninitialized ints
arr{str, 5} names;              // Array of 5 strings
arr{deci, 100} temperature;     // Array of 100 decimals
arr{bool, 8} flags = {true, false, true};
```

## Array Indexing

**Syntax:**
```
array_name{index}
```

**Get element:**
```ras
arr{int, 5} arr;
arr{0} = 42;
int first = arr{0};             // Access first element
show[arr{3}];                   // Access fourth element
```

**Zero-based indexing:**
```ras
arr{int, 5} arr = {10, 20, 30, 40, 50};
arr{0} = 10;                    // First element
arr{4} = 50;                    // Last element (size - 1)
arr{5} = 99;                    // ERROR: Out of bounds!
```

## Array Assignment

**Assign to element:**
```ras
arr{int, 10} nums;
nums{0} = 5;
nums{1} = 10;
nums{2} = nums{0} + nums{1};    // 15
```

From parser.c parse_array_assign:

```ras
array_name{index} = value;
```

Array indices can be:
- Literals: `arr{0}`, `arr{5}`
- Variables: `arr{i}`, `arr{counter}`
- Expressions: `arr{i + 1}`, `arr{size - 1}`

```ras
arr{int, 10} arr;
int idx = 3;
arr{idx} = 100;
arr{idx + 1} = 200;
arr{idx * 2} = 300;
```

## Array Initialization

**On declaration:**
```ras
arr{int, 5} primes = {2, 3, 5, 7, 11};
arr{str, 3} colors = {"red", "green", "blue"};
arr{deci, 2} coords = {3.14, 2.71};
```

**Multiple statements:**
```ras
arr{int, 3} nums;
nums{0} = 10;
nums{1} = 20;
nums{2} = 30;
```

**From expressions:**
```ras
arr{int, 4} results;
results{0} = 5 + 3;
results{1} = 10 * 2;
```

## Array Size

Size MUST be known at compile time:

```ras
const SIZE = 100;
arr{int, SIZE} arr;             // ✓ OK: constant

arr{int, 10 * 2} arr2;          // ✓ OK: compile-time expression

int runtime_size = 50;
arr{int, runtime_size} arr3;    // ❓ Unclear if OK - probably not
```

To get array size at runtime: **No built-in `@len` for arrays** — track size yourself:

```ras
const ARR_SIZE = 10;
arr{int, ARR_SIZE} arr;

fnc sum_arr[]::int {
    int total = 0;
    loop[int i = 0; i < ARR_SIZE; i++] {
        total = total + arr{i};
    }
    get[total];
}
```

## Looping Through Arrays

```ras
arr{int, 5} numbers = {1, 2, 3, 4, 5};

// With loop statement (for)
loop[int i = 0; i < 5; i++] {
    show[numbers{i}];
}

// With while
int i = 0;
while[i < 5] {
    show[numbers{i}];
    i++;
}
```

## Arrays of Groups

Arrays can contain custom types:

```ras
group Point {
    int x;
    int y;
}

fnc main[]::int {
    arr{Point, 3} points;
    
    points{0}.x = 10;
    points{0}.y = 20;
    
    points{1}.x = 30;
    points{1}.y = 40;
    
    show[points{0}.x];  // Prints 10
    get[0];
}
```

Syntax: `array{index}.member` for element member access.

## Multi-dimensional Arrays

**Not directly supported**, but can simulate:

```ras
// Simulate 2D array (5x10)
// Flatten: index = row * COLS + col
const ROWS = 5;
const COLS = 10;
const SIZE = ROWS * COLS;  // 50

arr{int, SIZE} matrix;

fnc set_element[row, col, value]::int {
    int flat_idx = (row * COLS) + col;
    matrix{flat_idx} = value;
    get[0];
}

fnc get_element[row, col]::int {
    int flat_idx = (row * COLS) + col;
    get[matrix{flat_idx}];
}
```

## Array Parameters

```ras
// Arrays as function parameters - tricky, not directly supported
// Must pass individual elements or handle manually
```

RASLang doesn't have array parameters in function declarations. Pass individual elements:

```ras
fnc process_element[val]::int {
    val = val * 2;
    get[val];
}

fnc main[]::int {
    arr{int, 5} nums = {1, 2, 3, 4, 5};
    nums{0} = process_element[nums{0}];
    get[0];
}
```

Or pass pointer-like value (address):
```ras
fnc modify_at_addr[addr, index, value]::int {
    @poke[addr + index, value];
    get[0];
}
```

## Common Array Patterns

**Finding a value:**
```ras
arr{int, 5} arr = {1, 2, 3, 4, 5};
int target = 3;
int found = -1;

loop[int i = 0; i < 5; i++] {
    if[arr{i} == target] {
        found = i;
    }
}

if[found >= 0] {
    show["Found at index: "];
    show[found];
}
```

**Sum of elements:**
```ras
arr{int, 5} nums = {10, 20, 30, 40, 50};
int sum = 0;

loop[int i = 0; i < 5; i++] {
    sum = sum + nums{i};
}

show["Sum: "];
show[sum];  // 150
```

**Average:**
```ras
arr{deci, 4} values = {10.5, 20.3, 15.7, 8.2};
deci sum = 0.0;

loop[int i = 0; i < 4; i++] {
    sum = sum + values{i};
}

deci average = sum / 4.0;
show["Average: "];
show[average];  // 13.675
```

## Array Limitations

**No dynamic sizing:**
```ras
arr{int, 10} arr;
// Can't grow or shrink after declaration
```

If you need dynamic collections, use manual memory allocation + maps:

```ras
int size = 0;
map{int, int} indexed_map;      // Map from index to value

indexed_map->set[0, 100];
indexed_map->set[1, 200];
int val = indexed_map->get[0];  // 100
```

**No array copying:**
```ras
arr{int, 5} a = {1, 2, 3, 4, 5};
arr{int, 5} b = a;             // ERROR: No implicit copy
```

Copy manually:
```ras
loop[int i = 0; i < 5; i++] {
    b{i} = a{i};
}
```

**No slicing:**
```ras
arr{int, 10} arr;
arr{int, 5} slice = arr{2..6};  // Not supported
```

**No range operators:**
```ras
for(int i : arr) { }            // Not supported
```

## Memory Management

Arrays are stack-allocated:
```ras
fnc test[]::int {
    arr{int, 1000000} big;      // HUGE stack allocation!
    get[0];                     // Stack unwound
}
```

For very large arrays, use heap allocation + manual indexing:
```ras
const SIZE = 1000000;
int ptr = @alloc[SIZE * 4];     // 4 bytes per int

@poke[ptr + 0, 42];
@poke[ptr + 4, 100];

@free[ptr];
get[0];
```

## Related Documentation
- [03-DATA-TYPES.md](03-DATA-TYPES.md) — Type system
- [06-OPERATORS.md](06-OPERATORS.md) — Operators with arrays
- [11-LOOPS.md](11-LOOPS.md) — Iterating arrays
- [08-MAPS.md](08-MAPS.md) — Alternative: dynamic key-value storage
- [17-MEMORY-OPERATIONS.md](17-MEMORY-OPERATIONS.md) — Heap-allocated arrays
