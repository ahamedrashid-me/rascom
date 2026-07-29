// RasCode Advanced Concurrency Primitives
// Reader/writer locks, barriers, events, queues

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Reader/Writer Lock
typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t read_cond;
    pthread_cond_t write_cond;
    int readers;
    int writers;
    int waiting_writers;
} sc_rwlock_t;

static sc_rwlock_t *sc_rwlocks[64] = {0};
static int sc_rwlock_count = 0;

// Create reader/writer lock
// Usage: @rwlock_create[] -> lock_id
long sc_rwlock_create(void) {
    if (sc_rwlock_count >= 64) return -1;
    
    sc_rwlock_t *lock = malloc(sizeof(sc_rwlock_t));
    if (!lock) return -1;
    
    pthread_mutex_init(&lock->lock, NULL);
    pthread_cond_init(&lock->read_cond, NULL);
    pthread_cond_init(&lock->write_cond, NULL);
    lock->readers = 0;
    lock->writers = 0;
    lock->waiting_writers = 0;
    
    sc_rwlocks[sc_rwlock_count] = lock;
    return sc_rwlock_count++;
}

// Read lock
// Usage: @rwlock_read[id] -> 1 on success
long sc_rwlock_read(long id) {
    if (id < 0 || id >= sc_rwlock_count) return -1;
    
    sc_rwlock_t *lock = sc_rwlocks[id];
    pthread_mutex_lock(&lock->lock);
    
    while (lock->writers || lock->waiting_writers) {
        pthread_cond_wait(&lock->read_cond, &lock->lock);
    }
    
    lock->readers++;
    pthread_mutex_unlock(&lock->lock);
    return 1;
}

// Read unlock
// Usage: @rwlock_read_unlock[id] -> 1 on success
long sc_rwlock_read_unlock(long id) {
    if (id < 0 || id >= sc_rwlock_count) return -1;
    
    sc_rwlock_t *lock = sc_rwlocks[id];
    pthread_mutex_lock(&lock->lock);
    
    lock->readers--;
    if (lock->readers == 0) {
        pthread_cond_signal(&lock->write_cond);
    }
    
    pthread_mutex_unlock(&lock->lock);
    return 1;
}

// Write lock
// Usage: @rwlock_write[id] -> 1 on success
long sc_rwlock_write(long id) {
    if (id < 0 || id >= sc_rwlock_count) return -1;
    
    sc_rwlock_t *lock = sc_rwlocks[id];
    pthread_mutex_lock(&lock->lock);
    
    lock->waiting_writers++;
    while (lock->readers || lock->writers) {
        pthread_cond_wait(&lock->write_cond, &lock->lock);
    }
    lock->waiting_writers--;
    lock->writers++;
    
    pthread_mutex_unlock(&lock->lock);
    return 1;
}

// Write unlock
// Usage: @rwlock_write_unlock[id] -> 1 on success
long sc_rwlock_write_unlock(long id) {
    if (id < 0 || id >= sc_rwlock_count) return -1;
    
    sc_rwlock_t *lock = sc_rwlocks[id];
    pthread_mutex_lock(&lock->lock);
    
    lock->writers--;
    pthread_cond_broadcast(&lock->write_cond);
    pthread_cond_broadcast(&lock->read_cond);
    
    pthread_mutex_unlock(&lock->lock);
    return 1;
}

// Barrier
typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int count;
    int target;
    int generation;
} sc_barrier_t;

static sc_barrier_t *sc_barriers[64] = {0};
static int sc_barrier_count = 0;

// Create barrier
// Usage: @barrier_create[num_threads] -> barrier_id
long sc_barrier_create(long num_threads) {
    if (sc_barrier_count >= 64 || num_threads <= 0) return -1;
    
    sc_barrier_t *bar = malloc(sizeof(sc_barrier_t));
    if (!bar) return -1;
    
    pthread_mutex_init(&bar->lock, NULL);
    pthread_cond_init(&bar->cond, NULL);
    bar->count = 0;
    bar->target = num_threads;
    bar->generation = 0;
    
    sc_barriers[sc_barrier_count] = bar;
    return sc_barrier_count++;
}

// Wait on barrier
// Usage: @barrier_wait[id] -> 1 when released
long sc_barrier_wait(long id) {
    if (id < 0 || id >= sc_barrier_count) return -1;
    
    sc_barrier_t *bar = sc_barriers[id];
    pthread_mutex_lock(&bar->lock);
    
    int gen = bar->generation;
    bar->count++;
    
    if (bar->count >= bar->target) {
        bar->count = 0;
        bar->generation++;
        pthread_cond_broadcast(&bar->cond);
    } else {
        while (bar->generation == gen) {
            pthread_cond_wait(&bar->cond, &bar->lock);
        }
    }
    
    pthread_mutex_unlock(&bar->lock);
    return 1;
}

// Event
typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int signaled;
} sc_event_t;

static sc_event_t *sc_events[64] = {0};
static int sc_event_count = 0;

// Create event
// Usage: @event_create[] -> event_id
long sc_event_create(void) {
    if (sc_event_count >= 64) return -1;
    
    sc_event_t *event = malloc(sizeof(sc_event_t));
    if (!event) return -1;
    
    pthread_mutex_init(&event->lock, NULL);
    pthread_cond_init(&event->cond, NULL);
    event->signaled = 0;
    
    sc_events[sc_event_count] = event;
    return sc_event_count++;
}

// Signal event
// Usage: @event_signal[id] -> 1
long sc_event_signal(long id) {
    if (id < 0 || id >= sc_event_count) return -1;
    
    sc_event_t *event = sc_events[id];
    pthread_mutex_lock(&event->lock);
    event->signaled = 1;
    pthread_cond_broadcast(&event->cond);
    pthread_mutex_unlock(&event->lock);
    
    return 1;
}

// Wait for event
// Usage: @event_wait[id] -> 1 when signaled
long sc_event_wait(long id) {
    if (id < 0 || id >= sc_event_count) return -1;
    
    sc_event_t *event = sc_events[id];
    pthread_mutex_lock(&event->lock);
    
    while (!event->signaled) {
        pthread_cond_wait(&event->cond, &event->lock);
    }
    
    pthread_mutex_unlock(&event->lock);
    return 1;
}

// Reset event
// Usage: @event_reset[id] -> 1
long sc_event_reset(long id) {
    if (id < 0 || id >= sc_event_count) return -1;
    
    sc_event_t *event = sc_events[id];
    pthread_mutex_lock(&event->lock);
    event->signaled = 0;
    pthread_mutex_unlock(&event->lock);
    
    return 1;
}
