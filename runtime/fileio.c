#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

/**
 * @fread[fd, ptr, len] - Read from file descriptor
 * fd: file descriptor
 * ptr: memory address to store data
 * len: number of bytes to read
 * Returns: bytes read, 0=EOF, -1 on error
 */
long sc_fread(long fd, long ptr, long len) {
    if (fd < 0 || len <= 0) return -1;
    
    ssize_t ret = read((int)fd, (void *)ptr, (size_t)len);
    return ret >= 0 ? ret : -1;
}

/**
 * @fseek[fd, offset] - Seek to position in file
 * fd: file descriptor
 * offset: byte offset from start of file
 * Returns: new position, -1 on error
 */
long sc_fseek(long fd, long offset) {
    if (fd < 0 || offset < 0) return -1;
    
    off_t ret = lseek((int)fd, (off_t)offset, SEEK_SET);
    return ret >= 0 ? ret : -1;
}

/**
 * @fwrite[fd, ptr, len] - Write to file descriptor
 * fd: file descriptor
 * ptr: memory address of data to write
 * len: number of bytes to write
 * Returns: bytes written, -1 on error
 */
long sc_fwrite(long fd, long ptr, long len) {
    if (fd < 0 || len <= 0) return -1;
    
    ssize_t ret = write((int)fd, (void *)ptr, (size_t)len);
    return ret >= 0 ? ret : -1;
}

/**
 * @fopen[path, flags] - Open file
 * path: pointer to null-terminated path string
 * flags: 0=read, 1=write, 2=append, 3=read+write
 * Returns: file descriptor, -1 on error
 */
long sc_fopen(long path, long flags) {
    if (!path) return -1;
    
    int oflags = O_RDONLY;
    if (flags == 1) oflags = O_WRONLY | O_CREAT | O_TRUNC;
    else if (flags == 2) oflags = O_WRONLY | O_CREAT | O_APPEND;
    else if (flags == 3) oflags = O_RDWR | O_CREAT;
    
    int fd = open((const char *)path, oflags, 0644);
    return fd >= 0 ? fd : -1;
}

/**
 * @fclose[fd] - Close file descriptor
 * fd: file descriptor
 * Returns: 0 on success, -1 on error
 */
long sc_fclose(long fd) {
    if (fd < 0) return -1;
    int ret = close((int)fd);
    return ret >= 0 ? 0 : -1;
}

/**
 * @fdelete[path] - Delete/remove file
 * path: file path to delete
 * Returns: 0 on success, -1 on error
 */
long sc_fdelete(long path) {
    if (!path) return -1;
    int ret = unlink((const char *)path);
    return ret >= 0 ? 0 : -1;
}
