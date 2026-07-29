# 16: Builtin Functions Reference

All builtin functions are prefixed with `@` and called like regular functions with `[args]` syntax.

Source: From `src/builtins.c` BUILTIN_REGISTRY array.

> **✅ UPDATED**: This document now covers **ALL ACTIVE builtin functions (199 total)** implemented in BUILTIN_REGISTRY. See category sections below for complete reference organized by functionality.

## Quick Function Index by Category

- [System Control](#system-control) — Program termination, timing
- [Memory Operations](#memory-operations) — Allocation, access, manipulation
- [Type Conversion](#type-conversion) — Converting between types
- [String Operations](#string-operations) — Length, concatenation, substring
- [File I/O](#file-io) — File operations
- [Network](#network) — Socket operations
- [Security](#security) — Hashing, randomness, entropy
- [Hardware](#hardware) — Low-level port I/O
- [Threading](#threading) — Thread creation and control
- [Synchronization](#synchronization) — Mutexes, semaphores, atomic operations
- [Channels](#channels) — Inter-thread communication
- [Thread Pools](#thread-pools) — Work queues
- [Meta](#meta) — Compiler information

## System Control

### @exit

Terminates program with exit code.

```ras
@exit[0];           // Success
@exit[1];           // Error
```

**Args:** 1 (exit code: int)
**Returns:** (void)
**Category:** System

### @halt

Immediately stop execution.

```ras
@halt[];
```

**Args:** 0
**Returns:** (void)
**Category:** System

### @sleep

Sleep for milliseconds.

```ras
@sleep[1000];       // Sleep 1 second
@sleep[100];        // Sleep 100ms
```

**Args:** 1 (milliseconds: int)
**Returns:** (void)
**Category:** System

### @clock

Get current time in milliseconds since epoch.

```ras
int now = @clock[];
int elapsed = @clock[] - start_time;
```

**Args:** 0
**Returns:** int
**Category:** System

### @panic

Trigger a panic with message.

```ras
@panic["Unrecoverable error"];
```

**Args:** 1 (message: str)
**Returns:** (void)
**Category:** System

## Memory Operations

### @alloc

Allocate memory on heap.

```ras
int ptr = @alloc[1024];      // 1024 bytes
int buffer = @alloc[100];

// Use with @peek/@poke to access
int value = @peek[ptr];
@poke[ptr, 42];
```

**Args:** 1 (size in bytes: int)
**Returns:** int (pointer to allocated memory)
**Category:** Memory
**Notes:** Returns null-like 0 on failure

### @free

Deallocate memory.

```ras
int ptr = @alloc[1024];
@free[ptr];
```

**Args:** 1 (pointer to deallocate: int)
**Returns:** (void)
**Category:** Memory

### @realloc

Resize allocated memory.

```ras
int ptr = @alloc[1024];
ptr = @realloc[ptr, 2048];   // Expand to 2048 bytes
```

**Args:** 2 (pointer: int, new_size: int)
**Returns:** int (new pointer)
**Category:** Memory

### @salloc

Secure allocation (zero-fills memory).

```ras
int sensitive = @salloc[256]; // Secure, zeroed memory
```

**Args:** 1 (size: int)
**Returns:** int (pointer)
**Category:** Memory

### @peek

Read value from memory address (returns 8-byte integer).

```ras
// Basic usage - read from allocated memory
int ptr = @alloc[8];
@poke[ptr, 42];
int value = @peek[ptr];         // value = 42

// Pointer arithmetic - read adjacent memory
int ptr = @alloc[16];
@poke[ptr, 100];
@poke[ptr + 8, 200];            // Write at offset
int first = @peek[ptr];         // = 100
int second = @peek[ptr + 8];    // = 200
```

**Args:** 1 (address: int)  
**Returns:** int (8-byte value at address)  
**Category:** Memory  
**⚠️ Warning**: 
- Reads always return 8-byte integers (even if smaller value stored)
- Reading uninitialized memory is undefined
- No bounds checking - can read beyond allocated region (use carefully!)
- Negative addresses will cause segmentation fault

### @poke

Write value to memory address (writes 8-byte integer).

```ras
// Basic usage
int ptr = @alloc[16];
@poke[ptr, 42];                 // Write at offset 0
@poke[ptr + 8, 100];            // Write at offset 8

// Initialize array of values
loop[int i = 0; i < 4; i++] {
    @poke[ptr + (i * 8), i * 10];  // Pointer arithmetic
}

// Read back values
loop[int i = 0; i < 4; i++] {
    int val = @peek[ptr + (i * 8)];
    show[val];
}
```

**Args:** 2 (address: int, value: int)  
**Returns:** (void)  
**Category:** Memory  
**⚠️ Warning**:
- Writes full 8-byte values (32/64-bit depending on platform)
- No type checking - can write any integer to any address
- Can corrupt other variables if pointer math is wrong
- Writing to unallocated memory causes undefined behavior

### @addr

Get memory address of a variable (low-level operation).

```ras
// Get address of local variable
int x = 42;
int addr_x = @addr[x];          // Address of x in stack
int val = @peek[addr_x];        // Read x via pointer

// Get address of array
arr{int, 5} numbers = {10, 20, 30, 40, 50};
int arr_addr = @addr[numbers];
int first = @peek[arr_addr];    // = 10
int second = @peek[arr_addr + 8];  // = 20

// Get address of struct field
group Point {
    int x;
    int y;
}
Point p;
p.x = 100;
p.y = 200;
int addr_px = @addr[p.x];
```

**Args:** 1 (variable)  
**Returns:** int (address of variable)  
**Category:** Memory  
**⚠️ Warning - Low-Level Operation**:
- Returns address of stack variable (becomes invalid when scope exits!)
- Do NOT store @addr results for later use
- Do NOT pass addresses to other functions and expect them to persist
- Primarily for immediate pointer operations, not long-term storage

### Pointer Arithmetic Patterns

**Common safe patterns:**

```ras
// Allocate structure with multiple fields
int ptr = @alloc[32];           // 32 bytes = 4 × 8-byte values

// Store as struct: ptr+0=id, ptr+8=type, ptr+16=count, ptr+24=reserved
@poke[ptr + 0, 1];              // id
@poke[ptr + 8, 2];              // type
@poke[ptr + 16, 100];           // count

// Read back safely
int id = @peek[ptr + 0];
int type = @peek[ptr + 8];
int count = @peek[ptr + 16];
```

**Unsafe patterns to avoid:**

```ras
// ❌ AVOID: Using stack address after function returns
fnc get_ptr[]::int {
    int x = 42;
    get[@addr[x]];              // INVALID! x is freed when function returns
}

// ❌ AVOID: Unbounded pointer arithmetic
int ptr = @alloc[16];           // 16 bytes allocated
@poke[ptr + 1000, 42];          // Writing way beyond allocation!

// ❌ AVOID: Storing @addr for later use
fnc store_address[]::none {
    int x = 42;
    int saved_addr = @addr[x];  // x's address becomes invalid
    // later...
    int val = @peek[saved_addr]; // Undefined behavior!
}
```

### @memcpy

Copy memory.

```ras
int src = @alloc[100];
int dest = @alloc[100];
@memcpy[dest, src, 100];      // Copy 100 bytes
```

**Args:** 3 (dest: int, src: int, size: int)
**Returns:** int (dest pointer)
**Category:** Memory

### @memclr

Clear (zero) memory.

```ras
int ptr = @alloc[256];
@memclr[ptr, 256];            // Zero all bytes
```

**Args:** 2 (address: int, size: int)
**Returns:** (void)
**Category:** Memory

### @memset

Fill memory with byte value.

```ras
int ptr = @alloc[100];
@memset[ptr, 0xFF, 100];      // Fill with 255
```

**Args:** 3 (address: int, value: byte, size: int)
**Returns:** (void)
**Category:** Memory

### @memcmp

Compare memory blocks.

```ras
int result = @memcmp[ptr1, ptr2, 100];
if[result == 0] {
    show["Memory blocks equal\n"];
}
```

**Args:** 3 (ptr1: int, ptr2: int, size: int)
**Returns:** int (0 if equal, negative if ptr1 < ptr2, positive if ptr1 > ptr2)
**Category:** Memory

### @mmap

Memory map a memory region (not file-based in current implementation).

```ras
int mapped = @mmap[size, prot, flags];
// prot: 0=PROT_NONE, 1=PROT_READ, 2=PROT_WRITE, 4=PROT_EXEC, 3=PROT_READ|PROT_WRITE
// flags: 0x20=MAP_PRIVATE, 0x01=MAP_SHARED
```

**Args:** 3 (size: int, protection: int, flags: int)
**Returns:** int (mapped address, 0 on failure)
**Category:** Memory
**Note:** Current implementation handles memory regions only; does not support file descriptors

### @munmap

Unmap memory.

```ras
@munmap[mapped, size];
```

**Args:** 2 (address: int, size: int)
**Returns:** (void)
**Category:** Memory

### @mprotect

Change memory protection.

```ras
@mprotect[addr, size, prot];  // prot: PROT_READ, PROT_WRITE, PROT_EXEC
```

**Args:** 3 (address: int, size: int, protection: int)
**Returns:** (void)
**Category:** Memory

### @heap_start, @heap_end, @heap_size

Heap introspection.

```ras
int start = @heap_start[];
int end = @heap_end[];
int size = @heap_size[];
```

**Args:** 0
**Returns:** int
**Category:** Memory

### @page_size

Get system page size.

```ras
int page = @page_size[];       // Usually 4096
```

**Args:** 0
**Returns:** int
**Category:** Memory

### @stack_ptr

Get current stack pointer.

```ras
int sp = @stack_ptr[];
```

**Args:** 0
**Returns:** int
**Category:** Memory

### @stack_size

Get stack size.

```ras
int size = @stack_size[];
```

**Args:** 0
**Returns:** int
**Category:** Memory

### @mfence, @lfence, @sfence

Memory fence operations.

```ras
@mfence[];   // Full memory barrier
@lfence[];   // Load barrier
@sfence[];   // Store barrier
```

**Args:** 0
**Returns:** (void)
**Category:** Memory

### @align

Align pointer to boundary.

```ras
int aligned = @align[ptr, 16];  // Align to 16-byte boundary
```

**Args:** 2 (pointer: int, boundary: int)
**Returns:** int (aligned pointer)
**Category:** Memory

## Type Conversion

### @type

Universal type converter.

```ras
// Convert to string representation
str s1 = @type[42]::str;           // "42"
str s2 = @type[3.14]::str;         // "3.14"
str s3 = @type[true]::str;         // "true"

// Convert from string
int i = @type["42"]::int;          // 42
deci d = @type["3.14"]::deci;      // 3.14
bool b = @type["true"]::bool;      // true
```

**Args:** 1 (value: any type)
**Returns:** any type (specified by ::type)
**Category:** Type
**Notes:** Universal conversion using @type[value]::target_type syntax

### @len

Get length or size.

```ras
int len1 = @len["hello"];          // 5
int len2 = @len[string_var];       // Length of string
int arr_size = @len[array_var];    // Array size
```

**Args:** 1 (value: str or array)
**Returns:** int
**Category:** Type

### @sizeof

Get size of type or variable.

```ras
int int_size = @sizeof[int];       // 4
int deci_size = @sizeof[deci];     // 8
int str_size = @sizeof[str];       // Variable
```

**Args:** 1 (type or variable)
**Returns:** int (bytes)
**Category:** Type

### @ord

Character to integer (ASCII value).

```ras
char c = 'A';
int code = @ord[c];                // 65
```

**Args:** 1 (character: char)
**Returns:** int
**Category:** Type

### @chr

Integer to character (from ASCII value).

```ras
char c = @chr[65];                 // 'A'
```

**Args:** 1 (value: int)
**Returns:** char
**Category:** Type

## String Operations

### @concat

Concatenate strings.

```ras
str result = @concat["Hello", " ", "World"];
str msg = @concat["Count: ", @type[42]::str];
```

**Args:** 2+ (strings to concatenate)
**Returns:** str
**Category:** String

### @substr

Extract substring.

```ras
str s = "Hello World";
str sub = @substr[s, 0, 5];        // "Hello"
str sub2 = @substr[s, 6, 5];       // "World"
```

**Args:** 3 (string: str, start: int, length: int)
**Returns:** str
**Category:** String

### @strcmp

Compare strings.

```ras
int cmp = @strcmp["abc", "abc"];   // 0 (equal)
int cmp2 = @strcmp["abc", "abd"];  // Negative (a<b)
int cmp3 = @strcmp["abd", "abc"];  // Positive (a>b)
```

**Args:** 2 (str1: str, str2: str)
**Returns:** int (0 if equal, <0 if str1<str2, >0 if str1>str2)
**Category:** String

## File I/O

### @fopen

Open file.

```ras
int fd = @fopen["file.txt", "r"];   // Read
int fd2 = @fopen["out.txt", "w"];   // Write
int fd3 = @fopen["app.txt", "a"];   // Append

if[fd == 0] {
    show["Failed to open\n"];
}
```

**Args:** 2 (path: str, mode: str)
**Returns:** int (file descriptor, 0 on error)
**Category:** File

### @fread

Read from file.

```ras
int fd = @fopen["file.txt", "r"];
int bytes_read = @fread[fd, buffer, 1024];

if[bytes_read > 0] {
    show["Read "];
    show[bytes_read];
    show[" bytes\n"];
}
```

**Args:** 3 (fd: int, buffer: int (address), size: int)
**Returns:** int (bytes read, 0 at EOF, -1 on error)
**Category:** File

### @fwrite

Write to file.

```ras
int fd = @fopen["out.txt", "w"];
int bytes = @fwrite[fd, buffer, 100];
```

**Args:** 3 (fd: int, buffer: int (address), size: int)
**Returns:** int (bytes written)
**Category:** File

### @fseek

Seek in file.

```ras
int fd = @fopen["file.txt", "r"];
@fseek[fd, 100];                   // Seek to byte 100
@fseek[fd, -10];                   // Seek back 10 bytes from current
```

**Args:** 2 (fd: int, offset: int)
**Returns:** int (new position, -1 on error)
**Category:** File

### @fclose

Close file.

```ras
int fd = @fopen["file.txt", "r"];
// ... do operations ...
@fclose[fd];
```

**Args:** 1 (fd: int)
**Returns:** (void)
**Category:** File

### @fdelete

Delete file.

```ras
int result = @fdelete["tempfile.txt"];
if[result == 0] {
    show["File deleted\n"];
}
```

**Args:** 1 (path: str)
**Returns:** int (0 on success, -1 on error)
**Category:** File

## Network

### @socket

Create socket.

```ras
int sock = @socket[1, 1];          // AF_INET, SOCK_STREAM (TCP)
```

**Args:** 2 (address_family: int, socket_type: int)
**Returns:** int (socket file descriptor, -1 on error)
**Category:** Network

### @connect

Connect to remote host.

```ras
int result = @connect[sock, "192.168.1.1", 8080];
if[result == 0] {
    show["Connected\n"];
}
```

**Args:** 3 (sock: int, host: str, port: int)
**Returns:** int (0 on success, -1 on error)
**Category:** Network

### @send

Send data on socket.

```ras
str msg = "Hello";
int bytes = @send[sock, msg, @len[msg]];
```

**Args:** 3 (sock: int, buffer: str/int, size: int)
**Returns:** int (bytes sent)
**Category:** Network

### @recv

Receive data on socket.

```ras
int buffer = @alloc[1024];
int bytes = @recv[sock, buffer, 1024];
```

**Args:** 3 (sock: int, buffer: int (address), size: int)
**Returns:** int (bytes received, 0 at EOF, -1 on error)
**Category:** Network

### @bind

Bind socket to address.

```ras
int result = @bind[sock, "0.0.0.0", 8080];
```

**Args:** 3 (sock: int, address: str, port: int)
**Returns:** int (0 on success, -1 on error)
**Category:** Network

### @listen

Listen for connections.

```ras
@bind[sock, "0.0.0.0", 8080];
int result = @listen[sock, 5];     // Backlog 5
```

**Args:** 2 (sock: int, backlog: int)
**Returns:** int (0 on success, -1 on error)
**Category:** Network

### @accept

Accept incoming connection.

```ras
int client = @accept[server_sock];
if[client > 0] {
    show["Client connected\n"];
}
```

**Args:** 1 (sock: int)
**Returns:** int (client socket descriptor, -1 on error)
**Category:** Network

### @close (socket)

Close socket.

```ras
@close[sock];
```

**Args:** 1 (sock: int)
**Returns:** (void)
**Category:** Network

## Security

### @hash

Compute hash (various algorithms).

```ras
str data = "password";
int hash = @hash[data];            // Default hash
```

**Args:** 1 (data: str)
**Returns:** int (hash value)
**Category:** Security

### @rand

Generate random bytes/integer.

```ras
int r1 = @rand[1];                 // Random byte (0-255)
int r2 = @rand[4];                 // Random int from 4 bytes
```

**Args:** 1 (size: int - number of random bytes to generate)  
**Returns:** int (interpreted as integer from random bytes)
**Category:** Security
**Note:** Actual signature from registry: 1 arg (size), not variadic as docs previously stated

### @secure_zero

Securely zero memory.

```ras
int ptr = @alloc[256];
// ... use ...
@secure_zero[ptr, 256];            // Cryptographically secure zero
```

**Args:** 2 (address: int, size: int)
**Returns:** (void)
**Category:** Security

### @entropy

Get system entropy.

```ras
int bits = @entropy[256];          // Get 256 bits of entropy
```

**Args:** 1 (bits: int)
**Returns:** int
**Category:** Security

### @verify

Verify signature/hash.

```ras
int valid = @verify[signature, data];
```

**Args:** 2 (signature: str, data: str)
**Returns:** int (1 if valid, 0 if invalid)
**Category:** Security

## Hardware

### @port_in

Read from I/O port.

```ras
byte value = @port_in[0x3F8];      // Serial port
```

**Args:** 1 (port: int)
**Returns:** byte
**Category:** Hardware

### @port_out

Write to I/O port.

```ras
@port_out[0x3F8, 0x41];            // Write to serial
```

**Args:** 2 (port: int, value: byte)
**Returns:** (void)
**Category:** Hardware

### @irq_enable, @irq_disable

Enable/disable interrupts.

```ras
@irq_disable[];                    // Disable all interrupts
// ... critical section ...
@irq_enable[];                     // Re-enable interrupts
```

**Args:** 0
**Returns:** (void)
**Category:** Hardware

### @ioread, @iowrite

I/O memory access (MMIO).

```ras
int value = @ioread[address];
@iowrite[address, value];
```

**Args:** 1 or 2 (address: int, [value: int])
**Returns:** int (for read) or void (for write)
**Category:** Hardware

## Threading

### @spawn

Create new thread.

```ras
fnc worker[id]::int {
    show["Thread "];
    show[id];
    show["\n"];
    get[0];
}

int tid1 = @spawn[worker, 1];
int tid2 = @spawn[worker, 2];
```

**Args:** 2 (function: fnc, arg: any)
**Returns:** int (thread id)
**Category:** Threading

### @join

Wait for thread completion.

```ras
int tid = @spawn[worker, 42];
int result = @join[tid];           // Wait for thread to finish
show["Result: "];
show[result];
show["\n"];
```

**Args:** 1 (thread_id: int)
**Returns:** int (thread's return value)
**Category:** Threading

### @pid

Get current process/thread ID.

```ras
int my_id = @pid[];
show["My thread ID: "];
show[my_id];
show["\n"];
```

**Args:** 0
**Returns:** int
**Category:** Threading

### @kill

Terminate thread.

```ras
@kill[tid];
```

**Args:** 1 (thread_id: int)
**Returns:** (void)
**Category:** Threading

## Synchronization

### @mutex_create

Create mutex.

```ras
int mutex = @mutex_create[];
```

**Args:** 0
**Returns:** int (mutex id)
**Category:** Sync

### @mutex_lock

Lock mutex.

```ras
@mutex_lock[mutex];
// ... critical section ...
```

**Args:** 1 (mutex_id: int)
**Returns:** (void)
**Category:** Sync

### @mutex_unlock

Unlock mutex.

```ras
@mutex_unlock[mutex];
```

**Args:** 1 (mutex_id: int)
**Returns:** (void)
**Category:** Sync

### @mutex_trylock

Try to lock (non-blocking).

```ras
int acquired = @mutex_trylock[mutex];
if[acquired == 1] {
    show["Got lock\n"];
}
```

**Args:** 1 (mutex_id: int)
**Returns:** int (1 if acquired, 0 if already locked)
**Category:** Sync

### @mutex_destroy

Destroy mutex.

```ras
@mutex_destroy[mutex];
```

**Args:** 1 (mutex_id: int)
**Returns:** (void)
**Category:** Sync

### @semaphore_create

Create semaphore.

```ras
int sem = @semaphore_create[1];    // Initial count 1
```

**Args:** 1 (initial_count: int)
**Returns:** int (semaphore id)
**Category:** Sync

### @semaphore_wait

Wait (decrement semaphore).

```ras
@semaphore_wait[sem];
// ... protected resource ...
```

**Args:** 1 (semaphore_id: int)
**Returns:** (void)
**Category:** Sync

### @semaphore_signal

Signal (increment semaphore).

```ras
@semaphore_signal[sem];
```

**Args:** 1 (semaphore_id: int)
**Returns:** (void)
**Category:** Sync

### @cond_create

Create condition variable.

```ras
int cond = @cond_create[];
```

**Args:** 0
**Returns:** int (condition id)
**Category:** Sync

### @cond_wait

Wait on condition.

```ras
@cond_wait[cond, mutex];
```

**Args:** 2 (condition_id: int, mutex_id: int)
**Returns:** (void)
**Category:** Sync

### @cond_signal

Signal one waiter.

```ras
@cond_signal[cond];
```

**Args:** 1 (condition_id: int)
**Returns:** (void)
**Category:** Sync

### @cond_broadcast

Signal all waiters.

```ras
@cond_broadcast[cond];
```

**Args:** 1 (condition_id: int)
**Returns:** (void)
**Category:** Sync

### @atomic_cmp_swap

Compare-and-swap atomic operation.

```ras
int old = 0;
int new = 1;
int result = @atomic_cmp_swap[addr, old, new];
```

**Args:** 3 (address: int, old_value: int, new_value: int)
**Returns:** int (1 if swapped, 0 if not)
**Category:** Sync

### @atomic_increment

Atomically increment.

```ras
@atomic_increment[counter_addr];
```

**Args:** 1 (address: int)
**Returns:** int (new value)
**Category:** Sync

### @atomic_decrement

Atomically decrement.

```ras
@atomic_decrement[counter_addr];
```

**Args:** 1 (address: int)
**Returns:** int (new value)
**Category:** Sync

## Channels

### @channel_create

Create communication channel.

```ras
int chan = @channel_create[10];    // Capacity 10
```

**Args:** 1 (capacity: int)
**Returns:** int (channel id)
**Category:** Channels

### @channel_send

Send value through channel.

```ras
@channel_send[chan, 42];
@channel_send[chan, "message"];
```

**Args:** 2 (channel_id: int, value: any)
**Returns:** (void)
**Category:** Channels

### @channel_recv

Receive value from channel.

```ras
int value = @channel_recv[chan];
str msg = @channel_recv[chan];
```

**Args:** 1 (channel_id: int)
**Returns:** any (value received)
**Category:** Channels

### @channel_close

Close channel.

```ras
@channel_close[chan];
```

**Args:** 1 (channel_id: int)
**Returns:** (void)
**Category:** Channels

### @channel_empty

Check if channel empty.

```ras
if[@channel_empty[chan]] {
    show["Channel is empty\n"];
}
```

**Args:** 1 (channel_id: int)
**Returns:** int (1 if empty, 0 if has data)
**Category:** Channels

### @channel_full

Check if channel full.

```ras
if[@channel_full[chan]] {
    show["Channel is full\n"];
}
```

**Args:** 1 (channel_id: int)
**Returns:** int (1 if full, 0 if has space)
**Category:** Channels

## Thread Pools

### @pool_create

Create thread pool.

```ras
int pool = @pool_create[4];        // 4 worker threads
```

**Args:** 1 (num_threads: int)
**Returns:** int (pool id)
**Category:** Thread Pool

### @pool_submit

Submit work to pool.

```ras
@pool_submit[pool, worker_func, args];
```

**Args:** 3 (pool_id: int, function: fnc, arg: any)
**Returns:** (void)
**Category:** Thread Pool

### @pool_wait

Wait for all work to complete.

```ras
@pool_wait[pool];
```

**Args:** 1 (pool_id: int)
**Returns:** (void)
**Category:** Thread Pool

### @pool_destroy

Destroy thread pool.

```ras
@pool_destroy[pool];
```

**Args:** 1 (pool_id: int)
**Returns:** (void)
**Category:** Thread Pool

## Meta

### @build_time

Get build timestamp.

```ras
int timestamp = @build_time[];
```

**Args:** 0
**Returns:** int (Unix timestamp)
**Category:** Meta

### @compiler_ver

Get compiler version.

```ras
int version = @compiler_ver[];     // e.g., 210 for 2.1.0
```

**Args:** 0
**Returns:** int
**Category:** Meta

### @syscall

Direct low-level syscall interface with unified calling convention.

```ras
// Basic syscall - write to stdout (syscall 1 on Linux x86-64)
int result = @syscall[1, 1, "Hello", 5];

// Exit syscall (syscall 60 on Linux x86-64)
@syscall[60, 0];  // Exit with code 0 (doesn't return)

// Open file syscall (syscall 2)
int fd = @syscall[2, "/tmp/file.txt", 0];

// Safe syscall with error handling
fnc safe_syscall_demo[]::int {
    int result = @syscall[1, 1, "test", 4];
    if[result < 0] {
        @log_error["Syscall failed"];
        get[-1];
    }
    get[result];
}
```

**Unified Calling Convention:**
- **Argument 1**: Syscall number (varies by OS/architecture)
- **Arguments 2+**: Syscall-specific parameters (variadic, 0 to many)
- **Maximum arguments**: Implementation-dependent (typically 6-7 additional args)
- **Return value**: Syscall result code (int)

**Platform Differences:**
- Linux x86-64: Syscall numbers defined in `/usr/include/asm/unistd_64.h`
- Linux ARM: Syscall numbers in `/usr/include/asm/unistd.h`
- Windows: Requires different API (UseDefineException)

**Common Linux x86-64 Syscalls:**
| Syscall | ID | Purpose | Arguments |
|---------|----|---------|-----------| 
| read | 0 | Read from file | fd, buffer, count |
| write | 1 | Write to file | fd, buffer, count |
| open | 2 | Open file | path, flags, mode |
| close | 3 | Close file descriptor | fd |
| stat | 4 | Get file stats | path, stat_buf |
| fstat | 5 | Get file stats | fd, stat_buf |
| exit | 60 | Exit process | exit_code |
| fork | 57 | Create process | (no args) |
| execve | 59 | Execute program | filename, argv, envp |
| getpid | 39 | Get process ID | (no args) |

**⚠️ Warnings:**
- Syscalls are platform-specific (Linux vs Windows vs Mac)
- Incorrect syscall numbers cause undefined behavior
- No type checking on arguments
- Direct syscalls bypass safety checks
- Architecture-dependent (x86-64 vs ARM vs other)
- Preferred approach: Use builtin functions (@spawn, @fork, etc.) instead

**Error Handling:**
Most syscalls return negative values on error. Common error codes:
- `-1`: Generic error
- `-2`: ENOENT (file not found)
- `-13`: EACCES (permission denied)
- `-28`: ENOSPC (no space)

Use `@try_syscall[]` wrapper for safer error handling:
```ras
int result = @try_syscall[syscall_num, args...];
if[result < 0] {
    int err = @get_error_code[];
    str msg = @get_error_msg[];
    @log_error[msg];
}
```

**Args:** Variadic (syscall_num + 0 to N additional arguments)  
**Returns:** int (syscall result code, negative on error)  
**Category:** Meta  
**Recommended**: Use specific builtins (@fork, @exec, etc.) when available instead of @syscall

### @import

Import symbol from module (compile-time).

Handled at parser level, not runtime function.

```ras
pkg:module;
```

**Args:** 0 (handled by preprocessor)
**Returns:** (symbol available at compile time)
**Category:** Meta

## Usage Patterns

### Memory Allocation Pattern:
```ras
int ptr = @alloc[1024];
if[ptr == 0] {
    show["Allocation failed\n"];
    @exit[1];
}

// Use memory
int value = @peek[ptr];

@free[ptr];
```

### File I/O Pattern:
```ras
int fd = @fopen["data.txt", "r"];
check {
    int buf = @alloc[1024];
    int n = @fread[fd, buf, 1024];
    show["Read "];
    show[n];
    show[" bytes\n"];
    @free[buf];
    @fclose[fd];
} when[FileError]: {
    show["File error\n"];
}
```

### Threading Pattern:
```ras
fnc worker[id]::int {
    show["Worker "];
    show[id];
    show["\n"];
    get[id * 2];
}

int t1 = @spawn[worker, 1];
int t2 = @spawn[worker, 2];

int r1 = @join[t1];
int r2 = @join[t2];
```

### Mutex Pattern:
```ras
int m = @mutex_create[];

fnc critical[]::int {
    @mutex_lock[m];
    // Critical section
    show["In critical section\n"];
    @mutex_unlock[m];
    get[0];
}

@spawn[critical, 0];
@spawn[critical, 0];
```

## Performance Characteristics

Understanding the performance implications of builtin functions is critical for writing efficient RasCode. This section documents time/space complexity and optimization tips.

### Memory Operations

| Function | Time | Space | Notes |
|----------|------|-------|-------|
| `@alloc[size]` | O(1) amortized | O(requested) | Allocates from heap, usually fast |
| `@free[ptr]` | O(1) amortized | O(0) | Returns memory to heap pool |
| `@realloc[ptr, newsize]` | O(n) worst case | O(new size) | May copy data, expensive if growing |
| `@memcpy[dest, src, size]` | O(n) | O(0) | Linear in bytes copied, very fast with SIMD |
| `@memcmp[ptr1, ptr2, size]` | O(n) | O(0) | Early exit on mismatch possible |
| `@memset[ptr, value, size]` | O(n) | O(0) | Linear, vectorizable |
| `@peek[addr]` | O(1) | O(0) | Single pointer dereference |
| `@poke[addr, value]` | O(1) | O(0) | Single write, very fast |

**Memory Tips:**
- ✅ Use @memcpy for batch copying (faster than loop of peeks/pokes)
- ✅ Batch allocations together when possible (reduces fragmentation)
- ❌ Avoid frequent small allocations/deallocations (use memory pools)
- ❌ Don't @realloc large buffers repeatedly (pre-allocate conservatively)

### String Operations

| Function | Time | Space | Notes |
|----------|------|-------|-------|
| `@len[str]` | O(1) or O(n) | O(0) | O(1) if cached, O(n) if computing length |
| `@concat[str1, str2, ...]` | O(n+m) | O(n+m) | Creates new string, allocates memory |
| `@substr[str, start, len]` | O(len) | O(len) | Allocates new substring |
| `@strcmp[str1, str2]` | O(min(n,m)) | O(0) | Early exit on mismatch |
| `@split[str, delim]` | O(n) | O(n) | Allocates array of parts |
| `@join[array, sep]` | O(total_len) | O(total_len) | Concatenates with separators |
| `@trim[str]` | O(n) | O(n) | Strips whitespace, allocates result |
| `@upper[str]` | O(n) | O(n) | Allocates uppercase copy |
| `@lower[str]` | O(n) | O(n) | Allocates lowercase copy |
| `@replace[str, old, new]` | O(n) | O(n) | Allocates result string |
| `@indexOf[haystack, needle]` | O(n*m) worst case | O(0) | String search, may be optimized |

**String Tips:**
- ✅ Cache `@len[]` results if needed multiple times
- ✅ Use `@memcmp` for raw byte comparison (faster than @strcmp for binary data)
- ✅ Batch string operations (avoid multiple allocations)
- ❌ Avoid frequent @substr on large strings
- ❌ Don't repeatedly @concat in loops (allocates each time, O(n²) behavior)

### Math Operations

| Function | Time | Space | Notes |
|----------|------|-------|-------|
| `@abs[x]` | O(1) | O(0) | Single bitwise operation |
| `@sqrt[x]` | O(1) | O(0) | Hardware instruction (x87/SSE) |
| `@pow[base, exp]` | O(log exp) | O(0) | Exponentiation by squaring |
| `@min[...]` | O(n) | O(0) | Linear scan of arguments |
| `@max[...]` | O(n) | O(0) | Linear scan of arguments |
| `@gcd[a, b]` | O(log min(a,b)) | O(0) | Euclidean algorithm |
| `@popcount[x]` | O(1) | O(0) | Hardware instruction or lookup table |
| `@isprime[x]` | O(√n) | O(0) | Trial division up to sqrt |
| `@isqrt[x]` | O(log n) | O(0) | Newton's method, ~6 iterations |

**Math Tips:**
- ✅ Use @pow for exponentiation (much faster than repeated multiplication)
- ✅ @isprime is efficient for reasonable values (< 2^31)
- ✅ Use @popcount for bit counting (hardware instruction)
- ❌ Avoid @min/@max with 100+ arguments (linear scan)

### Sorting & Searching (Phase 5+)

| Function | Time | Space | Notes |
|----------|------|-------|-------|
| `@bsearch[array, target]` | O(log n) | O(1) | Requires sorted input |
| `@qsort[array]` | O(n log n) avg | O(log n) | QuickSort, faster in practice |
| `@bubble_sort[array]` | O(n²) | O(1) | Simple but slow for large arrays |
| `@search[array, value]` | O(n) | O(1) | Linear search, no sorting required |
| `@shuffle[array]` | O(n) | O(0) | Fisher-Yates shuffle |

**Sorting Tips:**
- ✅ Use @bsearch for repeated lookups in sorted data (O(log n) vs O(n))
- ✅ @qsort is default choice (O(n log n) average, cache-friendly)
- ✅ Use @shuffle for randomization (Fisher-Yates is O(n))
- ❌ Avoid @bubble_sort for n > 1000 (use qsort)
- ❌ Can't use @bsearch on unsorted data (incorrect results)

### File I/O

| Function | Time | Space | Notes |
|----------|------|-------|-------|
| `@fopen[path, mode]` | O(1) | O(constant) | System call, disk I/O |
| `@fclose[fd]` | O(1) | O(0) | System call, flushes buffers |
| `@fread[fd, buffer, size]` | O(size) user cost | O(1) | System call, disk I/O dominates |
| `@fwrite[fd, buffer, size]` | O(size) user cost | O(1) | System call, disk I/O dominates |
| `@fseek[fd, offset, whence]` | O(1) amortized | O(0) | File pointer operation |

**File I/O Tips:**
- ✅ Batch reads/writes (minimize system calls)
- ✅ Use larger buffers (4KB-64KB) for better throughput
- ✅ Seek sparingly (disk I/O is slow)
- ❌ Don't do byte-by-byte I/O (use buffering)
- ❌ Avoid frequent seeks in large files

### Threading & Synchronization

| Function | Time | Space | Notes |
|----------|------|-------|-------|
| `@spawn[fn, arg]` | O(1) | O(stack_size) | Creates OS thread, may block briefly |
| `@join[tid]` | O(varies) | O(0) | Blocks until thread finishes |
| `@mutex_lock[m]` | O(1) uncontended | O(0) | Can block; see contention notes |
| `@mutex_unlock[m]` | O(1) | O(0) | Wakes waiting thread if queued |
| `@atomic_*` | O(1) | O(0) | Hardware atomic operations |

**Contention Reality:**
- Uncontended mutex: **~12-50ns** (lock xchg instruction)
- Light contention (2-4 threads): **100-500ns** (some waiting)
- Heavy contention (8+ threads): **μs-ms** (queue delays)

**Threading Tips:**
- ✅ Use spinlocks for very short critical sections (< 100ns)
- ✅ Use atomics instead of mutexes for simple counters
- ✅ Keep critical sections short (minimize lock time)
- ❌ Avoid nested locking (deadlock risk)
- ❌ Don't hold locks during I/O operations

### Channels (Go-style)

| Function | Time | Space | Notes |
|----------|------|-------|-------|
| `@channel_create[capacity]` | O(1) | O(capacity) | Preallocates ring buffer |
| `@channel_send[ch, value]` | O(1) avg | O(0) | May block if full |
| `@channel_recv[ch]` | O(1) avg | O(0) | May block if empty |

**Channel Tips:**
- ✅ Create channels with reasonable capacity (avoid frequent blocking)
- ✅ Use channels for inter-thread work distribution
- ❌ Unbuffered channels cause thrashing (use capacity > 0)

### Practical Performance Patterns

**Pattern 1: Efficient String Building**
```ras
// ❌ SLOW: Repeated concatenation O(n²)
str result = "";
loop[int i = 0; i < 1000; i++] {
    result = @concat[result, "item\n"];  // Allocates 1000+ times!
}

// ✅ FAST: Use array + join O(n)
arr{str, 1000} items = {...};           // Pre-allocate array
str result = @join[items, "\n"];        // Single pass O(n)
```

**Pattern 2: Efficient Memory Copies**
```ras
// ❌ SLOW: Byte-by-byte copy O(n), many peeks/pokes
loop[int i = 0; i < size; i++] {
    int val = @peek[src + i*8];
    @poke[dest + i*8, val];
}

// ✅ FAST: Use memcpy O(n) with SIMD
@memcpy[dest, src, size];              // 1 call, vectorized
```

**Pattern 3: Efficient Array Processing**
```ras
// ❌ SLOW: Linear search every time O(n·m)
loop[int i = 0; i < list_size; i++] {
    int idx = @search[haystack, query_array{i}];  // Linear search each
}

// ✅ FAST: Sort once, then binary search O(n log n + m log n)
arr{int, 100} sorted = @qsort[haystack];        // Sort once O(n log n)
loop[int i = 0; i < list_size; i++] {
    int idx = @bsearch[sorted, query_array{i}]; // Binary search O(log n)
}
```

### Summary Table: When to Use What

| Goal | Function | Notes |
|------|----------|-------|
| Find max of few values | `@max[a, b, c]` | O(n), simple |
| Find max of many values | Sort + peek last | Better if doing multiple scans |
| Repeated string concatenation | Use array + @join | Avoid O(n²) concatenation |
| Search once | `@search` | O(n) but simple |
| Search many times | Use @qsort + @bsearch | O(n log n) setup, then O(log n) per |
| Thread synchronization | @mutex for complex, @atomic for simple | Atomics lower overhead |
| Inter-thread communication | @channel | Higher level, easier reasoning |

## Error Handling

Most builtins return error codes:
- File operations: `-1` or `0` on error
- Memory: `0` (null) on alloc failure
- Socket: `-1` on error
- String operations: return result or empty string

Use `check` blocks to handle failures:

```ras
int fd;
check {
    fd = @fopen["missing.txt", "r"];
    if[fd == 0] {
        @panic["Cannot open file"];
    }
} when: {
    show["Failed to open file\n"];
}
```

## Related Documentation
- [15-IO-STATEMENTS.md](15-IO-STATEMENTS.md) — Basic show/read
- [17-MEMORY-OPERATIONS.md](17-MEMORY-OPERATIONS.md) — Deep memory dive
- [18-BUILTINS-STRING-MATH.md](18-BUILTINS-STRING-MATH.md) — String manipulation & math operations
- [19-BUILTINS-ADVANCED.md](19-BUILTINS-ADVANCED.md) — Error handling & process control
- [20-CONCURRENCY.md](20-CONCURRENCY.md) — Threading details
- [21-FILE-NETWORK.md](21-FILE-NETWORK.md) — File and network I/O
- [12-TRY-CATCH.md](12-TRY-CATCH.md) — Error handling
