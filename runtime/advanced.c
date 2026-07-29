#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/types.h>

/* Direct syscall support */

/**
 * @syscall[num, arg1, arg2, arg3, arg4, arg5, arg6] - Direct Linux syscall
 * num: syscall number
 * arg1-6: syscall arguments
 * Returns: syscall return value
 */
long sc_syscall(long num, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    return syscall(num, arg1, arg2, arg3, arg4, arg5, arg6);
}

/**
 * @irq_enable[] - Enable interrupts (set IF flag)
 * Returns: 0 on success
 */
long sc_irq_enable(void) {
    /* x86 STI instruction - enable interrupts */
    /* Note: Requires kernel mode or special privileges */
    __asm__ volatile("sti");
    return 0;
}

/**
 * @irq_disable[] - Disable interrupts (clear IF flag)
 * Returns: 0 on success
 */
long sc_irq_disable(void) {
    /* x86 CLI instruction - disable interrupts */
    /* Note: Requires kernel mode or special privileges */
    __asm__ volatile("cli");
    return 0;
}

/**
 * @verify[signature, publickey, data] - Verify cryptographic signature
 * signature: pointer to signature bytes
 * publickey: pointer to public key
 * data: pointer to data to verify
 * Returns: 1=valid, 0=invalid, -1=error
 */
long sc_verify(long signature, long publickey, long data) {
    /* Stub for cryptographic verification */
    /* Full implementation would require crypto library */
    if (!signature || !publickey || !data) return -1;
    
    /* Basic validation: check if signature exists and is non-zero */
    unsigned char *sig = (unsigned char *)signature;
    if (*sig == 0) return 0;  /* Invalid signature */
    
    return 1;  /* Assume valid for stub */
}
