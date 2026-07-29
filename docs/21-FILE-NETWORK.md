# 19: File and Network I/O

## File Operations

### Open File

```ras
int fd = @fopen[path, mode];
```

**Modes:**
- `"r"` — Read
- `"w"` — Write (truncate)
- `"a"` — Append
- `"r+"` — Read/write
- `"w+"` — Write/read (truncate)

**Example:**
```ras
int fd = @fopen["data.txt", "r"];
if[fd == 0] {
    show["Cannot open file\n"];
    @exit[1];
}
```

### Read from File

```ras
int fd = @fopen["file.txt", "r"];
int buffer = @alloc[1024];
int bytes_read = @fread[fd, buffer, 1024];

if[bytes_read == 0] {
    show["EOF or error\n"];
}
if[bytes_read > 0] {
    show["Read "];
    show[bytes_read];
    show[" bytes\n"];
}
```

**Returns:**
- `> 0` — Bytes read
- `0` — End of file
- `-1` — Error

### Write to File

```ras
int fd = @fopen["output.txt", "w"];
str message = "Hello World\n";
int bytes_written = @fwrite[fd, message, @len[message]];

show["Wrote "];
show[bytes_written];
show[" bytes\n"];
```

### Seek in File

```ras
int fd = @fopen["file.bin", "r+"];
@fseek[fd, 100];                  // Seek to byte 100
int data = @alloc[4];
@fread[fd, data, 4];              // Read from offset 100
```

### Close File

```ras
int fd = @fopen["file.txt", "r"];
// ... read ...
@fclose[fd];
```

### Delete File

```ras
int result = @fdelete["tempfile.txt"];
if[result == 0] {
    show["File deleted\n"];
} or {
    show["Delete failed\n"];
}
```

## File Reading Patterns

### Read Entire File

```ras
fnc read_file[path]::str {
    int fd = @fopen[path, "r"];
    if[fd == 0] {
        get[""];
    }
    
    int buffer = @alloc[4096];
    int bytes = @fread[fd, buffer, 4096];
    
    @fclose[fd];
    
    // Convert buffer to string (if needed)
    get[@type[buffer]::str];
}
```

### Line-by-Line Reading

```ras
fnc process_file_lines[path]::int {
    int fd = @fopen[path, "r"];
    check {
        if[fd == 0] {
            @panic["Cannot open"];
        }
        
        int line_count = 0;
        loop[int ok = 1; ok == 1; ok = 0] {
            int buf = @alloc[256];
            int n = @fread[fd, buf, 256];
            
            if[n <= 0] {
                ok = 0;
            } or {
                line_count++;
                show["Line "];
                show[line_count];
                show[": "];
                show[@type[buf]::str];
                show["\n"];
            }
            
            @free[buf];
        }
        
        @fclose[fd];
        get[line_count];
        
    } when: {
        @fclose[fd];
        get[-1];
    }
}
```

### Binary File Reading

```ras
fnc read_binary[path]::int {
    int fd = @fopen[path, "r"];
    int data = @alloc[1024];
    
    int bytes = @fread[fd, data, 1024];
    
    // Access binary data with @peek
    int first_int = @peek[data];
    int second_int = @peek[data + 4];
    
    @fclose[fd];
    get[bytes];
}
```

## File Writing Patterns

### Write Text

```ras
fnc write_log[path, message]::int {
    int fd = @fopen[path, "a"];    // Append
    if[fd == 0] {
        get[-1];
    }
    
    int len = @len[message];
    int written = @fwrite[fd, message, len];
    
    // Add newline
    @fwrite[fd, "\n", 1];
    
    @fclose[fd];
    get[written];
}
```

### Path Construction

```ras
str dir = "/home/user";
str filename = "data.txt";
str path = @concat[dir, "/", filename];

int fd = @fopen[path, "w"];
```

## Network Operations

### Create Socket

```ras
// Both require platform knowledge
int sock = @socket[1, 1];         // AF_INET=1, SOCK_STREAM=1 (TCP)
```

