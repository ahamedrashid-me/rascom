# RasCode Packages (`.rcp`) & RasBox

How packages, runtime linking, and the RasBox tool fit together on Linux.

---

## Goals

| Piece | Role |
|-------|------|
| **rascom** | Compiler: source → executable; links system runtime `.o` files |
| **Runtime** | Shared object modules under fixed system paths |
| **`.rcp`** | Installable **package archive** (like `.deb` / tarball), not a linkable binary |
| **RasBox** | Project build tool + package manager + thin VCS wrapper |

`.rclib` is **removed**. It is no longer a supported extension.

---

## File types

| Extension | Meaning | Who handles it |
|-----------|---------|----------------|
| `.rco` | RasCode source (current) | rascom |
| `.ras` | Legacy source | rascom |
| **`.rcp`** | Package **archive** (download / install unit) | **RasBox only** |
| `.o` | Native object (runtime / link units) | gcc / ld via rascom |

**Do not** pass a `.rcp` file to `rascom` as input. Install it with RasBox first, then import the package by name.

---

## System layout (Linux)

```text
/usr/lib/rascom/
├── runtime/                 # Linked into every user program
│   ├── sync.o
│   ├── channels.o
│   ├── strings.o
│   ├── math.o
│   └── …                    # all runtime/*.c → *.o
└── packages/                # Extracted packages (from .rcp)
    └── <name>/
        ├── package.toml     # metadata
        ├── main.rco         # or src/main.rco
        └── …                # optional extra sources / assets

/usr/local/lib/rascom/       # same layout (local admin install)
├── runtime/
└── packages/

~/.rascom/packages/          # per-user packages (no root)

./obj/runtime/               # DEVELOPMENT ONLY (CWD while building rascom)
```

### Runtime lookup order (rascom linker)

1. `/usr/lib/rascom/runtime/`
2. `/usr/local/lib/rascom/runtime/`
3. `./obj/runtime/` (dev only)

### Package lookup order (rascom imports)

1. `.` / `./lib` / `./packages`
2. `~/.rascom/packages` (and legacy `~/.rascode/packages`)
3. `/usr/lib/rascom/packages`
4. `/usr/local/lib/rascom/packages`

For a name `foo`, rascom looks for:

- `<root>/foo/main.rco`
- `<root>/foo/src/main.rco`
- `<root>/foo.rco`
- `<root>/foo.ras` (legacy)

---

## What a `.rcp` is

`.rcp` is an **archive**, not a single binary.

Recommended format: **tar + xz** (or tar + zstd) with a fixed internal layout.

```text
hello-utils-1.0.0.rcp          # archive file
│
├── package.toml               # required metadata
├── main.rco                   # or src/main.rco
├── src/                       # optional
│   └── …
└── runtime/                   # optional: extra .o shipped by this package
    └── hello_helpers.o
```

### `package.toml` (minimal)

```toml
[package]
name = "hello-utils"
version = "1.0.0"
description = "Helpers for demos"
license = "MIT"
arch = "x86_64"
os = "linux"

[depends]
# rascom = ">=0.0.1"

[install]
# Paths relative to archive root → system destinations
sources = ["main.rco", "src/"]           # → .../packages/hello-utils/
runtime = ["runtime/*.o"]                # → .../runtime/  (if any)
```

RasBox **extracts** the archive and copies files into the system (or user) tree. After that, rascom only sees normal files under `packages/` and `runtime/`.

---

## How RasBox should work

### Commands (target behavior)

| Command | Behavior |
|---------|----------|
| `rasbox` / `rasbox build` / `rasbox -B` | Build current project with rascom |
| `rasbox init` / `rasbox new <name>` | Scaffold `src/`, `rasbox.toml`, hello `main.rco` |
| `rasbox clean` | Remove `./build` |
| `rasbox grab <spec>` | Fetch a `.rcp` (or git URL), install into package roots |
| `rasbox install <file.rcp>` | Install a local `.rcp` archive |
| `rasbox pull` / `rasbox push` | Thin git wrappers (+ optional identity) |
| `rasbox run` | Build then run `build/app` |
| `rasbox usr:<name>` / `mail:<email>` | Developer identity in `rasbox.toml` |

### Build flow

```text
rasbox build
  → read rasbox.toml  (name, compiler = "rascom", …)
  → rascom src/main.rco -o build/app -O2
  → rascom links against /usr/lib/rascom/runtime/*.o
                        (or /usr/local/... or ./obj/runtime in dev)
```



### Package install flow

