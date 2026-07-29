# Advanced System-Level Test Suite for RasCode

## Overview

This directory contains comprehensive system-level feature tests for RasCode's builtin functions organized into 7 specialized test categories covering advanced system capabilities.

## Test Organization

### 1. Memory Tests (`memory_tests/`)
**Coverage**: Memory management, allocation, deallocation, and buffer operations

**Files**:
- **01_memory_allocation.ras** (8 tests)
  - Basic allocation/deallocation with @alloc/@free
  - Peek/poke memory read/write operations
  - Multiple simultaneous allocations
  - Memory initialization with @memset
  - Memory copying with @memcpy
  - Large allocations (64KB)
  - Allocation loops
  - Null pointer validation

- **02_advanced_memory.ras** (10 tests)
  - Memory pool allocation patterns
  - Allocation/deallocation cycles
  - Large block handling (1MB+)
  - Memory copy chains
  - Memory bounds protection
  - Zero-fill operations with @calloc
  - Page-aligned allocations
  - Memory reallocation patterns
  - Multi-buffer operations
  - Memory guard validation

**Key Builtins Tested**: 
`@alloc`, `@free`, `@peek`, `@poke`, `@memset`, `@memcpy`, `@realloc`, `@check_alloc`

### 2. Networking Tests (`networking_tests/`)
**Coverage**: Socket operations, client-server patterns, and network I/O

**Files**:
- **01_socket_operations.ras** (15 tests)
  - TCP socket creation and configuration
  - UDP socket operations
  - Socket binding and listening
  - Connection establishment
  - Data transmission (@send/@recv)
  - Socket acceptance
  - Graceful shutdown
  - Timeout configuration
  - Multiple simultaneous sockets
  - Peer information retrieval
  - Broadcast socket setup
  - Socket pair creation for IPC

- **02_advanced_networking.ras** (10 tests)
  - TCP server setup patterns
  - UDP broadcast messaging
  - Non-blocking socket modes
  - Send operations with flags
  - Message fragmentation handling
  - Connection timeouts
  - Socket buffer size configuration
  - Address reuse settings
  - Keep-alive settings
  - Multi-function server sockets

**Key Builtins Tested**: 
`@socket`, `@connect`, `@bind`, `@listen`, `@accept`, `@send`, `@recv`, `@shutdown`, `@setsockopt`, `@socket_close`, `@socketpair`, `@getpeername`, `@getsockname`

### 3. File I/O Tests (`file_io_tests/`)
**Coverage**: File operations, positioning, and filesystem management

**Files**:
- **01_file_operations.ras** (15 tests)
  - File open/read/write operations
  - Append mode manipulation
  - File positioning with @fseek
  - File position tracking with @ftell
  - File existence checking
  - File deletion operations
  - File copying patterns
  - File renaming
  - Buffer flushing with @fflush

- **02_advanced_file_io.ras** (12 tests)
  - Binary file handling
  - File position tracking
  - Seek and read patterns
  - EOF (end-of-file) detection
  - File truncation
  - Directory operations (@mkdir/@rmdir)
  - Directory listing with @opendir/@readdir
  - File permission management (@chmod)
  - Symbolic link creation
  - File stat information
  - Multiple file handling
  - Line-by-line file reading

**Key Builtins Tested**: 
`@fopen`, `@fread`, `@fwrite`, `@fseek`, `@ftell`, `@fclose`, `@fsize`, `@exists`, `@delete_file`, `@copy_file`, `@rename_file`, `@fflush`, `@mkdir`, `@rmdir`, `@opendir`, `@readdir`, `@chmod`, `@symlink`

### 4. Error Handling Tests (`error_handling_tests/`)
**Coverage**: Exception handling, assertions, and error recovery

**Files**:
- **01_error_handling.ras** (15 tests)
  - Error setting and retrieval with @error/@get_error_code
  - Error message handling with @get_error_msg
  - Error clearing with @clear_error
  - Assert functionality with @assert
  - DIV/0 error handling
  - Function-level error propagation
  - Error stacking patterns
  - Condition-based error handling
  - Nested error handling
  - Error propagation through calls

