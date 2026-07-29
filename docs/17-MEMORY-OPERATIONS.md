# 17: Memory Operations

## Memory Model

**RASLang Phase 9:** Mandatory memory safety with Rust/Zig-equivalent guarantees.

RASLang uses SCMM (Secure Capability Memory Management) with:
- **Stack:** Function-local variables, parameters (automatic + canary protection)
- **Heap:** Dynamic allocation via @alloc, protected (versioning + guard pages)
- **Static:** Global constants, groups, strings (read-only)

### Safety Features (Mandatory, Always Active)

**All allocations include:**
- ✅ **Allocation Versioning** — Use-after-free detection via version invalidation
- ✅ **Guard Pages** — 16-byte zones before/after allocations (buffer overflow detection)
- ✅ **Overflow Checks** — Integer overflow validation on all multiplications
- ✅ **Stack Canaries** — Return address corruption detection
- ✅ **Zero-Fill** — All memory initialized to 0 (no uninitialized reads)

These protections cannot be disabled and apply to all compiler operations.

## Heap Allocation

### Basic Allocation

```ras
int ptr = @alloc[size];     // Allocate size bytes
```

Returns pointer (int address) or 0 on failure.

**Example:**
```ras
int buffer = @alloc[1024];
if[buffer == 0] {
    show["Allocation failed\n"];
    @exit[1];
}
```

### Allocation Sizes

```ras
// Allocate for 100 integers (400 bytes on 32-bit)
int arr_ptr = @alloc[400];

// Allocate for string (typical max 256)
int str_ptr = @alloc[256];

// Allocate for structure
int struct_ptr = @alloc[64];
```

## Reading Memory

### @peek

Read 8 bytes from address.

```ras
int ptr = @alloc[8];
int value = @peek[ptr];           // Read from ptr
```

**Usage patterns:**
```ras
// Read first element
int first = @peek[base];

// Read offset element
int second = @peek[base + 4];     // Assuming 4-byte ints

// Chain reads
int val = @peek[@peek[ptr]];      // Follow pointer
```

## Writing Memory

### @poke

Write 8-byte value to address.

```ras
@poke[ptr, 42];                   // Write 42 to ptr
@poke[ptr + 4, 100];              // Write 100 to offset
```

## Memory Copying

### @memcpy

Copy block of memory.

```ras
int src = @alloc[100];
int dst = @alloc[100];
@memcpy[dst, src, 100];           // Copy 100 bytes from src to dst
```

**Use cases:**
```ras
// Copy initialization data
int init_ptr = @alloc[256];
int work_ptr = @alloc[256];
@memcpy[work_ptr, init_ptr, 256];

// Duplicate structure
int orig = @alloc[64];
int copy = @alloc[64];
@memcpy[copy, orig, 64];
```

## Memory Clearing

### @memclr

Zero memory block.

```ras
int ptr = @alloc[256];
@memclr[ptr, 256];                // Zero all 256 bytes
```

**Security use:**
```ras
int sensitive = @salloc[256];     // Secure allocation
// ... use sensitive data ...
@memclr[sensitive, 256];          // Clear before free
@free[sensitive];
```

### @secure_zero

Cryptographic zero (prevents compiler optimization).

```ras
int key = @salloc[32];            // 32-byte key
// ... use key ...
@secure_zero[key, 32];            // Cannot be optimized away
@free[key];
```

## Memory Filling

### @memset

Fill memory with byte value.

```ras
int ptr = @alloc[256];
@memset[ptr, 0x00, 256];          // Fill with 0x00
@memset[ptr, 0xFF, 256];          // Fill with 0xFF
@memset[ptr, 0x42, 256];          // Fill with 0x42 ('B')
```

**Patterns:**
```ras
// Initialize array to zeros
int arr = @alloc[1000];
@memset[arr, 0, 1000];

// Initialize with pattern
@memset[arr, 255, 1000];

// Partial fill
@memset[arr, 0, 500];              // First 500 bytes
```

## Memory Comparison

### @memcmp

