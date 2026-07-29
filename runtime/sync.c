// ============================================================
// RasCode Runtime: Synchronization Library
// ============================================================
// Implements mutex, semaphore, condition variable, and atomic 
// operations for thread-safe concurrent programming
// ============================================================

#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#define MAX_MUTEXES 1024
#define MAX_SEMAPHORES 1024
#define MAX_CONDITIONS 1024

// ============================================================
// MUTEX POOL
// ============================================================

typedef struct {
    pthread_mutex_t lock;
    int in_use;
} sc_mutex_t;

static sc_mutex_t mutex_pool[MAX_MUTEXES];
static pthread_mutex_t mutex_pool_lock = PTHREAD_MUTEX_INITIALIZER;

// @mutex_create[] -> int
// Creates a new mutex and returns its ID
// Returns: Mutex ID (>= 0) or -1 on failure
int sc_mutex_create() {
    pthread_mutex_lock(&mutex_pool_lock);
    
    for (int i = 0; i < MAX_MUTEXES; i++) {
        if (!mutex_pool[i].in_use) {
            if (pthread_mutex_init(&mutex_pool[i].lock, NULL) == 0) {
                mutex_pool[i].in_use = 1;
                pthread_mutex_unlock(&mutex_pool_lock);
                return i;
            }
        }
    }
    
    pthread_mutex_unlock(&mutex_pool_lock);
    return -1;  // No free mutexes
}

// @mutex_lock[id] -> int
// Acquires lock (blocking if necessary)
// Returns: 0 on success, -1 on error
int sc_mutex_lock(int id) {
    if (id < 0 || id >= MAX_MUTEXES) return -1;
    
    pthread_mutex_lock(&mutex_pool_lock);
    if (!mutex_pool[id].in_use) {
        pthread_mutex_unlock(&mutex_pool_lock);
        return -1;
    }
    pthread_mutex_unlock(&mutex_pool_lock);
    
    int result = pthread_mutex_lock(&mutex_pool[id].lock);
    return (result == 0) ? 0 : -1;
}

// @mutex_unlock[id] -> int
// Releases lock if owned by current thread
// Returns: 0 on success, -1 on error
int sc_mutex_unlock(int id) {
    if (id < 0 || id >= MAX_MUTEXES) return -1;
    
    pthread_mutex_lock(&mutex_pool_lock);
    if (!mutex_pool[id].in_use) {
        pthread_mutex_unlock(&mutex_pool_lock);
        return -1;
    }
    pthread_mutex_unlock(&mutex_pool_lock);
    
    int result = pthread_mutex_unlock(&mutex_pool[id].lock);
    return (result == 0) ? 0 : -1;
}

// @mutex_trylock[id] -> int
// Non-blocking lock attempt
// Returns: 1 if locked, 0 if already locked, -1 on error
int sc_mutex_trylock(int id) {
    if (id < 0 || id >= MAX_MUTEXES) return -1;
    
    pthread_mutex_lock(&mutex_pool_lock);
    if (!mutex_pool[id].in_use) {
        pthread_mutex_unlock(&mutex_pool_lock);
        return -1;
    }
    pthread_mutex_unlock(&mutex_pool_lock);
    
    int result = pthread_mutex_trylock(&mutex_pool[id].lock);
    if (result == 0) return 1;        // Successfully locked
    if (result == EBUSY) return 0;    // Already locked
    return -1;                         // Error
}

// @mutex_destroy[id] -> int
// Destroys mutex and frees resources
// Returns: 0 on success, -1 on error
int sc_mutex_destroy(int id) {
    if (id < 0 || id >= MAX_MUTEXES) return -1;
    
    pthread_mutex_lock(&mutex_pool_lock);
    
    if (mutex_pool[id].in_use) {
        pthread_mutex_destroy(&mutex_pool[id].lock);
        mutex_pool[id].in_use = 0;
        pthread_mutex_unlock(&mutex_pool_lock);
        return 0;
    }
    
    pthread_mutex_unlock(&mutex_pool_lock);
    return -1;
}

// ============================================================
// SEMAPHORE POOL
// ============================================================

typedef struct {
    sem_t semaphore;
    int in_use;
} sc_semaphore_t;

static sc_semaphore_t semaphore_pool[MAX_SEMAPHORES];
static pthread_mutex_t sem_pool_lock = PTHREAD_MUTEX_INITIALIZER;

// @semaphore_create[count] -> int
// Creates a counting semaphore with initial count
// Returns: Semaphore ID or -1 on error
int sc_semaphore_create(int initial_count) {
    if (initial_count < 0) return -1;
    
    pthread_mutex_lock(&sem_pool_lock);
    
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (!semaphore_pool[i].in_use) {
            if (sem_init(&semaphore_pool[i].semaphore, 0, initial_count) == 0) {
                semaphore_pool[i].in_use = 1;
                pthread_mutex_unlock(&sem_pool_lock);
                return i;
            }
        }
    }
    
    pthread_mutex_unlock(&sem_pool_lock);
    return -1;
}

