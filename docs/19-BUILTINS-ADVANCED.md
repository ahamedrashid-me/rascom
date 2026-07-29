# 19. Advanced Builtins: Error Handling & Process Control

This document covers two advanced categories: **Error Handling** (10 functions) and **Process/Resource Management** (10 functions). These are essential for robust systems programming and process lifecycle management.

---

## Part 1: Error Handling Functions

RASLang provides comprehensive error handling capabilities through dedicated builtins that complement the native `check`/`when` exception mechanism.

### @error[code, message] :: none
Raises an error/exception with optional code and message.

**Signature:**
```ras
fnc error_demo[]::int {
    // Trigger error
    @error[1, "Custom error occurred"];
    
    // Execution stops here
    show["This won't print"];
    
    get[0];
}
```

**Parameters:**
- `code`: Integer error code (0-255 typical, application-defined)
- `message`: Error description string

**Returns:** none (does not return; halts execution)

**Behavior:** Stops all execution immediately with error state

**Use Cases:** Fatal error reporting, assertion failures, unrecoverable states

---

### @assert[condition] :: none
Assertion check—fails if condition is false.

**Signature:**
```ras
fnc assert_demo[]::int {
    int x = 5;
    
    @assert[x > 0];  // Passes
    @assert[x == 5];  // Passes
    // @assert[x > 10];  // Would fail with assertion error
    
    show["Assertions passed"];
    
    get[0];
}
```

**Parameters:**
- `condition`: Boolean expression

**Returns:** none (does not return if fails; halts program)

**Behavior:** If false, halts with assertion failure message

**Use Cases:** Precondition checks, invariant validation, defensive programming

---

### @get_error_code[]::int
Retrieves the current error code from last exception.

**Signature:**
```ras
fnc error_code_demo[]::int {
    check {
        @error[42, "Something bad"];
    } when {
        int code = @get_error_code[];
        show["Error code: "];
        show[code];  // Output: 42
    }
    
    get[0];
}
```

**Parameters:** None

**Returns:** Integer error code from last caught exception

**Use Cases:** Exception handling, error recovery, conditional error response

**Note:** Must be called within a `when` block to be meaningful

---

### @get_error_msg[]::str
Retrieves the error message string from last exception.

**Signature:**
```ras
fnc error_msg_demo[]::int {
    check {
        @error[1, "Operation failed safely"];
    } when {
        str msg = @get_error_msg[];
        show["Error: "];
        show[msg];  // Output: Operation failed safely
    }
    
    get[0];
}
```

**Parameters:** None

**Returns:** Error message string from last caught exception

**Use Cases:** User-facing error messages, error logging, debugging

**Note:** Must be called within a `when` block to capture the message

---

### @clear_error[]::none
Clears the current error state, resetting error code and message.

**Signature:**
```ras
fnc clear_error_demo[]::int {
    check {
        @error[1, "First error"];
    } when {
        show["Caught first error"];
        @clear_error[];  // Clear error state
    }
    
    // Error state is now clear
    show["Continuing after error"];
    
    get[0];
}
```

**Parameters:** None

**Returns:** none

**Use Cases:** Error recovery, multi-step operations with independent error handling, state reset

---

### @check_alloc[pointer] :: bool
Validates that a memory allocation succeeded (pointer is non-null).

**Signature:**
```ras
fnc check_alloc_demo[]::int {
    byte memory = @alloc[1024];
    
    if @check_alloc[memory] {
        show["Memory allocated successfully"];
    } or {
        @error[2, "Allocation failed"];
    }
    
    @free[memory];
    get[0];
}
```

**Parameters:**
- `pointer`: Pointer returned from @alloc or similar

**Returns:** true if valid (non-null), false if null

**Use Cases:** Memory allocation validation, preventing null pointer dereference

---

### @try_syscall[syscall_num, args...] :: int
Safely executes a system call with error handling.

**Signature:**
```ras
fnc try_syscall_demo[]::int {
    // Example: open() syscall (sys_open = 2 on x86_64)
    // Returns fd or negative error code
    
    int fd = @try_syscall[2, "/tmp/test.txt", 0, 0644];
    
    if fd >= 0 {
        show["File opened, fd: "];
        show[fd];
    } or {
        show["Failed to open file"];
    }
    
    get[0];
}
```

**Parameters:**
- `syscall_num`: System call number (x86_64 ABI)
- `args`: Variable number of arguments passed to syscall

**Returns:** Return value from syscall (typically fd or status)

