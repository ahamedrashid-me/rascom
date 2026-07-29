# 📦 box

> **A modern, blazingly fast, single-file parallel build tool.**  
> Pattern rules • Out-of-tree builds • Dynamic `.d` header dependency tracking • Zero dependencies.

`box` is a minimal, lightweight build tool created as a modern alternative to GNU Make. Built in Rust with standard-library-only primitives, `box` compiles complex C/C++ projects, generates automatic dependency graphs, and builds targets concurrently out-of-the-box.

---

## 🌟 Why `box`?

Traditional build systems often suffer from bloated syntax, slow execution overhead, or complex multi-file dependency setups. `box` addresses these pain points directly:

* **⚡ Fast & Parallel by Default:** Automatically detects CPU cores (`available_parallelism`) and compiles targets in parallel dependency levels using lightweight OS threads.
* **📦 Zero Dependencies & Portable:** Implemented as a single Rust source file with **no external crates**. Works seamlessly across Linux, macOS, and Windows.
* **🔍 Dynamic `.d` Header Tracking:** Native parsing of GCC/Clang generated dependency files (`-MD -MP`) ensuring modified header files automatically trigger incremental rebuilds without manual `make clean` calls.
* **🎯 Modern Variable & Pattern Expansion:** Clean stem (`%` / `$*`) replacement, path handling, and single-target auto-splitting for space-separated variable lists.
* **💡 Developer Friendly:** Features built-in system checks, such as recommending `ccache` integration when detected.

---

## 🚀 Getting Started

### Installation

Compile `box` using standard Rust tools:

```bash
# Using rustc directly
rustc -O -o box main.rs

# Or move it to your local bin directory
mv box /usr/local/bin/
Quick StartCreate a Boxfile in your project root directory.Run box:Bash# Build the default target using maximum available CPU cores
box

# Rebuild everything from scratch
box -B

# Force compilation with 2 parallel threads
box -j2
🛠 Command-Line UsagePlaintextbox – minimal parallel build tool

Usage:
  box [options] [target]

Options:
  -jN, -j N          Number of parallel jobs (default: available CPU cores)
  -B, --force        Force rebuild all targets regardless of modification times
  -k, --keep-going   Continue building unaffected targets after an error
  -h, --help         Display help information
🏛 Architecture & Internal Designbox processes builds through a structured, multi-stage pipeline:Plaintext ┌───────────┐     ┌─────────────────────┐     ┌────────────────────────┐
 │  Boxfile  │ ──> │ Variable Expansion  │ ──> │ Header Dep File (.d)   │
 └───────────┘     │ & Target Splitting  │     │ Parsing & Injection    │
                   └─────────────────────┘     └────────────────────────┘
                                                           │
                                                           ▼
 ┌───────────┐     ┌─────────────────────┐     ┌────────────────────────┐
 │ Compilation│ <── │ Level-Based Thread  │ <── │ Directed Acyclic Graph │
 │ Execution │     │ Execution Pool      │     │ (DAG) Cycle Detection  │
 └───────────┘     └─────────────────────┘     └────────────────────────┘
Parser & Variable Expander: Reads the Boxfile, parses variable declarations, target rules, and pattern rules. Automatically splits space-separated variable lists (e.g., $(OBJECTS)) into individual concrete build rules.Dynamic Header Injector: Evaluates -include directives, parses generated .d depfiles, and merges header prerequisites directly into the memory rule table.DAG Builder & Topological Sorter: Evaluates file modification times (mtime) and builds a dependency levels queue. It runs cycle detection to prevent infinite recursion loops.Parallel Thread Executor: Dispatches independent targets level-by-level across a thread pool guarded by atomic semaphores.📖 Complete Guide to BoxfileThe Boxfile is the configuration file used by box to define targets, prerequisites, and build recipes.1. Variables & SyntaxVariables are defined using KEY = VALUE and referenced with $(KEY).Code snippetCC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c99 -Iinclude -MD -MP
OBJDIR  = obj
2. Automatic Variablesbox supports traditional automatic recipe variables:VariableDescription$@Evaluates to the target file name.$<Evaluates to the first dependency file.$^Evaluates to all dependency files separated by spaces.$*Evaluates to the pattern stem matched by %.3. Pattern Rules (%)Pattern rules allow you to define generic compilation steps for entire directories:Code snippet# Compile any C file under src/ into obj/
obj/%.o: src/%.c
	$(CC) $(CFLAGS) -MF obj/$*.d -c $< -o$@

# Compile any runtime module
obj/runtime/%.o: runtime/%.c
	$(CC) $(CFLAGS) -MF obj/runtime/$*.d -c $< -o$@
4. Directives: .PHONY and .DEFAULT.PHONY: Declares non-file targets that should always run (e.g., clean)..DEFAULT: Specifies which target to build when no argument is given to box.Code snippet.PHONY: clean distclean
.DEFAULT: all
📋 Comprehensive Boxfile TemplateHere is a full, real-world Boxfile setup for a C project with multiple directories, binary embedding, and dynamic header dependencies:Code snippet# ============================================================
# Project Build Configuration (Boxfile)
# ============================================================

CC        = gcc
CFLAGS    = -Wall -Wextra -O2 -std=c99 -fPIE -Iinclude -MD -MP
LDFLAGS   = -pie -lm

TARGET    = rascom

# Core Object Files
OBJECTS = obj/main.o obj/lexer.o obj/parser.o obj/ast.o obj/codegen.o obj/common.o

# Subdirectory Runtime Object Files
RUNTIME_OBJECTS = obj/runtime/sync.o obj/runtime/network.o obj/runtime/time.o

LOGO_OBJ = obj/logo.o

.PHONY: clean distclean
.DEFAULT: all

# Import GCC/Clang generated header dependencies
-include obj/*.d
-include obj/runtime/*.d

# ============================================================
# Build Rules
# ============================================================

all: $(TARGET)

# Final Binary Linking
$(TARGET):$(OBJECTS) $(RUNTIME_OBJECTS)$(LOGO_OBJ)
	$(CC)$(OBJECTS) $(RUNTIME_OBJECTS)$(LOGO_OBJ) -o $@ $(LDFLAGS)

# Resource Embedding (Binary to Object conversion)
$(LOGO_OBJ): src/RasCom.jpg
	objcopy -I binary -O elf64-x86-64 -B i386 --rename-section .data=.logo,alloc,load,readonly,data,contents $<$@

# Pattern Rule: Runtime C Files
obj/runtime/%.o: runtime/%.c
	$(CC) $(CFLAGS) -MF obj/runtime/$*.d -c $< -o$@

# Pattern Rule: Source C Files
obj/%.o: src/%.c
	$(CC) $(CFLAGS) -MF obj/$*.d -c $< -o$@

# Clean Build Artifacts
clean:
	rm -rf obj

distclean: clean
	rm -f $(TARGET)
📜 LicenseThis project is open-source and available under the MIT License.