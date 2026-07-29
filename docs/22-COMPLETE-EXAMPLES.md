# 20: Complete Examples

Full working RASLang programs demonstrating language features.

## Example 1: Hello World

Simplest program.

```ras
fnc main[]::int {
    show["Hello World\n"];
    get[0];
}
```

**Run:** `rascom hello.ras && ./a.out`
**Output:** `Hello World`

## Example 2: Factorial (Recursion)

Demonstrates functions, recursion, conditionals.

```ras
fnc factorial[n]::int {
    if[n <= 1] {
        get[1];
    }
    get[n * factorial[n - 1]];
}

fnc main[]::int {
    int result = factorial[5];
    show["5! = "];
    show[result];
    show["\n"];
    get[0];
}
```

**Output:** `5! = 120`

## Example 3: Fibonacci (Loop)

Demonstrates loops, variables, operators.

```ras
fnc fib[n]::int {
    if[n <= 1] {
        get[n];
    }
    
    int a = 0;
    int b = 1;
    
    loop[int i = 2; i <= n; i++] {
        int temp = a + b;
        a = b;
        b = temp;
    }
    
    get[b];
}

fnc main[]::int {
    loop[int i = 0; i <= 10; i++] {
        show["F("];
        show[i];
        show[") = "];
        show[fib[i]];
        show["\n"];
    }
    get[0];
}
```

**Output:**
```
F(0) = 0
F(1) = 1
F(2) = 1
F(3) = 2
...
F(10) = 55
```

## Example 4: Prime Number Checker

Demonstrates loops, conditionals, functions.

```ras
fnc is_prime[n]::int {
    if[n < 2] {
        get[0];
    }
    
    loop[int i = 2; i * i <= n; i++] {
        if[n % i == 0] {
            get[0];
        }
    }
    
    get[1];
}

fnc main[]::int {
    loop[int num = 2; num <= 20; num++] {
        if[is_prime[num]] {
            show[num];
            show[" is prime\n"];
        }
    }
    get[0];
}
```

**Output:**
```
2 is prime
3 is prime
5 is prime
7 is prime
...
```

## Example 5: Sum Array Elements

Demonstrates arrays, loops, variables.

```ras
fnc sum_array[arr, size]::int {
    int sum = 0;
    loop[int i = 0; i < size; i++] {
        sum = sum + arr{i};
    }
    get[sum];
}

fnc main[]::int {
    arr{int, 5} numbers = {10, 20, 30, 40, 50};
    
    int total = sum_array[numbers, 5];
    show["Sum: "];
    show[total];
    show["\n"];
    
    get[0];
}
```

**Output:** `Sum: 150`

## Example 6: String Interpolation

Demonstrates string interpolation, variables.

```ras
fnc main[]::int {
    str name = "Alice";
    int age = 30;
    deci height = 5.7;
    
    show["Name: $name\n"];
    show["Age: $age\n"];
    show["Height: ${height} feet\n"];
    show["Next year: ${age + 1}\n"];
    
    get[0];
}
```

**Output:**
```
Name: Alice
Age: 30
Height: 5.7 feet
Next year: 31
```

## Example 7: Structures (Groups)

Demonstrates group definitions, fields, nesting.

```ras
group Point {
    int x;
    int y;
}

group Rectangle {
    Point top_left;
    Point bottom_right;
}

fnc distance[p1, p2]::deci {
    int dx = p1.x - p2.x;
    int dy = p1.y - p2.y;
    deci dist = @type[dx * dx + dy * dy]::deci;
    get[dist];
}

fnc main[]::int {
    Point p1;
    p1.x = 0;
    p1.y = 0;
    
    Point p2;
    p2.x = 3;
    p2.y = 4;
    
    deci dist = distance[p1, p2];
    show["Distance: "];
    show[dist];
    show["\n"];
    
    get[0];
}
```

**Output:** `Distance: 5`

## Example 8: Associative Array (Map)

Demonstrates maps, key-value operations.