**Use Cases:** Raw system calls, low-level I/O, advanced file operations

**Note:** Returns negative values for errors instead of throwing

---

### @try_fopen[path, mode] :: int
Safely opens a file, returning file descriptor or error code.

**Signature:**
```ras
fnc try_fopen_demo[]::int {
    int fd = @try_fopen["/etc/passwd", "r"];
    
    if fd >= 0 {
        show["File opened successfully"];
        @fclose[fd];
    } or {
        show["Failed to open file"];
    }
    
    get[0];
}
```

**Parameters:**
- `path`: File path string
- `mode`: Open mode ("r", "w", "a", "rb", "wb", etc.)

**Returns:** File descriptor (>= 0) on success, negative errno on failure

**Use Cases:** Conditional file operations, error-aware file handling, resource management

**Note:** Returns error code instead of throwing exception

---

### @log_error[level, message] :: none
Writes error message to system log with severity level.

**Signature:**
```ras
fnc log_error_demo[]::int {
    @log_error[1, "Info: Application started"];
    @log_error[2, "Warn: Low memory condition"];
    @log_error[3, "Error: File not found"];
    @log_error[4, "Fatal: Database unavailable"];
    
    get[0];
}
```

**Parameters:**
- `level`: Severity level (1=Info, 2=Warn, 3=Error, 4=Fatal)
- `message`: Log message string

**Returns:** none

**Behavior:** Writes to system log (syslog on Unix-like systems)

**Use Cases:** Application logging, audit trails, debugging, monitoring

---

### @recover[handler_function] :: none
Sets up error recovery handler function for graceful shutdown.

**Signature:**
```ras
fnc cleanup[]::int {
    show["Cleaning up resources"];
    get[0];
}

fnc recover_demo[]::int {
    // Set up recovery handler
    @recover[cleanup];
    
    // If error occurs later, cleanup() runs first
    @error[1, "Something went wrong"];
    
    get[0];
}
```

**Parameters:**
- `handler_function`: Function to call before terminating

**Returns:** none

**Behavior:** Calls handler function when program exits due to error

**Use Cases:** Resource cleanup, graceful shutdown, state persistence on error

**Note:** Handler runs even if error halts execution

---

## Part 2: Process & Resource Management Functions

These functions manage process lifecycle, resource allocation, and external process execution.

### @fork[]::int
Creates a child process (POSIX fork).

**Signature:**
```ras
fnc fork_demo[]::int {
    int pid = @fork[];
    
    if pid == 0 {
        show["Child process"];
    } or {
        if pid > 0 {
            show["Parent process, child PID: "];
            show[pid];
        } or {
            show["Fork failed"];
        }
    }
    
    get[0];
}
```

**Returns:**
- 0 in child process
- PID (> 0) in parent process
- -1 on failure

**Use Cases:** Parallel processing, daemon creation, subprocess spawning

**Note:** Child inherits memory state but has separate address space

---

### @wait[pid] :: int
Waits for child process to terminate.

**Signature:**
```ras
fnc wait_demo[]::int {
    int pid = @fork[];
    
    if pid == 0 {
        // Child: do work, then exit
        show["Child working"];
        get[0];
    } or {
        // Parent: wait for child
        int status = @wait[pid];
        show["Child exited with status: "];
        show[status];
    }
    
    get[0];
}
```

**Parameters:**
- `pid`: Process ID to wait for

**Returns:** Exit status of child process

**Use Cases:** Process synchronization, waiting for results, producer-consumer patterns

---

### @wait_any[]::int
Waits for any child process to terminate.

**Signature:**
```ras
fnc wait_any_demo[]::int {
    int pid1 = @fork[];
    if pid1 == 0 { get[0]; }
    
    int pid2 = @fork[];
    if pid2 == 0 { get[0]; }
    
    // Parent: wait for any child
    int finished_pid = @wait_any[];
    show["Process terminated: "];
    show[finished_pid];
    
    get[0];
}
```

**Returns:** PID of child that terminated

**Use Cases:** Managing multiple children, event-driven process handling

---

### @getpid[]::int
Returns current process ID.

**Signature:**
```ras
fnc getpid_demo[]::int {
    int my_pid = @getpid[];
    show["Current PID: "];
    show[my_pid];
    
    get[0];
}
```

**Returns:** Process ID (PID) of current process

**Use Cases:** Logging, process identification, inter-process communication

---

### @signal[signal_num, handler] :: int
Registers signal handler for a specific signal.