Compare two memory blocks.

```ras
int result = @memcmp[ptr1, ptr2, size];
if[result == 0] {
    show["Blocks are equal\n"];
}
if[result < 0] {
    show["ptr1 < ptr2\n"];
}
if[result > 0] {
    show["ptr1 > ptr2\n"];
}
```

**Validation:**
```ras
int expected = @alloc[256];
int actual = @alloc[256];
// ... populate both ...

check {
    if[@memcmp[expected, actual, 256] != 0] {
        @panic["Data mismatch"];
    }
} when: {
    show["Corruption detected\n"];
}
```

## Resizing Memory

### @realloc

Resize allocated block (may move in memory).

```ras
int ptr = @alloc[1024];
ptr = @realloc[ptr, 2048];        // Resize to 2048 (result may differ!)

if[ptr == 0] {
    show["Resize failed\n"];
}
```

**IMPORTANT:** Result pointer may be different!

```ras
int p = @alloc[100];
show["Old ptr: "];
show[p];
show["\n"];

p = @realloc[p, 200];             // Re-assign!

show["New ptr: "];
show[p];
show["\n"];
```

## Secure Allocation

### @salloc

Allocate and zero memory.

```ras
int key = @salloc[32];            // Allocated AND zeroed
int password = @salloc[256];      // For sensitive data
```

Equivalent to:
```ras
int key = @alloc[32];
@memclr[key, 32];
```

## Memory Freeing

### @free

Deallocate memory.

```ras
int ptr = @alloc[1024];
// ... use ...
@free[ptr];
ptr = 0;                          // Good practice: clear reference
```

**Pattern:**
```ras
fnc process_data[size]::int {
    int buffer = @alloc[size];
    if[buffer == 0] {
        get[-1];                  // Error
    }
    
    // ... use buffer ...
    
    @free[buffer];
    get[0];                       // Success
}
```

## Heap Introspection

### @heap_start, @heap_end, @heap_size

Get heap boundaries.

```ras
int start = @heap_start[];        // First heap address
int end = @heap_end[];            // Last heap address
int size = @heap_size[];          // Total heap size

show["Heap: "];
show[start];
show[" - "];
show[end];
show[" ("];
show[size];
show[" bytes)\n"];
```

**Validation:**
```ras
int ptr = @alloc[256];
int start = @heap_start[];
int end = @heap_end[];

if[ptr >= start && ptr < end] {
    show["Valid pointer\n"];
}
```

## Stack Operations

### @stack_ptr

Get current stack pointer.

```ras
int sp = @stack_ptr[];
show["Stack pointer: "];
show[sp];
show["\n"];
```

**Stack depth detection:**
```ras
int initial_sp = 0;

fnc recurse[depth]::int {
    int current_sp = @stack_ptr[];
    
    if[initialsp == 0] {
        initial_sp = current_sp;
    }
    
    int used = initial_sp - current_sp;
    show["Stack depth: "];
    show[depth];
    show[" (used: "];
    show[used];
    show[")\n"];
    
    if[depth > 100] {
        get[depth];
    }
    get[recurse[depth + 1]];
}
```

### @stack_size

Get total stack size.

```ras
int stack_size = @stack_size[];
show["Stack size: "];
show[stack_size];
show[" bytes\n"];
```

## Memory Alignment

### @align

Align pointer to boundary.

```ras
int ptr = @alloc[256];
int aligned = @align[ptr, 16];    // Align to 16-byte boundary

if[aligned == ptr] {
    show["Already aligned\n"];
}
```

**Cache-line alignment:**
```ras
// Many CPUs use 64-byte cache lines
int cache_aligned = @align[ptr, 64];
```

## Memory Protection

### @mprotect

Change protection on memory region.

Requires allocated region via @mmap.

```ras
// Protection constants (system-dependent)
const PROT_READ = 1;
const PROT_WRITE = 2;
const PROT_EXEC = 4;

int region = @mmap[fd, 4096, PROT_READ, 0, 0];
@mprotect[region, 4096, PROT_READ | PROT_WRITE];  // Add write
@munmap[region, 4096];
```