### Connect to Server

```ras
int sock = @socket[1, 1];
int result = @connect[sock, "example.com", 80];

if[result == 0] {
    show["Connected\n"];
} or {
    show["Connection failed\n"];
    @exit[1];
}
```

### Send Data

```ras
str request = "GET / HTTP/1.1\r\nHost: example.com\r\n\r\n";
int bytes = @send[sock, request, @len[request]];
show["Sent "];
show[bytes];
show[" bytes\n"];
```

### Receive Data

```ras
int response = @alloc[4096];
int bytes = @recv[sock, response, 4096];

if[bytes > 0] {
    show["Received "];
    show[bytes];
    show[" bytes\n"];
} or {
    show["Connection closed\n"];
}
```

### Close Socket

```ras
@close[sock];
```

## Server Operations

### Listen for Connections

```ras
int server = @socket[1, 1];
int result = @bind[server, "0.0.0.0", 8080];

if[result == 0] {
    @listen[server, 5];           // Backlog 5
    show["Server listening on port 8080\n"];
} or {
    show["Bind failed\n"];
}
```

### Accept Connection

```ras
int client = @accept[server];
if[client > 0] {
    show["Client connected\n"];
    
    // Handle client
    int buf = @alloc[256];
    int bytes = @recv[client, buf, 256];
    
    @close[client];
} or {
    show["Accept failed\n"];
}
```

## TCP Client Example

```ras
fnc tcp_echo_client[]::int {
    int sock = @socket[1, 1];
    
    check {
        // Connect
        if[@connect[sock, "127.0.0.1", 9999] < 0] {
            @panic["Cannot connect"];
        }
        
        // Send
        str msg = "Hello Server\n";
        @send[sock, msg, @len[msg]];
        
        // Receive
        int buf = @alloc[256];
        int n = @recv[sock, buf, 256];
        show["Echo: "];
        show[@type[buf]::str];
        show["\n"];
        
        @free[buf];
        @close[sock];
        
        get[0];
        
    } when: {
        @close[sock];
        get[-1];
    }
}
```

## TCP Server Example

```ras
fnc handle_client[client_sock]::int {
    int buf = @alloc[256];
    int n = @recv[client_sock, buf, 256];
    
    if[n > 0] {
        str response = "Received your message\n";
        @send[client_sock, response, @len[response]];
    }
    
    @close[client_sock];
    @free[buf];
    get[0];
}

fnc tcp_echo_server[]::int {
    int server = @socket[1, 1];
    
    check {
        if[@bind[server, "0.0.0.0", 9999] < 0] {
            @panic["Bind failed"];
        }
        
        @listen[server, 5];
        show["Server listening on port 9999\n"];
        
        loop[int ok = 1; ok == 1; ok = 0] {
            int client = @accept[server];
            
            if[client > 0] {
                // For production: spawn thread
                handle_client[client];
            } or {
                // Accept error
                ok = 0;
            }
        }
        
        @close[server];
        get[0];
        
    } when: {
        @close[server];
        get[-1];
    }
}
```

## UDP Socket Example

```ras
fnc udp_example[]::int {
    // Creates should specify SOCK_DGRAM=2
    int sock = @socket[1, 2];     // AF_INET, SOCK_DGRAM (UDP)
    
    // Bind to local port
    @bind[sock, "0.0.0.0", 5555];
    
    int buf = @alloc[256];
    int n = @recv[sock, buf, 256];
    
    show["Received datagram\n"];
    
    @close[sock];
    get[0];
}
```

## HTTP Client Example

```ras
fnc fetch_url[host, path]::str {
    int sock = @socket[1, 1];
    
    check {
        @connect[sock, host, 80];
        
        // Build HTTP request
        str request = @concat["GET ", path, " HTTP/1.1\\r\\nHost: ", host, "\\r\\n\\r\\n"];
        @send[sock, request, @len[request]];
        
        // Read response
        int buf = @alloc[4096];
        int n = @recv[sock, buf, 4096];
        
        @close[sock];
        
        get[@type[buf]::str];
        
    } when: {
        @close[sock];
        get[""];
    }
}
```