**Signature:**
```ras
fnc handle_sigterm[]::int {
    show["SIGTERM received, cleaning up"];
    @halt[];
    get[0];
}

fnc signal_demo[]::int {
    // Register handler for SIGTERM (signal 15)
    @signal[15, handle_sigterm];
    
    show["Waiting for signal"];
    
    @sleep[10000];  // Wait 10 seconds
    
    get[0];
}
```

**Parameters:**
- `signal_num`: Signal number (UNIX signal)
- `handler`: Function pointer to call when signal received

**Returns:** Previous handler (for restoring)

**Use Cases:** Graceful shutdown, resource cleanup on termination, signal-driven events

---

### @environ[]::str
Gets entire environment variable block as string.

**Signature:**
```ras
fnc environ_demo[]::int {
    str env = @environ[];
    show["Environment size: "];
    show[@len[env]];
    
    get[0];
}
```

**Returns:** All environment variables as null-separated string

**Use Cases:** Accessing environment state, process diagnostics, shell integration

---

### @argv[index]::str
Gets command-line argument by index.

**Signature:**
```ras
fnc argv_demo[]::int {
    // Program invoked as: ./program arg1 arg2
    
    str first_arg = @argv[0];  // Program name
    str second_arg = @argv[1];  // "arg1"
    str third_arg = @argv[2];   // "arg2"
    
    show["Args: "];
    show[first_arg];
    show[second_arg];
    show[third_arg];
    
    get[0];
}
```

**Parameters:**
- `index`: Argument index (0 = program name)

**Returns:** Command-line argument string

**Use Cases:** Parsing CLI arguments, script parameters, configuration

---

### @spawn[command, args...] :: int
Spawns external process and returns PID.

**Signature:**
```ras
fnc spawn_demo[]::int {
    int pid = @spawn["/bin/echo", "Hello from subprocess"];
    
    show["Spawned process: "];
    show[pid];
    
    int status = @wait[pid];
    show["Process exited"];
    
    get[0];
}
```

**Parameters:**
- `command`: Path to executable
- `args`: Arguments to pass to executable

**Returns:** Child process ID (PID)

**Use Cases:** Running external tools, system commands, subprocess orchestration

**Note:** Parent doesn't wait automatically; must call @wait[] to synchronize

---

### @getenv[name] :: str
Gets value of named environment variable.

**Signature:**
```ras
fnc getenv_demo[]::int {
    str home = @getenv["HOME"];
    show["Home directory: "];
    show[home];
    
    str path = @getenv["PATH"];
    show["PATH: "];
    show[path];
    
    get[0];
}
```

**Parameters:**
- `name`: Environment variable name

**Returns:** Variable value (empty string if not found)

**Use Cases:** Configuration via env vars, portable paths, system settings

---

## Advanced Error Handling Pattern

```ras
fnc robust_file_operation[]::int {
    check {
        // Try file operation
        int fd = @try_fopen["/data.txt", "r"];
        
        if fd < 0 {
            @error[10, "Cannot open file"];
        }
        
        // Read data (hypothetical)
        
        @fclose[fd];
        
    } when {
        int code = @get_error_code[];
        str msg = @get_error_msg[];
        
        @log_error[3, @concat["Error ", @to_str[code], ": ", msg]];
        @clear_error[];
        
        // Continue with fallback logic
        show["Using default data"];
    }
    
    get[0];
}
```

## Advanced Process Management Pattern

```ras
fnc pipeline_demo[]::int {
    // Create producer process
    int producer = @fork[];
    if producer == 0 {
        show["Producer running"];
        get[0];
    }
    
    // Create consumer process
    int consumer = @fork[];
    if consumer == 0 {
        show["Consumer running"];
        get[0];
    }
    
    // Parent waits for both
    @wait[producer];
    @wait[consumer];
    
    show["Pipeline complete"];
    
    get[0];
}
```

---

## Summary Table: Error Handling

| Function | Input | Output | Purpose |
|----------|-------|--------|---------|
| @error | code, msg | none | Raise error |
| @assert | condition | none | Check assertion |
| @get_error_code | — | int | Retrieve error code |
| @get_error_msg | — | str | Retrieve error message |
| @clear_error | — | none | Clear error state |
| @check_alloc | ptr | bool | Validate pointer |
| @try_syscall | num, args | int | Safe syscall |
| @try_fopen | path, mode | int | Safe file open |
| @log_error | level, msg | none | Write to syslog |
| @recover | function | none | Set recovery handler |

