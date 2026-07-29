# 11: Loops

## While Loop

**Syntax** (from parser.c parse_while):

```
while[condition] {
    statements;
}
```

**Simple loop:**
```ras
int i = 0;

while[i < 5] {
    show[i];
    i++;
}
```

**Condition checked before each iteration:**
- If condition is false, loop doesn't execute
- Loop continues as long as condition is true

```ras
int count = 0;

while[count < 3] {
    show["Count: "];
    show[count];
    count++;
}
// Output: Count: 0, Count: 1, Count: 2
```

**Infinite loop (be careful!):**
```ras
while[true] {
    show["Looping"];
    // Must break out somehow
}
```

## Loop Statement (For Loop)

**Syntax** (from parser.c parse_loop):

```
loop[init; condition; increment] {
    statements;
}
```

**Traditional for loop:**
```ras
loop[int i = 0; i < 5; i++] {
    show[i];
}
```

**Breakdown:**
1. `int i = 0;` — Initialization (executed once)
2. `i < 5;` — Condition (checked before each iteration)
3. `i++` — Increment (executed after each iteration)

**Iterate through array:**
```ras
arr{int, 5} nums = {10, 20, 30, 40, 50};

loop[int i = 0; i < 5; i++] {
    show[nums{i}];
}
```

**Decrement loop:**
```ras
loop[int i = 5; i > 0; i--] {
    show[i];
}
// Output: 5, 4, 3, 2, 1
```

**Increment by 2:**
```ras
loop[int i = 0; i < 10; i++] {
    i = i + 1;              // Skip every other (complicated)
}

// Better: just do it in increment
loop[int i = 0; i < 10; i = i + 2] {
    show[i];
}
// Output: 0, 2, 4, 6, 8
```

## Loop Components

### Initialization

Can be:
- Variable declaration: `int i = 0`
- Or not used: `; condition; increment;` (uncommon)

```ras
// Standard
loop[int i = 0; i < 10; i++] { }

// Multiple declarations? Not supported by parser
loop[int i = 0, int j = 0; i < 10; i++] { }  // ERROR
```

### Condition

Boolean expression evaluated before each iteration:

```ras
loop[int i = 0; i < 10; i++] { }         // While i < 10
loop[int i = 10; i > 0; i--] { }         // While i > 0
loop[int i = 0; i < arr_size; i++] { }   // While i < arr_size
```

If condition is initially false, loop body never executes:

```ras
loop[int i = 10; i < 5; i++] {
    show["Never"]           // Never prints
}
```

### Increment

Executed after each iteration. Options:

**`i++`** or **`i--`** (implicit add/sub 1):
```ras
loop[int i = 0; i < 5; i++] { }
loop[int i = 5; i > 0; i--] { }
```

**`i = i + expr`** (explicit arithmetic):
```ras
loop[int i = 0; i < 100; i = i + 10] { }  // Step by 10
loop[int i = 20; i > 0; i = i - 3] { }    // Decrement by 3
```

Note: `i += 10` NOT supported; must use `i = i + 10`

## Loop Control

### Breaking from a loop

No explicit `break` statement in RASLang. Use flag:

```ras
bool found = false;
int target = 42;

loop[int i = 0; i < 100 && !found; i++] {
    if[arr{i} == target] {
        show["Found at"];
        show[i];
        found = true;
    }
}
```

### Continuing to next iteration

No explicit `continue` statement. Use if/else:

```ras
loop[int i = 0; i < 10; i++] {
    if[i == 5] {
        // Skip this iteration
        // (implicitly continues to i++)
    } or {
        show[i];            // Show only non-5 values
    }
}
```

## Nested Loops

```ras
// 2D iteration
loop[int i = 0; i < 3; i++] {
    loop[int j = 0; j < 4; j++] {
        show["("];
        show[i];
        show[","];
        show[j];
        show[")"];
    }
}
```