```text
rasbox grab hello-utils
  1. Resolve spec → download hello-utils-*.rcp
  2. Verify checksum (when registry exists)
  3. Extract archive to a temp dir
  4. Read package.toml
  5. Install:
       sources  → /usr/lib/rascom/packages/hello-utils/
                  (or ~/.rascom/packages/ if no root)
       runtime  → /usr/lib/rascom/runtime/   (if the package ships .o)
  6. Record dependency in rasbox.toml / lockfile
```

```text
rasbox install ./my-lib-1.0.0.rcp
  → same steps, local file
```

### Using a package in code

After install, source projects import by **name** (not by `.rcp` path):

```text
use:"hello-utils";     # resolves to .../packages/hello-utils/main.rco
```

(Exact import syntax depends on language grammar; the package manager resolves the name to a `.rco` path.)

---

## Installing the compiler runtime (one-time / packaging)

When shipping **rascom** itself as a system tool (or as a future `rascom.rcp`):

```bash
# From rascom source tree
mkdir -p obj/runtime
for f in runtime/*.c; do
  gcc -Wall -O2 -std=c99 -fPIC -Iinclude -c "$f" \
      -o "obj/runtime/$(basename "$f" .c).o"
done

sudo mkdir -p /usr/lib/rascom/runtime /usr/lib/rascom/packages
sudo cp obj/runtime/*.o /usr/lib/rascom/runtime/

# optional local prefix
sudo mkdir -p /usr/local/lib/rascom/runtime
sudo cp obj/runtime/*.o /usr/local/lib/rascom/runtime/
```

Development without install: keep `obj/runtime/*.o` in the rascom build directory and run builds from there, or rely on CWD fallback only while developing the compiler.

---

## What was purged

| Removed | Replacement |
|---------|-------------|
| `.rclib` extension | Gone — do not generate or search for it |
| Linking `obj/runtime/*.o` from **CWD of the user project** | System paths only (+ `obj/runtime` for **compiler** dev) |
| Treating package files as direct linker inputs | Packages are source trees after install; runtime is `.o` under `runtime/` |

---

## Project layout (RasBox-managed app)

```text
my-app/
├── rasbox.toml
├── src/
│   └── main.rco
├── lib/                 # optional local deps
├── packages/            # optional local package trees
└── build/
    └── app              # output of rasbox build
```

### `rasbox.toml` (example)

```toml
[package]
name = "my-app"
version = "0.1.0"
compiler = "rascom"

[developer]
user = "dev"
email = "dev@example.com"

[remote]
type = "git"
url = ""

[dependencies]
# "hello-utils" = "1.0.0"
```

---

## End-to-end picture

```text
┌─────────────┐     grab/install .rcp      ┌──────────────────────────┐
│   Registry  │ ─────────────────────────► │ /usr/lib/rascom/packages │
│  or git URL │                            │ /usr/lib/rascom/runtime  │
└─────────────┘                            └────────────┬─────────────┘
                                                       │
┌─────────────┐     rasbox build                       │
│  src/*.rco  │ ──► rascom -o build/app  ◄─────────────┘
└─────────────┘         │                    (link runtime .o)
                        ▼
                   build/app
```

1. **RasBox** installs packages (archives → filesystem).
2. **rascom** compiles source and links **runtime `.o`** from fixed paths.
3. Users never hand `.rcp` files to the compiler.

---

## Checklist for implementers

**rascom**

- [x] Runtime search: `/usr/lib/rascom/runtime`, `/usr/local/lib/rascom/runtime`, `./obj/runtime`
- [x] No hardcoded `obj/runtime` relative to the *user project* CWD as the only path
- [x] Package search under `/usr/lib/rascom/packages` and `/usr/local/...`
- [x] `.rclib` purged from package resolution
- [x] Reject direct `.rcp` as compiler input (install first)

**RasBox**

- [x] Build: `rascom <src> -o build/app -O2` (no `-c`)
- [ ] `grab` / `install`: unpack `.rcp` → system or user package roots
- [ ] Optional lockfile for dependency versions
- [ ] `rascom` itself distributable as a bootstrap `.rcp` later

**Packaging**

- [ ] Document `.rcp` tar layout + `package.toml` schema (v1)
- [ ] Ship compiler runtime `.o` set with the rascom install package

---

## Version note

This document describes the **intended** Linux layout and RasBox behavior as of the package-system update (runtime paths + `.rcp` as archive + purge of `.rclib`). Adjust paths only by deliberate policy change in both rascom and RasBox together.
