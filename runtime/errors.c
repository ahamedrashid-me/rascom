// RasCode Error Handling Framework
// Structured exception handling for production code

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Error codes
#define SC_ERR_NONE          0
#define SC_ERR_GENERIC       1
#define SC_ERR_MEMORY        2
#define SC_ERR_BOUNDS        3
#define SC_ERR_TYPE          4
#define SC_ERR_IO            5
#define SC_ERR_NETWORK       6
#define SC_ERR_PERMISSION    7
#define SC_ERR_TIMEOUT       8
#define SC_ERR_INVALID_ARG   9

// SECURITY: Thread-local error context for multi-threaded safety
// Each thread has its own error code and message, preventing race conditions
_Thread_local long sc_error_code = SC_ERR_NONE;
_Thread_local char sc_error_msg[256] = {0};

// Set error with message
// Usage: @error[code, message] -> error_code
long sc_error(long code, const char *msg) {
    sc_error_code = code;
    if (msg) {
        strncpy(sc_error_msg, msg, sizeof(sc_error_msg) - 1);
        sc_error_msg[sizeof(sc_error_msg) - 1] = '\0';
    } else {
        sc_error_msg[0] = '\0';
    }
    return code;
}

// Get last error code
// Usage: @get_error_code[] -> error_code
long sc_get_error_code(void) {
    return sc_error_code;
}

// Get last error message
// Usage: @get_error_msg[] -> error_message_ptr
long sc_get_error_msg(void) {
    return (long)sc_error_msg;
}

// Clear error
// Usage: @clear_error[] -> 1
long sc_clear_error(void) {
    sc_error_code = SC_ERR_NONE;
    sc_error_msg[0] = '\0';
    return 1;
}

// Assert with message
// Usage: @assert[condition, message] -> 1 if passes, triggers error if fails
long sc_assert(long condition, const char *msg) {
    if (!condition) {
        sc_error(SC_ERR_INVALID_ARG, msg ? msg : "Assertion failed");
        return 0;
    }
    return 1;
}

// Check allocation result
// Usage: @check_alloc[ptr, size] -> ptr or error
long sc_check_alloc(long ptr, long size) {
    if (!ptr) {
        char buf[128];
        snprintf(buf, sizeof(buf), "Memory allocation failed: %ld bytes", size);
        sc_error(SC_ERR_MEMORY, buf);
        return 0;
    }
    return ptr;
}

// Try-wrapper for system calls
// Usage: @try_syscall[result, syscall_num] -> result or error
long sc_try_syscall(long result, long syscall_num) {
    if (result < 0) {
        char buf[128];
        snprintf(buf, sizeof(buf), "Syscall %ld failed with code %ld", syscall_num, result);
        sc_error(SC_ERR_IO, buf);
        return -1;
    }
    return result;
}

// Try-wrapper for file operations
// Usage: @try_fopen[fd, filename] -> fd or error
long sc_try_fopen(long fd, const char *filename) {
    if (fd < 0) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Failed to open file: %s", filename ? filename : "unknown");
        sc_error(SC_ERR_IO, buf);
        return -1;
    }
    return fd;
}

// Panic with message (immediate shutdown)
// Usage: @panic_error[code, msg]
void sc_panic_error(long code, const char *msg) {
    fprintf(stderr, "\n=== PANIC ===\n");
    fprintf(stderr, "Error Code: %ld\n", code);
    // SECURITY: Always use format string, never pass user data as format string
    fprintf(stderr, "Message: %s\n", msg ? msg : "Unknown error");
    exit(1);
}

// Log error to stderr
// Usage: @log_error[code] -> 1
long sc_log_error(long code) {
    if (code == SC_ERR_NONE) return 0;
    
    const char *type = "Unknown";
    switch (code) {
        case SC_ERR_GENERIC: type = "Generic"; break;
        case SC_ERR_MEMORY: type = "Memory"; break;
        case SC_ERR_BOUNDS: type = "Bounds"; break;
        case SC_ERR_TYPE: type = "Type"; break;
        case SC_ERR_IO: type = "I/O"; break;
        case SC_ERR_NETWORK: type = "Network"; break;
        case SC_ERR_PERMISSION: type = "Permission"; break;
        case SC_ERR_TIMEOUT: type = "Timeout"; break;
        case SC_ERR_INVALID_ARG: type = "Invalid Arg"; break;
    }
    
    // SECURITY: Always use format string, never pass user data as format string
    // sc_error_msg is user-controlled and could contain format specifiers like %x, %p, etc.
    fprintf(stderr, "[ERROR] %s: %s\n", type, sc_error_msg);
    return 1;
}

// Recovery function (continue after error)
// Usage: @recover[] -> 1
long sc_recover(void) {
    long code = sc_error_code;
    sc_clear_error();
    return code;
}
