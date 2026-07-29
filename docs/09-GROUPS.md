# 09: Groups

## Group Definition

**Syntax** (from parser.c parse_group):

```
group GroupName {
    type field1;
    type field2;
    [grouptype field3;]
}
```

**Rules:**
- Name is typically PascalCase (convention)
- Fields are declared like variables
- Field type: any primitive OR another group name
- Semicolon after each field required
- No methods inside groups (functions are separate)
- No default values for fields

## Definition Examples

```ras
group Point {
    int x;
    int y;
}

group User {
    str username;
    str email;
    int age;
    bool active;
}

group Address {
    str street;
    str city;
    str zipcode;
}

group Person {
    str name;
    int age;
    Address home;           // Nested group
}
```

## Creating Group Instances

**Declaration:**
```
GroupName variable_name;
```

**Examples:**
```ras
Point p;
User user;
Person person;
```

**Scope:** Group instances follow variable scoping rules:

```ras
fnc process[]::int {
    Point p;                    // Local to process()
    p.x = 10;
    p.y = 20;
    get[0];
}

// p is out of scope here
```

## Accessing Fields

**Syntax:**
```
instance.field_name
```

**Examples:**
```ras
Point p;
p.x = 5;
p.y = 10;

int px = p.x;               // Read field
show[p.y];
```

## Setting Fields

```ras
User user;
user.username = "alice";
user.email = "alice@example.com";
user.age = 30;
user.active = true;

show[user.username];        // Prints "alice"
```

## Nested Groups

**Definition:**
```ras
group Address {
    str street;
    str city;
}

group Person {
    str name;
    Address home;
}
```

**Access nested fields:**
```ras
Person p;
p.name = "Bob";
p.home.street = "123 Main St";
p.home.city = "NYC";

show[p.home.city];          // Prints "NYC"
```

**Multiple levels:**
```ras
Address addr = p.home;      // Get nested group
addr.street = "456 Oak Ave";
```

## Groups in Functions

**As parameters:**
```ras
fnc describe_person[person]::int {
    show[person.name];
    show[" is ";
    show[person.age];
    get[0];
}

fnc main[]::int {
    Person p;
    p.name = "Alice";
    p.age = 28;
    describe_person[p];
    get[0];
}
```

Note: Parameters pass by value, so modifications in the function don't affect caller.

```ras
fnc modify[person]::int {
    person.name = "Changed";
    get[0];
}

fnc main[]::int {
    Person p;
    p.name = "Original";
    modify[p];
    show[p.name];           // Still "Original" (copy passed)
    get[0];
}
```

**Return groups:**
```ras
fnc create_point[x, y]::Point {
    Point p;
    p.x = x;
    p.y = y;
    get[p];
}

fnc main[]::int {
    Point p = create_point[10, 20];
    show[p.x];              // Prints 10
    get[0];
}
```

## Assignment Between Groups

**Copy non-nested fields:**
```ras
User u1;
u1.username = "alice";
u1.age = 30;

User u2;
u2.username = u1.username;     // Copy field by field
u2.age = u1.age;

show[u2.username];              // "alice"
```

**No direct group assignment:**
```ras
User u1;
u1.username = "alice";

User u2 = u1;                   // Probably ERROR: No implicit copy
```

## Groups in Arrays

```ras
group Point {
    int x;
    int y;
}

fnc main[]::int {
    arr{Point, 3} points;
    
    points{0}.x = 0;
    points{0}.y = 0;
    
    points{1}.x = 10;
    points{1}.y = 20;
    
    points{2}.x = 30;
    points{2}.y = 40;
    
    show[points{1}.x];          // Prints 10
    show[points{1}.y];          // Prints 20
    get[0];
}
```

## Groups in Maps

```ras
group Person {
    str name;
    int age;
}

fnc main[]::int {
    map{str, Person} directory;
    
    Person p1;
    p1.name = "Alice";
    p1.age = 30;
    
    directory->set["alice123", p1];
    
    Person retrieved = directory->get["alice123"];
    show[retrieved.name];       // "Alice"
    show[retrieved.age];        // 30
    get[0];
}
```

## Member Assignment from Expressions

```ras
group Config {
    int port;
    deci timeout;
    str host;
}

fnc main[]::int {
    Config cfg;
    
    cfg.port = 8080 + 1;        // 8081
    cfg.timeout = 3.0 * 1.5;    // 4.5
    cfg.host = @concat["local", "host"];  // "localhost"
    
    get[0];
}
```

## Common Group Patterns

**Data bundle:**
```ras
group Rect {
    int x;
    int y;
    int width;
    int height;
}

fnc contains_point[rect, px, py]::bool {
    bool h_overlap = px >= rect.x && px < rect.x + rect.width;
    bool v_overlap = py >= rect.y && py < rect.y + rect.height;
    get[h_overlap && v_overlap];
}
```

**State container:**
```ras
group GameState {
    int score;
    int level;
    bool paused;
    str player_name;
}

fnc update_score[state, points]::int {
    state.score = state.score + points;
    get[0];
}
```

**Cache/Registry:**
```ras
group CacheEntry {
    str key;
    str value;
    int timestamp;
}

arr{CacheEntry, 100} cache;
int cache_count = 0;
```

## Default Values

Fields don't have default values — must be initialized manually:

```ras
Point p;
show[p.x];                      // Prints ??? (random/zero)
p.x = 0;                        // Now safe
```

Good practice: Initialize all fields:

```ras
fnc init_point[p, x, y]::int {
    p.x = x;
    p.y = y;
    get[0];
}

fnc main[]::int {
    Point p;
    init_point[p, 10, 20];      // Initialize explicitly
    get[0];
}
```

## No Methods

Groups can't contain functions:

```ras
group Point {
    int x;
    int y;
    
    fnc distance[]::deci {      // ERROR: Methods not allowed
        get[0.0];
    }
}
```

Use separate functions instead:

```ras
fnc distance[p]::deci {
    deci dx = @type[p.x]::deci;
    deci dy = @type[p.y]::deci;
    get[dx * dx + dy * dy];     // Simplified
}
```

## Size and Memory

Use `@sizeof` to get group size:

```ras
group Person {
    str name;                   // Dynamic
    int age;                    // 4 bytes
}

int size = @sizeof[Person];     // ???
```

Groups are typically stack-allocated:

```ras
fnc test[]::int {
    arr{Person, 1000} people;   // Many group instances on stack
    get[0];
}
```

For very large arrays of groups, consider heap allocation.

## Limitations

**No inheritance:**
```ras
group Animal {
    str name;
}

group Dog : Animal {            // ERROR: Not supported
    str breed;
}
```

**No private/public visibility:**
```ras
group Point {
    private int x;             // No access modifiers
}
```

All fields are public.

**No methods/behaviors attached:**
```ras
group Point {
    int x;
    int y;
    
    // Can't attach functions
}
```

## Related Documentation
- [01-PROGRAM-STRUCTURE.md](01-PROGRAM-STRUCTURE.md) — Group definitions
- [04-VARIABLES.md](04-VARIABLES.md) — Group instances as variables
- [13-FUNCTIONS.md](13-FUNCTIONS.md) — Functions with group parameters
- [07-ARRAYS.md](07-ARRAYS.md) — Arrays of groups
- [08-MAPS.md](08-MAPS.md) — Maps with groups as values
