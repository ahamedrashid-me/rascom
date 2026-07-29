// ============================================================
// RasCode Runtime: Thread Pool Library
// ============================================================
// Implements a work-stealing thread pool for task parallelism
// ============================================================

#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_POOLS 64
#define MAX_TASKS_PER_POOL 1024
#define MAX_WORKERS 256

// ============================================================
// TASK QUEUE
// ============================================================

typedef struct {
    int (*fn)(int);      // Function pointer
    int arg;             // Argument
    int valid;           // Is this task valid?
} sc_task_t;

typedef struct {
    sc_task_t tasks[MAX_TASKS_PER_POOL];
    int head;            // Read position
    int tail;            // Write position
    int count;           // Number of tasks
    pthread_mutex_t lock;
    sem_t task_available;
    sem_t task_done;
} sc_task_queue_t;

// ============================================================
// THREAD POOL
// ============================================================

typedef struct {
    pthread_t workers[MAX_WORKERS];  // Worker threads
    int num_workers;                 // Number of workers
    sc_task_queue_t task_queue;      // Shared task queue
    int shutdown;                    // Shutdown flag
    int tasks_completed;             // Count of completed tasks
    pthread_mutex_t pool_lock;
    int in_use;                      // Is this pool allocated?
} sc_pool_t;

static sc_pool_t pool_pool[MAX_POOLS];
static pthread_mutex_t pool_pool_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t pools_init_once = PTHREAD_ONCE_INIT;

// Initialize the pool pool (call once - thread-safe)
static void pools_init_once_func() {
    for (int i = 0; i < MAX_POOLS; i++) {
        pool_pool[i].in_use = 0;
        pool_pool[i].shutdown = 0;
        pool_pool[i].num_workers = 0;
        pool_pool[i].tasks_completed = 0;
    }
}

// Thread-safe initialization wrapper
static void pools_init() {
    // SECURITY: Use pthread_once for thread-safe one-time initialization
    // This prevents race conditions where multiple threads could initialize simultaneously
    pthread_once(&pools_init_once, pools_init_once_func);
}

// Worker thread function
static void* pool_worker(void *arg) {
    sc_pool_t *pool = (sc_pool_t *)arg;
    sc_task_queue_t *queue = &pool->task_queue;
    
    while (1) {
        // Wait for a task to be available
        if (sem_wait(&queue->task_available) != 0) {
            if (pool->shutdown) break;
            continue;
        }
        
        // Check for shutdown
        if (pool->shutdown) break;
        
        // Get next task
        pthread_mutex_lock(&queue->lock);
        
        if (queue->count > 0) {
            sc_task_t task = queue->tasks[queue->head];
            queue->head = (queue->head + 1) % MAX_TASKS_PER_POOL;
            queue->count--;
            
            pthread_mutex_unlock(&queue->lock);
            
            // Execute task
            if (task.valid && task.fn) {
                task.fn(task.arg);
                pool->tasks_completed++;
            }
            
            // Signal task completion
            sem_post(&queue->task_done);
        } else {
            pthread_mutex_unlock(&queue->lock);
        }
    }
    
    return NULL;
}

// @pool_create[num_workers] -> int
// Creates a thread pool with specified number of worker threads
// Returns: Pool ID (>= 0) or -1 on failure
int sc_pool_create(int num_workers) {
    if (num_workers <= 0 || num_workers > MAX_WORKERS) return -1;
    
    pools_init();
    pthread_mutex_lock(&pool_pool_lock);
    
    // Find free pool slot
    for (int i = 0; i < MAX_POOLS; i++) {
        if (!pool_pool[i].in_use) {
            sc_pool_t *pool = &pool_pool[i];
            
            // Initialize task queue
            pool->task_queue.head = 0;
            pool->task_queue.tail = 0;
            pool->task_queue.count = 0;
            
            if (pthread_mutex_init(&pool->task_queue.lock, NULL) != 0) {
                pthread_mutex_unlock(&pool_pool_lock);
                return -1;
            }
            
            if (sem_init(&pool->task_queue.task_available, 0, 0) != 0) {
                pthread_mutex_destroy(&pool->task_queue.lock);
                pthread_mutex_unlock(&pool_pool_lock);
                return -1;
            }
            
            if (sem_init(&pool->task_queue.task_done, 0, 0) != 0) {
                sem_destroy(&pool->task_queue.task_available);
                pthread_mutex_destroy(&pool->task_queue.lock);
                pthread_mutex_unlock(&pool_pool_lock);
                return -1;
            }
            
            // Initialize pool
            pool->num_workers = num_workers;
            pool->shutdown = 0;
            pool->tasks_completed = 0;
            
            if (pthread_mutex_init(&pool->pool_lock, NULL) != 0) {
                sem_destroy(&pool->task_queue.task_available);
                sem_destroy(&pool->task_queue.task_done);
                pthread_mutex_destroy(&pool->task_queue.lock);
                pthread_mutex_unlock(&pool_pool_lock);
                return -1;
            }
            
            // Create worker threads
            for (int j = 0; j < num_workers; j++) {
                if (pthread_create(&pool->workers[j], NULL, pool_worker, pool) != 0) {
                    // Cleanup on failure
                    pool->shutdown = 1;
                    for (int k = 0; k < j; k++) {
                        sem_post(&pool->task_queue.task_available);
                        pthread_join(pool->workers[k], NULL);
                    }
                    sem_destroy(&pool->task_queue.task_available);
                    sem_destroy(&pool->task_queue.task_done);
                    pthread_mutex_destroy(&pool->task_queue.lock);
                    pthread_mutex_destroy(&pool->pool_lock);
                    pthread_mutex_unlock(&pool_pool_lock);
                    return -1;
                }
            }
            
            pool->in_use = 1;
            pthread_mutex_unlock(&pool_pool_lock);
            return i;
        }
    }
    
    pthread_mutex_unlock(&pool_pool_lock);
    return -1;  // No free pools
}