// @semaphore_wait[id] -> int
// Waits/decrements semaphore (blocking if needed)
// Returns: 0 on success, -1 on error
int sc_semaphore_wait(int id) {
    if (id < 0 || id >= MAX_SEMAPHORES) return -1;
    
    pthread_mutex_lock(&sem_pool_lock);
    if (!semaphore_pool[id].in_use) {
        pthread_mutex_unlock(&sem_pool_lock);
        return -1;
    }
    pthread_mutex_unlock(&sem_pool_lock);
    
    int result = sem_wait(&semaphore_pool[id].semaphore);
    return (result == 0) ? 0 : -1;
}

// @semaphore_signal[id] -> int
// Signals/increments semaphore
// Returns: 0 on success, -1 on error
int sc_semaphore_signal(int id) {
    if (id < 0 || id >= MAX_SEMAPHORES) return -1;
    
    pthread_mutex_lock(&sem_pool_lock);
    if (!semaphore_pool[id].in_use) {
        pthread_mutex_unlock(&sem_pool_lock);
        return -1;
    }
    pthread_mutex_unlock(&sem_pool_lock);
    
    int result = sem_post(&semaphore_pool[id].semaphore);
    return (result == 0) ? 0 : -1;
}

// ============================================================
// CONDITION VARIABLE POOL
// ============================================================

typedef struct {
    pthread_cond_t cond;
    int in_use;
} sc_cond_t;

static sc_cond_t cond_pool[MAX_CONDITIONS];
static pthread_mutex_t cond_pool_lock = PTHREAD_MUTEX_INITIALIZER;

// @cond_create[] -> int
// Creates a condition variable
// Returns: Condition variable ID or -1 on error
int sc_cond_create() {
    pthread_mutex_lock(&cond_pool_lock);
    
    for (int i = 0; i < MAX_CONDITIONS; i++) {
        if (!cond_pool[i].in_use) {
            if (pthread_cond_init(&cond_pool[i].cond, NULL) == 0) {
                cond_pool[i].in_use = 1;
                pthread_mutex_unlock(&cond_pool_lock);
                return i;
            }
        }
    }
    
    pthread_mutex_unlock(&cond_pool_lock);
    return -1;
}

// @cond_wait[mutex_id, cond_id] -> int
// Condition variable wait (releases mutex, waits for signal)
// Returns: 0 on success, -1 on error
int sc_cond_wait(int mutex_id, int cond_id) {
    if (mutex_id < 0 || mutex_id >= MAX_MUTEXES) return -1;
    if (cond_id < 0 || cond_id >= MAX_CONDITIONS) return -1;
    
    pthread_mutex_lock(&mutex_pool_lock);
    if (!mutex_pool[mutex_id].in_use) {
        pthread_mutex_unlock(&mutex_pool_lock);
        return -1;
    }
    pthread_mutex_unlock(&mutex_pool_lock);
    
    pthread_mutex_lock(&cond_pool_lock);
    if (!cond_pool[cond_id].in_use) {
        pthread_mutex_unlock(&cond_pool_lock);
        return -1;
    }
    pthread_mutex_unlock(&cond_pool_lock);
    
    int result = pthread_cond_wait(&cond_pool[cond_id].cond, 
                                   &mutex_pool[mutex_id].lock);
    return (result == 0) ? 0 : -1;
}

// @cond_signal[cond_id] -> int
// Wakes one thread waiting on condition
// Returns: 0 on success, -1 on error
int sc_cond_signal(int cond_id) {
    if (cond_id < 0 || cond_id >= MAX_CONDITIONS) return -1;
    
    pthread_mutex_lock(&cond_pool_lock);
    if (!cond_pool[cond_id].in_use) {
        pthread_mutex_unlock(&cond_pool_lock);
        return -1;
    }
    pthread_mutex_unlock(&cond_pool_lock);
    
    int result = pthread_cond_signal(&cond_pool[cond_id].cond);
    return (result == 0) ? 0 : -1;
}

// @cond_broadcast[cond_id] -> int
// Wakes all threads waiting on condition
// Returns: 0 on success, -1 on error
int sc_cond_broadcast(int cond_id) {
    if (cond_id < 0 || cond_id >= MAX_CONDITIONS) return -1;
    
    pthread_mutex_lock(&cond_pool_lock);
    if (!cond_pool[cond_id].in_use) {
        pthread_mutex_unlock(&cond_pool_lock);
        return -1;
    }
    pthread_mutex_unlock(&cond_pool_lock);
    
    int result = pthread_cond_broadcast(&cond_pool[cond_id].cond);
    return (result == 0) ? 0 : -1;
}

// ============================================================
// ATOMIC OPERATIONS (lock-free)
// ============================================================

// @atomic_cmp_swap[ptr, expected, new_value] -> int
// Atomically compare and swap (CAS operation)
// Returns: 1 if swap successful, 0 if value didn't match
int sc_atomic_cmp_swap(int *ptr, int expected, int new_value) {
    return __sync_bool_compare_and_swap(ptr, expected, new_value) ? 1 : 0;
}

// @atomic_increment[ptr] -> int
// Atomically increment value
// Returns: New value
int sc_atomic_increment(int *ptr) {
    return __sync_add_and_fetch(ptr, 1);
}

// @atomic_decrement[ptr] -> int
// Atomically decrement value
// Returns: New value
int sc_atomic_decrement(int *ptr) {
    return __sync_sub_and_fetch(ptr, 1);
}