```ras
fnc main[]::int {
    map{str, int} inventory;
    
    // Add items
    inventory->set["apple", 10];
    inventory->set["banana", 5];
    inventory->set["orange", 8];
    
    // Check and retrieve
    if[inventory->has["apple"]] {
        show["Apples: "];
        show[inventory->get["apple"]];
        show["\n"];
    }
    
    // Update
    inventory->set["apple", 12];
    show["Updated apples: "];
    show[inventory->get["apple"]];
    show["\n"];
    
    get[0];
}
```

**Output:**
```
Apples: 10
Updated apples: 12
```

## Example 9: Try-Catch Error Handling

Demonstrates check/when error handling.

```ras
fnc divide[a, b]::int {
    check {
        if[b == 0] {
            @panic["Division by zero"];
        }
        get[a / b];
    } when[SystemError]: {
        show["Math error occurred\n"];
        get[-1];
    }
}

fnc main[]::int {
    int result = divide[10, 2];
    show["10 / 2 = "];
    show[result];
    show["\n"];
    
    result = divide[10, 0];
    show["Should show error above\n"];
    
    get[0];
}
```

**Output:**
```
10 / 2 = 5
Math error occurred
Should show error above
```

## Example 10: Binary Search

Demonstrates loops, conditionals, arrays, algorithms.

```ras
fnc binary_search[arr, size, target]::int {
    int left = 0;
    int right = size - 1;
    
    loop[int ok = 1; ok == 1; ok = 0] {
        if[left > right] {
            ok = 0;
        } or {
            int mid = (left + right) / 2;
            int mid_val = arr{mid};
            
            if[mid_val == target] {
                ok = 0;
                get[mid];
            } or if[mid_val < target] {
                left = mid + 1;
            } or {
                right = mid - 1;
            }
        }
    }
    
    get[-1];
}

fnc main[]::int {
    arr{int, 10} sorted = {1, 3, 5, 7, 9, 11, 13, 15, 17, 19};
    
    int index = binary_search[sorted, 10, 7];
    show["Found at index: "];
    show[index];
    show["\n"];
    
    get[0];
}
```

**Output:** `Found at index: 3`

## Example 11: Word Frequency (Map + Strings)

Demonstrates maps, string operations, loops.

```ras
fnc main[]::int {
    map{str, int} freq;
    
    str words = "apple banana apple cherry banana apple";
    
    // Simple word parsing (manual for demo)
    freq->set["apple", 0];
    freq->set["banana", 0];
    freq->set["cherry", 0];
    
    freq->set["apple", freq->get["apple"] + 3];
    freq->set["banana", freq->get["banana"] + 2];
    freq->set["cherry", freq->get["cherry"] + 1];
    
    show["Word frequencies:\n"];
    show["apple: "];
    show[freq->get["apple"]];
    show["\n"];
    
    show["banana: "];
    show[freq->get["banana"]];
    show["\n"];
    
    show["cherry: "];
    show[freq->get["cherry"]];
    show["\n"];
    
    get[0];
}
```

**Output:**
```
Word frequencies:
apple: 3
banana: 2
cherry: 1
```

## Example 12: Memory Management

Demonstrates dynamic memory, allocation, deallocation.

```ras
fnc create_buffer[size]::int {
    int ptr = @alloc[size];
    if[ptr == 0] {
        show["Memory allocation failed\n"];
        get[0];
    }
    
    // Initialize with pattern
    @memset[ptr, 42, size];
    
    get[ptr];
}

fnc main[]::int {
    int buf = create_buffer[256];
    
    if[buf > 0] {
        // Write value
        @poke[buf, 100];
        
        // Read value
        int val = @peek[buf];
        show["Value: "];
        show[val];
        show["\n"];
        
        // Cleanup
        @free[buf];
    }
    
    get[0];
}
```

**Output:** `Value: 100`

## Example 13: Multithreading

Demonstrates thread creation, joining, shared data.

