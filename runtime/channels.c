// ============================================================
// RasCode Runtime: Channel Communication Library
// ============================================================
// Implements Go-style channels for thread-safe message passing
// ============================================================

#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#define MAX_CHANNELS 256
#define MAX_CHANNEL_BUFFER 1024

// ============================================================
// CHANNEL STRUCTURE
// ============================================================

typedef struct {
    int buffer[MAX_CHANNEL_BUFFER];  // Circular buffer
    int capacity;                    // Max size
    int size;                        // Current size
    int read_pos;                    // Read position
    int write_pos;                   // Write position
    pthread_mutex_t lock;            // Access lock
    sem_t can_send;                  // Can send semaphore (size < capacity)
    sem_t can_recv;                  // Can receive semaphore (size > 0)
    int in_use;                      // Is this channel allocated?
} sc_channel_t;

static sc_channel_t channel_pool[MAX_CHANNELS];
static pthread_mutex_t channel_pool_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t channels_init_once = PTHREAD_ONCE_INIT;

// Initialize the channel pool (call once - thread-safe)
static void channels_init_once_func() {
    for (int i = 0; i < MAX_CHANNELS; i++) {
        channel_pool[i].in_use = 0;
        channel_pool[i].size = 0;
        channel_pool[i].read_pos = 0;
        channel_pool[i].write_pos = 0;
    }
}

// Thread-safe initialization wrapper
static void channels_init() {
    // SECURITY: Use pthread_once for thread-safe one-time initialization
    // This prevents race conditions where multiple threads could initialize simultaneously
    pthread_once(&channels_init_once, channels_init_once_func);
}

// @channel_create[capacity] -> int
// Creates a new channel with specified buffer capacity
// Returns: Channel ID (>= 0) or -1 on failure
int sc_channel_create(int capacity) {
    if (capacity <= 0 || capacity > MAX_CHANNEL_BUFFER) return -1;
    
    channels_init();
    pthread_mutex_lock(&channel_pool_lock);
    
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (!channel_pool[i].in_use) {
            // Initialize channel
            channel_pool[i].capacity = capacity;
            channel_pool[i].size = 0;
            channel_pool[i].read_pos = 0;
            channel_pool[i].write_pos = 0;
            
            // Initialize synchronization primitives
            if (pthread_mutex_init(&channel_pool[i].lock, NULL) != 0) {
                pthread_mutex_unlock(&channel_pool_lock);
                return -1;
            }
            
            // can_send: initially we can send (size < capacity)
            if (sem_init(&channel_pool[i].can_send, 0, capacity) != 0) {
                pthread_mutex_destroy(&channel_pool[i].lock);
                pthread_mutex_unlock(&channel_pool_lock);
                return -1;
            }
            
            // can_recv: initially we cannot receive (size == 0)
            if (sem_init(&channel_pool[i].can_recv, 0, 0) != 0) {
                sem_destroy(&channel_pool[i].can_send);
                pthread_mutex_destroy(&channel_pool[i].lock);
                pthread_mutex_unlock(&channel_pool_lock);
                return -1;
            }
            
            channel_pool[i].in_use = 1;
            pthread_mutex_unlock(&channel_pool_lock);
            return i;
        }
    }
    
    pthread_mutex_unlock(&channel_pool_lock);
    return -1;  // No free channels
}

// @channel_send[ch_id, value] -> int
// Sends a value into the channel (blocks if full)
// Returns: 0 on success, -1 on error
int sc_channel_send(int ch_id, int value) {
    if (ch_id < 0 || ch_id >= MAX_CHANNELS) return -1;
    
    channels_init();
    pthread_mutex_lock(&channel_pool_lock);
    if (!channel_pool[ch_id].in_use) {
        pthread_mutex_unlock(&channel_pool_lock);
        return -1;
    }
    pthread_mutex_unlock(&channel_pool_lock);
    
    sc_channel_t *ch = &channel_pool[ch_id];
    
    // Wait if buffer is full
    if (sem_wait(&ch->can_send) != 0) return -1;
    
    // Acquire mutex for buffer access
    if (pthread_mutex_lock(&ch->lock) != 0) return -1;
    
    // Add to buffer
    if (ch->size < ch->capacity) {
        ch->buffer[ch->write_pos] = value;
        ch->write_pos = (ch->write_pos + 1) % ch->capacity;
        ch->size++;
    }
    
    pthread_mutex_unlock(&ch->lock);
    
    // Signal that data is available
    sem_post(&ch->can_recv);
    
    return 0;
}