**Array iteration:**
```ras
arr{Point, 5} points;

loop[int i = 0; i < 5; i++] {
    show[points{i}.x];
    show[points{i}.y];
}
```

## Loop Patterns

**Sum all elements:**
```ras
arr{int, 5} nums = {1, 2, 3, 4, 5};
int total = 0;

loop[int i = 0; i < 5; i++] {
    total = total + nums{i};
}

show[total];  // 15
```

**Find maximum:**
```ras
arr{int, 5} nums = {3, 1, 4, 1, 5};
int max = nums{0};

loop[int i = 1; i < 5; i++] {
    if[nums{i} > max] {
        max = nums{i};
    }
}

show[max];  // 5
```

**Count occurrences:**
```ras
arr{str, 6} colors = {"red", "blue", "red", "green", "red", "blue"};
int red_count = 0;

loop[int i = 0; i < 6; i++] {
    if[colors{i} == "red"] {
        red_count++;
    }
}

show[red_count];  // 3
```

**Process map entries:**
```ras
map{str, int} scores;
scores->set["Alice", 100];
scores->set["Bob", 85];
scores->set["Carol", 92];

// Manual iteration (must track keys)
arr{str, 3} names = {"Alice", "Bob", "Carol"};

loop[int i = 0; i < 3; i++] {
    str name = names{i};
    int score = scores->get[name];
    show[name];
    show[score];
}
```

## While vs Loop

**Use while when:**
- Condition is complex
- Don't know iteration count upfront
- Exit condition emerges during loop

```ras
int x = 1;
while[x < 1000] {
    x = x * 2;              // Repeat until >= 1000
}
show[x];  // 1024
```

**Use loop when:**
- Know exact iteration count
- Traditional for-loop pattern
- Iterating arrays/fixed ranges

```ras
loop[int i = 0; i < array_size; i++] {
    // Process array[i]
}
```

## Cycle Statement (Switch Statement)

The `cycle` statement is a **switch-like control structure** for value matching. It is documented in detail in [10-CONDITIONALS.md - Cycle Statement](10-CONDITIONALS.md#cycle-statement-switch-statement).

**Quick syntax:**
```
cycle[expression] {
    when[value1]: { body; }
    when[value2]: { body; }
    fixed: { default_body; }
}
```

**Key point**: Unlike C `switch`, each `when` clause has an implicit **break** — no fall-through.

## Common Mistakes

**Off-by-one errors:**
```ras
arr{int, 5} arr;

// ERROR: i < 5 correct, i <= 5 would be out of bounds
loop[int i = 0; i <= 5; i++] {  // BAD: Tries to access arr{5}
    arr{i} = i;
}

// CORRECT
loop[int i = 0; i < 5; i++] {
    arr{i} = i;
}
```

**Infinite loops:**
```ras
loop[int i = 0; i < 10; ] {    // No increment
    show[i];
    // i never changes, loops forever
}

// CORRECT
loop[int i = 0; i < 10; i++] {
    show[i];
}
```

**Modifying loop variable incorrectly:**
```ras
loop[int i = 0; i < 5; i++] {
    i = 0;                  // Reset i each iteration!
    // Infinite loop
}
```

## Performance

Loops are compiled to efficient code:

```ras
loop[int i = 0; i < 1000000; i++] {
    // Runs in compiled assembly, fast
}
```

Nested loops multiply iterations:

```ras
loop[int i = 0; i < 1000; i++] {
    loop[int j = 0; j < 1000; j++] {
        // 1,000,000 iterations total
    }
}
```

Be aware of performance implications with large bounds.

## Related Documentation
- [10-CONDITIONALS.md](10-CONDITIONALS.md) — Conditions in loops
- [06-OPERATORS.md](06-OPERATORS.md) — Increment/decrement operators
- [07-ARRAYS.md](07-ARRAYS.md) — Iterating arrays
- [08-MAPS.md](08-MAPS.md) — Iterating maps