```ras
int counter = 0;
int counter_lock = @mutex_create[];

fnc increment[times]::int {
    loop[int i = 0; i < times; i++] {
        @mutex_lock[counter_lock];
        counter++;
        @mutex_unlock[counter_lock];
    }
    get[0];
}

fnc main[]::int {
    int t1 = @spawn[increment, 100];
    int t2 = @spawn[increment, 100];
    
    int r1 = @join[t1];
    int r2 = @join[t2];
    
    show["Final counter: "];
    show[counter];
    show["\n"];
    
    get[0];
}
```

**Output:** `Final counter: 200`

## Example 14: Thread Pool

Demonstrates thread pools, work distribution.

```ras
fnc square[x]::int {
    show["Square of "];
    show[x];
    show[" = "];
    show[x * x];
    show["\n"];
    get[x * x];
}

fnc main[]::int {
    int pool = @pool_create[4];
    
    loop[int i = 1; i <= 10; i++] {
        @pool_submit[pool, square, i];
    }
    
    @pool_wait[pool];
    @pool_destroy[pool];
    
    show["All tasks completed\n"];
    get[0];
}
```

**Output:**
```
Square of 1 = 1
Square of 2 = 4
...
Square of 10 = 100
All tasks completed
```

## Example 15: Channels (Producer-Consumer)

Demonstrates channel communication between threads.

```ras
int log_channel;

fnc producer[num_items]::int {
    loop[int i = 1; i <= num_items; i++] {
        @channel_send[log_channel, i];
        @sleep[50];
    }
    get[0];
}

fnc consumer[num_items]::int {
    loop[int i = 0; i < num_items; i++] {
        int item = @channel_recv[log_channel];
        show["Consumed: "];
        show[item];
        show["\n"];
        @sleep[75];
    }
    get[0];
}

fnc main[]::int {
    log_channel = @channel_create[5];
    
    int p = @spawn[producer, 5];
    int c = @spawn[consumer, 5];
    
    @join[p];
    @join[c];
    @channel_close[log_channel];
    
    get[0];
}
```

**Output:**
```
Consumed: 1
Consumed: 2
...
Consumed: 5
```

## Example 16: File Writing

Demonstrates file I/O, file operations.

```ras
fnc write_sum[filename]::int {
    int fd = @fopen[filename, "w"];
    
    check {
        if[fd == 0] {
            @panic["Cannot open file"];
        }
        
        // Write data
        int sum = 0;
        loop[int i = 1; i <= 10; i++] {
            sum = sum + i;
        }
        
        str output = @concat["Sum of 1-10: ", @type[sum]::str, "\n"];
        @fwrite[fd, output, @len[output]];
        
        @fclose[fd];
        get[0];
        
    } when: {
        show["File write error\n"];
        get[-1];
    }
}

fnc main[]::int {
    write_sum["output.txt"];
    show["Data written to output.txt\n"];
    get[0];
}
```

**Output:** `Data written to output.txt` (file contains sum)

## Example 17: Type Conversion

Demonstrates @type builtin for conversions.

```ras
fnc main[]::int {
    // String to int
    str num_str = "42";
    int num = @type[num_str]::int;
    show["Parsed: "];
    show[num];
    show["\n"];
    
    // Int to string
    int val = 100;
    str val_str = @type[val]::str;
    show["String: "];
    show[val_str];
    show["\n"];
    
    // Bool to string
    bool flag = true;
    str flag_str = @type[flag]::str;
    show["Bool as string: "];
    show[flag_str];
    show["\n"];
    
    get[0];
}
```

**Output:**
```
Parsed: 42
String: 100
Bool as string: true
```

## Example 18: Character Operations

Demonstrates character handling, ASCII conversion.

```ras
fnc main[]::int {
    // Character to ASCII
    char ch = 'A';
    int ascii = @ord[ch];
    show["'A' = "];
    show[ascii];
    show["\n"];
    
    // ASCII to character
    char ch2 = @chr[65];
    show["65 = '"];
    show[ch2];
    show["'\n"];
    
    // Iterate ASCII
    loop[int i = 65; i <= 90; i++] {
        if[i % 10 == 0] {
            show["\n"];
        }
        show[@chr[i]];
        show[" "];
    }
    show["\n"];
    
    get[0];
}
```

