# 10: Conditionals

## If Statement

**Syntax** (from parser.c parse_if):

```
if[condition] {
    statements;
}
```

**Simple Example:**
```ras
int x = 5;

if[x > 0] {
    show["Positive"];
}
```

## If-Else Chain

**Syntax:**
```
if[condition1] {
    body1;
} or[condition2] {
    body2;
} or {
    default_body;
}
```

**New: `else` alias for final block (no condition):**
```
if[condition1] {
    body1;
} or[condition2] {
    body2;
} else {
    default_body;
}
```

The keyword `or` creates else-if and else branches. `else` can be used as an alias for the final unconditional `or`:

```ras
int score = 75;

if[score >= 90] {
    show["Grade: A"];
} or[score >= 80] {
    show["Grade: B"];
} or[score >= 70] {
    show["Grade: C"];
} else {
    show["Grade: F"];
}
```

Both forms are equivalent - choose whichever is more readable:

```ras
// Traditional: final 'or' without condition
if[x < 0] {
    show["Negative"];
} or {
    show["Non-negative"];
}

// New: 'else' for final block
if[x < 0] {
    show["Negative"];
} else {
    show["Non-negative"];
}
```

**Deep nesting:**
```ras
if[age < 13] {
    show["Child"];
} or[age < 18] {
    show["Teen"];
} or[age < 65] {
    show["Adult"];
} else {
    show["Senior"];
}
```

## Conditions

**Boolean expressions:**
```ras
if[flag] { }
if[!flag] { }
if[x > 5] { }
if[x <= 10] { }
if[x == 5] { }
if[x != 5] { }
```

**Logical operators:**
```ras
if[x > 0 && y < 100] {
    show["Both conditions met"];
}

if[x < 0 || y > 100] {
    show["At least one is true"];
}

if[!(x == 5)] {
    show["Not equal to 5"];
}
```

**Short-circuit evaluation:**
```ras
if[x > 0 && arr{x} != 0] {
    // arr{x} only accessed if x > 0
}

if[found || expensive_check[]] {
    // expensive_check[] only called if found is false
}
```

## Nested Conditions

```ras
if[age >= 18] {
    if[has_license] {
        show["Can drive"];
    } or {
        show["Need license"];
    }
} or {
    show["Too young"];
}
```

Cleaner: use logical operators
```ras
if[age >= 18 && has_license] {
    show["Can drive"];
}
```

## Ternary Operator

**Syntax** (from parser.c parse_ternary):
```
condition ? value_if_true : value_if_false
```

**Examples:**
```ras
int abs_x = x > 0 ? x : -x;

str status = active ? "Online" : "Offline";

int result = score >= 60 ? 1 : 0;
```

**Chaining:**
```ras
str grade = score >= 90 ? "A" :
            score >= 80 ? "B" :
            score >= 70 ? "C" :
            score >= 60 ? "D" : "F";
```

**In assignments:**
```ras
i = condition ? 10 : 20;

show[flag ? "yes" : "no"];

arr{0} = max_val > 100 ? 100 : max_val;  // Clamp value
```

## Type Checking Conditions

Conditions must be boolean or something that converts to boolean:

```ras
if[x] {                         // ERROR: x is int, not bool
    show["x"];
}

if[x != 0] {                    // ✓ Condition is bool
    show["x is not zero"];
}
```

## Multiple Conditions

**AND - All must be true:**
```ras
if[age >= 18 && has_id && passed_test] {
    show["Approved"];
}
```

**OR - At least one must be true:**
```ras
if[is_admin || is_owner || is_moderator] {
    show["Has privileges"];
}
```

## Comparison Operators in Conditions

| Operator | Meaning | Example |
|----------|---------|---------|
| `==` | Equal | `if[x == 5]` |
| `!=` | Not equal | `if[x != 5]` |
| `<` | Less than | `if[x < 10]` |
| `>` | Greater than | `if[x > 0]` |
| `<=` | Less or equal | `if[x <= 100]` |
| `>=` | Greater or equal | `if[x >= 0]` |

## Range Checking

Check if value is in range:

```ras
if[x >= 0 && x < 100] {
    show["In range"];
}

if[x >= -10 && x <= 10] {
    show["Around zero"];
}
```

Or use operators directly:

```ras
if[x > min && x < max] {
    show["Within bounds"];
}
```

## Checking Collections

**Array element existence:**
```ras
arr{int, 10} arr;

if[arr{0} != 0] {
    show["First element is set"];
}
```

**Map key existence:**
```ras
map{str, int} ages;

if[ages->has["Alice"]] {
    show["Alice is tracked"];
}
```

## Common Predicates

**Is null/empty:**
```ras
if[@len[s] == 0] {
    show["String is empty"];
}

// For maps, check manually
if[map->has[key]] {
    show["Key exists"];
}
```

**Is non-negative:**
```ras
if[x >= 0] {
    show["Non-negative"];
}
```

**Is between values:**
```ras
if[x > min && x < max] {
    show["Between"];
}
```

**All conditions met:**
```ras
if[a && b && c && d] {
    show["All true"];
}
```

**Any condition met:**
```ras
if[a || b || c || d] {
    show["At least one true"];
}
```

## Side Effects in Conditions

**Functions in conditions** (evaluated during condition check):

```ras
if[expensive_check[]] {         // Function called during if evaluation
    show["Check passed"];
}

if[process[x] > 0] {            // process[x] called, result compared
    show["Success"];
}
```

**Assignment in conditions** (probably ERROR):

```ras
if[(x = 5) > 0] {               // Unclear if supported
    show[x];
}
```

