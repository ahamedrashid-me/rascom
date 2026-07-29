/*
 * Pure RasCode Runtime - No External Dependencies
 * Direct Linux x86-64 syscalls only
 * No pthread, libm, libdl, or libc required
 */

#include <stdint.h>
#include <stddef.h>

#ifndef SC_PURE_RUNTIME_IMPL
#define SC_PURE_RUNTIME_IMPL

/* ============================================================
 * x86-64 SYSCALL DEFINITIONS (Linux)
 * ============================================================ */

#define SYS_read            0
#define SYS_write           1
#define SYS_open            2
#define SYS_close           3
#define SYS_stat            4
#define SYS_fstat           5
#define SYS_lstat           6
#define SYS_poll            7
#define SYS_lseek           8
#define SYS_mmap            9
#define SYS_mprotect        10
#define SYS_munmap          11
#define SYS_brk             12
#define SYS_rt_sigaction    13
#define SYS_rt_sigprocmask  14
#define SYS_rt_sigpending   15
#define SYS_rt_sigtimedwait 16
#define SYS_rt_sigqueueinfo 17
#define SYS_rt_sigreturn    18
#define SYS_ioctl           16
#define SYS_pread64         17
#define SYS_pwrite64        18
#define SYS_readv           19
#define SYS_writev          20
#define SYS_access          21
#define SYS_pipe            22
#define SYS_select          23
#define SYS_sched_yield     24
#define SYS_mremap          25
#define SYS_msync           26
#define SYS_mincore         27
#define SYS_madvise         28
#define SYS_shmget          29
#define SYS_shmat           30
#define SYS_shmctl          31
#define SYS_dup             32
#define SYS_dup2            33
#define SYS_pause           34
#define SYS_nanosleep       35
#define SYS_getitimer       36
#define SYS_alarm           37
#define SYS_setitimer       38
#define SYS_getpid          39
#define SYS_sendfile        40
#define SYS_socket          41
#define SYS_connect         42
#define SYS_accept          43
#define SYS_sendto          44
#define SYS_recvfrom        45
#define SYS_sendmsg         46
#define SYS_recvmsg         47
#define SYS_shutdown        48
#define SYS_bind            49
#define SYS_listen          50
#define SYS_getsockname     51
#define SYS_getpeername     52
#define SYS_socketpair      53
#define SYS_setsockopt      54
#define SYS_getsockopt      55
#define SYS_clone           56
#define SYS_fork            57
#define SYS_vfork           58
#define SYS_execve          59
#define SYS_exit            60
#define SYS_wait4           61
#define SYS_kill            62
#define SYS_uname           63
#define SYS_fcntl           72
#define SYS_flock           73
#define SYS_fsync           74
#define SYS_fdatasync       75
#define SYS_truncate        76
#define SYS_ftruncate       77
#define SYS_getdents        78
#define SYS_getcwd          79
#define SYS_chdir           80
#define SYS_fchdir          81
#define SYS_rename          82
#define SYS_mkdir           83
#define SYS_rmdir           84
#define SYS_creat           85
#define SYS_link            86
#define SYS_unlink          87
#define SYS_symlink         88
#define SYS_readlink        89
#define SYS_chmod           90
#define SYS_fchmod          91
#define SYS_chown           92
#define SYS_fchown          93
#define SYS_lchown          94
#define SYS_umask           95
#define SYS_gettimeofday    96
#define SYS_getrlimit       97
#define SYS_getrusage       98
#define SYS_sysinfo         99
#define SYS_times           100
#define SYS_ptrace          101
#define SYS_getuid          102
#define SYS_syslog          103
#define SYS_getgid          104
#define SYS_setuid          105
#define SYS_setgid          106
#define SYS_geteuid         107
#define SYS_getegid         108
#define SYS_setpgid         109
#define SYS_getppid         110
#define SYS_getpgrp         111
#define SYS_setsid          112
#define SYS_setreuid        113
#define SYS_setregid        114
#define SYS_getgroups       115
#define SYS_setgroups       116
#define SYS_setresuid       117
#define SYS_getresuid       118
#define SYS_setresgid       119
#define SYS_getresgid       120
#define SYS_getpgid         121
#define SYS_setfsuid        122
#define SYS_setfsgid        123
#define SYS_getsid          124
#define SYS_capget          125
#define SYS_capset          126
#define SYS_rt_pending      127
#define SYS_rt_sigaltstack  129
#define SYS_utime           132
#define SYS_mknod           133
#define SYS_uselib          134
#define SYS_personality     135
#define SYS_ustat           136
#define SYS_statfs          137
#define SYS_fstatfs         138
#define SYS_sysfs           139
#define SYS_getpriority     140
#define SYS_setpriority     141
#define SYS_sched_setparam  142
#define SYS_sched_getparam  143
#define SYS_sched_setscheduler 144
#define SYS_sched_getscheduler 145
#define SYS_sched_get_priority_max 146
#define SYS_sched_get_priority_min 147
#define SYS_sched_rr_get_interval 148
#define SYS_mlock           149
#define SYS_munlock         150
#define SYS_mlockall        151
#define SYS_munlockall      152
#define SYS_vhangup         153
#define SYS_modify_ldt      154
#define SYS_pivot_root      155
#define SYS__sysctl         156
#define SYS_prctl           157
#define SYS_arch_specific_syscall 158
#define SYS_adjtimex        159
#define SYS_setrlimit       160
#define SYS_chroot          161
#define SYS_sync            162
#define SYS_acct            163
#define SYS_settimeofday    164
#define SYS_mount           165
#define SYS_umount2         166
#define SYS_swapon          167
#define SYS_swapoff         168
#define SYS_reboot          169
#define SYS_sethostname     170
#define SYS_setdomainname   171
#define SYS_iopl            172
#define SYS_ioperm          173
#define SYS_create_module   174
#define SYS_init_module     175
#define SYS_delete_module   176
#define SYS_get_kernel_syms 177
#define SYS_query_module    178
#define SYS_quotactl        179
#define SYS_nfsservctl      180
#define SYS_getpmsg         181
#define SYS_putpmsg         182
#define SYS_afs_syscall     183
#define SYS_tuxcall         184
#define SYS_security        185
#define SYS_gettid          186
#define SYS_readahead       187
#define SYS_setxattr        188
#define SYS_lsetxattr       189
#define SYS_fsetxattr       190
#define SYS_getxattr        191
#define SYS_lgetxattr       192
#define SYS_fgetxattr       193
#define SYS_listxattr       194
#define SYS_llistxattr      195
#define SYS_flistxattr      196
#define SYS_removexattr     197
#define SYS_lremovexattr    198
#define SYS_fremovexattr    199
#define SYS_tkill           200
#define SYS_time            201
#define SYS_futex           202
#define SYS_sched_setaffinity 203
#define SYS_sched_getaffinity 204
#define SYS_set_thread_area 205
#define SYS_io_setup        206
#define SYS_io_destroy      207
#define SYS_io_getevents    208
#define SYS_io_submit       209
#define SYS_io_cancel       210
#define SYS_get_thread_area 211
#define SYS_lookup_dcookie  212
#define SYS_epoll_create    213
#define SYS_epoll_ctl_old   214
#define SYS_epoll_wait_old  215
#define SYS_remap_file_pages 216
#define SYS_getdents64      217
#define SYS_set_tid_address 218
#define SYS_restart_syscall 219
#define SYS_semtimedop      220
#define SYS_fadvise64       221
#define SYS_timer_create    222
#define SYS_timer_settime   223
#define SYS_timer_gettime   224
#define SYS_timer_getoverrun 225
#define SYS_timer_delete    226
#define SYS_clock_settime   227
#define SYS_clock_gettime   228
#define SYS_clock_getres    229
#define SYS_clock_nanosleep 230
#define SYS_exit_group      231
#define SYS_epoll_wait      232
#define SYS_epoll_ctl       233
#define SYS_tgkill          234
#define SYS_utimes          235
#define SYS_vserver         236
#define SYS_mbind           259
#define SYS_set_mempolicy   261
#define SYS_get_mempolicy   262
#define SYS_mq_open         263
#define SYS_mq_unlink       264
#define SYS_mq_timedsend    265
#define SYS_mq_timedreceive 266
#define SYS_mq_notify       267
#define SYS_mq_getsetattr   268
#define SYS_kexec_load      246
#define SYS_waitid          247
#define SYS_add_key         248
#define SYS_request_key     249
#define SYS_keyctl          250
#define SYS_ioprio_set      251
#define SYS_ioprio_get      252
#define SYS_inotify_init    253
#define SYS_inotify_add_watch 254
#define SYS_inotify_rm_watch 255
#define SYS_migrate_pages   256
#define SYS_openat          257
#define SYS_mkdirat         258
#define SYS_mknodat         259
#define SYS_fchownat        260
#define SYS_futimesat       261
#define SYS_newfstatat      262
#define SYS_unlinkat        263
#define SYS_renameat        264
#define SYS_linkat          265
#define SYS_symlinkat       266
#define SYS_readlinkat      267
#define SYS_fchmodat        268
#define SYS_faccessat       269
#define SYS_pselect6        270
#define SYS_ppoll           271
#define SYS_unshare         272
#define SYS_set_robust_list 273
#define SYS_get_robust_list 274
#define SYS_splice          275
#define SYS_tee             276
#define SYS_sync_file_range 277
#define SYS_vmsplice        278
#define SYS_move_pages      279
#define SYS_utimensat       280
#define SYS_epoll_pwait     281
#define SYS_signalfd        282
#define SYS_timerfd_create  283
#define SYS_eventfd         284
#define SYS_fallocate       285