- **02_advanced_error_handling.ras** (10 tests)
  - Error chaining patterns
  - Nested try-catch blocks
  - Error context preservation
  - Multiple error type handling
  - Resource cleanup on error paths
  - Retry logic with error recovery
  - Custom assertion messages
  - Allocation error handling
  - Loop error conditions
  - State machine error handling

**Key Builtins Tested**: 
`@error`, `@assert`, `@get_error_code`, `@get_error_msg`, `@clear_error`, `check/when` blocks

### 5. String Tests (`string_tests/`)
**Coverage**: String manipulation, parsing, and text processing

**Files**:
- **01_string_operations.ras** (15 tests)
  - String length with @len
  - Concatenation with @concat
  - Substring extraction with @substr
  - Case conversion (@upper/@lower)
  - String searching with @find/@index
  - Whitespace trimming with @trim
  - String replacement with @replace
  - String reversal with @reverse
  - Array splitting with @split/@join
  - String repetition with @repeat
  - String comparison with @strcmp
  - Character conversion (@chr/@ord)

- **02_advanced_strings.ras** (12 tests)
  - Multi-delimiter split patterns
  - Case conversion chains
  - Template string substitution
  - String validation patterns
  - Pattern matching
  - Whitespace normalization
  - CSV parsing
  - String escape sequence handling
  - Encoding detection
  - Prefix/suffix checking
  - Space compression patterns
  - String checksum computation

**Key Builtins Tested**: 
`@len`, `@concat`, `@substr`, `@upper`, `@lower`, `@find`, `@trim`, `@replace`, `@reverse`, `@split`, `@join`, `@repeat`, `@strcmp`, `@chr`, `@ord`

### 6. Threading Tests (`threading_tests/`)
**Coverage**: Thread creation, synchronization primitives, and concurrent patterns

**Files**:
- **01_threading_synchronization.ras** (15 tests)
  - Thread creation with @thread
  - Thread joining with @thread_join
  - Thread detachment with @thread_detach
  - Mutex creation/destruction
  - Mutex lock/unlock operations
  - Semaphore creation and operations (@sem_new/@sem_wait/@sem_signal)
  - Condition variables (@cond_new/@cond_signal/@cond_wait)
  - Read-write locks (@rwlock_new/@rwlock_rdlock/@rwlock_wrlock)
  - Spin locks (@spinlock_new/@spinlock_lock)
  - Barrier synchronization
  - Thread-local storage (TLS)
  - Multi-threaded counter

- **02_advanced_threading.ras** (10 tests)
  - Producer-consumer pattern
  - Readers-writers problem
  - Barrier synchronization
  - Mutex try-lock operations
  - Semaphore counting
  - Condition variable broadcast
  - Thread-safe shared counters
  - Pipeline patterns with multiple stages
  - Lock ordering disciplines
  - Atomic operations

**Key Builtins Tested**: 
`@thread`, `@thread_join`, `@thread_detach`, `@mutex_new`, `@mutex_lock`, `@mutex_unlock`, `@mutex_trylock`, `@sem_new`, `@sem_wait`, `@sem_signal`, `@cond_new`, `@cond_signal`, `@cond_wait`, `@rwlock_new`, `@rwlock_rdlock`, `@rwlock_wrlock`, `@spinlock_new`, `@spinlock_lock`, `@barrier_new`, `@barrier_wait`, `@tls_create`, `@tls_set`, `@tls_get`, `@atomic_add`, `@atomic_sub`

### 7. System Tests (`system_tests/`)
**Coverage**: System control, timing, process management, and resource monitoring

**Files**:
- **01_system_operations.ras** (20 tests)
  - Sleep functionality with @sleep/@usleep
  - Clock timing with @clock
  - System time with @time
  - CPU time tracking with @cputime
  - Process ID retrieval (@getpid/@getppid)
  - User/group ID (@getuid/@getgid)
  - Environment variables (@getenv/@setenv)
  - Working directory (@getcwd/@chdir)
  - Memory information with @get_memory
  - Processor count with @get_nprocs
  - System commands with @system
  - Process signal handling

- **02_advanced_system.ras** (12 tests)
  - Timing benchmarks
  - CPU benchmarking
  - Memory usage tracking
  - Processor information queries
  - Load average calculation
  - Process group operations
  - File descriptor limits
  - Hostname retrieval
  - Command execution and waiting
  - Environment iteration
  - Working directory patterns
  - Timing analysis

