# RasCode / rascom — Week-by-week path to v0.1

**Goal:** First honest stable release in ~8 weeks  
**Rule:** No new language features until v0.1 is tagged  
**Cadence:** Assume ~12–20 hours/week (adjust if you have more/less)

---

## Week 1 — Safety net + freeze

**Theme:** Never break what already works; stop expanding.

### Tasks
- [ ] Create branch `v0.1-stabilize` (do all work here)
- [ ] Freeze language design: no new keywords, builtins, or syntax
- [ ] Add `examples/` folder with programs that **currently pass**
- [ ] Write `scripts/smoke_test.sh` that compiles + runs every example
- [ ] Document current truth in `STATUS.md`:
  - Works
  - Partial
  - Broken
- [ ] Pin version string to `0.1.0-dev` in binary help output

### Smoke examples to lock in (must stay green)
```
01_hello.ras
02_vars_arith.ras
03_while.ras
04_if_else.ras
05_cycle.ras
06_fn_typed.ras
07_factorial.ras
08_array.ras
09_group_int.ras
10_loop_sum.ras          # uses fixed loop
```

### Exit criteria
`./scripts/smoke_test.sh` exits 0 on a clean machine with `nasm` + `gcc`.

---

## Week 2 — Crash-proof codegen

**Theme:** Compiler must not segfault on valid core programs.

### Tasks
- [ ] Confirm loop null-deref fix is in `src/codegen.c` and committed
- [ ] Confirm unroll condition-recheck fix is committed
- [ ] Grep codegen for other unguarded pointer uses (`loop2->`, `loop3->`, etc.)
- [ ] Run every example at `-O0` and `-O3`
- [ ] Add tests that previously crashed (simple loop, nested 2-deep, nested 3-deep)
- [ ] Optional: build with `-fsanitize=address` debug binary for one week of testing

### New tests
```
11_loop_nested_2.ras
12_loop_nested_3.ras
13_loop_countdown.ras
14_break_while.ras
15_continue_while.ras
```

### Exit criteria
No segfault on any file in `examples/` at `-O0` or `-O3`.

---

## Week 3 — Syntax honesty (parser + docs)

**Theme:** Documented syntax must match the binary.

### Tasks
- [ ] **Params decision (pick one and stick to it):**
  - Option A: Accept untyped `fnc add[a, b]::int` (default type `int`)
  - Option B: Require typed only — update all docs
- [ ] Implement the chosen option in `parser.c`
- [ ] Logical ops: either implement `and`/`or`/`not` **or** remove from docs (keep `&&` `||` `!`)
- [ ] Align `check`/`when` docs with real grammar (`when[Type]:`)
- [ ] Update `docs/13-FUNCTIONS.md`, `06-OPERATORS.md`, `12-TRY-CATCH.md`
- [ ] Rewrite root `README.md` “Quick start” using only verified syntax

### Tests
```
16_fn_params_chosen_form.ras
17_logic_ops.ras
18_check_when.ras
```

### Exit criteria
Following README alone, a stranger can write a small program without parse surprises.

---

## Week 4 — Constants, strings, interpolation

**Theme:** Values print and compute correctly.

### Tasks
- [ ] Fix `const` for `str` and `deci` (store type; emit correct load)
- [ ] Fix `showf` / interpolation for simple consts
- [ ] Fix `$obj.field` **or** document: “assign to temp first”
- [ ] Verify `deci` arithmetic still correct
- [ ] Add string edge cases: empty string, concat, len

### Tests
```
19_const_int.ras
20_const_str.ras
21_const_deci.ras
22_interp_basic.ras
23_interp_member.ras    # pass or xfail with note
24_string_builtins.ras
```

### Exit criteria
Int/str/deci constants and basic interpolation behave as README states.

---

## Week 5 — Groups solid; maps decision

**Theme:** Structs trustworthy; maps either fixed or demoted.

