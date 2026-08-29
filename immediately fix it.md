**Detailed bug list + segfault root-cause analysis** (from reading parser/codegen and running under gdb)

---

## Segfault root cause (confirmed)

### Crash location
```
SIGSEGV in codegen_statement[cold]
instruction: mov 0x10, %rax   ← classic NULL + offset 0x10 dereference
call stack: main → codegen_generate → codegen_function → codegen_statement
```

### Why `loop` crashes the **compiler**

1. **Aggressive optimization always on**  
   In `codegen.c` init:
   ```c
   gen->enable_simd = true;  // always
   ```
   For any simple `loop`, this path runs:
   ```c
   if (gen->enable_simd && is_innermost_loop(node->loop_stmt.body)) {
       codegen_loop_unrolled(gen, node, unroll_factor);  // 8x or 16x
       return;
   }
   ```

2. **`is_innermost_loop` has inverted logic** (real bug):
   ```c
   // WRONG — returns false when block has NO nested loops
   if (stmt->type == AST_IF && stmt->if_stmt.then_block) {
       if (is_innermost_loop(stmt->if_stmt.then_block)) return false;
   }
   ```
   Should return `false` only when a nested loop **is found**.

3. **Missing NULL checks** in the optimization path  
   Code walks `body->block.statements->nodes[0]` and similar without always verifying pointers. GDB shows a load from absolute address `0x10` (NULL + 16), which matches accessing the first union field of a NULL `ASTNode*` or a NULL list field.

4. **Unroll path is unsafe**  
   `codegen_loop_unrolled` emits body + increment 8–16 times without verifying that init/condition/body/increment form a consistent, fully-initialized AST. Any partial node from the parser becomes a NULL deref during codegen.

**Primary fix for loop segfault:** disable the optimization path until it is hardened, then fix NULL checks and `is_innermost_loop`.

---

## Complete segfault fix plan

### Fix A — Immediate (stop all compiler segfaults on `loop`)

**File:** `src/codegen.c` (backend / codegen)

```c
// In codegen_new / init (~line 443):
gen->enable_simd = false;   // was true — disable until optimizations are safe

// In codegen_loop (~line 2491), force simple path:
// DELETE or guard:
// if (gen->enable_simd && is_innermost_loop(...)) { codegen_loop_unrolled(...); return; }
// if (gen->enable_simd && gen->loop_depth == 1) { /* interchange / blocking */ }

// Keep only the regular path:
if (node->loop_stmt.init)
    codegen_statement(gen, node->loop_stmt.init);
emit(".L%d:", start_label);
if (node->loop_stmt.condition) { ... }
if (node->loop_stmt.body)
    codegen_statement(gen, node->loop_stmt.body);
if (node->loop_stmt.increment)
    codegen_statement(gen, node->loop_stmt.increment);
```

This alone should stop the `loop` compiler crash.

### Fix B — Harden `is_innermost_loop`

**File:** `src/codegen.c`

```c
static bool is_innermost_loop(ASTNode *node) {
    if (!node) return true;

    if (node->type == AST_LOOP) return false;

    if (node->type == AST_BLOCK) {
        if (!node->block.statements) return true;
        for (int i = 0; i < node->block.statements->count; i++) {
            ASTNode *stmt = node->block.statements->nodes[i];
            if (!stmt) continue;
            if (stmt->type == AST_LOOP) return false;
            if (stmt->type == AST_IF) {
                // FIXED logic: nested loop => not innermost
                if (stmt->if_stmt.then_block && !is_innermost_loop(stmt->if_stmt.then_block))
                    return false;
                if (stmt->if_stmt.else_block && !is_innermost_loop(stmt->if_stmt.else_block))
                    return false;
            }
        }
        return true;
    }
    // ... same pattern for AST_IF at top level
    return true;
}
```

### Fix C — Defensive NULL checks in codegen

**File:** `src/codegen.c`

```c
static void codegen_statement(CodeGen *gen, ASTNode *node) {
    if (!node) return;

    switch (node->type) {
    case AST_BLOCK:
        if (!node->block.statements) return;
        // ...
    case AST_SHOWF:
        if (!node->showf.interpolated) return;
        if (!node->showf.interpolated->interpolated_string.chunks) return;
        if (!node->showf.interpolated->interpolated_string.expressions) return;
        // ...
    case AST_LOOP:
        if (!node->loop_stmt.body) {
            fprintf(stderr, "Error: loop missing body\n");
            return;
        }
        codegen_loop(gen, node);
        break;
    // ...
    }
}
```

### Fix D — Parser: explicit init of loop fields

**File:** `src/parser.c` — `parse_loop`

```c
ASTNode *node = ast_node_new(AST_LOOP);  // already memset 0
node->loop_stmt.init = NULL;
node->loop_stmt.condition = NULL;
node->loop_stmt.increment = NULL;
node->loop_stmt.body = NULL;

node->loop_stmt.init = parse_statement(parser);
// if parse fails, do not continue with garbage

node->loop_stmt.condition = parse_expression(parser);
// ...

// only set increment when fully parsed
if (/* valid increment */) {
    node->loop_stmt.increment = ...;
} else {
    node->loop_stmt.increment = NULL;
}

node->loop_stmt.body = parse_block(parser);
if (!node->loop_stmt.body)
    error_at(..., "loop requires a body block");
```

### Fix E — Runtime map segfault

