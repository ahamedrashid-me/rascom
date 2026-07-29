#ifndef BUILTINS_H
#define BUILTINS_H

#include "common.h"
#include "ast.h"
#include "codegen.h"

// Builtin function categories
typedef enum {
    BUILTIN_CAT_SYSTEM,             // System control
    BUILTIN_CAT_MEMORY,             // Memory operations
    BUILTIN_CAT_FILE,               // File I/O
    BUILTIN_CAT_NETWORK,            // Network operations
    BUILTIN_CAT_SECURITY,           // Security functions
    BUILTIN_CAT_CONVERSION,         // Type conversion
    BUILTIN_CAT_HARDWARE,           // Hardware access
    BUILTIN_CAT_PROCESS,            // Process/thread
    BUILTIN_CAT_SYNC,               // Synchronization
    BUILTIN_CAT_CHANNEL,            // Channel communication
    BUILTIN_CAT_THREADPOOL,         // Thread pool
    BUILTIN_CAT_META,               // Compiler metadata
    BUILTIN_CAT_STRING,             // String manipulation (Phase 5)
    BUILTIN_CAT_MATH,               // Math operations (Phase 5)
    BUILTIN_CAT_ERROR,              // Error handling (Phase 5)
    BUILTIN_CAT_ADVANCED_PROCESS,   // Advanced process (Phase 5)
    BUILTIN_CAT_CONCURRENCY_ADV,    // Advanced concurrency (Phase 5)
    BUILTIN_CAT_TIME,               // Date/time/random (Phase 5)
    BUILTIN_CAT_SORTING,            // Sort/search (Phase 5)
    BUILTIN_CAT_VECTOR,             // Array/vector operations (Phase 1)
} BuiltinCategory;