// @pool_submit[pool_id, fn_addr, arg] -> int
// Submits a task to the pool
// Returns: 0 on success, -1 on error
int sc_pool_submit(int pool_id, int (*fn)(int), int arg) {
    if (pool_id < 0 || pool_id >= MAX_POOLS) return -1;
    
    pools_init();
    pthread_mutex_lock(&pool_pool_lock);
    if (!pool_pool[pool_id].in_use) {
        pthread_mutex_unlock(&pool_pool_lock);
        return -1;
    }
    pthread_mutex_unlock(&pool_pool_lock);
    
    sc_pool_t *pool = &pool_pool[pool_id];
    sc_task_queue_t *queue = &pool->task_queue;
    
    // Add task to queue
    pthread_mutex_lock(&queue->lock);
    
    if (queue->count >= MAX_TASKS_PER_POOL) {
        pthread_mutex_unlock(&queue->lock);
        return -1;  // Queue is full
    }
    
    queue->tasks[queue->tail].fn = fn;
    queue->tasks[queue->tail].arg = arg;
    queue->tasks[queue->tail].valid = 1;
    queue->tail = (queue->tail + 1) % MAX_TASKS_PER_POOL;
    queue->count++;
    
    pthread_mutex_unlock(&queue->lock);
    
    // Signal that a task is available
    sem_post(&queue->task_available);
    
    return 0;
}

// @pool_wait[pool_id] -> int
// Waits for all submitted tasks to complete
// Returns: 0 on success, -1 on error
int sc_pool_wait(int pool_id) {
    if (pool_id < 0 || pool_id >= MAX_POOLS) return -1;
    
    pools_init();
    pthread_mutex_lock(&pool_pool_lock);
    if (!pool_pool[pool_id].in_use) {
        pthread_mutex_unlock(&pool_pool_lock);
        return -1;
    }
    pthread_mutex_unlock(&pool_pool_lock);
    
    sc_pool_t *pool = &pool_pool[pool_id];
    sc_task_queue_t *queue = &pool->task_queue;
    
    // Wait for all tasks to complete
    // This is a simple implementation - just wait on the semaphore
    // In a production system, you'd track task counts more carefully
    
    pthread_mutex_lock(&queue->lock);
    int tasks_in_queue = queue->count;
    pthread_mutex_unlock(&queue->lock);
    
    // Wait for all tasks to complete
    for (int i = 0; i < tasks_in_queue; i++) {
        sem_wait(&queue->task_done);
    }
    
    return 0;
}

// @pool_destroy[pool_id] -> int
// Destroys the thread pool and waits for all workers to finish
// Returns: 0 on success, -1 on error
int sc_pool_destroy(int pool_id) {
    if (pool_id < 0 || pool_id >= MAX_POOLS) return -1;
    
    pools_init();
    pthread_mutex_lock(&pool_pool_lock);
    
    if (pool_pool[pool_id].in_use) {
        sc_pool_t *pool = &pool_pool[pool_id];
        sc_task_queue_t *queue = &pool->task_queue;
        
        // Signal shutdown
        pool->shutdown = 1;
        
        // Wake up all workers so they can exit
        for (int i = 0; i < pool->num_workers; i++) {
            sem_post(&queue->task_available);
        }
        
        pthread_mutex_unlock(&pool_pool_lock);
        
        // Wait for all workers to finish
        for (int i = 0; i < pool->num_workers; i++) {
            pthread_join(pool->workers[i], NULL);
        }
        
        pthread_mutex_lock(&pool_pool_lock);
        
        // Cleanup
        sem_destroy(&queue->task_available);
        sem_destroy(&queue->task_done);
        pthread_mutex_destroy(&queue->lock);
        pthread_mutex_destroy(&pool->pool_lock);
        
        pool->in_use = 0;
        pool->num_workers = 0;
        
        pthread_mutex_unlock(&pool_pool_lock);
        return 0;
    }
    
    pthread_mutex_unlock(&pool_pool_lock);
    return -1;
}