## DNS Resolution Pattern

```ras
fnc resolve_hostname[hostname]::int {
    // RASLang may not have built-in DNS
    // Alternative: call system binary or use hardcoded IPs
    show["DNS resolution not built-in\n"];
    show["Please use IP address directly\n"];
    get[0];
}
```

## Error Codes

Common error returns:
- `0` or `-1` — Error (check context)
- File functions: `-1` on error
- Socket functions: `-1` on error
- File not found: fd = 0
- Connection refused: result = -1

## File Permission Patterns

```ras
fnc safe_file_write[path, content]::int {
    // Try write
    int fd = @fopen[path, "w"];
    
    check {
        if[fd == 0] {
            @panic["Cannot write file"];
        }
        
        int n = @fwrite[fd, content, @len[content]];
        @fclose[fd];
        
        get[n];
        
    } when: {
        show["Write error\n"];
        get[-1];
    }
}
```

## Performance Tips

1. **Batch reads:** Use larger buffers
2. **Async I/O:** Spawn threads for network
3. **Connection reuse:** Keep socket open
4. **Buffer pooling:** Allocate once, reuse

```ras
// Good: Reuse buffer
int buf = @alloc[4096];
loop[int i = 0; i < 100; i++] {
    int n = @fread[fd, buf, 4096];
    // Process buf
}
@free[buf];

// Bad: Allocate each time
loop[int i = 0; i < 100; i++] {
    int buf = @alloc[4096];
    int n = @fread[fd, buf, 4096];
    @free[buf];
    // Slow!
}
```

## Secure File Operations

```ras
fnc secure_read_file[path]::int {
    int fd = @fopen[path, "r"];
    check {
        int buf = @salloc[256];   // Secure (zero'd)
        
        int n = @fread[fd, buf, 256];
        
        // Use content...
        
        @secure_zero[buf, 256];   // Crypto zero
        @free[buf];
        
        @fclose[fd];
        get[n];
        
    } when: {
        get[-1];
    }
}
```

## File I/O Error Codes & Status Values

### @fopen Return Values

| Value | Meaning | Cause | Solution |
|-------|---------|-------|----------|
| `> 0` | Success | File opened | Use as file descriptor |
| `0` | Null/Invalid | Various errors | Check errno or @get_error_code |
| `-1` | Error flag | File not found, permission denied | Verify path and permissions |

**Common @fopen Errors:**

| Errno | Symbol | Cause | Example |
|-------|--------|-------|---------|
| 2 | ENOENT | File not found | Path doesn't exist |
| 13 | EACCES | Permission denied | No read/write permission |
| 21 | EISDIR | Is a directory | Tried to open directory as file |
| 24 | EMFILE | Too many open files | Reached system fd limit |
| 28 | ENOSPC | No space on device | Disk full (on write) |
| 30 | EROFS | Read-only file system | Tried to write to read-only FS |

**Safe fopen Pattern:**
```ras
int fd = @fopen[path, "r"];
check {
    if[fd == 0] {
        int errno = @get_error_code[];
        str msg = @get_error_msg[];
        @log_error[@concat["Cannot open file: ", msg]];
        @panic["File open failed"];
    }
    // ... use fd ...
    @fclose[fd];
} when: {
    show["Error during file operation\n"];
}
```

---

### @fread Return Values

| Value | Meaning | Cause |
|-------|---------|-------|
| `> 0` | Bytes read | Successfully read data |
| `0` | EOF reached | End of file (no more data) |
| `-1` | Read error | I/O error, corrupted data, or permissions |

**Common @fread Errors:**