Avoid this; assign separately:

```ras
x = 5;
if[x > 0] {
    show[x];
}
```

## Empty Blocks

**Legal but not useful:**
```ras
if[condition] {
    // Empty block
}
```

## Patterns

**Guard clause:**
```ras
fnc process[x]::int {
    if[x < 0] {
        get[-1];                // Return early
    }
    show["Processing"];
    get[0];
}
```

**Switch-like with multiple ifs:**
```ras
int status = get_status[];

if[status == 0] {
    show["Success"];
} or[status == 1] {
    show["Warning"];
} or[status == 2] {
    show["Error"];
} or {
    show["Unknown"];
}
```

**Boolean variable:**
```ras
bool can_proceed = age >= 18 && has_permission;

if[can_proceed] {
    show["Proceeding"];
}
```

**Error checking:**
```ras
int result = operation[];

if[result < 0] {
    show["Error occurred"];
} or {
    show["Success"];
}
```

## Related Documentation
- [06-OPERATORS.md](06-OPERATORS.md) — Comparison and logical operators
- [12-TRY-CATCH.md](12-TRY-CATCH.md) — Exception handling (check/when)
- [11-LOOPS.md](11-LOOPS.md) — Loops with conditions

---

## Cycle Statement (Switch Statement)

**Syntax** (from parser.c parse_cycle):

```
cycle[expression] {
    when[value1]: { statements; }
    when[value2]: { statements; }
    fixed: { default_statements; }
}
```

**Components:**
- `cycle` keyword
- `[expression]` - The value to match against
- Multiple `when[case]: { body }` clauses
- Optional `fixed: { body }` default clause

**Important**: Unlike C's `switch`, each `when` clause acts like it has an implicit **break**. No fall-through behavior.

### Simple Switch

```ras
int day = 3;

cycle[day] {
    when[1]: {
        show["Monday"];
    }
    when[2]: {
        show["Tuesday"];
    }
    when[3]: {
        show["Wednesday"];
    }
    fixed: {
        show["Unknown day"];
    }
}
// Output: Wednesday
```

### with Default Case

```ras
str status = read_status[];

cycle[status] {
    when["online"]: {
        show["User is online"];
    }
    when["offline"]: {
        show["User is offline"];
    }
    when["away"]: {
        show["User is away"];
    }
    fixed: {
        show["Status unknown"];
    }
}
```

### No Fall-Through (Major Difference from C)

Each `when` clause STOPS after execution; there is no fall-through to the next case:

```ras
// C-style would fall through; RASLang does NOT
cycle[value] {
    when[1]: {
        show["Case 1"];
        // Automatically breaks here - does NOT continue to case 2
    }
    when[2]: {
        show["Case 2"];
    }
}
```

To handle multiple cases, use nested conditions:

```ras
cycle[value] {
    when[1]: {
        show["Case 1 or 2"];
    }
    when[2]: {
        show["Case 1 or 2"];
    }
}
// Use if/or for shared logic:
if[value == 1] {
    show["Case 1"];
} or[value == 2] {
    show["Case 1 or 2"];
}
```

### Cycle vs If/Or

**Use cycle when:**
- Matching against a single value with many specific cases
- Want cleaner code than multiple if/or chains
- Cases are simple value comparisons

```ras
cycle[user_role] {
    when["admin"]: { /* admin actions */ }
    when["moderator"]: { /* mod actions */ }
    when["user"]: { /* user actions */ }
    fixed: { /* deny access */ }
}
```

**Use if/or when:**
- Conditions are complex boolean expressions
- Need range checks or multiple conditions
- Cases overlap

```ras
if[age >= 18 && has_license] {
    show["Can drive"];
} or[age < 18 && has_permit] {
    show["Can drive with supervision"];
} or {
    show["Cannot drive"];
}
```

### Common Patterns

**String dispatch:**
```ras
str command = read_command[];

cycle[command] {
    when["help"]: {
        show["Available commands: help, status, quit"];
    }
    when["status"]: {
        show_system_status[];
    }
    when["quit"]: {
        @exit[0];
    }
    fixed: {
        show["Unknown command"];
    }
}
```

**Numeric dispatch:**
```ras
int response_code = get_response[];

cycle[response_code] {
    when[200]: { show["OK"]; }
    when[404]: { show["Not found"]; }
    when[500]: { show["Server error"]; }
    when[503]: { show["Service unavailable"]; }
    fixed: { show["Unknown status"]; }
}
```

**Enumeration handling:**
```ras
int state = get_state[];

cycle[state] {
    when[STATE_INIT]: {
        initialize[];
    }
    when[STATE_RUNNING]: {
        update[];
    }
    when[STATE_PAUSED]: {
        pause[];
    }
    when[STATE_STOPPED]: {
        cleanup[];
    }
}
```

### Edge Cases

**Empty case:**
```ras
cycle[value] {
    when[1]: {
        // Empty block allowed but might indicate error
    }
    fixed: {
        show["Default"];
    }
}
```

**No default clause (legal):**
```ras
cycle[value] {
    when["yes"]: { response = true; }
    when["no"]: { response = false; }
    // No fixed clause - does nothing if neither match
}
```

**Type matching:**
```ras
// cycle works with strings, ints, chars, etc.
char letter = read_letter[];

cycle[letter] {
    when['a']: { show["Vowel A"]; }
    when['e']: { show["Vowel E"]; }
    when['i']: { show["Vowel I"]; }
    when['o']: { show["Vowel O"]; }
    when['u']: { show["Vowel U"]; }
    fixed: { show["Consonant"]; }
}
```