// Builtin function enumeration
typedef enum {
    // System Control (0-9)
    BUILTIN_EXIT = 0,
    BUILTIN_HALT,
    BUILTIN_SLEEP,
    BUILTIN_CLOCK,
    BUILTIN_PANIC,
    
    // Memory Operations (10-29) - Extended for SCMM
    BUILTIN_ADDR = 10,
    BUILTIN_PEEK,
    BUILTIN_POKE,
    BUILTIN_MEMCPY,
    BUILTIN_MEMCLR,
    BUILTIN_ALIGN,
    BUILTIN_ALLOC,          // @alloc[size] - Heap allocation
    BUILTIN_FREE,           // @free[ptr] - Free memory
    BUILTIN_REALLOC,        // @realloc[ptr, size] - Resize allocation
    BUILTIN_SALLOC,         // @salloc[size] - Stack allocation
    BUILTIN_MEMSET,         // @memset[ptr, val, size] - Set memory
    BUILTIN_MEMCMP,         // @memcmp[ptr1, ptr2, size] - Compare memory
    BUILTIN_MMAP,           // @mmap[size, prot, flags] - Memory map
    BUILTIN_MUNMAP,         // @munmap[ptr, size] - Unmap memory
    BUILTIN_MPROTECT,       // @mprotect[ptr, size, prot] - Change protection
    BUILTIN_HEAP_START,     // @heap_start[] - Get heap start
    BUILTIN_HEAP_END,       // @heap_end[] - Get heap end
    BUILTIN_HEAP_SIZE,      // @heap_size[] - Get heap size
    BUILTIN_PAGE_SIZE,      // @page_size[] - Get page size
    BUILTIN_STACK_PTR,      // @stack_ptr[] - Get stack pointer
    BUILTIN_STACK_SIZE,     // @stack_size[] - Get stack usage
    BUILTIN_MFENCE,         // @mfence[] - Memory fence
    BUILTIN_LFENCE,         // @lfence[] - Load fence
    BUILTIN_SFENCE,         // @sfence[] - Store fence
    
    // Conversion/Type Casting (30-39)
    BUILTIN_CAST = 30,      // @type[value]::type - Unified type conversion (single function for all)
    BUILTIN_TO_INT,         // @to_int[value] - DEPRECATED: Use @type[x]::int instead
    BUILTIN_TO_DECI,        // @to_deci[value] - DEPRECATED: Use @type[x]::deci instead
    BUILTIN_TO_BYTE,        // @to_byte[value] - DEPRECATED: Use @type[x]::byte instead
    BUILTIN_TO_BOOL,        // @to_bool[value] - DEPRECATED: Use @type[x]::bool instead
    BUILTIN_TO_STR,         // @to_str[value] - DEPRECATED: Use @type[x]::str instead
    
    // File I/O (40-49)
    BUILTIN_FOPEN = 40,
    BUILTIN_FREAD,
    BUILTIN_FWRITE,
    BUILTIN_FSEEK,
    BUILTIN_FCLOSE,
    BUILTIN_FDELETE,
    
    // Network (50-59)
    BUILTIN_SOCKET = 50,
    BUILTIN_CONNECT,
    BUILTIN_SEND,
    BUILTIN_RECV,
    BUILTIN_BIND,
    BUILTIN_LISTEN,
    BUILTIN_ACCEPT,
    BUILTIN_CLOSE,
    
    // Security (60-69)
    BUILTIN_HASH = 60,
    BUILTIN_RAND,
    BUILTIN_SECURE_ZERO,
    BUILTIN_ENTROPY,
    BUILTIN_VERIFY,
    
    // Utility (70-79)
    BUILTIN_LEN = 70,       // @len[str] - Get string length
    BUILTIN_TYPE,           // @type[value] - Get type name
    BUILTIN_SIZEOF,         // @sizeof[type] - Get size in bytes
    BUILTIN_CONCAT,         // @concat[str1, str2] - Concatenate strings
    BUILTIN_SUBSTR,         // @substr[str, start, len] - Get substring
    BUILTIN_STRCMP,         // @strcmp[str1, str2] - Compare strings
    BUILTIN_CHR,            // @chr[int] - Convert int to char
    BUILTIN_ORD,            // @ord[char] - Convert char to int
    
    // Hardware (80-89)
    BUILTIN_PORT_IN = 80,
    BUILTIN_PORT_OUT,
    BUILTIN_IRQ_ENABLE,
    BUILTIN_IRQ_DISABLE,
    BUILTIN_IOREAD,
    BUILTIN_IOWRITE,
    
    // Process/Thread (90-99)
    BUILTIN_SPAWN = 90,
    BUILTIN_JOIN,
    BUILTIN_PID,
    BUILTIN_KILL,
    
    // Synchronization (110-129)
    BUILTIN_MUTEX_CREATE = 110,
    BUILTIN_MUTEX_LOCK,
    BUILTIN_MUTEX_UNLOCK,
    BUILTIN_MUTEX_TRYLOCK,
    BUILTIN_MUTEX_DESTROY,
    
    BUILTIN_SEMAPHORE_CREATE = 115,
    BUILTIN_SEMAPHORE_WAIT,
    BUILTIN_SEMAPHORE_SIGNAL,
    
    BUILTIN_COND_CREATE = 120,
    BUILTIN_COND_WAIT,
    BUILTIN_COND_SIGNAL,
    BUILTIN_COND_BROADCAST,
    
    BUILTIN_ATOMIC_CMP_SWAP = 125,
    BUILTIN_ATOMIC_INCREMENT,
    BUILTIN_ATOMIC_DECREMENT,
    
    // Channel Communication (130-139)
    BUILTIN_CHANNEL_CREATE = 130,
    BUILTIN_CHANNEL_SEND,
    BUILTIN_CHANNEL_RECV,
    BUILTIN_CHANNEL_CLOSE,
    BUILTIN_CHANNEL_EMPTY,
    BUILTIN_CHANNEL_FULL,
    
    // Thread Pool (140-149)
    BUILTIN_POOL_CREATE = 140,
    BUILTIN_POOL_SUBMIT,
    BUILTIN_POOL_WAIT,
    BUILTIN_POOL_DESTROY,
    
    // String Manipulation (150-159)
    BUILTIN_SPLIT = 150,
    BUILTIN_STR_JOIN,
    BUILTIN_TRIM,
    BUILTIN_UPPER,
    BUILTIN_LOWER,
    BUILTIN_INDEX,
    BUILTIN_REPLACE,
    BUILTIN_STARTSWITH,
    BUILTIN_ENDSWITH,
    BUILTIN_REVERSE,
    BUILTIN_REPEAT,
    BUILTIN_PAD,
    
    // Math Operations (160-174)
    BUILTIN_ISQRT = 160,
    BUILTIN_POW,
    BUILTIN_ABS,
    BUILTIN_MIN,
    BUILTIN_MAX,
    BUILTIN_CLZ,
    BUILTIN_CTZ,
    BUILTIN_POPCOUNT,
    BUILTIN_GCD,
    BUILTIN_LCM,
    BUILTIN_ISPRIME,
    BUILTIN_MODPOW,
    BUILTIN_SQRT,          // @sqrt[deci] - Floating-point square root
    BUILTIN_FLOOR,         // @floor[deci] - Floor function
    BUILTIN_CEIL,          // @ceil[deci] - Ceiling function
    
    // Error Handling (170-179)
    BUILTIN_ERROR = 170,
    BUILTIN_GET_ERROR_CODE,
    BUILTIN_GET_ERROR_MSG,
    BUILTIN_CLEAR_ERROR,
    BUILTIN_ASSERT,
    BUILTIN_CHECK_ALLOC,
    BUILTIN_TRY_SYSCALL,
    BUILTIN_TRY_FOPEN,
    BUILTIN_LOG_ERROR,
    BUILTIN_RECOVER,
    
    // Process/Resource (180-189)
    BUILTIN_FORK = 180,
    BUILTIN_WAIT,
    BUILTIN_WAIT_ANY,
    BUILTIN_GETPID,
    BUILTIN_GETPPID,
    BUILTIN_CHDIR,
    BUILTIN_GETCWD,
    BUILTIN_GETENV,
    BUILTIN_SETENV,
    BUILTIN_UNSETENV,
    BUILTIN_GETENV_INT,
    BUILTIN_SETENV_INT,
    BUILTIN_EXEC,
    BUILTIN_SYSTEM_CALL,
    BUILTIN_GETRLIMIT,
    BUILTIN_SETRLIMIT,
    BUILTIN_THREAD_COUNT,
    
    // Advanced Concurrency (190-199)
    BUILTIN_RWLOCK_CREATE = 190,
    BUILTIN_RWLOCK_READ,
    BUILTIN_RWLOCK_READ_UNLOCK,
    BUILTIN_RWLOCK_WRITE,
    BUILTIN_RWLOCK_WRITE_UNLOCK,
    BUILTIN_BARRIER_CREATE,
    BUILTIN_BARRIER_WAIT,
    BUILTIN_EVENT_CREATE,
    BUILTIN_EVENT_SIGNAL,
    BUILTIN_EVENT_WAIT,
    BUILTIN_EVENT_RESET,
    
    // Date/Time (200-209)
    BUILTIN_SRAND = 200,
    BUILTIN_RAND_NEW,
    BUILTIN_RAND_RANGE,
    BUILTIN_RAND_BETWEEN,
    BUILTIN_TIME,
    BUILTIN_TIME_MS,
    BUILTIN_TIME_US,
    BUILTIN_YEAR_FROM_TIME,
    BUILTIN_MONTH_FROM_TIME,
    BUILTIN_DAY_FROM_TIME,
    BUILTIN_HOUR_FROM_TIME,
    BUILTIN_MINUTE_FROM_TIME,
    BUILTIN_SECOND_FROM_TIME,
    BUILTIN_STRFTIME,
    BUILTIN_STRPTIME,
    BUILTIN_DAY_OF_WEEK,
    BUILTIN_DAY_OF_YEAR,
    BUILTIN_IS_LEAP_YEAR,
    BUILTIN_DAYS_IN_MONTH,
    
    // Sort/Search (210-219)
    BUILTIN_QSORT = 210,
    BUILTIN_BSEARCH,
    BUILTIN_SEARCH,
    BUILTIN_SHUFFLE,
    BUILTIN_BUBBLE_SORT,
    BUILTIN_SELECTION_SORT,
    BUILTIN_INSERTION_SORT,
    BUILTIN_FIND_MIN,
    BUILTIN_FIND_MAX,
    BUILTIN_FIND_MIN_IDX,
    BUILTIN_FIND_MAX_IDX,
    BUILTIN_COUNT_VAL,
    BUILTIN_SUM,
    BUILTIN_AVERAGE,
    
    // Array/Vector Operations (220-229)
    BUILTIN_VEC_NEW = 220,      // @vec_new[type, capacity] - Create vector
    BUILTIN_VEC_PUSH,           // @vec_push[vec, value] - Add element
    BUILTIN_VEC_POP,            // @vec_pop[vec] - Remove/return last element
    BUILTIN_VEC_GET,            // @vec_get[vec, index] - Get element
    BUILTIN_VEC_SET,            // @vec_set[vec, index, value] - Set element
    BUILTIN_VEC_LEN,            // @vec_len[vec] - Get length
    BUILTIN_VEC_CAP,            // @vec_cap[vec] - Get capacity
    BUILTIN_VEC_FREE,           // @vec_free[vec] - Free vector
    BUILTIN_VEC_CLEAR,          // @vec_clear[vec] - Clear all elements
    BUILTIN_VEC_RESIZE,         // @vec_resize[vec, new_size] - Resize vector
    
    BUILTIN_SHOWF = 230,        // showf[] - Formatted output with interpolation
    
    // Meta/Build (100-109)
    BUILTIN_BUILD_TIME = 100,
    BUILTIN_COMPILER_VER,
    BUILTIN_SYSCALL,
    BUILTIN_IMPORT,
} BuiltinFunction;