**Output:**
```
'A' = 65
65 = 'A'
A B C D E F G H I J 
K L M N O P Q R S T 
U V W X Y Z 
```

## Example 19: Sorting (Bubble Sort)

Demonstrates algorithm implementation with arrays.

```ras
fnc bubble_sort[arr, len]::int {
    loop[int i = 0; i < len; i++] {
        loop[int j = 0; j < len - i - 1; j++] {
            if[arr{j} > arr{j + 1}] {
                // Swap
                int temp = arr{j};
                arr{j} = arr{j + 1};
                arr{j + 1} = temp;
            }
        }
    }
    get[0];
}

fnc main[]::int {
    arr{int, 6} numbers = {64, 34, 25, 12, 22, 11};
    
    show["Before: "];
    loop[int i = 0; i < 6; i++] {
        show[numbers{i}];
        show[" "];
    }
    show["\n"];
    
    bubble_sort[numbers, 6];
    
    show["After: "];
    loop[int i = 0; i < 6; i++] {
        show[numbers{i}];
        show[" "];
    }
    show["\n"];
    
    get[0];
}
```

**Output:**
```
Before: 64 34 25 12 22 11 
After: 11 12 22 25 34 64 
```

## Example 20: Configuration Reader

Demonstrates practical usage with constants and maps.

```ras
const CONFIG_VERSION = 1;

group Config {
    str app_name;
    int max_connections;
    int timeout_ms;
    bool debug_mode;
}

fnc load_config[]::int {
    Config cfg;
    cfg.app_name = "MyApp";
    cfg.max_connections = 100;
    cfg.timeout_ms = 5000;
    cfg.debug_mode = true;
    
    show["=== Configuration v"];
    show[CONFIG_VERSION];
    show[" ===\n"];
    
    show["App: "];
    show[cfg.app_name];
    show["\n"];
    
    show["Max Connections: "];
    show[cfg.max_connections];
    show["\n"];
    
    show["Timeout: "];
    show[cfg.timeout_ms];
    show["ms\n"];
    
    show["Debug: "];
    show[cfg.debug_mode];
    show["\n"];
    
    get[0];
}

fnc main[]::int {
    load_config[];
    get[0];
}
```

**Output:**
```
=== Configuration v1 ===
App: MyApp
Max Connections: 100
Timeout: 5000ms
Debug: true
```

## Running Examples

**Compile:**
```
rascom example.ras -o example
```

**Execute:**
```
./example
```

**Compile and run:**
```
rascom source.ras && ./a.out
rascom source.ras -o myapp && ./myapp
```

## Common Patterns Used in Examples

1. **Recursion:** Factorial, functions calling themselves
2. **Iteration:** Loops for arrays, ranges
3. **Conditionals:** if/or chains for branching
4. **Data Structures:** Groups, maps, arrays for organization
5. **Memory:** Allocation/deallocation, cleanup
6. **Concurrency:** Threads, mutexes, channels
7. **File I/O:** Reading/writing files
8. **Error Handling:** Check/when for exceptions
9. **String Manipulation:** Interpolation, concatenation, conversion
10. **Algorithms:** Sorting, searching, mathematical sequences

## Related Documentation
- [01-PROGRAM-STRUCTURE.md](01-PROGRAM-STRUCTURE.md) — Program layout
- [15-IO-STATEMENTS.md](15-IO-STATEMENTS.md) — I/O operations
- [16-BUILTINS.md](16-BUILTINS.md) — Builtin functions
- [18-BUILTINS-STRING-MATH.md](18-BUILTINS-STRING-MATH.md) — String & math operations
- [19-BUILTINS-ADVANCED.md](19-BUILTINS-ADVANCED.md) — Error handling & processes
- [17-MEMORY-OPERATIONS.md](17-MEMORY-OPERATIONS.md) — Memory management
- [20-CONCURRENCY.md](20-CONCURRENCY.md) — Threading
- [21-FILE-NETWORK.md](21-FILE-NETWORK.md) — File/network I/O