// @channel_recv[ch_id] -> int
// Receives a value from the channel (blocks if empty)
// Returns: Value from channel (or -1 if error)
int sc_channel_recv(int ch_id) {
    if (ch_id < 0 || ch_id >= MAX_CHANNELS) return -1;
    
    channels_init();
    pthread_mutex_lock(&channel_pool_lock);
    if (!channel_pool[ch_id].in_use) {
        pthread_mutex_unlock(&channel_pool_lock);
        return -1;
    }
    pthread_mutex_unlock(&channel_pool_lock);
    
    sc_channel_t *ch = &channel_pool[ch_id];
    
    // Wait if buffer is empty
    if (sem_wait(&ch->can_recv) != 0) return -1;
    
    // Acquire mutex for buffer access
    if (pthread_mutex_lock(&ch->lock) != 0) return -1;
    
    int value = -1;
    if (ch->size > 0) {
        value = ch->buffer[ch->read_pos];
        ch->read_pos = (ch->read_pos + 1) % ch->capacity;
        ch->size--;
    }
    
    pthread_mutex_unlock(&ch->lock);
    
    // Signal that space is available
    sem_post(&ch->can_send);
    
    return value;
}

// @channel_close[ch_id] -> int
// Closes the channel and frees resources
// Returns: 0 on success, -1 on error
int sc_channel_close(int ch_id) {
    if (ch_id < 0 || ch_id >= MAX_CHANNELS) return -1;
    
    channels_init();
    pthread_mutex_lock(&channel_pool_lock);
    
    if (channel_pool[ch_id].in_use) {
        sc_channel_t *ch = &channel_pool[ch_id];
        
        // Destroy synchronization primitives
        sem_destroy(&ch->can_send);
        sem_destroy(&ch->can_recv);
        pthread_mutex_destroy(&ch->lock);
        
        // Mark as unused
        ch->in_use = 0;
        ch->size = 0;
        
        pthread_mutex_unlock(&channel_pool_lock);
        return 0;
    }
    
    pthread_mutex_unlock(&channel_pool_lock);
    return -1;
}

// @channel_empty[ch_id] -> int
// Checks if channel is empty
// Returns: 1 if empty, 0 if has data, -1 on error
int sc_channel_empty(int ch_id) {
    if (ch_id < 0 || ch_id >= MAX_CHANNELS) return -1;
    
    channels_init();
    pthread_mutex_lock(&channel_pool_lock);
    if (!channel_pool[ch_id].in_use) {
        pthread_mutex_unlock(&channel_pool_lock);
        return -1;
    }
    pthread_mutex_unlock(&channel_pool_lock);
    
    sc_channel_t *ch = &channel_pool[ch_id];
    
    pthread_mutex_lock(&ch->lock);
    int result = (ch->size == 0) ? 1 : 0;
    pthread_mutex_unlock(&ch->lock);
    
    return result;
}

// @channel_full[ch_id] -> int
// Checks if channel is full
// Returns: 1 if full, 0 if has space, -1 on error
int sc_channel_full(int ch_id) {
    if (ch_id < 0 || ch_id >= MAX_CHANNELS) return -1;
    
    channels_init();
    pthread_mutex_lock(&channel_pool_lock);
    if (!channel_pool[ch_id].in_use) {
        pthread_mutex_unlock(&channel_pool_lock);
        return -1;
    }
    pthread_mutex_unlock(&channel_pool_lock);
    
    sc_channel_t *ch = &channel_pool[ch_id];
    
    pthread_mutex_lock(&ch->lock);
    int result = (ch->size >= ch->capacity) ? 1 : 0;
    pthread_mutex_unlock(&ch->lock);
    
    return result;
}

// @channel_destroy[ch_id] -> int
// Destroys and deallocates a channel
// Returns: 1 on success, -1 on error
// SECURITY: Properly cleanup semaphores and mutexes to prevent resource leaks
int sc_channel_destroy(int ch_id) {
    if (ch_id < 0 || ch_id >= MAX_CHANNELS) return -1;
    
    pthread_mutex_lock(&channel_pool_lock);
    if (!channel_pool[ch_id].in_use) {
        pthread_mutex_unlock(&channel_pool_lock);
        return -1;
    }
    
    sc_channel_t *ch = &channel_pool[ch_id];
    
    // Clean up synchronization primitives
    pthread_mutex_lock(&ch->lock);
    
    sem_destroy(&ch->can_send);
    sem_destroy(&ch->can_recv);
    pthread_mutex_unlock(&ch->lock);
    
    pthread_mutex_destroy(&ch->lock);
    
    // Mark as free
    ch->in_use = 0;
    ch->size = 0;
    
    pthread_mutex_unlock(&channel_pool_lock);
    
    return 1;
}