// Builtin function information
typedef struct {
    const char *name;           // Function name (without @)
    BuiltinFunction id;         // Function ID
    BuiltinCategory category;   // Category
    int min_args;               // Minimum arguments
    int max_args;               // Maximum arguments (-1 for variadic)
    const char *return_type;    // Return type
    const char *description;    // Function description
} BuiltinInfo;

// Builtin function registry
extern const BuiltinInfo BUILTIN_REGISTRY[];
extern const int BUILTIN_REGISTRY_SIZE;

// Builtin function lookup
const BuiltinInfo *builtin_lookup(const char *name);
BuiltinFunction builtin_get_id(const char *name);
BuiltinCategory builtin_get_category(BuiltinFunction fn);

// Code generation for builtins
void codegen_builtin_call(CodeGen *gen, BuiltinFunction fn, ASTList *args);

// Category-specific code generation
void codegen_builtin_system(CodeGen *gen, BuiltinFunction fn, ASTList *args);
void codegen_builtin_memory(CodeGen *gen, BuiltinFunction fn, ASTList *args);
void codegen_builtin_file(CodeGen *gen, BuiltinFunction fn, ASTList *args);
void codegen_builtin_network(CodeGen *gen, BuiltinFunction fn, ASTList *args);
void codegen_builtin_security(CodeGen *gen, BuiltinFunction fn, ASTList *args);
void codegen_builtin_conversion(CodeGen *gen, BuiltinFunction fn, ASTList *args);
void codegen_builtin_hardware(CodeGen *gen, BuiltinFunction fn, ASTList *args);
void codegen_builtin_process(CodeGen *gen, BuiltinFunction fn, ASTList *args);
void codegen_builtin_sync(CodeGen *gen, BuiltinFunction fn, ASTList *args);
void codegen_builtin_channel(CodeGen *gen, BuiltinFunction fn, ASTList *args);
void codegen_builtin_threadpool(CodeGen *gen, BuiltinFunction fn, ASTList *args);
void codegen_builtin_meta(CodeGen *gen, BuiltinFunction fn, ASTList *args);

#endif // BUILTINS_H