| Condition | errno | Meaning |
|-----------|-------|---------|
| Bad file descriptor | 9 (EBADF) | fd is invalid or already closed |
| Not a file | 21 (EISDIR) | fd points to directory, not file |
| I/O error | 5 (EIO) | Physical read error (disk corruption?) |
| Interrupted | 4 (EINTR) | Signal interrupted read (retry usually works) |
| Invalid argument | 22 (EINVAL) | Invalid fd or buffer alignment |

**Safe fread Pattern:**
```ras
int fd = @fopen[path, "r"];
int buffer = @alloc[4096];
int n = @fread[fd, buffer, 4096];

check {
    if[n < 0] {
        int err = @get_error_code[];
        str msg = @get_error_msg[];
        @log_error[msg];
        @panic["Read failed"];
    }
    if[n == 0] {
        show["End of file reached\n"];
    }
    if[n > 0] {
        show["Read "];
        show[n];
        show[" bytes\n"];
    }
} when: {
    show["Error during file read\n"];
}

@free[buffer];
@fclose[fd];
```

---

### @fwrite Return Values

| Value | Meaning | Cause |
|-------|---------|-------|
| `> 0` | Bytes written | Successfully wrote data |
| `0` | No bytes written | Buffer empty or write would block |
| `-1` | Write error | Disk full, permission denied, or I/O error |

**Common @fwrite Errors:**

| Condition | errno | Meaning |
|-----------|-------|---------|
| Disk full | 28 (ENOSPC) | No space left on device |
| Permission denied | 13 (EACCES) | No write permission on file |
| Read-only file system | 30 (EROFS) | Cannot write to read-only FS |
| File too large | 27 (EFBIG) | Write would exceed max file size |
| Bad file descriptor | 9 (EBADF) | File opened in read mode, not write |
| I/O error | 5 (EIO) | Hardware failure or bad sector |

**Safe fwrite Pattern:**
```ras
int fd = @fopen["output.txt", "w"];
str data = "Hello World\n";
int total_written = 0;

check {
    int n = @fwrite[fd, data, @len[data]];
    if[n < 0] {
        int err = @get_error_code[];
        if[err == 28] {
            @panic["Disk full"];
        }
        str msg = @get_error_msg[];
        @log_error[msg];
    }
    total_written = n;
} when: {
    show["Write error occurred\n"];
}

if[total_written == @len[data]] {
    show["✓ All data written\n"];
} or {
    show["✗ Partial write: "];
    show[total_written];
    show[" of "];
    show[@len[data]];
    show[" bytes\n"];
}

@fclose[fd];
```

---

### @fseek Return Values

| Value | Meaning | Cause |
|-------|---------|-------|
| `0` | Success | Seek completed |
| `-1` | Error | Invalid offset or closed file |

**Seek Modes (whence parameter):**

| Value | Symbol | Meaning |
|-------|--------|---------|
| `0` | SEEK_SET | Absolute position from start |
| `1` | SEEK_CUR | Relative to current position |
| `2` | SEEK_END | Relative to end of file |

**Common @fseek Errors:**

| Condition | errno | Meaning |
|-----------|-------|---------|
| Bad file descriptor | 9 (EBADF) | fd is invalid or closed |
| Illegal seek | 29 (ESPIPE) | File doesn't support seeking (pipe/socket) |
| Invalid argument | 22 (EINVAL) | Invalid whence or offset |

**Safe fseek Pattern:**
```ras
int fd = @fopen["data.bin", "r+"];

check {
    // Seek to byte 1000
    int result = @fseek[fd, 1000, 0];
    if[result < 0] {
        int err = @get_error_code[];
        if[err == 29] {
            @panic["File does not support seeking"];
        }
        @panic["Seek failed"];
    }
    
    // Read from offset 1000
    int buffer = @alloc[256];
    int n = @fread[fd, buffer, 256];
    // ... use buffer ...
    @free[buffer];
} when: {
    show["Seek/read error\n"];
}

@fclose[fd];
```

---

### @fclose Return Values

| Value | Meaning | Cause |
|-------|---------|-------|
| `0` | Success | File closed cleanly |
| `-1` | Error | Invalid fd or I/O error during flush |

