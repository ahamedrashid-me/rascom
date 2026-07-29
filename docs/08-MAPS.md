# 08: Maps

## Map Declaration

**Syntax** (from parser.c parse_map_decl):

```
map{key_type, value_type} map_name;
```

**Rules:**
- Key type: primitive type only (`int`, `str`, `deci`, `bool`, `char`, `byte`, `ubyte`)
- Value type: any type (primitive or group)
- Semicolon required
- Maps are hash tables, not ordered collections

## Declaration Examples

```ras
map{str, int} ages;             // String keys → Integer values
map{int, str} names;            // Integer keys → String values
map{str, deci} prices;          // String keys → Decimal values
```

## Map Operations

### Set Operation: `map->set[key, value]`

**Syntax:**
```ras
map_name->set[key, value];
```

Add or update an entry:

```ras
map{str, int} ages;
ages->set["Alice", 30];
ages->set["Bob", 25];
ages->set["Alice", 31];         // Updates Alice's age
```

**Type checking:**
- Key must match map's key type
- Value must match map's value type

```ras
map{str, int} scores;
scores->set["Player1", 100];    // ✓ OK
scores->set["Player2", "high"]; // ERROR: Value should be int
scores->set[1, 50];             // ERROR: Key should be str
```

### Get Operation: `map->get[key]`

**Syntax:**
```ras
result = map_name->get[key];
```

Retrieve value associated with key:

```ras
map{str, int} ages;
ages->set["Alice", 30];

int alice_age = ages->get["Alice"];
show[alice_age];                // Prints 30
```

**On missing key:** Returns default value for type (0, 0.0, false, "", etc.)

```ras
map{str, int} map;
int missing = map->get["NotThere"];  // Returns 0 (default int)
```

### Has Operation: `map->has[key]`

**Syntax:**
```ras
if[map_name->has[key]] { ... }
```

Check if key exists:

```ras
map{str, int} ages;
ages->set["Alice", 30];

if[ages->has["Alice"]] {
    show["Alice found"];
}

if[!ages->has["Bob"]] {
    show["Bob not in map"];
}
```

### Remove Operation: `map->remove[key]`

**Syntax:**
```ras
map_name->remove[key];
```

Delete an entry:

```ras
map{str, int} ages;
ages->set["Alice", 30];
ages->set["Bob", 25];

ages->remove["Alice"];

if[ages->has["Alice"]] {
    show["Found"];
} or {
    show["Removed"];           // This prints
}
```

## Map Use Cases

**Counting occurrences:**
```ras
map{str, int} word_count;

fnc add_word[word]::int {
    if[word_count->has[word]] {
        word_count->set[word, word_count->get[word] + 1];
    } or {
        word_count->set[word, 1];
    }
    get[0];
}
```

**Configuration storage:**
```ras
map{str, str} config;
config->set["version", "1.0"];
config->set["author", "Me"];
config->set["license", "MIT"];

show[config->get["version"]];   // Prints "1.0"
```

**Cached lookups:**
```ras
map{int, str} user_cache;
user_cache->set[1001, "Alice"];
user_cache->set[1002, "Bob"];

str user = user_cache->get[1001]; // Cached lookup
```

**Tracking state:**
```ras
map{str, bool} flags;
flags->set["debug", true];
flags->set["verbose", false];
flags->set["unsafe", true];

if[flags->has["debug"] && flags->get["debug"]] {
    show["Debug mode enabled"];
}
```

## Maps of Complex Types

Values can be groups:

```ras
group Person {
    str name;
    int age;
}

fnc main[]::int {
    map{int, Person} directory;
    
    Person p1;
    p1.name = "Alice";
    p1.age = 30;
    directory->set[1001, p1];
    
    Person retrieved = directory->get[1001];
    show[retrieved.name];       // Prints "Alice"
    get[0];
}
```

## Map Iteration

**No built-in iteration** — must track keys manually:

```ras
map{str, int} ages;
ages->set["Alice", 30];
ages->set["Bob", 25];
ages->set["Carol", 28];

// Must manually track keys in another structure
arr{str, 3} names = {"Alice", "Bob", "Carol"};

loop[int i = 0; i < 3; i++] {
    str key = names{i};
    int val = ages->get[key];
    show[key];
    show[val];
}
```

Or use has/get pattern with known keys:
```ras
if[ages->has["Alice"]] {
    show[ages->get["Alice"]];
}
if[ages->has["Bob"]] {
    show[ages->get["Bob"]];
}
```

## Map Limitations

**No iteration over all entries:**
```ras
for(key, value : map) { }       // Not supported
```

**Keys must be hashable types:**
```ras
map{Point, str} map;            // ERROR: Can't use group as key
```

Only primitives can be keys.

**No nested maps directly:**
```ras
map{str, map{int, str}} nested; // Probably ERROR
```

Workaround: Use group containing map or use custom indices.

**No map-of-maps without work:**

Use array of maps or encoded keys:
```ras
// Option 1: Array of maps
arr{map{str, int}, 10} maps_array;

// Option 2: Composite key (string concatenation)
map{str, int} compound_map;
compound_map->set["map1:key1", 100];
compound_map->set["map2:key1", 200];
```

## Default Map Behavior

Maps are created with initial empty state:

```ras
map{str, int} map;              // Created empty

show[map->has["any_key"]];      // false (no entries)
int val = map->get["empty"];    // Returns 0 (default int)
```

## Memory

Maps use dynamic memory (hash table implementation):

```ras
map{str, deci} large_map;

// Can handle many entries
loop[int i = 0; i < 10000; i++] {
    str key = @concat["key_", @type[i]::str];
    large_map->set[key, 3.14];
}
```

Removing entries frees memory associated with that key.

## Common Pattern: Config Storage

```ras
fnc main[]::int {
    map{str, str} settings;
    settings->set["debug", "false"];
    settings->set["output", "verbose"];
    settings->set["timeout", "3000"];
    
    if[settings->get["debug"] == "true"] {
        @panic["Debug: Config loaded"];
    }
    
    get[0];
}
```

## Comparison with Arrays

| Aspect | Arrays | Maps |
|--------|--------|------|
| Indexing | By position (0, 1, 2, ...) | By key (any type) |
| Size | Fixed at declaration | Dynamic |
| Ordering | Ordered | Unordered |
| Speed | O(1) access | O(1) average |
| Memory | Stack-allocated | Heap-allocated |

Choose:
- **Arrays** for ordered collections with known size
- **Maps** for key-value lookups and dynamic collections

## Related Documentation
- [03-DATA-TYPES.md](03-DATA-TYPES.md) — Type system
- [07-ARRAYS.md](07-ARRAYS.md) — Array alternative
- [09-GROUPS.md](09-GROUPS.md) — Storing groups in maps
- [16-BUILTINS.md](16-BUILTINS.md) — Builtin functions for keys/values
