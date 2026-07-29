#include "../include/builtins.h"
#include <time.h>

// Builtin function registry
const BuiltinInfo BUILTIN_REGISTRY[] = {
    // System Control (0-9)
    {"exit", BUILTIN_EXIT, BUILTIN_CAT_SYSTEM, 1, 1, "none", "Exit program with status code"},
    {"halt", BUILTIN_HALT, BUILTIN_CAT_SYSTEM, 0, 0, "none", "Halt CPU (embedded systems)"},
    {"sleep", BUILTIN_SLEEP, BUILTIN_CAT_SYSTEM, 1, 1, "none", "Sleep for milliseconds"},
    {"clock", BUILTIN_CLOCK, BUILTIN_CAT_SYSTEM, 0, 0, "int", "Get system uptime in ms"},
    {"panic", BUILTIN_PANIC, BUILTIN_CAT_SYSTEM, 1, 1, "none", "Force termination with error message"},
    
    // Memory Operations (10-29) - SCMM Extended
    {"addr", BUILTIN_ADDR, BUILTIN_CAT_MEMORY, 1, 1, "int", "Get memory address of variable"},
    {"peek", BUILTIN_PEEK, BUILTIN_CAT_MEMORY, 1, 1, "byte", "Read byte from memory address"},
    {"poke", BUILTIN_POKE, BUILTIN_CAT_MEMORY, 2, 2, "none", "Write byte to memory address"},
    {"memcpy", BUILTIN_MEMCPY, BUILTIN_CAT_MEMORY, 3, 3, "none", "Copy memory block"},
    {"memclr", BUILTIN_MEMCLR, BUILTIN_CAT_MEMORY, 2, 2, "none", "Clear memory block"},
    {"align", BUILTIN_ALIGN, BUILTIN_CAT_MEMORY, 2, 2, "int", "Align pointer to boundary"},
    {"alloc", BUILTIN_ALLOC, BUILTIN_CAT_MEMORY, 1, 1, "int", "Allocate heap memory (size in bytes)"},
    {"free", BUILTIN_FREE, BUILTIN_CAT_MEMORY, 1, 1, "none", "Free allocated memory (ptr)"},
    {"realloc", BUILTIN_REALLOC, BUILTIN_CAT_MEMORY, 2, 2, "int", "Resize allocation (ptr, new_size)"},
    {"salloc", BUILTIN_SALLOC, BUILTIN_CAT_MEMORY, 1, 1, "int", "Stack allocate (size in bytes)"},
    {"memset", BUILTIN_MEMSET, BUILTIN_CAT_MEMORY, 3, 3, "none", "Set memory to value (ptr, val, size)"},
    {"memcmp", BUILTIN_MEMCMP, BUILTIN_CAT_MEMORY, 3, 3, "int", "Compare memory blocks (ptr1, ptr2, size)"},
    {"mmap", BUILTIN_MMAP, BUILTIN_CAT_MEMORY, 3, 3, "int", "Memory map (size, prot, flags)"},
    {"munmap", BUILTIN_MUNMAP, BUILTIN_CAT_MEMORY, 2, 2, "int", "Unmap memory (ptr, size)"},
    {"mprotect", BUILTIN_MPROTECT, BUILTIN_CAT_MEMORY, 3, 3, "int", "Change memory protection (ptr, size, prot)"},
    {"heap_start", BUILTIN_HEAP_START, BUILTIN_CAT_MEMORY, 0, 0, "int", "Get heap start address"},
    {"heap_end", BUILTIN_HEAP_END, BUILTIN_CAT_MEMORY, 0, 0, "int", "Get heap end address"},
    {"heap_size", BUILTIN_HEAP_SIZE, BUILTIN_CAT_MEMORY, 0, 0, "int", "Get current heap size"},
    {"page_size", BUILTIN_PAGE_SIZE, BUILTIN_CAT_MEMORY, 0, 0, "int", "Get system page size"},
    {"stack_ptr", BUILTIN_STACK_PTR, BUILTIN_CAT_MEMORY, 0, 0, "int", "Get stack pointer"},
    {"stack_size", BUILTIN_STACK_SIZE, BUILTIN_CAT_MEMORY, 0, 0, "int", "Get stack usage"},
    {"mfence", BUILTIN_MFENCE, BUILTIN_CAT_MEMORY, 0, 0, "none", "Memory fence (full barrier)"},
    {"lfence", BUILTIN_LFENCE, BUILTIN_CAT_MEMORY, 0, 0, "none", "Load fence"},
    {"sfence", BUILTIN_SFENCE, BUILTIN_CAT_MEMORY, 0, 0, "none", "Store fence"},
    
    // Type Conversion (30-39)
    {"type", BUILTIN_CAST, BUILTIN_CAT_CONVERSION, 1, -1, "varies", "Unified type conversion: @type[val1, val2]::int, str or @type[val]::deci"},
    {"to_int", BUILTIN_TO_INT, BUILTIN_CAT_CONVERSION, 1, 1, "int", "Convert value to int (DEPRECATED: use @type[x]::int)"},
    {"to_deci", BUILTIN_TO_DECI, BUILTIN_CAT_CONVERSION, 1, 1, "deci", "Convert value to deci (DEPRECATED: use @type[x]::deci)"},
    {"to_byte", BUILTIN_TO_BYTE, BUILTIN_CAT_CONVERSION, 1, 1, "byte", "Convert value to byte (DEPRECATED: use @type[x]::byte)"},
    {"to_bool", BUILTIN_TO_BOOL, BUILTIN_CAT_CONVERSION, 1, 1, "bool", "Convert value to bool (DEPRECATED: use @type[x]::bool)"},
    {"to_str", BUILTIN_TO_STR, BUILTIN_CAT_CONVERSION, 1, 1, "str", "Convert value to string (DEPRECATED: use @type[x]::str)"},
    
    // File I/O (40-49)
    {"fopen", BUILTIN_FOPEN, BUILTIN_CAT_FILE, 2, 2, "int", "Open file (path, mode)"},
    {"fread", BUILTIN_FREAD, BUILTIN_CAT_FILE, 3, 3, "int", "Read from file (fd, buffer, size)"},
    {"fwrite", BUILTIN_FWRITE, BUILTIN_CAT_FILE, 3, 3, "int", "Write to file (fd, buffer, size)"},
    {"fseek", BUILTIN_FSEEK, BUILTIN_CAT_FILE, 2, 2, "int", "Seek file position (fd, offset)"},
    {"fclose", BUILTIN_FCLOSE, BUILTIN_CAT_FILE, 1, 1, "int", "Close file descriptor"},
    {"fdelete", BUILTIN_FDELETE, BUILTIN_CAT_FILE, 1, 1, "int", "Delete file by path"},
    
    // Network (50-59)
    {"socket", BUILTIN_SOCKET, BUILTIN_CAT_NETWORK, 2, 2, "int", "Create socket (type, protocol)"},
    {"connect", BUILTIN_CONNECT, BUILTIN_CAT_NETWORK, 3, 3, "int", "Connect to server (socket, addr, port)"},
    {"send", BUILTIN_SEND, BUILTIN_CAT_NETWORK, 3, 3, "int", "Send data (socket, buffer, length)"},
    {"recv", BUILTIN_RECV, BUILTIN_CAT_NETWORK, 3, 3, "int", "Receive data (socket, buffer, length)"},
    {"bind", BUILTIN_BIND, BUILTIN_CAT_NETWORK, 3, 3, "int", "Bind socket (socket, addr, port)"},
    {"listen", BUILTIN_LISTEN, BUILTIN_CAT_NETWORK, 2, 2, "int", "Listen for connections (socket, backlog)"},
    {"accept", BUILTIN_ACCEPT, BUILTIN_CAT_NETWORK, 1, 1, "int", "Accept connection (socket)"},
    {"close", BUILTIN_CLOSE, BUILTIN_CAT_NETWORK, 1, 1, "int", "Close socket"},
    
    // Security (60-69)
    {"hash", BUILTIN_HASH, BUILTIN_CAT_SECURITY, 2, 2, "int", "Hash data (buffer, algorithm)"},
    {"rand", BUILTIN_RAND, BUILTIN_CAT_SECURITY, 1, 1, "int", "Generate random bytes (size)"},
    {"secure_zero", BUILTIN_SECURE_ZERO, BUILTIN_CAT_SECURITY, 2, 2, "none", "Securely zero memory (ptr, size)"},
    {"entropy", BUILTIN_ENTROPY, BUILTIN_CAT_SECURITY, 0, 0, "int", "Read hardware entropy"},
    {"verify", BUILTIN_VERIFY, BUILTIN_CAT_SECURITY, 3, 3, "bool", "Verify signature (sig, pub, data)"},
    
    // Utility (70-79)
    {"len", BUILTIN_LEN, BUILTIN_CAT_CONVERSION, 1, 1, "int", "Get length of string/array"},
    // {"type", BUILTIN_TYPE, BUILTIN_CAT_CONVERSION, 1, 1, "str", "Get type name of value"},  // REMOVED - unified to @type for conversion
    {"sizeof", BUILTIN_SIZEOF, BUILTIN_CAT_CONVERSION, 1, 1, "int", "Get size of type in bytes"},
    {"concat", BUILTIN_CONCAT, BUILTIN_CAT_CONVERSION, 2, 2, "str", "Concatenate two strings"},
    {"substr", BUILTIN_SUBSTR, BUILTIN_CAT_CONVERSION, 3, 3, "str", "Get substring (str, start, len)"},
    {"strcmp", BUILTIN_STRCMP, BUILTIN_CAT_CONVERSION, 2, 2, "int", "Compare strings (0=equal)"},
    {"chr", BUILTIN_CHR, BUILTIN_CAT_CONVERSION, 1, 1, "char", "Convert int to char"},
    {"ord", BUILTIN_ORD, BUILTIN_CAT_CONVERSION, 1, 1, "int", "Convert char to int"},
    
    // Hardware (80-89)
    {"port_in", BUILTIN_PORT_IN, BUILTIN_CAT_HARDWARE, 1, 1, "byte", "Read from I/O port"},
    {"port_out", BUILTIN_PORT_OUT, BUILTIN_CAT_HARDWARE, 2, 2, "none", "Write to I/O port (port, value)"},
    {"irq_enable", BUILTIN_IRQ_ENABLE, BUILTIN_CAT_HARDWARE, 1, 1, "none", "Enable interrupt"},
    {"irq_disable", BUILTIN_IRQ_DISABLE, BUILTIN_CAT_HARDWARE, 1, 1, "none", "Disable interrupt"},
    {"ioread", BUILTIN_IOREAD, BUILTIN_CAT_HARDWARE, 1, 1, "int", "Read memory-mapped I/O"},
    {"iowrite", BUILTIN_IOWRITE, BUILTIN_CAT_HARDWARE, 2, 2, "none", "Write memory-mapped I/O (addr, value)"},
    
    // Process/Thread (90-99)
    {"spawn", BUILTIN_SPAWN, BUILTIN_CAT_PROCESS, 2, 2, "int", "Create thread/process (fn, arg)"},
    {"join", BUILTIN_JOIN, BUILTIN_CAT_PROCESS, 1, 1, "int", "Wait for thread (tid)"},
    {"pid", BUILTIN_PID, BUILTIN_CAT_PROCESS, 0, 0, "int", "Get process ID"},
    {"kill", BUILTIN_KILL, BUILTIN_CAT_PROCESS, 1, 1, "int", "Terminate process (pid)"},
    
    // Synchronization (110-129)
    {"mutex_create", BUILTIN_MUTEX_CREATE, BUILTIN_CAT_SYNC, 0, 0, "int", "Create mutex and return ID"},
    {"mutex_lock", BUILTIN_MUTEX_LOCK, BUILTIN_CAT_SYNC, 1, 1, "int", "Lock mutex (blocking)"},
    {"mutex_unlock", BUILTIN_MUTEX_UNLOCK, BUILTIN_CAT_SYNC, 1, 1, "int", "Unlock mutex"},
    {"mutex_trylock", BUILTIN_MUTEX_TRYLOCK, BUILTIN_CAT_SYNC, 1, 1, "int", "Try to lock (non-blocking) - returns 1 if locked, 0 if already locked"},
    {"mutex_destroy", BUILTIN_MUTEX_DESTROY, BUILTIN_CAT_SYNC, 1, 1, "int", "Destroy mutex and free resources"},
    
    {"semaphore_create", BUILTIN_SEMAPHORE_CREATE, BUILTIN_CAT_SYNC, 1, 1, "int", "Create semaphore with initial count"},
    {"semaphore_wait", BUILTIN_SEMAPHORE_WAIT, BUILTIN_CAT_SYNC, 1, 1, "int", "Wait on semaphore (decrement)"},
    {"semaphore_signal", BUILTIN_SEMAPHORE_SIGNAL, BUILTIN_CAT_SYNC, 1, 1, "int", "Signal semaphore (increment)"},
    
    {"cond_create", BUILTIN_COND_CREATE, BUILTIN_CAT_SYNC, 0, 0, "int", "Create condition variable"},
    {"cond_wait", BUILTIN_COND_WAIT, BUILTIN_CAT_SYNC, 2, 2, "int", "Wait on condition variable (releases mutex atomically)"},
    {"cond_signal", BUILTIN_COND_SIGNAL, BUILTIN_CAT_SYNC, 1, 1, "int", "Signal one waiting thread"},
    {"cond_broadcast", BUILTIN_COND_BROADCAST, BUILTIN_CAT_SYNC, 1, 1, "int", "Signal all waiting threads"},
    
    {"atomic_cmp_swap", BUILTIN_ATOMIC_CMP_SWAP, BUILTIN_CAT_SYNC, 3, 3, "int", "Atomic compare-and-swap (lock-free)"},
    {"atomic_increment", BUILTIN_ATOMIC_INCREMENT, BUILTIN_CAT_SYNC, 1, 1, "int", "Atomic increment and return new value"},
    {"atomic_decrement", BUILTIN_ATOMIC_DECREMENT, BUILTIN_CAT_SYNC, 1, 1, "int", "Atomic decrement and return new value"},
    
    // Channel Communication (130-139)
    {"channel_create", BUILTIN_CHANNEL_CREATE, BUILTIN_CAT_CHANNEL, 1, 1, "int", "Create channel with capacity"},
    {"channel_send", BUILTIN_CHANNEL_SEND, BUILTIN_CAT_CHANNEL, 2, 2, "int", "Send value to channel (blocking if full)"},
    {"channel_recv", BUILTIN_CHANNEL_RECV, BUILTIN_CAT_CHANNEL, 1, 1, "int", "Receive value from channel (blocking if empty)"},
    {"channel_close", BUILTIN_CHANNEL_CLOSE, BUILTIN_CAT_CHANNEL, 1, 1, "int", "Close channel and free resources"},
    {"channel_empty", BUILTIN_CHANNEL_EMPTY, BUILTIN_CAT_CHANNEL, 1, 1, "int", "Check if channel is empty (1=empty, 0=not empty)"},
    {"channel_full", BUILTIN_CHANNEL_FULL, BUILTIN_CAT_CHANNEL, 1, 1, "int", "Check if channel is full (1=full, 0=not full)"},
    
    // Thread Pool (140-149)
    {"pool_create", BUILTIN_POOL_CREATE, BUILTIN_CAT_THREADPOOL, 1, 1, "int", "Create thread pool with N worker threads"},
    {"pool_submit", BUILTIN_POOL_SUBMIT, BUILTIN_CAT_THREADPOOL, 3, 3, "int", "Submit task to pool (pool_id, function_addr, arg)"},
    {"pool_wait", BUILTIN_POOL_WAIT, BUILTIN_CAT_THREADPOOL, 1, 1, "int", "Wait for all tasks in pool to complete"},
    {"pool_destroy", BUILTIN_POOL_DESTROY, BUILTIN_CAT_THREADPOOL, 1, 1, "int", "Destroy thread pool and free resources"},
    
    // String Manipulation (150-159)
    {"split", BUILTIN_SPLIT, BUILTIN_CAT_STRING, 2, 2, "str", "Split string by delimiter"},
    {"join", BUILTIN_STR_JOIN, BUILTIN_CAT_STRING, 2, 2, "str", "Join strings"},
    {"trim", BUILTIN_TRIM, BUILTIN_CAT_STRING, 1, 1, "str", "Trim whitespace from string"},
    {"upper", BUILTIN_UPPER, BUILTIN_CAT_STRING, 1, 1, "str", "Convert string to uppercase"},
    {"lower", BUILTIN_LOWER, BUILTIN_CAT_STRING, 1, 1, "str", "Convert string to lowercase"},
    {"indexOf", BUILTIN_INDEX, BUILTIN_CAT_STRING, 2, 2, "int", "Find index of substring"},
    {"replace", BUILTIN_REPLACE, BUILTIN_CAT_STRING, 3, 3, "str", "Replace substring in string"},
    {"startsWith", BUILTIN_STARTSWITH, BUILTIN_CAT_STRING, 2, 2, "bool", "Check if string starts with prefix"},
    {"endsWith", BUILTIN_ENDSWITH, BUILTIN_CAT_STRING, 2, 2, "bool", "Check if string ends with suffix"},
    {"reverse", BUILTIN_REVERSE, BUILTIN_CAT_STRING, 1, 1, "str", "Reverse string"},
    {"repeat", BUILTIN_REPEAT, BUILTIN_CAT_STRING, 2, 2, "str", "Repeat string N times"},
    {"pad", BUILTIN_PAD, BUILTIN_CAT_STRING, 3, 3, "str", "Pad string to length"},
    
    // Math Operations (160-174)
    {"isqrt", BUILTIN_ISQRT, BUILTIN_CAT_MATH, 1, 1, "int", "Integer square root"},
    {"pow", BUILTIN_POW, BUILTIN_CAT_MATH, 2, 2, "int", "Power function"},
    {"abs", BUILTIN_ABS, BUILTIN_CAT_MATH, 1, 1, "int", "Absolute value"},
    {"min", BUILTIN_MIN, BUILTIN_CAT_MATH, 2, -1, "int", "Minimum of values"},
    {"max", BUILTIN_MAX, BUILTIN_CAT_MATH, 2, -1, "int", "Maximum of values"},
    {"clz", BUILTIN_CLZ, BUILTIN_CAT_MATH, 1, 1, "int", "Count leading zeros"},
    {"ctz", BUILTIN_CTZ, BUILTIN_CAT_MATH, 1, 1, "int", "Count trailing zeros"},
    {"popcount", BUILTIN_POPCOUNT, BUILTIN_CAT_MATH, 1, 1, "int", "Population count (number of 1 bits)"},
    {"gcd", BUILTIN_GCD, BUILTIN_CAT_MATH, 2, 2, "int", "Greatest common divisor"},
    {"lcm", BUILTIN_LCM, BUILTIN_CAT_MATH, 2, 2, "int", "Least common multiple"},
    {"isprime", BUILTIN_ISPRIME, BUILTIN_CAT_MATH, 1, 1, "int", "Check if prime"},
    {"modpow", BUILTIN_MODPOW, BUILTIN_CAT_MATH, 3, 3, "int", "Modular exponentiation"},
    {"sqrt", BUILTIN_SQRT, BUILTIN_CAT_MATH, 1, 1, "deci", "Floating-point square root"},
    {"floor", BUILTIN_FLOOR, BUILTIN_CAT_MATH, 1, 1, "deci", "Floor function"},
    {"ceil", BUILTIN_CEIL, BUILTIN_CAT_MATH, 1, 1, "deci", "Ceiling function"},
    
    // Error Handling (170-179)
    {"error", BUILTIN_ERROR, BUILTIN_CAT_ERROR, 1, 1, "none", "Trigger error with message"},
    {"get_error_code", BUILTIN_GET_ERROR_CODE, BUILTIN_CAT_ERROR, 0, 0, "int", "Get last error code"},
    {"get_error_msg", BUILTIN_GET_ERROR_MSG, BUILTIN_CAT_ERROR, 0, 0, "str", "Get last error message"},
    {"clear_error", BUILTIN_CLEAR_ERROR, BUILTIN_CAT_ERROR, 0, 0, "none", "Clear error state"},
    {"assert", BUILTIN_ASSERT, BUILTIN_CAT_ERROR, 2, 2, "none", "Assert condition (condition, message)"},
    {"check_alloc", BUILTIN_CHECK_ALLOC, BUILTIN_CAT_ERROR, 1, 1, "int", "Check if pointer is valid allocation"},
    {"try_syscall", BUILTIN_TRY_SYSCALL, BUILTIN_CAT_ERROR, 1, -1, "int", "Try system call with error handling"},
    {"try_fopen", BUILTIN_TRY_FOPEN, BUILTIN_CAT_ERROR, 2, 2, "int", "Try file open with error handling (path, mode)"},
    {"log_error", BUILTIN_LOG_ERROR, BUILTIN_CAT_ERROR, 1, 1, "none", "Log error to stderr"},
    {"recover", BUILTIN_RECOVER, BUILTIN_CAT_ERROR, 0, 0, "int", "Recover from error (returns error code)"},
    
    // Advanced Process/Resource (180-196)
    {"fork", BUILTIN_FORK, BUILTIN_CAT_ADVANCED_PROCESS, 1, 1, "int", "Fork process (returns PID in parent, 0 in child)"},
    {"wait", BUILTIN_WAIT, BUILTIN_CAT_ADVANCED_PROCESS, 1, 1, "int", "Wait for specific process (pid)"},
    {"wait_any", BUILTIN_WAIT_ANY, BUILTIN_CAT_ADVANCED_PROCESS, 1, 1, "int", "Wait for any child process"},
    {"getpid", BUILTIN_GETPID, BUILTIN_CAT_ADVANCED_PROCESS, 0, 0, "int", "Get current process ID"},
    {"getppid", BUILTIN_GETPPID, BUILTIN_CAT_ADVANCED_PROCESS, 0, 0, "int", "Get parent process ID"},
    {"chdir", BUILTIN_CHDIR, BUILTIN_CAT_ADVANCED_PROCESS, 1, 1, "int", "Change current directory (path)"},
    {"getcwd", BUILTIN_GETCWD, BUILTIN_CAT_ADVANCED_PROCESS, 0, 0, "str", "Get current working directory"},
    {"getenv", BUILTIN_GETENV, BUILTIN_CAT_ADVANCED_PROCESS, 1, 1, "str", "Get environment variable (name)"},
    {"setenv", BUILTIN_SETENV, BUILTIN_CAT_ADVANCED_PROCESS, 2, 2, "int", "Set environment variable (name, value)"},
    {"unsetenv", BUILTIN_UNSETENV, BUILTIN_CAT_ADVANCED_PROCESS, 1, 1, "int", "Unset environment variable (name)"},
    {"getenv_int", BUILTIN_GETENV_INT, BUILTIN_CAT_ADVANCED_PROCESS, 1, 1, "int", "Get integer environment variable"},
    {"setenv_int", BUILTIN_SETENV_INT, BUILTIN_CAT_ADVANCED_PROCESS, 2, 2, "int", "Set integer environment variable"},
    {"exec", BUILTIN_EXEC, BUILTIN_CAT_ADVANCED_PROCESS, 2, 2, "int", "Execute program (path, args)"},
    {"system_call", BUILTIN_SYSTEM_CALL, BUILTIN_CAT_ADVANCED_PROCESS, 1, 1, "int", "Execute shell command (command)"},
    {"getrlimit", BUILTIN_GETRLIMIT, BUILTIN_CAT_ADVANCED_PROCESS, 1, 1, "int", "Get resource limit (resource)"},
    {"setrlimit", BUILTIN_SETRLIMIT, BUILTIN_CAT_ADVANCED_PROCESS, 2, 2, "int", "Set resource limit (resource, limit)"},
    {"thread_count", BUILTIN_THREAD_COUNT, BUILTIN_CAT_ADVANCED_PROCESS, 0, 0, "int", "Get current thread count in process"},
    
    // Advanced Concurrency (190-200)
    {"rwlock_create", BUILTIN_RWLOCK_CREATE, BUILTIN_CAT_CONCURRENCY_ADV, 0, 0, "int", "Create reader-writer lock"},
    {"rwlock_read", BUILTIN_RWLOCK_READ, BUILTIN_CAT_CONCURRENCY_ADV, 1, 1, "int", "Acquire read lock (lock_id)"},
    {"rwlock_read_unlock", BUILTIN_RWLOCK_READ_UNLOCK, BUILTIN_CAT_CONCURRENCY_ADV, 1, 1, "int", "Release read lock (lock_id)"},
    {"rwlock_write", BUILTIN_RWLOCK_WRITE, BUILTIN_CAT_CONCURRENCY_ADV, 1, 1, "int", "Acquire write lock (lock_id)"},
    {"rwlock_write_unlock", BUILTIN_RWLOCK_WRITE_UNLOCK, BUILTIN_CAT_CONCURRENCY_ADV, 1, 1, "int", "Release write lock (lock_id)"},
    {"barrier_create", BUILTIN_BARRIER_CREATE, BUILTIN_CAT_CONCURRENCY_ADV, 1, 1, "int", "Create barrier (count)"},
    {"barrier_wait", BUILTIN_BARRIER_WAIT, BUILTIN_CAT_CONCURRENCY_ADV, 1, 1, "int", "Wait at barrier (barrier_id)"},
    {"event_create", BUILTIN_EVENT_CREATE, BUILTIN_CAT_CONCURRENCY_ADV, 0, 0, "int", "Create event object"},
    {"event_signal", BUILTIN_EVENT_SIGNAL, BUILTIN_CAT_CONCURRENCY_ADV, 1, 1, "int", "Signal event (event_id)"},
    {"event_wait", BUILTIN_EVENT_WAIT, BUILTIN_CAT_CONCURRENCY_ADV, 1, 1, "int", "Wait for event (event_id)"},
    {"event_reset", BUILTIN_EVENT_RESET, BUILTIN_CAT_CONCURRENCY_ADV, 1, 1, "int", "Reset event (event_id)"},
    
    // Meta/Build (100-109)
    {"build_time", BUILTIN_BUILD_TIME, BUILTIN_CAT_META, 0, 0, "str", "Get compile timestamp"},
    {"compiler_ver", BUILTIN_COMPILER_VER, BUILTIN_CAT_META, 0, 0, "str", "Get compiler version"},
    {"syscall", BUILTIN_SYSCALL, BUILTIN_CAT_META, 1, -1, "int", "Direct system call (num, args...)"},
    {"import", BUILTIN_IMPORT, BUILTIN_CAT_META, 1, 1, "none", "Import library (.sulib)"},
    
    // Date/Time (200-219)
    {"srand", BUILTIN_SRAND, BUILTIN_CAT_TIME, 1, 1, "none", "Seed random number generator"},
    {"rand_new", BUILTIN_RAND_NEW, BUILTIN_CAT_TIME, 1, 1, "int", "Create new PRNG with seed"},
    {"rand_range", BUILTIN_RAND_RANGE, BUILTIN_CAT_TIME, 2, 2, "int", "Random int in range (min, max)"},
    {"rand_between", BUILTIN_RAND_BETWEEN, BUILTIN_CAT_TIME, 2, 2, "int", "Random between two values"},
    {"time", BUILTIN_TIME, BUILTIN_CAT_TIME, 0, 0, "int", "Get current unix timestamp"},
    {"time_ms", BUILTIN_TIME_MS, BUILTIN_CAT_TIME, 0, 0, "int", "Get current time in milliseconds"},
    {"time_us", BUILTIN_TIME_US, BUILTIN_CAT_TIME, 0, 0, "int", "Get current time in microseconds"},
    {"year_from_time", BUILTIN_YEAR_FROM_TIME, BUILTIN_CAT_TIME, 1, 1, "int", "Extract year from timestamp"},
    {"month_from_time", BUILTIN_MONTH_FROM_TIME, BUILTIN_CAT_TIME, 1, 1, "int", "Extract month from timestamp"},
    {"day_from_time", BUILTIN_DAY_FROM_TIME, BUILTIN_CAT_TIME, 1, 1, "int", "Extract day from timestamp"},
    {"hour_from_time", BUILTIN_HOUR_FROM_TIME, BUILTIN_CAT_TIME, 1, 1, "int", "Extract hour from timestamp"},
    {"minute_from_time", BUILTIN_MINUTE_FROM_TIME, BUILTIN_CAT_TIME, 1, 1, "int", "Extract minute from timestamp"},
    {"second_from_time", BUILTIN_SECOND_FROM_TIME, BUILTIN_CAT_TIME, 1, 1, "int", "Extract second from timestamp"},
    {"strftime", BUILTIN_STRFTIME, BUILTIN_CAT_TIME, 2, 2, "str", "Format time to string (time, format)"},
    {"strptime", BUILTIN_STRPTIME, BUILTIN_CAT_TIME, 2, 2, "int", "Parse time from string (str, format)"},
    {"day_of_week", BUILTIN_DAY_OF_WEEK, BUILTIN_CAT_TIME, 1, 1, "int", "Get day of week (0=Sunday)"},
    {"day_of_year", BUILTIN_DAY_OF_YEAR, BUILTIN_CAT_TIME, 1, 1, "int", "Get day of year"},
    {"is_leap_year", BUILTIN_IS_LEAP_YEAR, BUILTIN_CAT_TIME, 1, 1, "int", "Check if leap year"},
    {"days_in_month", BUILTIN_DAYS_IN_MONTH, BUILTIN_CAT_TIME, 2, 2, "int", "Days in month (year, month)"},
    
    // Sort/Search (210-224)
    {"qsort", BUILTIN_QSORT, BUILTIN_CAT_SORTING, 3, 3, "none", "Quicksort array (ptr, size, cmp)"},
    {"bsearch", BUILTIN_BSEARCH, BUILTIN_CAT_SORTING, 3, 3, "int", "Binary search in array (ptr, key, cmp)"},
    {"search", BUILTIN_SEARCH, BUILTIN_CAT_SORTING, 2, 2, "int", "Linear search in array (ptr, value)"},
    {"shuffle", BUILTIN_SHUFFLE, BUILTIN_CAT_SORTING, 2, 2, "none", "Shuffle array (ptr, size)"},
    {"bubble_sort", BUILTIN_BUBBLE_SORT, BUILTIN_CAT_SORTING, 3, 3, "none", "Bubble sort (ptr, size, cmp)"},
    {"selection_sort", BUILTIN_SELECTION_SORT, BUILTIN_CAT_SORTING, 3, 3, "none", "Selection sort (ptr, size, cmp)"},
    {"insertion_sort", BUILTIN_INSERTION_SORT, BUILTIN_CAT_SORTING, 3, 3, "none", "Insertion sort (ptr, size, cmp)"},
    {"find_min", BUILTIN_FIND_MIN, BUILTIN_CAT_SORTING, 2, 2, "int", "Find minimum value in array"},
    {"find_max", BUILTIN_FIND_MAX, BUILTIN_CAT_SORTING, 2, 2, "int", "Find maximum value in array"},
    {"find_min_idx", BUILTIN_FIND_MIN_IDX, BUILTIN_CAT_SORTING, 2, 2, "int", "Find index of minimum value"},
    {"find_max_idx", BUILTIN_FIND_MAX_IDX, BUILTIN_CAT_SORTING, 2, 2, "int", "Find index of maximum value"},
    {"count_val", BUILTIN_COUNT_VAL, BUILTIN_CAT_SORTING, 2, 2, "int", "Count value occurrences in array"},
    {"sum", BUILTIN_SUM, BUILTIN_CAT_SORTING, 2, 2, "int", "Sum all values in array"},
    {"average", BUILTIN_AVERAGE, BUILTIN_CAT_SORTING, 2, 2, "deci", "Calculate average of array"},
    
    // Vector/Array operations
    {"vec_new", BUILTIN_VEC_NEW, BUILTIN_CAT_VECTOR, 2, 2, "int", "Create a new dynamic vector"},
    {"vec_push", BUILTIN_VEC_PUSH, BUILTIN_CAT_VECTOR, 2, 2, "none", "Push element to vector"},
    {"vec_pop", BUILTIN_VEC_POP, BUILTIN_CAT_VECTOR, 1, 1, "int", "Pop and return last element"},
    {"vec_get", BUILTIN_VEC_GET, BUILTIN_CAT_VECTOR, 2, 2, "int", "Get element at index"},
    {"vec_set", BUILTIN_VEC_SET, BUILTIN_CAT_VECTOR, 3, 3, "none", "Set element at index"},
    {"vec_len", BUILTIN_VEC_LEN, BUILTIN_CAT_VECTOR, 1, 1, "int", "Get vector length"},
    {"vec_cap", BUILTIN_VEC_CAP, BUILTIN_CAT_VECTOR, 1, 1, "int", "Get vector capacity"},
    {"vec_free", BUILTIN_VEC_FREE, BUILTIN_CAT_VECTOR, 1, 1, "none", "Free vector memory"},
    {"vec_clear", BUILTIN_VEC_CLEAR, BUILTIN_CAT_VECTOR, 1, 1, "none", "Clear vector elements"},
    {"vec_resize", BUILTIN_VEC_RESIZE, BUILTIN_CAT_VECTOR, 2, 2, "none", "Resize vector to new size"},
};