Maps compile but `get`/`has` return garbage and sometimes crash at **runtime** (not in the compiler).

**Where:** `src/codegen.c` → `codegen_map_get` / `codegen_map_has` / `codegen_map_set` (~2970–3264) and any runtime helpers in `runtime/`.

**Likely causes:**
- Hash table not zero-initialized on `map` decl
- Key string pointer treated as integer (or vice versa)
- Probe loop with no empty-slot termination → OOB read

**Fix approach:**
1. On `codegen_map_decl`: allocate table, `memset` to 0, store capacity + count.
2. On `set`: hash key correctly (string vs int), open-address until empty slot, store key+value.
3. On `get`/`has`: same hash + probe; on miss return 0 / false, never read uninitialized slots.
4. Add bounds checks on every probe index.

---

## Full bug list (frontend = language/parser docs, backend = compiler/runtime)

| ID | Bug | Layer | Severity | How to fix |
|----|-----|--------|----------|------------|
| **S1** | `loop` → **compiler SIGSEGV** | Backend codegen | Critical | Fix A + B + C (disable SIMD path, fix `is_innermost_loop`, NULL checks) |
| **S2** | Map `get`/`has` → **runtime SIGSEGV** / garbage | Backend codegen + runtime | Critical | Fix E (correct hash table init + probe) |
| **B1** | Untyped params `fnc f[a,b]::int` rejected | Frontend parser | High | In `parse_function`, allow IDENT params and default type to `int`, **or** change all docs to require types |
| **B2** | Docs say `and`/`or`/`not` work; only `&&`/`\|\|`/`!` work | Frontend lexer/parser | Medium | Either implement keyword ops in parser expression rules, or remove from docs |
| **B3** | `const` str/deci print as numbers | Backend codegen / showf | High | Emit string/deci consts as proper data labels; fix `showf` to use type of symbol |
| **B4** | `$p.x` interpolation broken | Backend showf codegen | Medium | Lower member access to load before print; don’t treat `p.x` as a single ident |
| **B5** | Nested group fields wrong (e.g. `age=0`) | Backend layout / member offset | High | Fix group field offset calculation (str fields size, alignment) |
| **B6** | `when { }` rejected; needs `when[Type]:` | Frontend parser | Low | Allow bare `when` as catch-all, matching docs |
| **B7** | Docs say `break`/`continue` unsupported; they work | Docs | Low | Update `23-LIMITATIONS.md` |
| **B8** | `+=` correctly rejected; docs OK | — | — | No code change |
| **B9** | NASM required; no working built-in fallback | Backend main/linker | Medium | Implement or document “requires nasm”; don’t claim fallback |
| **B10** | `is_innermost_loop` inverted IF logic | Backend | High | Fix B |
| **B11** | Unroll/interchange/blocking enabled by default | Backend | High | Default `enable_simd = false`; opt-in via `-fsimd` later |
| **B12** | `@memcpy` wrong results in tests | Backend builtins | Medium | Fix size/pointer handling in memcpy builtin codegen |

---

## Recommended fix order (complete segfault elimination first)

### Phase 1 — Stop all segfaults (1–2 days)
1. Set `enable_simd = false` (Fix A).  
2. Add NULL guards in `codegen_statement` / `codegen_loop` / `codegen_block` (Fix C).  
3. Fix `is_innermost_loop` (Fix B).  
4. Re-test: every `loop[...]` program must compile without SIGSEGV.  
5. Fix map table init + probe (Fix E) until no runtime crash on `get`/`has`/`set`.

### Phase 2 — Correctness vs docs
6. B1: typed vs untyped params (pick one, align parser + docs).  
7. B3/B4: const + interpolation.  
8. B5: group field layout.  
9. B2/B6/B7: docs vs parser alignment.

### Phase 3 — Re-enable optimizations safely
10. Only after Phase 1 green: re-enable unroll behind `-O3 -fsimd` with assertions on non-NULL AST nodes.  
11. Fuzz simple/nested loops and maps.

---

## Minimal patch sketch for S1 (loop compiler crash)

```c
// codegen.c — codegen_new / init
gen->enable_simd = false;      // CRITICAL
gen->enable_prefetch = false;  // until safe

// codegen.c — codegen_loop: remove or comment out ALL of:
//   - interchange block (loop_depth == 1)
//   - blocked tiling block
//   - is_innermost_loop → codegen_loop_unrolled
// Keep only init → label → condition → body → increment → jmp
```

After that, rebuild the compiler binary and re-run:

```ras
fnc main[]::int {
    int sum = 0;
    loop[int i = 0; i < 5; i++] {
        sum = sum + i;
    }
    showf["sum=$sum\n"];
    get[0];
}
```

Expected: compile OK, print `sum=10`.

---

## Frontend vs backend roles

| Concern | Frontend (parser / language) | Backend (codegen / runtime) |
|---------|------------------------------|-----------------------------|
| Syntax accepted vs docs | B1, B2, B6 | — |
| Compiler crash on valid `loop` | — | **S1, B10, B11** |
| Wrong runtime values | — | S2, B3, B4, B5, B12 |
| Docs wrong | Update markdown | — |

**Most important:** S1 is entirely backend. Disabling the SIMD/unroll path and adding NULL checks is the complete fix for the loop **compiler** segfault. Map crashes are a separate backend hash-table bug (S2).

