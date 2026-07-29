# 12: Try-Catch (Error Handling)

## Check Statement (Try Block)

**Syntax** (from parser.c parse_check):

```
check {
    statements;
} when[ErrorType]: {
    handler;
} when: {
    catch_all;
}
```

The `check` keyword starts a try-catch block. Code that might fail goes in the block, and handlers catch errors.

## Basic Error Handling

**Simple try-catch:**
```ras
check {
    int result = risky_operation[];
    show[result];
} when: {
    show["Error occurred"];
}
```

## Exception Types

**Specific error types:**
```ras
check {
    int file = @fopen["data.txt", "r"];
    int data = @fread[file, buffer, 100];
    @fclose[file];
} when[FileNotFound]: {
    show["File not found"];
} when[PermissionDenied]: {
    show["Permission denied"];
} when: {
    show["Other error"];
}
```

**Catch-all handler:**
```ras
check {
    process[];
} when: {
    show["Something went wrong"];
}
```

## When Handlers

Each `when[ExceptionType]:` handler catches that specific type:

```ras
check {
    // risky code
} when[OutOfMemory]: {
    show["Ran out of memory"];
} when[DivisionByZero]: {
    show["Division by zero"];
} when[FileError]: {
    show["File operation failed"];
} when: {
    show["Unexpected error"];
}
```

**Multiple when clauses:**
- Each has a colon `:` before handler block
- Checked in order
- First matching handler executes
- Catch-all `when:` without condition catches remaining

## Error Codes

Some functions return error codes instead of raising exceptions:

```ras
// Return-based error handling
int result = some_function[];

if[result < 0] {
    show["Function failed"];
} or {
    show["Success"];
}
```

Use `check` for functions that explicitly throw exceptions (from builtins.c check  error types).

## Throwing Errors (Advanced)

From parser.c, seems like errors are thrown by builtin functions, not user throws. Builtin functions like `@panic` or file operations that fail will trigger when handlers.

**Note:** RASLang might not support explicit `throw` statements - errors are result of calling functions that fail.

## Common Error Scenarios

**File operations:**
```ras
check {
    int fd = @fopen["myfile.txt", "r"];
    int bytes_read = @fread[fd, buffer, 1024];
    @fclose[fd];
} when[FileError]: {
    show["Could not read file"];
} when: {
    show["Unknown file error"];
}
```

**Memory operations:**
```ras
check {
    int ptr = @alloc[megabytes];  // Might fail if OOM
    // Use ptr
    @free[ptr];
} when[OutOfMemory]: {
    show["Memory allocation failed"];
}
```

**Network operations:**
```ras
check {
    int sock = @socket[AF_INET, SOCK_STREAM];
    @connect[sock, address, port];
    @send[sock, data, length];
    @close[sock];
} when[NetworkError]: {
    show["Network connection failed"];
} when[SocketError]: {
    show["Socket error"];
}
```

## Nested Error Handling

```ras
check {
    check {
        inner_operation[];
    } when[InnerError]: {
        show["Inner error, attempting recovery"];
        recovery_action[];
    }
    
    outer_operation[];
} when[OuterError]: {
    show["Outer error"];
}
```

Inner handler processes first; if it doesn't re-throw, outer catch doesn't trigger.

## Resuming After Error

**Implicit resume:**
```ras
check {
    risky1[];
    risky2[];              // Only runs if risky1[] succeeds
} when[Error]: {
    show["An error occurred"];
}
// Continue here if error handled
show["Done"];
```

If error in `risky1[]`:
1. Handler executes
2. Control continues after check block
3. `risky2[]` is NOT executed

## Best Practices

**Specific handlers:**
```ras
check {
    operation[];
} when[FileNotFound]: {
    // Handle file not found specifically
} when[PermissionDenied]: {
    // Handle permission issue
} when: {
    // Catch-all for unexpected errors
}
```

**Cleanup resources:**
```ras
int fd;
check {
    fd = @fopen["file.txt", "r"];
    process_file[fd];
} when[FileError]: {
    show["Error processing file"];
} // File might not be closed if error!
@fclose[fd];        // Should check if file was opened first
```

Better:
```ras
int fd;
int result = check {
    fd = @fopen["file.txt", "r"];
    result = process_file[fd];
} when[FileError]: {
    show["Error"];
    result = -1;
}
@fclose[fd];        // Only if fd was opened
```

(Note: unclear if check returns value - might not support this pattern))

## Error Categories

From builtins.c BUILTIN_CAT_* categories, potential error types:

- **FileError** - File I/O problems
- **NetworkError** - Network operations fail
- **OutOfMemory** - Allocation fails
- **SecurityError** - Access denied
- **SystemError** - OS-level failures
- **Custom** - Application specific

Check documentation for specific function error types.

## No Exception Objects

RASLang catch blocks don't get exception object/message:

```ras
when[Error] {
    // Can't access error.message or similar
    // Just execute handler code
}
```

Functions that fail typically return error codes separately or use specific error types.

## Related Documentation
- [10-CONDITIONALS.md](10-CONDITIONALS.md) — Conditionals for error checking
- [13-FUNCTIONS.md](13-FUNCTIONS.md) — Functions that might fail
- [16-BUILTINS.md](16-BUILTINS.md) — Which builtins throw errors
- [19-FILE-NETWORK.md](19-FILE-NETWORK.md) — Common error scenarios