## Memory Mapping

### @mmap

Map file or device into memory.

```ras
int fd = @fopen["data.bin", "r"];
int mapped = @mmap[fd, 1024, 1, 0, 0];  // Map 1024 bytes read-only

// Access via memory
int value = @peek[mapped];

@munmap[mapped, 1024];
@fclose[fd];
```

### @munmap

Unmap memory.

```ras
@munmap[mapped_addr, size];
```

## System Pages

### @page_size

Get system page size.

```ras
int page = @page_size[];          // Usually 4096 bytes
show["Page size: "];
show[page];
show[" bytes\n"];
```

**Allocation in page multiples:**
```ras
int page_size = @page_size[];
int pages = 4;
int size = pages * page_size;     // Allocate 4 pages
int ptr = @alloc[size];
```

## Memory Barriers

### @mfence, @lfence, @sfence

Memory synchronization (for concurrent access).

```ras
// Full barrier (all memory operations ordered)
@mfence[];

// Load barrier (only loads ordered)
@lfence[];

// Store barrier (only stores ordered)
@sfence[];
```

**Concurrent access pattern:**
```ras
int ready_flag = @alloc[4];
int data = @alloc[256];

// Thread 1: Writer
@poke[data, 42];
@mfence[];                        // Ensure write visible
@poke[ready_flag, 1];

// Thread 2: Reader
loop[int i = 0; i < 1000; i++] {
    int flag = @peek[ready_flag];
    if[flag == 1] {
        @lfence[];                // Ensure read ordered
        int value = @peek[data];
        show["Got: "];
        show[value];
        show["\n"];
        cycle;                    // break
    }
}
```

## Memory Debugging

### Address Display
```ras
int ptr = @alloc[256];
show["Allocated at: 0x"];
// Convert to hex would need custom function

// For now, show decimal
show[ptr];
show["\n"];
```

### Memory Validity
```ras
int heap_start = @heap_start[];
int heap_end = @heap_end[];

fnc is_heap_ptr[p]::int {
    if[p >= heap_start && p < heap_end] {
        get[1];
    }
    get[0];
}

int ptr = @alloc[256];
if[is_heap_ptr[ptr]] {
    show["Valid heap pointer\n"];
}
```

### Allocation Tracking
```ras
int allocation_count = 0;
map{int, str} allocations;        // address -> size

fnc alloc_tracked[size]::int {
    int ptr = @alloc[size];
    if[ptr > 0] {
        allocation_count++;
        allocations->set[ptr, @type[size]::str];
    }
    get[ptr];
}

fnc free_tracked[ptr]::int {
    if[allocations->has[ptr]] {
        allocations->remove[ptr];
        @free[ptr];
        allocation_count--;
    }
    get[0];
}
```

## Common Patterns

**Safe allocation:**
```ras
fnc safe_alloc[size]::int {
    int ptr = @alloc[size];
    if[ptr == 0] {
        @panic["Out of memory"];
    }
    get[ptr];
}
```

**Temporary buffer:**
```ras
fnc process[]::int {
    int buf = @alloc[1024];
    check {
        // ... do work ...
        @free[buf];
    } when: {
        @free[buf];
        @exit[1];
    }
    get[0];
}
```

**Memory pool:**
```ras
int pool = @alloc[10240];         // 10KB pool
int pool_used = 0;

fnc pool_alloc[size]::int {
    int result = pool + pool_used;
    pool_used = pool_used + size;
    
    if[pool_used > 10240] {
        get[0];                   // Pool exhausted
    }
    get[result];
}
```

## Related Documentation
- [16-BUILTINS.md](16-BUILTINS.md) — Memory builtins reference
- [18-CONCURRENCY.md](18-CONCURRENCY.md) — Concurrent memory access
- [12-TRY-CATCH.md](12-TRY-CATCH.md) — Error handling
- [04-VARIABLES.md](04-VARIABLES.md) — Stack variables
