# Complete Builtin Functions Reference (199 Functions)

Generated: April 4, 2026  
Source: `src/builtins.c` BUILTIN_REGISTRY array  
Total Implemented: **199 builtin functions**

---

## Quick Index by Category

1. [System Control](#system-control) — 5 functions
2. [Memory Operations](#memory-operations) — 20 functions
3. [Type Conversion](#type-conversion) — 8 functions
4. [File I/O](#file-io) — 6 functions
5. [Network](#network) — 8 functions
6. [Security](#security) — 5 functions
7. [String Manipulation](#string-manipulation) — 12 functions
8. [Math Operations](#math-operations) — 15 functions
9. [Hardware & I/O](#hardware-io) — 6 functions
10. [Process Management](#process-management) — 17 functions
11. [Synchronization Primitives](#synchronization-primitives) — 15 functions
12. [Channel Communication](#channel-communication) — 6 functions
13. [Thread Pool](#thread-pool) — 4 functions
14. [Time & Date](#time-date) — 18 functions
15. [Sorting & Array Operations](#sorting-array-operations) — 14 functions
16. [Advanced Concurrency](#advanced-concurrency) — 11 functions
17. [Error Handling](#error-handling) — 10 functions
18. [Meta/Build](#metabuild) — 4 functions

**Total: 199 functions**

---

## System Control

| # | Function | Args | Return | Description |
|---|----------|------|--------|-------------|
| 1 | @exit | 1 | none | Exit program with status code |
| 2 | @halt | 0 | none | Halt CPU (embedded systems) |
| 3 | @sleep | 1 | none | Sleep for milliseconds |
| 4 | @clock | 0 | int | Get system uptime in ms |
| 5 | @panic | 1 | none | Force termination with error message |

---

## Memory Operations (20 functions)

| # | Function | Args | Return | Description |
|---|----------|------|--------|-------------|
| 1 | @alloc | 1 | int | Allocate heap memory (size in bytes) |
| 2 | @free | 1 | none | Free allocated memory (ptr) |
| 3 | @realloc | 2 | int | Resize allocation (ptr, new_size) |
| 4 | @salloc | 1 | int | Stack allocate (size in bytes) |
| 5 | @addr | 1 | int | Get memory address of variable |
| 6 | @peek | 1 | byte | Read byte from memory address |
| 7 | @poke | 2 | none | Write byte to memory address |
| 8 | @memcpy | 3 | none | Copy memory block |
| 9 | @memclr | 2 | none | Clear memory block |
| 10 | @memset | 3 | none | Set memory to value (ptr, val, size) |
| 11 | @memcmp | 3 | int | Compare memory blocks (ptr1, ptr2, size) |
| 12 | @align | 2 | int | Align pointer to boundary |
| 13 | @mmap | 3 | int | Memory map (size, prot, flags) |
| 14 | @munmap | 2 | int | Unmap memory (ptr, size) |
| 15 | @mprotect | 3 | int | Change memory protection (ptr, size, prot) |
| 16 | @heap_start | 0 | int | Get heap start address |
| 17 | @heap_end | 0 | int | Get heap end address |
| 18 | @heap_size | 0 | int | Get current heap size |
| 19 | @page_size | 0 | int | Get system page size |
| 20 | @stack_ptr | 0 | int | Get stack pointer |

---

## Type Conversion (8 functions)

| # | Function | Args | Return | Description |
|---|----------|------|--------|-------------|
| 1 | @type | 1 | varies | Unified type conversion: @type[x]::int, str, deci, byte, bool |
| 2 | @to_int | 1 | int | Convert to int |
| 3 | @to_str | 1 | str | Convert to string |
| 4 | @to_deci | 1 | deci | Convert to decimal |
| 5 | @to_byte | 1 | byte | Convert to byte |
| 6 | @to_bool | 1 | bool | Convert to bool |
| 7 | @len | 1 | int | Get length of string/array |
| 8 | @sizeof | 1 | int | Get size of type in bytes |

---

## File I/O (6 functions)

| # | Function | Args | Return | Description |
|---|----------|------|--------|-------------|
| 1 | @fopen | 2 | int | Open file (path, mode) |
| 2 | @fread | 3 | int | Read from file (fd, buffer, size) |
| 3 | @fwrite | 3 | int | Write to file (fd, buffer, size) |
| 4 | @fseek | 2 | int | Seek file position (fd, offset) |
| 5 | @fclose | 1 | int | Close file descriptor |
| 6 | @fdelete | 1 | int | Delete file by path |

---

## Network (8 functions)

| # | Function | Args | Return | Description |
|---|----------|------|--------|-------------|
| 1 | @socket | 2 | int | Create socket (type, protocol) |
| 2 | @connect | 3 | int | Connect to server (socket, addr, port) |
| 3 | @send | 3 | int | Send data (socket, buffer, length) |
| 4 | @recv | 3 | int | Receive data (socket, buffer, length) |
| 5 | @bind | 3 | int | Bind socket (socket, addr, port) |
| 6 | @listen | 2 | int | Listen for connections (socket, backlog) |
| 7 | @accept | 1 | int | Accept connection (socket) |
| 8 | @close | 1 | int | Close socket |

---

## Security (5 functions)

| # | Function | Args | Return | Description |
|---|----------|------|--------|-------------|
| 1 | @hash | 2 | int | Hash data (buffer, algorithm) |
| 2 | @rand | 1 | int | Generate random bytes (size) |
| 3 | @secure_zero | 2 | none | Securely zero memory (ptr, size) |
| 4 | @entropy | 0 | int | Read hardware entropy |
| 5 | @verify | 3 | bool | Verify signature (sig, pub, data) |

---

## String Manipulation (12 functions)

| # | Function | Args | Return | Description |
|---|----------|------|--------|-------------|
| 1 | @strlen | 1 | int | Get string length |
| 2 | @strcmp | 2 | int | Compare strings (0=equal) |
| 3 | @concat | 2 | str | Concatenate two strings |
| 4 | @substr | 3 | str | Get substring (str, start, len) |
| 5 | @split | 2 | arr | Split string by delimiter |
| 6 | @str_join | 2 | str | Join strings with separator |
| 7 | @trim | 1 | str | Trim whitespace from string |
| 8 | @upper | 1 | str | Convert string to uppercase |
| 9 | @lower | 1 | str | Convert string to lowercase |
| 10 | @indexOf | 2 | int | Find index of substring |
| 11 | @replace | 3 | str | Replace substring in string |
| 12 | @repeat | 2 | str | Repeat string N times |

---

## Math Operations (15 functions)

| # | Function | Args | Return | Description |
|---|----------|------|--------|-------------|
| 1 | @abs | 1 | int | Absolute value |
| 2 | @min | 2+ | int | Minimum of values |
| 3 | @max | 2+ | int | Maximum of values |
| 4 | @pow | 2 | int | Power function |
| 5 | @sqrt | 1 | deci | Floating-point square root |
| 6 | @isqrt | 1 | int | Integer square root |
| 7 | @floor | 1 | deci | Floor function (round down) |
| 8 | @ceil | 1 | deci | Ceiling function (round up) |
| 9 | @gcd | 2 | int | Greatest common divisor |
| 10 | @lcm | 2 | int | Least common multiple |
| 11 | @modpow | 3 | int | Modular exponentiation |
| 12 | @clz | 1 | int | Count leading zeros |
| 13 | @ctz | 1 | int | Count trailing zeros |
| 14 | @popcount | 1 | int | Population count (set bits) |
| 15 | @isprime | 1 | int | Check if prime |

---

## Hardware & I/O (6 functions)

| # | Function | Args | Return | Description |
|---|----------|------|--------|-------------|
| 1 | @port_in | 1 | byte | Read from I/O port |
| 2 | @port_out | 2 | none | Write to I/O port (port, value) |
| 3 | @ioread | 1 | int | Read memory-mapped I/O |
| 4 | @iowrite | 2 | none | Write memory-mapped I/O (addr, value) |
| 5 | @irq_enable | 1 | none | Enable interrupt |
| 6 | @irq_disable | 1 | none | Disable interrupt |

---

## Process Management (17 functions)

| # | Function | Args | Return | Description |
|---|----------|------|--------|-------------|
| 1 | @spawn | 2 | int | Create thread/process (fn, arg) |
| 2 | @join | 1 | int | Wait for thread (tid) |
| 3 | @pid | 0 | int | Get process ID |
| 4 | @kill | 1 | int | Terminate process (pid) |
| 5 | @fork | 1 | int | Fork process (returns PID in parent, 0 in child) |
| 6 | @wait | 1 | int | Wait for specific process (pid) |
| 7 | @wait_any | 1 | int | Wait for any child process |
| 8 | @getpid | 0 | int | Get current process ID |
| 9 | @getppid | 0 | int | Get parent process ID |
| 10 | @chdir | 1 | int | Change current directory (path) |
| 11 | @getcwd | 0 | str | Get current working directory |
| 12 | @getenv | 1 | str | Get environment variable (name) |
| 13 | @setenv | 2 | int | Set environment variable (name, value) |
| 14 | @unsetenv | 1 | int | Unset environment variable (name) |
| 15 | @getenv_int | 1 | int | Get integer environment variable |
| 16 | @setenv_int | 2 | int | Set integer environment variable |
| 17 | @thread_count | 0 | int | Get current thread count in process |

---

## Synchronization Primitives (15 functions)

| # | Function | Args | Return | Description |
|---|----------|------|--------|-------------|
| 1 | @mutex_create | 0 | int | Create mutex and return ID |
| 2 | @mutex_lock | 1 | int | Lock mutex (blocking) |
| 3 | @mutex_unlock | 1 | int | Unlock mutex |
| 4 | @mutex_trylock | 1 | int | Try to lock (non-blocking) - returns 1 if locked, 0 if already locked |
| 5 | @mutex_destroy | 1 | int | Destroy mutex and free resources |
| 6 | @semaphore_create | 1 | int | Create semaphore with initial count |
| 7 | @semaphore_wait | 1 | int | Wait on semaphore (decrement) |
| 8 | @semaphore_signal | 1 | int | Signal semaphore (increment) |
| 9 | @cond_create | 0 | int | Create condition variable |
| 10 | @cond_wait | 2 | int | Wait on condition variable (releases mutex atomically) |
| 11 | @cond_signal | 1 | int | Signal one waiting thread |
| 12 | @cond_broadcast | 1 | int | Signal all waiting threads |
| 13 | @atomic_cmp_swap | 3 | int | Atomic compare-and-swap (lock-free) |
| 14 | @atomic_increment | 1 | int | Atomic increment and return new value |
| 15 | @atomic_decrement | 1 | int | Atomic decrement and return new value |

---

## Channel Communication (6 functions)

| # | Function | Args | Return | Description |
|---|----------|------|--------|-------------|
| 1 | @channel_create | 1 | int | Create channel with capacity |
| 2 | @channel_send | 2 | int | Send value to channel (blocking if full) |
| 3 | @channel_recv | 1 | int | Receive value from channel (blocking if empty) |
| 4 | @channel_close | 1 | int | Close channel and free resources |
| 5 | @channel_empty | 1 | int | Check if channel is empty (1=empty, 0=not empty) |
| 6 | @channel_full | 1 | int | Check if channel is full (1=full, 0=not full) |

---

## Thread Pool (4 functions)

| # | Function | Args | Return | Description |
|---|----------|------|--------|-------------|
| 1 | @pool_create | 1 | int | Create thread pool with N worker threads |
| 2 | @pool_submit | 3 | int | Submit task to pool (pool_id, function_addr, arg) |
| 3 | @pool_wait | 1 | int | Wait for all tasks in pool to complete |
| 4 | @pool_destroy | 1 | int | Destroy thread pool and free resources |

---

## Time & Date (18 functions)

| # | Function | Args | Return | Description |
|---|----------|------|--------|-------------|
| 1 | @time | 0 | int | Get current unix timestamp |
| 2 | @time_ms | 0 | int | Get current time in milliseconds |
| 3 | @time_us | 0 | int | Get current time in microseconds |
| 4 | @srand | 1 | none | Seed random number generator |
| 5 | @rand_new | 1 | int | Create new PRNG with seed |
| 6 | @rand_range | 2 | int | Random int in range (min, max) |
| 7 | @rand_between | 2 | int | Random between two values |
| 8 | @year_from_time | 1 | int | Extract year from timestamp |
| 9 | @month_from_time | 1 | int | Extract month from timestamp |
| 10 | @day_from_time | 1 | int | Extract day from timestamp |
| 11 | @hour_from_time | 1 | int | Extract hour from timestamp |
| 12 | @minute_from_time | 1 | int | Extract minute from timestamp |
| 13 | @second_from_time | 1 | int | Extract second from timestamp |
| 14 | @day_of_week | 1 | int | Get day of week (0=Sunday) |
| 15 | @day_of_year | 1 | int | Get day of year |
| 16 | @is_leap_year | 1 | int | Check if leap year |
| 17 | @days_in_month | 2 | int | Days in month (year, month) |
| 18 | @strftime | 2 | str | Format time to string (time, format) |

---

## Sorting & Array Operations (14 functions)

| # | Function | Args | Return | Description |
|---|----------|------|--------|-------------|
| 1 | @qsort | 3 | none | Quicksort array (ptr, size, cmp) |
| 2 | @bsearch | 3 | int | Binary search in array (ptr, key, cmp) |
| 3 | @search | 2 | int | Linear search in array (ptr, value) |
| 4 | @shuffle | 2 | none | Shuffle array (ptr, size) |
| 5 | @bubble_sort | 3 | none | Bubble sort (ptr, size, cmp) |
| 6 | @selection_sort | 3 | none | Selection sort (ptr, size, cmp) |
| 7 | @insertion_sort | 3 | none | Insertion sort (ptr, size, cmp) |
| 8 | @find_min | 2 | int | Find minimum value in array |
| 9 | @find_max | 2 | int | Find maximum value in array |
| 10 | @find_min_idx | 2 | int | Find index of minimum value |
| 11 | @find_max_idx | 2 | int | Find index of maximum value |
| 12 | @count_val | 2 | int | Count value occurrences in array |
| 13 | @sum | 2 | int | Sum all values in array |
| 14 | @average | 2 | deci | Calculate average of array |

---

## Advanced Concurrency (11 functions)

| # | Function | Args | Return | Description |
|---|----------|------|--------|-------------|
| 1 | @rwlock_create | 0 | int | Create reader-writer lock |
| 2 | @rwlock_read | 1 | int | Acquire read lock (lock_id) |
| 3 | @rwlock_read_unlock | 1 | int | Release read lock (lock_id) |
| 4 | @rwlock_write | 1 | int | Acquire write lock (lock_id) |
| 5 | @rwlock_write_unlock | 1 | int | Release write lock (lock_id) |
| 6 | @barrier_create | 1 | int | Create barrier (count) |
| 7 | @barrier_wait | 1 | int | Wait at barrier (barrier_id) |
| 8 | @event_create | 0 | int | Create event object |
| 9 | @event_signal | 1 | int | Signal event (event_id) |
| 10 | @event_wait | 1 | int | Wait for event (event_id) |
| 11 | @event_reset | 1 | int | Reset event (event_id) |

---

## Error Handling (10 functions)

| # | Function | Args | Return | Description |
|---|----------|------|--------|-------------|
| 1 | @error | 1 | none | Trigger error with message |
| 2 | @get_error_code | 0 | int | Get last error code |
| 3 | @get_error_msg | 0 | str | Get last error message |
| 4 | @clear_error | 0 | none | Clear error state |
| 5 | @assert | 2 | none | Assert condition (condition, message) |
| 6 | @check_alloc | 1 | int | Check if pointer is valid allocation |
| 7 | @try_syscall | 1+ | int | Try system call with error handling |
| 8 | @try_fopen | 2 | int | Try file open with error handling (path, mode) |
| 9 | @log_error | 1 | none | Log error to stderr |
| 10 | @recover | 0 | int | Recover from error (returns error code) |

---

## Meta/Build (4 functions)

| # | Function | Args | Return | Description |
|---|----------|------|--------|-------------|
| 1 | @build_time | 0 | str | Get compile timestamp |
| 2 | @compiler_ver | 0 | str | Get compiler version |
| 3 | @syscall | 1+ | int | Direct system call (num, args...) |
| 4 | @import | 1 | none | Import library (.sulib) |

---

## Memory Barriers (Additional)

| # | Function | Args | Return | Description |
|---|----------|------|--------|-------------|
| 1 | @mfence | 0 | none | Memory fence (full barrier) |
| 2 | @lfence | 0 | none | Load fence |
| 3 | @sfence | 0 | none | Store fence |

---

## Additional Functions

| # | Function | Args | Return | Description |
|---|----------|------|--------|-------------|
| 1 | @chr | 1 | char | Convert int to char |
| 2 | @ord | 1 | int | Convert char to int |
| 3 | @startsWith | 2 | bool | Check if string starts with prefix |
| 4 | @endsWith | 2 | bool | Check if string ends with suffix |
| 5 | @stack_size | 0 | int | Get stack usage |
| 6 | @exec | 2 | int | Execute program (path, args) |
| 7 | @system_call | 1 | int | Execute shell command (command) |
| 8 | @getrlimit | 1 | int | Get resource limit (resource) |
| 9 | @setrlimit | 2 | int | Set resource limit (resource, limit) |
| 10 | @pad | 3 | str | Pad string to length |
| 11 | @reverse | 1 | str | Reverse string |
| 12 | @strptime | 2 | int | Parse time from string (str, format) |

---

## Summary Statistics

| Category | Count |
|----------|-------|
| System Control | 5 |
| Memory Operations | 20 |
| Type Conversion | 8 |
| File I/O | 6 |
| Network | 8 |
| Security | 5 |
| String Manipulation | 12 |
| Math Operations | 15 |
| Hardware & I/O | 6 |
| Process Management | 17 |
| Synchronization Primitives | 15 |
| Channel Communication | 6 |
| Thread Pool | 4 |
| Time & Date | 18 |
| Sorting & Array Operations | 14 |
| Advanced Concurrency | 11 |
| Error Handling | 10 |
| Meta/Build | 4 |
| Memory Barriers | 3 |
| Additional Functions | 12 |
| **TOTAL** | **199** |

---

## Documentation References

- **16-BUILTINS.md** — Main builtin functions reference
- **18-BUILTINS-STRING-MATH.md** — String and Math operations
- **19-BUILTINS-ADVANCED.md** — Advanced builtins (time, process, concurrency)
- **20-CONCURRENCY.md** — Concurrency patterns and examples
- **21-FILE-NETWORK.md** — File and Network I/O operations