### Tasks
- [ ] Audit group field offsets (especially with `str` fields)
- [ ] Fix nested group field access if still wrong
- [ ] **Maps decision:**
  - Path A: Spend the week fixing `get`/`set`/`has` properly
  - Path B: Mark maps **experimental** — remove from “stable” examples, note in STATUS.md
- [ ] Prefer Path B if maps still look deep (better for hitting v0.1 on time)

### Tests
```
25_group_int_fields.ras
26_group_with_str.ras
27_group_nested.ras
28_map_basic.ras        # only if Path A
```

### Exit criteria
Groups work for documented stable cases. Maps are either correct or clearly non-stable.

---

## Week 6 — Builtins trim + runtime honesty

**Theme:** Only promise builtins that pass tests.

### Tasks
- [ ] Pick a **stable builtin set** (keep small), e.g.:
  - System: `@clock`, `@sleep`, `@exit`
  - Memory: `@alloc`, `@free`, `@peek`, `@poke`
  - String: `@len`, `@concat`
  - Math: `@abs`, `@min`, `@max`
- [ ] Test each; remove or mark experimental the rest in docs
- [ ] Fix any obvious wrong results in the stable set (e.g. earlier `@memcpy` weirdness)
- [ ] Ensure memory-safety banner text matches real behavior (or tone it down)

### Tests
```
29_builtins_system.ras
30_builtins_memory.ras
31_builtins_string.ras
32_builtins_math.ras
```

### Exit criteria
Every builtin listed under “Stable” in README has a passing test.

---

## Week 7 — Packaging + clean build

**Theme:** Others can build and run from a clean clone.

### Tasks
- [ ] One name everywhere: pick **rascom** or **RasCode** and consistent branding
- [ ] Clean `README.md`: install deps, build, run examples, STATUS link
- [ ] Ensure `Boxfile` / make path builds without tribal knowledge
- [ ] Document: requires `nasm` + `gcc` (or implement real assembler fallback — only if time)
- [ ] Add `CHANGELOG.md` with entries from Weeks 1–6
- [ ] License file present (MIT / Apache-2.0 / etc.)
- [ ] Strip or quiet debug spam in default compile output

### Exit criteria
Fresh clone → follow README → smoke tests pass.

---

## Week 8 — Release candidate → v0.1.0

**Theme:** Ship.

### Tasks
- [ ] Freeze code 2–3 days; only fix release blockers
- [ ] Full smoke run on a second machine/VM if possible
- [ ] Tag `v0.1.0`
- [ ] GitHub Release: binary (optional) + source tag + short notes
- [ ] Update README badge/version
- [ ] Write 5–10 line “What this is / isn’t” for the release body

### Release notes template
```text
rascom/RasCode v0.1.0 — first stable core

Works: while/loop, typed functions, arrays, groups (basic),
       if/or/else, cycle, core builtins, -O0 and -O3 without crashes.

Experimental: maps, advanced optimizers, full builtin catalog.

Not a production systems language yet — a solid foundation with honest docs.
```

### Exit criteria
Tag exists; README matches reality; smoke tests green.

---

## After v0.1 (do not start early)

| Order | Focus |
|-------|--------|
| v0.2 | Maps fixed, more builtins, better errors |
| v0.3 | Nested groups polished, more examples |
| v0.4 | Real test suite in CI (GitHub Actions) |
| Later | Tooling, package manager dreams, SOS plans |

---

## Weekly rhythm (suggested)

| Day | Focus |
|-----|--------|
| Mon | Pick 1–2 tasks; write failing tests first |
| Tue–Thu | Implement + run smoke |
| Fri | Update STATUS.md / README; commit |
| Weekend | Buffer or rest |

---

## Hard rules until v0.1

1. **No new features** — only fixes and docs alignment  
2. **Every fix gets a test** in `examples/` or `tests/`  
3. **STATUS.md updated weekly**  
4. **If blocked > 2 days on maps** → demote maps, move on  
5. **AI may write code; you must run the tests**

---

## Success metric

At end of Week 8 you can say:

> “Clone this, run the smoke script, every example passes. The README does not lie.”

That is a real first stable release — more valuable than a bigger, fragile language.