/* ============================================================
 * INLINE SYSCALL WRAPPERS
 * ============================================================ */

static inline int64_t sc_syscall0(int64_t number) {
    int64_t result;
    __asm__ volatile (
        "syscall"
        : "=a" (result)
        : "a" (number)
        : "rcx", "r11", "memory"
    );
    return result;
}

static inline int64_t sc_syscall1(int64_t number, int64_t arg1) {
    int64_t result;
    __asm__ volatile (
        "syscall"
        : "=a" (result)
        : "a" (number), "D" (arg1)
        : "rcx", "r11", "memory"
    );
    return result;
}

static inline int64_t sc_syscall2(int64_t number, int64_t arg1, int64_t arg2) {
    int64_t result;
    __asm__ volatile (
        "syscall"
        : "=a" (result)
        : "a" (number), "D" (arg1), "S" (arg2)
        : "rcx", "r11", "memory"
    );
    return result;
}

static inline int64_t sc_syscall3(int64_t number, int64_t arg1, int64_t arg2, int64_t arg3) {
    int64_t result;
    __asm__ volatile (
        "syscall"
        : "=a" (result)
        : "a" (number), "D" (arg1), "S" (arg2), "d" (arg3)
        : "rcx", "r11", "memory"
    );
    return result;
}

static inline int64_t sc_syscall4(int64_t number, int64_t arg1, int64_t arg2, int64_t arg3, int64_t arg4) {
    int64_t result;
    register int64_t r10 __asm__("r10") = arg4;
    __asm__ volatile (
        "syscall"
        : "=a" (result)
        : "a" (number), "D" (arg1), "S" (arg2), "d" (arg3), "r" (r10)
        : "rcx", "r11", "memory"
    );
    return result;
}