## Summary Table: Process Control

| Function | Input | Output | Purpose |
|----------|-------|--------|---------|
| @fork | — | int | Create child |
| @wait | pid | int | Wait for child |
| @wait_any | — | int | Wait for any child |
| @getpid | — | int | Get current PID |
| @signal | signum, handler | int | Register handler |
| @environ | — | str | Get env block |
| @argv | index | str | Get argument |
| @spawn | cmd, args | int | Spawn process |
| @getenv | name | str | Get env var |
| (reserved) | — | — | (10 total) |

---

## Part 3: Time & Date Functions

RASLang provides comprehensive time/date operations for timestamps, formatting, and scheduling.

### @time[] :: int
Get current UNIX timestamp (seconds since epoch).

```ras
int ts = @time[];
// Returns: seconds since 1970-01-01 00:00:00 UTC
```

### @time_ms[] :: int
Get current time in milliseconds.

```ras
int ms = @time_ms[];
```

### @time_us[] :: int
Get current time in microseconds.

```ras
int us = @time_us[];
```

### @srand[seed] :: none
Seed the random number generator.

```ras
@srand[42];  // Seed with value
```

### @rand_new[seed] :: int
Create new PRNG with specific seed (returns PRNG ID).

### @rand_range[min, max] :: int
Generate random integer in range [min, max].

```ras
int r = @rand_range[1, 100];  // Random 1-100
```

### @rand_between[low, high] :: int
Generate random integer between two values.

### Time Extraction Functions

Extract individual components from UNIX timestamp:

```ras
int ts = @time[];
int year = @year_from_time[ts];
int month = @month_from_time[ts];  // 1-12
int day = @day_from_time[ts];      // 1-31
int hour = @hour_from_time[ts];    // 0-23
int minute = @minute_from_time[ts]; // 0-59
int second = @second_from_time[ts]; // 0-59
int dow = @day_of_week[ts];        // 0=Sunday
int doy = @day_of_year[ts];        // 1-366
int is_leap = @is_leap_year[ts];   // 1 if leap year
int days = @days_in_month[2, 2024]; // Days in month
```

### @strftime[timestamp, format] :: str
Format time as string.

```ras
str formatted = @strftime[@time[], "%Y-%m-%d %H:%M:%S"];
// Returns: "2024-04-04 14:30:45"
```

### @strptime[date_string, format] :: int
Parse time string and return timestamp.

```ras
int ts = @strptime["2024-04-04", "%Y-%m-%d"];
```

---

## Part 4: Process & Environment Management

### @chdir[path] :: int
Change current working directory.

```ras
int result = @chdir["/home/user"];
// Returns: 0 if success, -1 if error
```

### @getcwd[] :: str
Get current working directory.

```ras
str cwd = @getcwd[];
show[cwd];
```

### @getenv[name] :: str
Get environment variable value.

```ras
str home = @getenv["HOME"];
```

### @setenv[name, value] :: int
Set environment variable.

```ras
@setenv["MY_VAR", "my_value"];
```

### @unsetenv[name] :: int
Unset/remove environment variable.

```ras
@unsetenv["MY_VAR"];
```

### @getenv_int[name] :: int
Get integer environment variable.

```ras
int threads = @getenv_int["NUM_THREADS"];
```

### @setenv_int[name, value] :: int
Set integer environment variable.

```ras
@setenv_int["NUM_THREADS", 4];
```

### @getppid[] :: int
Get parent process ID.

```ras
int ppid = @getppid[];
```

### @exec[path, args] :: int
Execute program and replace current process.

```ras
@exec["/bin/ls", "-la /tmp"];
```

### @system_call[command] :: int
Execute shell command.

```ras
int result = @system_call["echo 'Hello World'"];
```

### @getrlimit[resource] :: int
Get resource limit.

```ras
int limit = @getrlimit[0];  // 0=RLIMIT_CPU
```

### @setrlimit[resource, limit] :: int
Set resource limit.

```ras
@setrlimit[0, 3600];  // Limit CPU to 1 hour
```

### @thread_count[] :: int
Get current number of threads in process.

```ras
int count = @thread_count[];
```

---

## Part 5: Advanced Concurrency (Reader-Writer Locks)

### @rwlock_create[] :: int
Create reader-writer lock.

```ras
int lock_id = @rwlock_create[];
```

### @rwlock_read[lock_id] :: int
Acquire read lock (multiple readers allowed).

