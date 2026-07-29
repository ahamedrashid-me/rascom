#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

/* Network socket management */
#define MAX_SOCKETS 64

typedef struct {
    int fd;
    int type;
    int state;  /* 0=closed, 1=open, 2=listening, 3=connected */
} socket_entry_t;

static socket_entry_t socket_table[MAX_SOCKETS];
static pthread_mutex_t socket_table_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t socket_initialized_once = PTHREAD_ONCE_INIT;

// SECURITY: Thread-safe one-time initialization of socket table
// Prevents multiple threads from racing to initialize
static void init_socket_table_func() {
    for (int i = 0; i < MAX_SOCKETS; i++) {
        socket_table[i].fd = -1;
        socket_table[i].type = 0;
        socket_table[i].state = 0;
    }
}

static void init_socket_table() {
    // SECURITY: Use pthread_once for thread-safe one-time initialization
    // This ensures exactly one thread initializes the socket table
    pthread_once(&socket_initialized_once, init_socket_table_func);
}

/**
 * @socket[type, protocol] - Create a socket
 * type: 1=TCP (SOCK_STREAM), 2=UDP (SOCK_DGRAM)
 * protocol: 0=auto, 6=TCP, 17=UDP
 * Returns: socket handle (0-63) or -1 on error
 */
long sc_socket(long type, long protocol) {
    init_socket_table();
    
    int sock_type = SOCK_STREAM;
    int sock_protocol = IPPROTO_TCP;
    
    if (type == 2) {  /* UDP */
        sock_type = SOCK_DGRAM;
        sock_protocol = IPPROTO_UDP;
    } else if (type != 1) {  /* Invalid type */
        return -1;
    }
    
    /* If protocol specified, use it */
    if (protocol == 6) sock_protocol = IPPROTO_TCP;
    else if (protocol == 17) sock_protocol = IPPROTO_UDP;
    
    int fd = socket(AF_INET, sock_type, sock_protocol);
    if (fd < 0) return -1;
    
    // SECURITY: Protected socket_table access with mutex
    pthread_mutex_lock(&socket_table_lock);
    
    /* Find slot in socket table */
    long slot = -1;
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (socket_table[i].fd == -1) {
            socket_table[i].fd = fd;
            socket_table[i].type = type;
            socket_table[i].state = 1;  /* open */
            slot = i;
            break;
        }
    }
    
    pthread_mutex_unlock(&socket_table_lock);
    
    if (slot == -1) {
        close(fd);  /* No slots available */
        return -1;
    }
    
    return slot;
}

/**
 * @connect[sock, addr, port] - Connect to remote address
 * sock: socket handle from @socket[]
 * addr: IPv4 address as 32-bit integer (e.g., 127.0.0.1 = 0x7F000001)
 * port: port number (0-65535)
 * Returns: 0 on success, -1 on error
 */
long sc_connect(long sock, long addr, long port) {
    init_socket_table();
    
    if (sock < 0 || sock >= MAX_SOCKETS) return -1;
    
    pthread_mutex_lock(&socket_table_lock);
    int fd = socket_table[sock].fd;
    pthread_mutex_unlock(&socket_table_lock);
    
    if (fd < 0) return -1;
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t)port);
    server_addr.sin_addr.s_addr = htonl((uint32_t)addr);
    
    int ret = connect(fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0) return -1;
    
    pthread_mutex_lock(&socket_table_lock);
    socket_table[sock].state = 3;  /* connected */
    pthread_mutex_unlock(&socket_table_lock);
    
    return 0;
}

/**
 * @bind[sock, addr, port] - Bind socket to local address
 * sock: socket handle
 * addr: local address (0=INADDR_ANY)
 * port: port number
 * Returns: 0 on success, -1 on error
 */
long sc_bind(long sock, long addr, long port) {
    init_socket_table();
    
    if (sock < 0 || sock >= MAX_SOCKETS) return -1;
    
    pthread_mutex_lock(&socket_table_lock);
    int fd = socket_table[sock].fd;
    pthread_mutex_unlock(&socket_table_lock);
    
    if (fd < 0) return -1;
    
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons((uint16_t)port);
    local_addr.sin_addr.s_addr = htonl((uint32_t)addr);
    
    int ret = bind(fd, (struct sockaddr *)&local_addr, sizeof(local_addr));
    return ret < 0 ? -1 : 0;
}