static inline int64_t sc_syscall5(int64_t number, int64_t arg1, int64_t arg2, int64_t arg3, int64_t arg4, int64_t arg5) {
    int64_t result;
    register int64_t r10 __asm__("r10") = arg4;
    register int64_t r8  __asm__("r8") = arg5;
    __asm__ volatile (
        "syscall"
        : "=a" (result)
        : "a" (number), "D" (arg1), "S" (arg2), "d" (arg3), "r" (r10), "r" (r8)
        : "rcx", "r11", "memory"
    );
    return result;
}

static inline int64_t sc_syscall6(int64_t number, int64_t arg1, int64_t arg2, int64_t arg3, int64_t arg4, int64_t arg5, int64_t arg6) {
    int64_t result;
    register int64_t r10 __asm__("r10") = arg4;
    register int64_t r8  __asm__("r8") = arg5;
    register int64_t r9  __asm__("r9") = arg6;
    __asm__ volatile (
        "syscall"
        : "=a" (result)
        : "a" (number), "D" (arg1), "S" (arg2), "d" (arg3), "r" (r10), "r" (r8), "r" (r9)
        : "rcx", "r11", "memory"
    );
    return result;
}

/* ============================================================
 * I/O OPERATIONS (No libc required)
 * ============================================================ */

// write(fd, buf, count) - direct syscall
int sc_write(int fd, const void *buf, int count) {
    return (int)sc_syscall3(SYS_write, fd, (int64_t)buf, count);
}

// read(fd, buf, count) - direct syscall
int sc_read(int fd, void *buf, int count) {
    return (int)sc_syscall3(SYS_read, fd, (int64_t)buf, count);
}

// exit(code) - direct syscall
void sc_exit(int code) {
    sc_syscall1(SYS_exit_group, code);
    while(1);  // Should not reach here
}

/* ============================================================
 * THREADING OPERATIONS (futex-based, no pthread required)
 * ============================================================ */