**Common @fclose Errors:**

| Condition | errno | Meaning |
|-----------|-------|---------|
| Bad file descriptor | 9 (EBADF) | fd is invalid or already closed |
| I/O error | 5 (EIO) | Error flushing buffered data to disk |

**Important Note:** Even if @fclose fails, the file descriptor becomes invalid and should not be used again.

**Safe fclose Pattern:**
```ras
int fd = @fopen["file.txt", "w"];

// ... write data ...

check {
    int result = @fclose[fd];
    if[result < 0] {
        int err = @get_error_code[];
        @log_error["Warning: fclose error"];
        // Continue—fd is still invalid
    }
} when: {
    show["Error closing file (may have lost data)\n"];
}
```

---

### @fdelete Return Values

| Value | Meaning | Cause |
|-------|---------|-------|
| `0` | Success | File deleted |
| `-1` | Error | File not found, permission denied, or in use |

**Common @fdelete Errors:**

| Condition | errno | Meaning |
|-----------|-------|---------|
| File not found | 2 (ENOENT) | Path doesn't exist |
| Permission denied | 13 (EACCES) | No permission to delete |
| Is a directory | 21 (EISDIR) | Tried to delete directory (use rmdir) |
| File in use | 16 (EBUSY) | File still open or locked |
| Read-only file system | 30 (EROFS) | Cannot delete from read-only FS |

**Safe fdelete Pattern:**
```ras
check {
    int result = @fdelete["temp.txt"];
    if[result == 0] {
        show["✓ File deleted\n"];
    } or {
        int err = @get_error_code[];
        if[err == 2] {
            show["File not found (already deleted?)\n"];
        } or {
            str msg = @get_error_msg[];
            @log_error[msg];
        }
    }
} when: {
    show["Delete failed\n"];
}
```

---

## Error Handling Best Practices

### Pattern 1: Comprehensive Error Checking
```ras
fnc safe_file_op[path]::int {
    int fd = @fopen[path, "r"];
    if[fd == 0] {
        str msg = @get_error_msg[];
        show["Cannot open: "];
        show[msg];
        show["\n"];
        get[-1];
    }
    
    int buffer = @alloc[1024];
    int n = @fread[fd, buffer, 1024];
    if[n < 0] {
        @free[buffer];
        @fclose[fd];
        get[-2];
    }
    
    // ... use buffer ...
    
    @free[buffer];
    if[@fclose[fd] < 0] {
        get[-3];  // Close error
    }
    get[n];
}
```

### Pattern 2: Error Recovery
```ras
fnc read_with_retry[path]::int {
    int retries = 3;
    loop[int attempt = 0; attempt < retries; attempt++] {
        int fd = @fopen[path, "r"];
        if[fd > 0] {
            int n = @fread[fd, buffer, size];
            @fclose[fd];
            if[n >= 0] {
                get[n];  // Success
            }
        }
        @sleep[100];  // 100ms before retry
    }
    get[-1];  // All retries failed
}
```

### Pattern 3: Resource Cleanup Guarantee
```ras
check {
    int fd = @fopen[path, "r"];
    if[fd <= 0] {
        @panic["Open failed"];
    }
    
    // Always closes, even if error
    int buffer = @alloc[256];
    int n = @fread[fd, buffer, 256];
    
    // Process buffer...
    
    @free[buffer];
    @fclose[fd];
} when: {
    show["Error: resource cleanup triggered\n"];
}
```

## Related Documentation:
- [15-IO-STATEMENTS.md](15-IO-STATEMENTS.md) — Basic show/read
- [16-BUILTINS.md](16-BUILTINS.md) — File/network builtins
- [17-MEMORY-OPERATIONS.md](17-MEMORY-OPERATIONS.md) — Memory for buffers
- [20-CONCURRENCY.md](20-CONCURRENCY.md) — Concurrent servers
- [12-TRY-CATCH.md](12-TRY-CATCH.md) — Error handling
