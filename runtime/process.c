// RasCode Process & Resource Management
// POSIX process management adapted for systems programming

#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Fork process
// Usage: @fork[] -> child_pid in parent, 0 in child, -1 on error
long sc_fork(void) {
    pid_t pid = fork();
    return (long)pid;
}

// Wait for child process
// Usage: @wait[pid] -> exit_status
long sc_wait(long pid) {
    if (pid <= 0) return -1;
    
    int status;
    pid_t result = waitpid((pid_t)pid, &status, 0);
    
    if (result < 0) return -1;
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return -WTERMSIG(status);
    
    return 0;
}

// Wait for any child process
// Usage: @wait_any[] -> child_pid, or -1 if none
long sc_wait_any(void) {
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    return (long)pid;
}

// Get current process ID
// Usage: @getpid[] -> process_id
long sc_getpid(void) {
    return (long)getpid();
}

// Get parent process ID
// Usage: @getppid[] -> parent_pid
long sc_getppid(void) {
    return (long)getppid();
}

// Change working directory
// Usage: @chdir[path] -> 0 on success, -1 on error
long sc_chdir(const char *path) {
    if (!path) return -1;
    return (long)chdir(path);
}

// Get current working directory
// Usage: @getcwd[buf, size] -> buf or 0 on error
long sc_getcwd(char *buf, long size) {
    if (!buf || size <= 0) return 0;
    
    char *result = getcwd(buf, (size_t)size);
    return result ? (long)buf : 0;
}

// Get environment variable
// Usage: @getenv[name] -> value_ptr or 0
long sc_getenv(const char *name) {
    if (!name) return 0;
    
    const char *value = getenv(name);
    return value ? (long)value : 0;
}

// Set environment variable
// Usage: @setenv[name, value] -> 0 on success, -1 on error
long sc_setenv(const char *name, const char *value) {
    if (!name) return -1;
    
    int result = setenv(name, value ? value : "", 1);
    return (long)result;
}

// Unset environment variable
// Usage: @unsetenv[name] -> 0 on success, -1 on error
long sc_unsetenv(const char *name) {
    if (!name) return -1;
    
    int result = unsetenv(name);
    return (long)result;
}

// Get environment variable as integer
// Usage: @getenv_int[name, default] -> integer value
long sc_getenv_int(const char *name, long default_val) {
    if (!name) return default_val;
    
    const char *value = getenv(name);
    if (!value) return default_val;
    
    return strtol(value, NULL, 10);
}

// Set environment variable from integer
// Usage: @setenv_int[name, value] -> 0 on success
long sc_setenv_int(const char *name, long value) {
    if (!name) return -1;
    
    char buf[64];
    snprintf(buf, sizeof(buf), "%ld", value);
    
    int result = setenv(name, buf, 1);
    return (long)result;
}

// Execute program (replaces current process)
// Usage: @exec[program, args] -> does not return on success, -1 on error
long sc_exec(const char *program, const char *args) {
    if (!program) return -1;
    
    // Simple implementation: program without args
    execl(program, program, NULL);
    return -1;  // Only reached on error
}

// System call (spawn shell command)
// Usage: @system[command] -> exit_code
long sc_system(const char *command) {
    if (!command) return -1;
    
    int status = system(command);
    if (status < 0) return -1;
    
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return status;
}

// Get resource limits
// Usage: @getrlimit[resource] -> current_limit
long sc_getrlimit(long resource) {
    // Simplified: return arbitrary limits per resource
    switch (resource) {
        case 0: return 1024 * 1024;        // RLIMIT_CPU
        case 1: return 256 * 1024 * 1024;  // RLIMIT_DATA
        case 2: return 8 * 1024 * 1024;    // RLIMIT_STACK
        case 3: return 1024 * 1024 * 1024; // RLIMIT_CORE
        default: return -1;
    }
}

// Set resource limit
// Usage: @setrlimit[resource, limit] -> 0 on success
long sc_setrlimit(long resource, long limit) {
    if (limit < 0) return -1;
    // Simplified: just return success
    return 0;
}

// Get thread count estimate
// Usage: @thread_count[] -> number of processors
long sc_thread_count(void) {
    return (long)sysconf(_SC_NPROCESSORS_ONLN);
}