#define FUTEX_WAIT  0
#define FUTEX_WAKE  1
#define FUTEX_FD    2
#define FUTEX_REQUEUE 3
#define FUTEX_CMP_REQUEUE 4
#define FUTEX_WAKE_OP 5
#define FUTEX_LOCK_PI 6
#define FUTEX_UNLOCK_PI 7

typedef struct {
    int32_t futex_value;
    int32_t in_use;
} sc_futex_t;

// futex(uaddr, op, val, timeout, uaddr2, val3)
int sc_futex(int32_t *uaddr, int op, int32_t val) {
    return (int)sc_syscall3(SYS_futex, (int64_t)uaddr, op | 0, val);
}

// futex_wait - wait on futex
int sc_futex_wait(int32_t *futex, int32_t expected) {
    return sc_futex(futex, FUTEX_WAIT, expected);
}

// futex_wake - wake waiters on futex
int sc_futex_wake(int32_t *futex, int32_t max_waiters) {
    return sc_futex(futex, FUTEX_WAKE, max_waiters);
}

/* ============================================================
 * MEMORY OPERATIONS
 * ============================================================ */

// mmap - memory mapping
void *sc_mmap(void *addr, int length, int prot, int flags, int fd, int offset) {
    return (void *)sc_syscall6(SYS_mmap, (int64_t)addr, length, prot, flags, fd, offset);
}

// munmap - unmap memory
int sc_munmap(void *addr, int length) {
    return (int)sc_syscall2(SYS_munmap, (int64_t)addr, length);
}

// brk - set program break
void *sc_brk(void *addr) {
    return (void *)sc_syscall1(SYS_brk, (int64_t)addr);
}

/* ============================================================
 * FILE OPERATIONS
 * ============================================================ */

// open(filename, flags, mode)
int sc_open(const char *filename, int flags, int mode) {
    return (int)sc_syscall3(SYS_open, (int64_t)filename, flags, mode);
}

// close(fd)
int sc_close(int fd) {
    return (int)sc_syscall1(SYS_close, fd);
}

// stat(filename, statbuf)
int sc_stat(const char *filename, void *statbuf) {
    return (int)sc_syscall2(SYS_stat, (int64_t)filename, (int64_t)statbuf);
}

/* ============================================================
 * TIME OPERATIONS
 * ============================================================ */

// clock_gettime(clockid, timespec)
int sc_clock_gettime(int clockid, void *timespec) {
    return (int)sc_syscall2(SYS_clock_gettime, clockid, (int64_t)timespec);
}

// nanosleep(req, rem)
int sc_nanosleep(const void *req, void *rem) {
    return (int)sc_syscall2(SYS_nanosleep, (int64_t)req, (int64_t)rem);
}

/* ============================================================
 * THREAD CREATION
 * ============================================================ */

// clone(flags, child_stack, ptid, ctid, tls)
int sc_clone(int flags, void *child_stack, void *ptid, void *ctid, void *tls) {
    return (int)sc_syscall5(SYS_clone, flags, (int64_t)child_stack, (int64_t)ptid, (int64_t)ctid, (int64_t)tls);
}

// gettid()
int sc_gettid(void) {
    return (int)sc_syscall0(SYS_gettid);
}

// getpid()
int sc_getpid(void) {
    return (int)sc_syscall0(SYS_getpid);
}

/* ============================================================
 * SIGNAL OPERATIONS
 * ============================================================ */

// rt_sigaction(sig, act, oldact, sigsetsize)
int sc_rt_sigaction(int sig, const void *act, void *oldact, int sigsetsize) {
    return (int)sc_syscall4(SYS_rt_sigaction, sig, (int64_t)act, (int64_t)oldact, sigsetsize);
}

// rt_sigprocmask(how, set, oldset, sigsetsize)
int sc_rt_sigprocmask(int how, const void *set, void *oldset, int sigsetsize) {
    return (int)sc_syscall4(SYS_rt_sigprocmask, how, (int64_t)set, (int64_t)oldset, sigsetsize);
}

/* ============================================================
 * PROCESS OPERATIONS
 * ============================================================ */

// fork()
int sc_fork(void) {
    return (int)sc_syscall0(SYS_fork);
}

// vfork()
int sc_vfork(void) {
    return (int)sc_syscall0(SYS_vfork);
}

// execve(filename, argv, envp)
int sc_execve(const char *filename, const void *argv, const void *envp) {
    return (int)sc_syscall3(SYS_execve, (int64_t)filename, (int64_t)argv, (int64_t)envp);
}

// wait4(pid, status, options, rusage)
int sc_wait4(int pid, int *status, int options, void *rusage) {
    return (int)sc_syscall4(SYS_wait4, pid, (int64_t)status, options, (int64_t)rusage);
}