```ras
@rwlock_read[lock_id];
// Now in read-locked section
```

### @rwlock_read_unlock[lock_id] :: int
Release read lock.

```ras
@rwlock_read_unlock[lock_id];
```

### @rwlock_write[lock_id] :: int
Acquire write lock (exclusive).

```ras
@rwlock_write[lock_id];
// Now in write-locked section
```

### @rwlock_write_unlock[lock_id] :: int
Release write lock.

```ras
@rwlock_write_unlock[lock_id];
```

---

## Part 6: Advanced Concurrency (Barriers & Events)

### @barrier_create[count] :: int
Create barrier for N threads.

```ras
int barrier_id = @barrier_create[4];  // Wait for 4 threads
```

### @barrier_wait[barrier_id] :: int
Wait at barrier (blocks until all threads arrive).

```ras
@barrier_wait[barrier_id];
```

### @event_create[] :: int
Create event object (signalable).

```ras
int event_id = @event_create[];
```

### @event_signal[event_id] :: int
Signal event (wake waiting threads).

```ras
@event_signal[event_id];
```

### @event_wait[event_id] :: int
Wait for event signal (blocks).

```ras
@event_wait[event_id];
```

### @event_reset[event_id] :: int
Reset event to unsignaled state.

```ras
@event_reset[event_id];
```

---

## Part 7: Type Conversion Functions

### @type[value]::target_type
Unified type conversion (compile-time determined by return type).

```ras
int i = @type["123"]::int;         // Parse string to int
str s = @type[456]::str;           // Convert int to string
deci d = @type[789]::deci;         // Convert to decimal
byte b = @type[65]::byte;          // Convert to byte
bool flag = @type[1]::bool;        // Convert to bool
```

### @to_int[value] :: int
Convert to integer (DEPRECATED: use @type[x]::int).

### @to_str[value] :: str
Convert to string (DEPRECATED: use @type[x]::str).

### @to_deci[value] :: deci
Convert to decimal (DEPRECATED: use @type[x]::deci).

### @to_byte[value] :: byte
Convert to byte (DEPRECATED: use @type[x]::byte).

### @to_bool[value] :: bool
Convert to boolean (DEPRECATED: use @type[x]::bool).

---

## Part 8: Memory Barrier Functions

### @mfence[] :: none
Full memory fence (acquire + release barrier).

```ras
@mfence[];  // Ensure all memory ops complete
```

### @lfence[] :: none
Load fence (load acquire barrier).

```ras
@lfence[];
```

### @sfence[] :: none
Store fence (store release barrier).

```ras
@sfence[];
```

---

## Part 9: Hardware & I/O

### @ioread[address] :: int
Read memory-mapped I/O register.

```ras
int value = @ioread[0xFFFF0000];  // Read from address
```

### @irq_enable[irq_num] :: none
Enable interrupt.

```ras
@irq_enable[14];  // Enable IRQ 14
```

---

## Part 10: Memory Info Functions

### @heap_start[] :: int
Get heap start address.

```ras
int start = @heap_start[];
```

### @heap_end[] :: int
Get heap end address.

```ras
int end = @heap_end[];
```

### Summary of New Functions

| Function | Category | Purpose |
|----------|----------|---------|
| @time, @time_ms, @time_us | Time | Get current time |
| @srand, @rand_range | Random | Seed/generate random |
| @*_from_time | Time | Extract date components |
| @strftime, @strptime | Time | Format/parse dates |
| @chdir, @getcwd | Process | Directory operations |
| @getenv, @setenv | Process | Environment variables |
| @exec, @system_call | Process | Execute programs |
| @rwlock_* | Concurrency | Reader-writer locks |
| @barrier_* | Concurrency | Thread barriers |
| @event_* | Concurrency | Event objects |
| @type | Conversion | Unified type conversion |
| @mfence, @lfence, @sfence | Memory | Memory fences |
| @ioread, @irq_enable | Hardware | I/O operations |
| @heap_start, @heap_end | Memory | Heap info |

```bash
rascom error_handling_demo.ras -o error_demo
./error_demo
# Shows error handling in action

rascom process_demo.ras -o process_demo
./process_demo
# Shows process management
```

---

**Next File:** See [22-COMPLETE-EXAMPLES.md](22-COMPLETE-EXAMPLES.md) for comprehensive integration examples.  
**Previous File:** See [18-BUILTINS-STRING-MATH.md](18-BUILTINS-STRING-MATH.md) for String and Math operations.
