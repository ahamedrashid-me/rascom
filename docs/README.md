That index is the old, over-claimed version. It does not match the honest README or the real compiler.

Here’s a rewritten **docs index** in the same tone as the official README — clear, accurate, and without Phase-9 marketing.

```markdown
# RasCode Language Reference

Documentation for the RasCode language and `rascom` compiler.

These docs are being aligned with the **actual** compiler behavior  
(lexer, parser, codegen, builtins) — not with planned or ideal features.

If a document and the compiler disagree, the compiler currently wins.  
That disagreement is treated as a bug.

**Status:** Pre-v0.1 — incomplete in places. Read with that in mind.

---

## Quick Navigation

### Part 1 — Fundamentals
- [01-INTRODUCTION.md](01-INTRODUCTION.md) — What RasCode is (and is not)
- [02-PROGRAM-STRUCTURE.md](02-PROGRAM-STRUCTURE.md) — Layout, entry point, modules
- [03-SYNTAX-RULES.md](03-SYNTAX-RULES.md) — Delimiters, statements, basic rules
- [04-DATA-TYPES.md](04-DATA-TYPES.md) — Primitive types and type system

### Part 2 — Variables, Constants, Operators
- [05-VARIABLES.md](05-VARIABLES.md) — Declaration, init, scope
- [06-CONSTANTS.md](06-CONSTANTS.md) — Constants
- [07-OPERATORS.md](07-OPERATORS.md) — Operators and precedence

### Part 3 — Data Structures
- [08-ARRAYS.md](08-ARRAYS.md) — Arrays
- [09-MAPS.md](09-MAPS.md) — Maps (experimental — known rough edges)
- [10-GROUPS.md](10-GROUPS.md) — Groups (struct-like types)

### Part 4 — Control Flow
- [11-CONDITIONALS.md](11-CONDITIONALS.md) — if / or chains, ternary
- [12-LOOPS.md](12-LOOPS.md) — while, loop, cycle
- [13-ERROR-HANDLING.md](13-ERROR-HANDLING.md) — check / when (as implemented)

### Part 5 — Functions & I/O
- [14-FUNCTIONS.md](14-FUNCTIONS.md) — `fnc`, parameters, returns
- [15-STRING-INTERPOLATION.md](15-STRING-INTERPOLATION.md) — `$var` / `${expr}`
- [16-IO.md](16-IO.md) — `show`, `read`, related I/O

### Part 6 — Builtins & Memory
- [17-BUILTINS.md](17-BUILTINS.md) — Builtins that actually exist today
- [18-MEMORY.md](18-MEMORY.md) — Allocation and memory model (as implemented)
- [19-STRING-MATH.md](19-STRING-MATH.md) — String and math helpers

### Part 7 — Advanced (partial / experimental)
- [20-CONCURRENCY.md](20-CONCURRENCY.md) — Threading and sync (incomplete)
- [21-FILE-NETWORK.md](21-FILE-NETWORK.md) — File and network (as far as implemented)

### Part 8 — Examples, Limits, Status
- [22-EXAMPLES.md](22-EXAMPLES.md) — Working examples (only programs that currently compile)
- [23-LIMITATIONS.md](23-LIMITATIONS.md) — What is **not** supported or is broken
- [STATUS.md](STATUS.md) — Honest feature matrix: works / partial / missing

---

## Memory safety (honest note)

There is a runtime memory-safety layer (allocation tracking, canaries, some guard behavior).  
It is **experimental**, not a finished “Rust/Zig-equivalent” guarantee.

Do not treat the old Phase-9 claims as current truth.  
See [18-MEMORY.md](18-MEMORY.md) and [23-LIMITATIONS.md](23-LIMITATIONS.md) for what is actually on.

---

## Key syntax (quick reference)

**Function**
```ras
fnc main[]::int {
    show["Hello\n"];
    get[0];
}
```

**Variables**
```ras
int x = 10;
str name = "RasCode";
```

**Array**
```ras
arr{int, 10} numbers;
numbers{0} = 5;
```

**Map** (experimental)
```ras
map{str, int} ages;
ages->set["Bob", 25];
```

**Group**
```ras
group Person {
    str name;
    int age;
}
```

**Control**
```ras
if[x > 5] {
    show["big"];
} or {
    show["other"];
}

while[i < 10] { i++; }

loop[int i = 0; i < 10; i++] { ... }
```

**Interpolation**
```ras
show["Hello $name"];
show["Sum: ${a + b}"];
```

---

## Compile & run

```bash
rascom source.ras              # default output a.out
rascom source.ras -o prog
./prog
```

Requirements: Linux x86-64, `gcc`, `nasm`.

---

## How these docs are maintained

Sources of truth:

- `src/lexer.c`
- `src/parser.c`
- `src/codegen.c`
- `src/builtins.c` (and related)
- Actual runs of `rascom`

Not: old marketing text, unfinished plans, or test files that no longer match.

When you find a mismatch, open a **Docs mismatch** issue with:

- the doc section
- the minimal `.ras` input
- the command and real output

---

**RasCode** — stay close to the metal.
```

---