/* ============================================================
 * SOCKET OPERATIONS (Network I/O)
 * ============================================================ */

// socket(domain, type, protocol)
int sc_socket(int domain, int type, int protocol) {
    return (int)sc_syscall3(SYS_socket, domain, type, protocol);
}

// connect(sockfd, addr, addrlen)
int sc_connect(int sockfd, const void *addr, int addrlen) {
    return (int)sc_syscall3(SYS_connect, sockfd, (int64_t)addr, addrlen);
}

// bind(sockfd, addr, addrlen)
int sc_bind(int sockfd, const void *addr, int addrlen) {
    return (int)sc_syscall3(SYS_bind, sockfd, (int64_t)addr, addrlen);
}

// listen(sockfd, backlog)
int sc_listen(int sockfd, int backlog) {
    return (int)sc_syscall2(SYS_listen, sockfd, backlog);
}

// accept(sockfd, addr, addrlen)
int sc_accept(int sockfd, void *addr, int *addrlen) {
    return (int)sc_syscall3(SYS_accept, sockfd, (int64_t)addr, (int64_t)addrlen);
}

// sendto(sockfd, buf, len, flags, dest_addr, addrlen)
int sc_sendto(int sockfd, const void *buf, int len, int flags, const void *dest_addr, int addrlen) {
    return (int)sc_syscall6(SYS_sendto, sockfd, (int64_t)buf, len, flags, (int64_t)dest_addr, addrlen);
}

// recvfrom(sockfd, buf, len, flags, src_addr, addrlen)
int sc_recvfrom(int sockfd, void *buf, int len, int flags, void *src_addr, int *addrlen) {
    return (int)sc_syscall6(SYS_recvfrom, sockfd, (int64_t)buf, len, flags, (int64_t)src_addr, (int64_t)addrlen);
}

/* ============================================================
 * DIRECTORY OPERATIONS
 * ============================================================ */

// mkdir(pathname, mode)
int sc_mkdir(const char *pathname, int mode) {
    return (int)sc_syscall2(SYS_mkdir, (int64_t)pathname, mode);
}

// rmdir(pathname)
int sc_rmdir(const char *pathname) {
    return (int)sc_syscall1(SYS_rmdir, (int64_t)pathname);
}

// unlink(pathname)
int sc_unlink(const char *pathname) {
    return (int)sc_syscall1(SYS_unlink, (int64_t)pathname);
}

// getcwd(buf, size)
char *sc_getcwd(char *buf, int size) {
    if (sc_syscall2(SYS_getcwd, (int64_t)buf, size) >= 0) {
        return buf;
    }
    return 0;
}

// chdir(path)
int sc_chdir(const char *path) {
    return (int)sc_syscall1(SYS_chdir, (int64_t)path);
}

/* ============================================================
 * UTILITY FUNCTIONS
 * ============================================================ */

// String utilities for pure environment
void sc_memcpy(void *dest, const void *src, int n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    for (int i = 0; i < n; i++) {
        d[i] = s[i];
    }
}

void sc_memset(void *s, int c, int n) {
    uint8_t *p = (uint8_t *)s;
    for (int i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }
}

int sc_strlen(const char *s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

int sc_strcmp(const char *s1, const char *s2) {
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (int)((unsigned char)*s1 - (unsigned char)*s2);
}

/* ============================================================
 * ENTRY POINT (No libc _start required)
 * ============================================================ */

// This is called from generated code
// Arguments in: rdi=argc, rsi=argv, rdx=envp
int RasCode_main(int argc, char **argv, char **envp);

// Assembly entry point (must be linked properly)
__attribute__((naked, noreturn))
void _start(void) {
    // Stack layout on entry:
    // [rbp] = 0
    // [rbp+8] = argc
    // [rbp+16] = argv
    // [rbp+24] = envp
    
    __asm__ volatile (
        "xor    %%rbp, %%rbp\n"       // Clear rbp (frame pointer)
        "pop    %%rdi\n"              // argc
        "mov    %%rsp, %%rsi\n"       // argv
        "lea    8(%%rsi, %%rdi, 8), %%rdx\n"  // envp
        "call   RasCode_main\n"            // Call RasCode main
        "mov    %%rax, %%rdi\n"       // Exit code in rdi
        "mov    $60, %%rax\n"         // SYS_exit
        "syscall\n"
        : : : "memory"
    );
}

#endif  // SC_PURE_RUNTIME_IMPL