const int BUILTIN_REGISTRY_SIZE = sizeof(BUILTIN_REGISTRY) / sizeof(BuiltinInfo);

// Builtin function lookup
const BuiltinInfo *builtin_lookup(const char *name) {
    for (int i = 0; i < BUILTIN_REGISTRY_SIZE; i++) {
        if (strcmp(BUILTIN_REGISTRY[i].name, name) == 0) {
            return &BUILTIN_REGISTRY[i];
        }
    }
    return NULL;
}

BuiltinFunction builtin_get_id(const char *name) {
    const BuiltinInfo *info = builtin_lookup(name);
    if (info) {
        return info->id;
    }
    return -1; // Invalid
}

BuiltinCategory builtin_get_category(BuiltinFunction fn) {
    if (fn >= 0 && fn < 10) return BUILTIN_CAT_SYSTEM;
    if (fn >= 10 && fn < 30) return BUILTIN_CAT_MEMORY;
    if (fn >= 30 && fn < 40) return BUILTIN_CAT_CONVERSION;
    if (fn >= 40 && fn < 50) return BUILTIN_CAT_FILE;
    if (fn >= 50 && fn < 60) return BUILTIN_CAT_NETWORK;
    if (fn >= 60 && fn < 70) return BUILTIN_CAT_SECURITY;
    if (fn >= 70 && fn < 80) return BUILTIN_CAT_CONVERSION; // Utility functions
    if (fn >= 80 && fn < 90) return BUILTIN_CAT_HARDWARE;
    if (fn >= 90 && fn < 100) return BUILTIN_CAT_PROCESS;
    if (fn >= 100 && fn < 110) return BUILTIN_CAT_META;
    if (fn >= 110 && fn < 130) return BUILTIN_CAT_SYNC;
    if (fn >= 130 && fn < 140) return BUILTIN_CAT_CHANNEL;
    if (fn >= 140 && fn < 150) return BUILTIN_CAT_THREADPOOL;
    if (fn >= 150 && fn < 160) return BUILTIN_CAT_STRING;    // String manipulation (150-159)
    if (fn >= 160 && fn < 180) return BUILTIN_CAT_MATH;      // Math operations (160-179, but currently 160-174)
    if (fn >= 170 && fn < 180) return BUILTIN_CAT_ERROR;     // Error handling (170-179)
    if (fn >= 180 && fn < 190) return BUILTIN_CAT_PROCESS;   // Process/resource (180-189)
    if (fn >= 190 && fn < 200) return BUILTIN_CAT_CONCURRENCY_ADV; // Advanced concurrency (190-199)
    if (fn >= 200 && fn < 210) return BUILTIN_CAT_TIME;       // Date/time (200-209)
    if (fn >= 210 && fn < 220) return BUILTIN_CAT_SORTING;    // Sorting/search (210-219)
    if (fn >= 220 && fn < 230) return BUILTIN_CAT_VECTOR;     // Vector/array operations (220-229)
    return BUILTIN_CAT_SYSTEM; // Default
}
