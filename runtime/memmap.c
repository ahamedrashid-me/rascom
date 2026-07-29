#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

/**
 * @mmap[size, prot, flags] - Memory map allocation
 * size: number of bytes to allocate
 * prot: protection flags (1=read, 2=write, 4=exec)
 * flags: 0=private anon, 1=shared anon, 2=shared file
 * Returns: memory address, or -1 on error
 */
long sc_mmap(long size, long prot, long flags) {
    if (size <= 0 || size > 0x40000000) return -1;  /* Max 1GB */
    
    int prot_flags = PROT_NONE;
    if (prot & 1) prot_flags |= PROT_READ;
    if (prot & 2) prot_flags |= PROT_WRITE;
    if (prot & 4) prot_flags |= PROT_EXEC;
    
    int mmap_flags = MAP_PRIVATE | MAP_ANONYMOUS;
    if (flags & 1) mmap_flags = MAP_SHARED | MAP_ANONYMOUS;
    
    void *addr = mmap(NULL, (size_t)size, prot_flags, mmap_flags, -1, 0);
    if (addr == MAP_FAILED) return -1;
    
    return (long)addr;
}

/**
 * @munmap[ptr, size] - Unmap memory region
 * ptr: address returned by @mmap
 * size: size used in @mmap
 * Returns: 0 on success, -1 on error
 */
long sc_munmap(long ptr, long size) {
    if (ptr <= 0 || size <= 0) return -1;
    
    int ret = munmap((void *)ptr, (size_t)size);
    return ret == 0 ? 0 : -1;
}

/**
 * @mprotect[ptr, size, prot] - Change memory protection
 * ptr: address
 * size: region size
 * prot: protection flags (1=read, 2=write, 4=exec)
 * Returns: 0 on success, -1 on error
 */
long sc_mprotect(long ptr, long size, long prot) {
    if (ptr <= 0 || size <= 0) return -1;
    
    int prot_flags = PROT_NONE;
    if (prot & 1) prot_flags |= PROT_READ;
    if (prot & 2) prot_flags |= PROT_WRITE;
    if (prot & 4) prot_flags |= PROT_EXEC;
    
    int ret = mprotect((void *)ptr, (size_t)size, prot_flags);
    return ret == 0 ? 0 : -1;
}