**Key Builtins Tested**: 
`@sleep`, `@usleep`, `@clock`, `@cputime`, `@time`, `@getpid`, `@getppid`, `@getuid`, `@getgid`, `@getenv`, `@setenv`, `@getcwd`, `@chdir`, `@get_memory`, `@get_nprocs`, `@system`, `@signal`, `@getloadavg`, `@gethostname`

## Test Statistics

### Comprehensive Coverage

| Category | Files | Tests | Focus |
|----------|-------|-------|-------|
| Memory | 2 | 18 | Allocation, buffers, memory patterns |
| Networking | 2 | 25 | Sockets, protocols, I/O |
| File I/O | 2 | 27 | Files, directories, filesystem |
| Error Handling | 2 | 25 | Exceptions, recovery, assertions |
| Strings | 2 | 27 | Parsing, manipulation, validation |
| Threading | 2 | 25 | Concurrency, synchronization, IPC |
| System | 2 | 32 | Timing, processes, resources |
| **TOTAL** | **14** | **179** | **System-level features** |

## Syntax Compliance

All tests use **100% RASCode syntax** compliance:
- ✅ Explicit type annotations on all function parameters
- ✅ Keyword-based operators (and, or, xor, not)
- ✅ Proper array syntax: `arr{type, size}` declaration and `arr{index}` access
- ✅ Control flow: `if[condition] { } or[condition] { }`
- ✅ Loop syntax: `while[condition]` and `loop[init; cond; inc]`
- ✅ Function syntax: `fnc name[type param1, type param2]::return_type`
- ✅ All statements terminated with semicolons

## Running the Tests

### Compile a Single Test
```bash
cd /home/void/Desktop/RASIDE/RasCode
./rascom memory_tests/01_memory_allocation.ras -o memory_test.asm
```

### Compile All Tests in a Category
```bash
for f in memory_tests/*.ras; do
  ./rascom "$f" -o "${f%.ras}.asm"
done
```

### Compile All Advanced Tests
```bash
for dir in *_tests; do
  for f in "$dir"/*.ras; do
    echo "Compiling $f..."
    ./rascom "$f" -o "${f%.ras}.asm"
  done
done
```

## Test Pattern Examples

### Memory Pattern (Safe Allocation)
```rascode
fnc test_safe_alloc[]::int {
    int ptr = @alloc[1024];
    if[@check_alloc[ptr]] {
        @poke[ptr, 42];
        @free[ptr];
        get[1];
    }
    get[0];
}
```

### Error Handling Pattern
```rascode
fnc test_with_recovery[]::int {
    check {
        int result = risky_operation[];
    } when[err] {
        @clear_error[];
        get[0];
    }
    get[1];
}
```

### Threading Pattern
```rascode
fnc test_synchronized[]::int {
    int mutex = @mutex_new[];
    @mutex_lock[mutex];
    // critical section
    @mutex_unlock[mutex];
    @mutex_destroy[mutex];
    get[1];
}
```

## Coverage Summary

### Advanced Builtin Functions (100+ functions)
- **Memory**: 8 functions tested
- **Networking**: 12+ socket functions tested
- **File I/O**: 15+ file functions tested
- **String**: 15+ string functions tested
- **Threading**: 20+ concurrency functions tested
- **System**: 15+ system functions tested
- **Error Handling**: 5 error management functions tested

## Quality Assurance

✅ **Syntax Validation**: All tests verified for RASCode compliance
✅ **Coverage**: All 7 system-level feature categories covered
✅ **Patterns**: Common programming patterns demonstrated
✅ **Edge Cases**: Boundary conditions and error paths tested
✅ **Documentation**: Inline comments explain test purpose
✅ **Modularity**: Each test function is independent

## Integration with Build System

These tests integrate with the comprehensive test suite:
- Compatible with `./rascom` compiler
- Same syntax rules as comprehensive_tests folder
- Organized for scalability
- Ready for CI/CD integration

## Future Enhancements

Potential additions:
- Performance profiling tests
- Stress testing scenarios
- Failure mode testing
- Cross-feature integration tests
- Real-world use case implementations
