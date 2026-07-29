#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/io.h>
#include <unistd.h>
#include <errno.h>

/**
 * @port_in[port] - Read from I/O port
 * port: port number (0-65535)
 * Returns: value read from port (0-255), -1 on error
 * Note: Requires root/CAP_SYS_RAWIO for direct I/O port access
 * Error codes: Returns -1 if port out of range or insufficient privileges
 */
long sc_port_in(long port) {
    if (port < 0 || port > 0xFFFF) return -1;
    
    /* PRIVILEGE: Check if we have permissions before attempting iopl */
    uid_t uid = geteuid();
    
    /* Request I/O privilege */
    errno = 0;
    if (iopl(3) < 0) {
        /* PRIVILEGE ERROR: iopl() failed - likely EPERM (insufficient privilege) */
        if (errno == EPERM) {
            /* Not root and no CAP_SYS_RAWIO capability */
            return -1;
        }
        return -1;
    }
    
    /* Read byte from port */
    unsigned char val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "d"((unsigned short)port));
    
    /* Restore privilege */
    iopl(0);
    
    return (long)val;
}

/**
 * @port_out[port, value] - Write to I/O port
 * port: port number (0-65535)
 * value: byte value to write (0-255)
 * Returns: 0 on success, -1 on error
 * Note: Requires root/CAP_SYS_RAWIO for direct I/O port access
 * Error codes: Returns -1 if port/value out of range or insufficient privileges
 */
long sc_port_out(long port, long value) {
    if (port < 0 || port > 0xFFFF) return -1;
    if (value < 0 || value > 0xFF) return -1;
    
    /* PRIVILEGE: Request I/O privilege (requires root/CAP_SYS_RAWIO) */
    errno = 0;
    if (iopl(3) < 0) {
        /* PRIVILEGE ERROR: iopl() failed - likely EPERM (insufficient privilege) */
        if (errno == EPERM) {
            /* Not root and no CAP_SYS_RAWIO capability */
            return -1;
        }
        return -1;
    }
    
    /* Write byte to port */
    __asm__ volatile("outb %0, %1" : : "a"((unsigned char)value), 
                     "d"((unsigned short)port));
    
    /* Restore privilege */
    iopl(0);
    
    return 0;
}

/**
 * @ioread[addr] - Read from memory-mapped I/O address
 * addr: virtual address of MMIO register (typically obtained from /dev/mem mapping)
 * Returns: 32-bit value from MMIO address, -1 on error (invalid address)
 * PRIVILEGE NOTE: Typically requires mapping via /dev/mem which needs root/CAP_SYS_RAWIO
 * The caller is responsible for obtaining valid MMIO addresses through privileged setup
 */
long sc_ioread(long addr) {
    if (addr <= 0) return -1;
    
    /* PRIVILEGE: Caller must have already obtained valid mapped address from /dev/mem */
    volatile unsigned int *ptr = (volatile unsigned int *)addr;
    
    /* SECURITY: Dereference via volatile to prevent optimization issues */
    return (long)(*ptr);
}

/**
 * @iowrite[addr, value] - Write to memory-mapped I/O address
 * addr: virtual address of MMIO register (typically obtained from /dev/mem mapping)
 * value: 32-bit value to write
 * Returns: 0 on success, -1 on error (invalid address)
 * PRIVILEGE NOTE: Typically requires mapping via /dev/mem which needs root/CAP_SYS_RAWIO
 * The caller is responsible for obtaining valid MMIO addresses through privileged setup
 */
long sc_iowrite(long addr, long value) {
    if (addr <= 0) return -1;
    
    /* PRIVILEGE: Caller must have already obtained valid mapped address from /dev/mem */
    volatile unsigned int *ptr = (volatile unsigned int *)addr;
    
    /* SECURITY: Write via volatile to prevent optimization issues */
    *ptr = (unsigned int)value;
    
    return 0;
}
