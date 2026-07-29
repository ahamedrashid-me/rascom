/*
 * RasCode Pure Runtime - No External Dependencies
 * Header file for syscall-based runtime
 */

#ifndef SC_PURE_RUNTIME_H
#define SC_PURE_RUNTIME_H

#include <stdint.h>
#include <stddef.h>

/* ============================================================
 * I/O OPERATIONS
 * ============================================================ */

int sc_write(int fd, const void *buf, int count);
int sc_read(int fd, void *buf, int count);
void sc_exit(int code);

/* ============================================================
 * THREADING (FUTEX-based)
 * ============================================================ */

int sc_futex_wait(int32_t *futex, int32_t expected);
int sc_futex_wake(int32_t *futex, int32_t max_waiters);

/* ============================================================
 * MEMORY OPERATIONS
 * ============================================================ */

void *sc_mmap(void *addr, int length, int prot, int flags, int fd, int offset);
int sc_munmap(void *addr, int length);
void *sc_brk(void *addr);

/* ============================================================
 * FILE OPERATIONS
 * ============================================================ */

int sc_open(const char *filename, int flags, int mode);
int sc_close(int fd);
int sc_stat(const char *filename, void *statbuf);

/* ============================================================
 * TIME OPERATIONS
 * ============================================================ */

int sc_clock_gettime(int clockid, void *timespec);
int sc_nanosleep(const void *req, void *rem);

/* ============================================================
 * THREAD CREATION
 * ============================================================ */

int sc_clone(int flags, void *child_stack, void *ptid, void *ctid, void *tls);
int sc_gettid(void);
int sc_getpid(void);

/* ============================================================
 * SIGNAL OPERATIONS
 * ============================================================ */

int sc_rt_sigaction(int sig, const void *act, void *oldact, int sigsetsize);
int sc_rt_sigprocmask(int how, const void *set, void *oldset, int sigsetsize);

/* ============================================================
 * PROCESS OPERATIONS
 * ============================================================ */

int sc_fork(void);
int sc_vfork(void);
int sc_execve(const char *filename, const void *argv, const void *envp);
int sc_wait4(int pid, int *status, int options, void *rusage);

/* ============================================================
 * SOCKET OPERATIONS (Network I/O)
 * ============================================================ */

int sc_socket(int domain, int type, int protocol);
int sc_connect(int sockfd, const void *addr, int addrlen);
int sc_bind(int sockfd, const void *addr, int addrlen);
int sc_listen(int sockfd, int backlog);
int sc_accept(int sockfd, void *addr, int *addrlen);
int sc_sendto(int sockfd, const void *buf, int len, int flags, const void *dest_addr, int addrlen);
int sc_recvfrom(int sockfd, void *buf, int len, int flags, void *src_addr, int *addrlen);

/* ============================================================
 * DIRECTORY OPERATIONS
 * ============================================================ */

int sc_mkdir(const char *pathname, int mode);
int sc_rmdir(const char *pathname);
int sc_unlink(const char *pathname);
char *sc_getcwd(char *buf, int size);
int sc_chdir(const char *path);

/* ============================================================
 * UTILITY FUNCTIONS
 * ============================================================ */

void sc_memcpy(void *dest, const void *src, int n);
void sc_memset(void *s, int c, int n);
int sc_strlen(const char *s);
int sc_strcmp(const char *s1, const char *s2);

/* ============================================================
 * ENTRY POINT
 * ============================================================ */

int sc_main(int argc, char **argv, char **envp);

#endif