/**
 * @listen[sock, backlog] - Listen for incoming connections
 * sock: socket handle bound to address
 * backlog: max queued connections
 * Returns: 0 on success, -1 on error
 */
long sc_listen(long sock, long backlog) {
    init_socket_table();
    
    if (sock < 0 || sock >= MAX_SOCKETS) return -1;
    
    pthread_mutex_lock(&socket_table_lock);
    int fd = socket_table[sock].fd;
    pthread_mutex_unlock(&socket_table_lock);
    
    if (fd < 0) return -1;
    
    int ret = listen(fd, (int)backlog);
    if (ret < 0) return -1;
    
    pthread_mutex_lock(&socket_table_lock);
    socket_table[sock].state = 2;  /* listening */
    pthread_mutex_unlock(&socket_table_lock);
    
    return 0;
}

/**
 * @accept[sock] - Accept incoming connection
 * sock: listening socket handle
 * Returns: new socket handle for connection, -1 on error
 */
long sc_accept(long sock) {
    init_socket_table();
    
    if (sock < 0 || sock >= MAX_SOCKETS) return -1;
    
    pthread_mutex_lock(&socket_table_lock);
    int fd = socket_table[sock].fd;
    int sock_type = socket_table[sock].type;
    pthread_mutex_unlock(&socket_table_lock);
    
    if (fd < 0) return -1;
    
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) return -1;
    
    pthread_mutex_lock(&socket_table_lock);
    
    /* Find slot for new connection */
    long new_slot = -1;
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (socket_table[i].fd == -1) {
            socket_table[i].fd = client_fd;
            socket_table[i].type = sock_type;
            socket_table[i].state = 3;  /* connected */
            new_slot = i;
            break;
        }
    }
    
    pthread_mutex_unlock(&socket_table_lock);
    
    if (new_slot == -1) {
        close(client_fd);
        return -1;
    }
    
    return new_slot;
}

/**
 * @send[sock, ptr, len] - Send data on socket
 * sock: socket handle
 * ptr: memory address of data to send
 * len: number of bytes to send
 * Returns: bytes sent, -1 on error
 */
long sc_send(long sock, long ptr, long len) {
    init_socket_table();
    
    if (sock < 0 || sock >= MAX_SOCKETS) return -1;
    if (len <= 0) return 0;
    
    pthread_mutex_lock(&socket_table_lock);
    int fd = socket_table[sock].fd;
    pthread_mutex_unlock(&socket_table_lock);
    
    if (fd < 0) return -1;
    
    ssize_t ret = send(fd, (void *)ptr, (size_t)len, 0);
    return ret >= 0 ? ret : -1;
}

/**
 * @recv[sock, ptr, len] - Receive data on socket
 * sock: socket handle
 * ptr: memory address to store received data
 * len: buffer size
 * Returns: bytes received, 0=connection closed, -1 on error
 */
long sc_recv(long sock, long ptr, long len) {
    init_socket_table();
    
    if (sock < 0 || sock >= MAX_SOCKETS) return -1;
    if (len <= 0) return 0;
    
    pthread_mutex_lock(&socket_table_lock);
    int fd = socket_table[sock].fd;
    pthread_mutex_unlock(&socket_table_lock);
    
    if (fd < 0) return -1;
    
    ssize_t ret = recv(fd, (void *)ptr, (size_t)len, 0);
    if (ret < 0) return -1;
    return ret;
}

/**
 * @close[sock] - Close socket
 * sock: socket handle
 * Returns: 0 on success, -1 on error
 */
long sc_close(long sock) {
    init_socket_table();
    
    if (sock < 0 || sock >= MAX_SOCKETS) return -1;
    
    pthread_mutex_lock(&socket_table_lock);
    int fd = socket_table[sock].fd;
    if (fd >= 0) {
        socket_table[sock].fd = -1;
        socket_table[sock].state = 0;
    }
    pthread_mutex_unlock(&socket_table_lock);
    
    if (fd < 0) return -1;
    
    int ret = close(fd);
    return ret < 0 ? -1 : 0;
}
