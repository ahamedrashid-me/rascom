# 18: Concurrency

## Threading Basics

### Create Thread

Use `@spawn` to create new thread.

```ras
fnc worker[id]::int {
    show["Worker "];
    show[id];
    show["\n"];
    get[0];                       // Return value
}

int tid = @spawn[worker, 42];     // Create thread with arg 42
show["Created thread: "];
show[tid];
show["\n"];
```

**Thread function must:**
- Accept one argument (any type)
- Return int

### Wait for Thread

Use `@join` to wait for thread completion.

```ras
int tid = @spawn[worker, 42];
int result = @join[tid];          // Block until thread finishes
show["Thread returned: "];
show[result];
show["\n"];
```

### Get Thread ID

```ras
int my_id = @pid[];                // Current thread's ID
show["My thread ID: "];
show[my_id];
show["\n"];
```

### Terminate Thread

```ras
int tid = @spawn[long_task, 0];
@sleep[1000];                     // Let it run 1 second
@kill[tid];                       // Force termination
```

## Multiple Threads

```ras
fnc worker[id]::int {
    loop[int i = 0; i < 3; i++] {
        show["Worker "];
        show[id];
        show[" step "];
        show[i];
        show["\n"];
        @sleep[100];
    }
    get[id];                      // Return worker ID
}

fnc main[]::int {
    int t1 = @spawn[worker, 1];
    int t2 = @spawn[worker, 2];
    int t3 = @spawn[worker, 3];
    
    show["Waiting for threads...\n"];
    
    int r1 = @join[t1];           // 1
    int r2 = @join[t2];           // 2
    int r3 = @join[t3];           // 3
    
    show["All done\n"];
    get[0];
}
```

## Mutexes

Protect shared data.

### Create Mutex

```ras
int mutex = @mutex_create[];
```

### Lock/Unlock

```ras
int counter = 0;
int m = @mutex_create[];

fnc increment[]::int {
    @mutex_lock[m];               // Acquire lock
    
    int val = counter;
    val = val + 1;
    counter = val;
    
    @mutex_unlock[m];             // Release lock
    get[0];
}

// Safe to call from multiple threads
int t1 = @spawn[increment, 0];
int t2 = @spawn[increment, 0];
@join[t1];
@join[t2];

show["Counter: "];
show[counter];
show["\n"];                       // Outputs: 2
```

### Try Lock (Non-blocking)

```ras
int acquired = @mutex_trylock[m];
if[acquired == 1] {
    // Got the lock!
    show["Lock acquired\n"];
    @mutex_unlock[m];
} or {
    // Already locked
    show["Couldn't acquire lock\n"];
}
```

### Destroy Mutex

```ras
@mutex_destroy[m];
```

## Semaphores

Control access to pool of resources.

### Create Semaphore

```ras
int sem = @semaphore_create[2];   // 2 resources available
```

### Wait (Decrement)

```ras
@semaphore_wait[sem];             // Acquire resource
// Use resource
@semaphore_signal[sem];           // Release resource
```

### Signal (Increment)

```ras
@semaphore_signal[sem];           // Make resource available
```

## Condition Variables

Wait for specific condition.

### Create Condition

```ras
int cond = @cond_create[];
int mutex = @mutex_create[];
int ready = 0;
```

### Wait on Condition

```ras
@mutex_lock[mutex];
loop[int ok = 1; ok == 1; ok = 0] {
    if[ready == 1] {
        ok = 0;                   // Exit loop
    } or {
        @cond_wait[cond, mutex];  // Release mutex, wait
    }
}
// Now have mutex again
@mutex_unlock[mutex];
```

### Signal Waiters

```ras
@mutex_lock[mutex];
ready = 1;
@cond_signal[cond];               // Wake one waiter
@mutex_unlock[mutex];
```

### Broadcast

```ras
@cond_broadcast[cond];            // Wake ALL waiters
```

## Atomic Operations

For lock-free programming.

### Compare and Swap

```ras
int addr = @alloc[8];
@poke[addr, 0];

int old = 0;
int new = 1;
int success = @atomic_cmp_swap[addr, old, new];

if[success == 1] {
    show["Successfully swapped\n"];
}
```

### Atomic Increment

```ras
int counter_ptr = @alloc[8];
@poke[counter_ptr, 0];

@atomic_increment[counter_ptr];   // Now 1
@atomic_increment[counter_ptr];   // Now 2

int val = @peek[counter_ptr];
show["Counter: "];
show[val];
show["\n"];
```

### Atomic Decrement

```ras
@atomic_decrement[counter_ptr];   // Decrement atomically
```

## Channels

Inter-thread communication.

### Create Channel

```ras
int chan = @channel_create[10];   // Capacity 10
```

### Send Data

```ras
@channel_send[chan, 42];
@channel_send[chan, "message"];
@channel_send[chan, true];
```

### Receive Data

```ras
int val = @channel_recv[chan];    // Blocks if empty
str msg = @channel_recv[chan];
bool flag = @channel_recv[chan];
```

### Channel Usage Pattern

```ras
fnc producer[chan]::int {
    loop[int i = 1; i <= 5; i++] {
        @channel_send[chan, i];
        show["Sent: "];
        show[i];
        show["\n"];
        @sleep[100];
    }
    get[0];
}

fnc consumer[chan]::int {
    loop[int i = 0; i < 5; i++] {
        int val = @channel_recv[chan];
        show["Received: "];
        show[val];
        show["\n"];
    }
    get[0];
}

fnc main[]::int {
    int chan = @channel_create[5];
    int p = @spawn[producer, chan];
    int c = @spawn[consumer, chan];
    
    @join[p];
    @join[c];
    @channel_close[chan];
    
    get[0];
}
```

### Check if Empty/Full

```ras
if[@channel_empty[chan]] {
    show["Channel is empty\n"];
}

if[@channel_full[chan]] {
    show["Channel is full\n"];
}
```

### Close Channel

```ras
@channel_close[chan];
```

## Thread Pools

A thread pool maintains a fixed number of worker threads that accept tasks from a queue. This avoids the overhead of creating a new thread for each task and provides better resource management.

### Pool Lifecycle

```ras
// Create pool with 8 worker threads
int pool = @pool_create[8];

// Submit tasks (non-blocking, queues if workers busy)
@pool_submit[pool, task_function, argument];

// Wait for all submitted tasks to complete
@pool_wait[pool];

// Destroy pool (must wait first)
@pool_destroy[pool];
```

### API Reference

#### @pool_create[num_threads] → int

Creates a new thread pool with specified number of worker threads.

**Parameters:**
- `num_threads` (int): Number of worker threads to spawn
  - Recommended: Number of CPU cores or slightly less
  - Minimum: 1
  - Maximum: Practical limit is ~1000 (OS-dependent)

**Returns:** Pool handle (int > 0) or error code

**Example:**
```ras
// Create pool with 4 workers
int pool = @pool_create[4];
if[pool <= 0] {
    show["Pool creation failed\n"];
    @exit[1];
}
```

**Performance Notes:**
- ✅ Thread creation: ~1-10ms per thread (OS scheduling)
- ✅ Memory overhead: ~1-2MB per thread (stack + data structures)
- ⚠️ Total memory: 4 threads × 2MB = ~8MB
- ⚠️ Too many threads: Context switching overhead exceeds benefit

#### @pool_submit[pool, function, argument] → int

Submits a task to the pool for execution.

**Parameters:**
- `pool` (int): Pool handle from @pool_create
- `function`: Function to execute (function pointer)
- `argument` (int): Single argument to pass to function

**Returns:**
- `>= 0`: Task accepted (queue position)
- `< 0`: Error (pool full or invalid)

**Behavior:**
- Non-blocking: Returns immediately
- If all workers busy: Task queued
- FIFO order: Tasks execute in submission order (if workers free)

**Example:**
```ras
fnc worker_task[job_id]::int {
    show["Processing job "];
    show[job_id];
    show["\n"];
    @sleep[100];  // Simulate work
    get[0];
}

fnc main[]::int {
    int pool = @pool_create[4];
    
    // Submit 10 tasks
    loop[int i = 1; i <= 10; i++] {
        int result = @pool_submit[pool, worker_task, i];
        if[result < 0] {
            show["Failed to submit task\n"];
        }
    }
    
    get[0];
}
```

**Returning Early from Tasks:**
```ras
fnc task_with_early_return[id]::int {
    if[id == 5] {
        show["Skipping job 5\n"];
        get[0];  // Return from task
    }
    show["Doing job "];
    show[id];
    show["\n"];
    get[0];
}
```

#### @pool_wait[pool] → int

Blocks until all previously submitted tasks are complete.

**Parameters:**
- `pool` (int): Pool handle

**Returns:**
- `0`: All tasks completed successfully
- `< 0`: Error (invalid pool handle)

**Behavior:**
- Blocking: Caller waits for completion
- Completes when queue empty AND all workers idle
- Does NOT stop new submissions from other threads

**Example:**
```ras
int pool = @pool_create[4];

// Submit batch 1
loop[int i = 1; i <= 10; i++] {
    @pool_submit[pool, process, i];
}

// Wait for batch 1 complete
@pool_wait[pool];
show["Batch 1 done\n"];

// Submit batch 2
loop[int i = 11; i <= 20; i++] {
    @pool_submit[pool, process, i];
}

// Wait for batch 2 complete
@pool_wait[pool];
show["Batch 2 done\n"];
```

**Timeout Pattern (simulated):**
```ras
// Poll with timeout
int start = @clock[];
int timeout_ms = 5000;

loop[int wait = 0; wait < 100; wait++] {
    if[@pool_wait[pool] == 0] {
        get[0];  // Success
    }
    int elapsed = @clock[] - start;
    if[elapsed > timeout_ms] {
        show["Timeout waiting for pool\n"];
        get[-1];
    }
    @sleep[50];  // Poll every 50ms
}
```

#### @pool_destroy[pool] → int

Cleanly shuts down the pool.

**Parameters:**
- `pool` (int): Pool handle

**Returns:**
- `0`: Successfully destroyed
- `< 0`: Error (e.g., tasks still running)

**Behavior:**
- MUST call @pool_wait first
- Frees all worker threads
- Deallocates queue
- Prevents further submissions

**Important:** Not calling @pool_destroy causes thread/memory leak!

**Example:**
```ras
int pool = @pool_create[4];

// ... submit and wait ...

@pool_wait[pool];       // Complete all tasks
int result = @pool_destroy[pool];
if[result < 0] {
    show["Destroy failed\n"];
}
```

**Resource Cleanup Pattern:**
```ras
check {
    int pool = @pool_create[4];
    
    // ... submit tasks ...
    
    @pool_wait[pool];
    @pool_destroy[pool];
} when: {
    show["Error—pool resources may leak\n"];
}
```

---

### Common Patterns

#### Pattern 1: Map-Reduce Style

```ras
fnc process_item[id]::int {
    // Expensive computation
    show["Processing "];
    show[id];
    show["\n"];
    @sleep[200];
    get[id];
}

fnc main[]::int {
    int pool = @pool_create[@clock[]];  // Match CPU cores
    
    // Map: Submit all tasks
    int num_items = 100;
    loop[int i = 0; i < num_items; i++] {
        @pool_submit[pool, process_item, i];
    }
    
    // Wait for completion
    @pool_wait[pool];
    
    // Reduce: Combine results (done sequentially in main)
    show["All items processed\n"];
    
    @pool_destroy[pool];
    get[0];
}
```

#### Pattern 2: Worker Pool with Shared State

```ras
// Shared counter (would need atomic in real scenario)
int total_processed = 0;

fnc worker[id]::int {
    loop[int item = 0; item < 10; item++] {
        show["Worker "];
        show[id];
        show[" processing item "];
        show[item];
        show["\n"];
        total_processed = total_processed + 1;
    }
    get[0];
}

fnc main[]::int {
    int pool = @pool_create[4];
    
    // Create 4 workers, each processes 10 items
    loop[int i = 1; i <= 4; i++] {
        @pool_submit[pool, worker, i];
    }
    
    @pool_wait[pool];
    show["Total processed: "];
    show[total_processed];
    show["\n"];
    
    @pool_destroy[pool];
    get[0];
}
```

#### Pattern 3: Batching with Chunked Processing

```ras
fnc process_chunk[start]::int {
    int chunk_size = 100;
    loop[int i = start; i < start + chunk_size; i++] {
        // Process item i...
    }
    get[0];
}

fnc main[]::int {
    int total_items = 10000;
    int chunk_size = 100;
    int num_chunks = total_items / chunk_size;
    
    int pool = @pool_create[8];
    
    // Submit chunks
    loop[int i = 0; i < num_chunks; i++] {
        int chunk_start = i * chunk_size;
        @pool_submit[pool, process_chunk, chunk_start];
    }
    
    @pool_wait[pool];
    @pool_destroy[pool];
    
    show["Processed "];
    show[total_items];
    show[" items in chunks\n"];
    get[0];
}
```

#### Pattern 4: Pipeline Architecture

```ras
int stage1_queue = @channel_create[100];
int stage2_queue = @channel_create[100];

fnc stage1_worker[id]::int {
    loop[int i = 0; i < 50; i++] {
        int result = i * 2;  // Transform
        @channel_send[stage1_queue, result];
    }
    get[0];
}

fnc stage2_worker[id]::int {
    loop[int i = 0; i < 50; i++] {
        int item = @channel_recv[stage1_queue];
        int result = item + 1;  // Transform
        @channel_send[stage2_queue, result];
    }
    get[0];
}

fnc main[]::int {
    int pool = @pool_create[8];
    
    // Launch stage workers
    loop[int i = 0; i < 4; i++] {
        @pool_submit[pool, stage1_worker, i];
        @pool_submit[pool, stage2_worker, i];
    }
    
    @pool_wait[pool];
    @pool_destroy[pool];
    get[0];
}
```

---

### Performance Characteristics

| Metric | Value | Notes |
|--------|-------|-------|
| Pool creation | ~1-10ms | Depends on thread count |
| Task submission | ~1-100μs | Enqueuing operation, very fast |
| Task dispatch | ~1-10μs | Worker pickup from queue |
| Worker switch | ~100-1000ns | Context switching overhead |
| Memory per thread | ~1-2MB | Stack + kernel structures |
| Maximum threads | ~1000 | OS-dependent, diminishing returns |

**Optimal thread count:**
- **CPU-bound tasks**: Number of CPU cores
- **I/O-bound tasks**: 2x to 10x CPU cores
- **Mixed tasks**: Start with CPU cores, benchmark

### Tuning Tips

| Scenario | Recommendation |
|----------|-----------------|
| CPU-bound (image processing) | Threads = CPU cores |
| I/O-bound (network requests) | Threads = 2-10x CPU cores |
| Many tiny tasks | Larger thread count, batch tasks |
| Long-running tasks | Fewer threads, avoid oversubscription |
| Memory-constrained | Minimize thread count (~2-4) |
| Latency-sensitive | Smaller queues, more threads |

### Common Mistakes

❌ **Mistake 1: Not calling @pool_destroy**
```ras
int pool = @pool_create[4];
// ... use pool ...
// Forgot @pool_destroy—threads and memory leak!
```
✅ **Fix:**
```ras
// Always pair create with destroy
@pool_destroy[pool];
```

❌ **Mistake 2: @pool_wait called without @pool_submit**
```ras
int pool = @pool_create[4];
@pool_wait[pool];  // Waits for nothing, returns immediately
```
✅ **Will work, but useful to wait for:**
```ras
@pool_submit[pool, task, arg];
@pool_wait[pool];  // Now waits for task
```

❌ **Mistake 3: Submitting too many tasks at once**
```ras
// Submit 1 million tasks before waiting
loop[int i = 0; i < 1000000; i++] {
    @pool_submit[pool, task, i];  // Memory issues!
}
```
✅ **Fix: Batch submissions**
```ras
const int BATCH_SIZE = 10000;
loop[int batch = 0; batch < 100; batch++] {
    loop[int i = 0; i < BATCH_SIZE; i++] {
        @pool_submit[pool, task, i];
    }
    @pool_wait[pool];  // Wait between batches
}
```

❌ **Mistake 4: Too many threads**
```ras
int pool = @pool_create[1000];  // Creates 1000 threads!
// Result: High memory use, context switching thrashing
```
✅ **Fix:**
```ras
int pool = @pool_create[8];  // Reasonable for 8 cores
```

---

## Common Patterns

### Producer-Consumer

```ras
int queue = @channel_create[10];
int done = 0;

fnc producer[]::int {
    loop[int i = 1; i <= 5; i++] {
        @channel_send[queue, i];
        @sleep[50];
    }
    get[0];
}

fnc consumer[]::int {
    loop[int count = 0; count < 5; count++] {
        int item = @channel_recv[queue];
        show["Consumed: "];
        show[item];
        show["\n"];
    }
    get[0];
}

fnc main[]::int {
    int p = @spawn[producer, 0];
    int c = @spawn[consumer, 0];
    
    @join[p];
    @join[c];
    
    @channel_close[queue];
    get[0];
}
```

### Work Distribution

```ras
int work_queue = @channel_create[100];
int result_queue = @channel_create[100];

fnc worker[]::int {
    loop[int ok = 1; ok == 1; ok = 0] {
        int job = @channel_recv[work_queue];
        if[job == -1] {           // Sentinel: end of work
            ok = 0;
        } or {
            int result = job * 2;
            @channel_send[result_queue, result];
        }
    }
    get[0];
}

fnc main[]::int {
    // Start 4 workers
    loop[int i = 0; i < 4; i++] {
        @spawn[worker, 0];
    }
    
    // Send work
    loop[int j = 1; j <= 20; j++] {
        @channel_send[work_queue, j];
    }
    
    // Signal end
    loop[int k = 0; k < 4; k++] {
        @channel_send[work_queue, -1];
    }
    
    // Collect results
    loop[int r = 0; r < 20; r++] {
        int result = @channel_recv[result_queue];
        show[result];
        show["\n"];
    }
    
    get[0];
}
```

### Read-Write Lock (Simulated)

```ras
int readers = 0;
int writers = 0;
int read_lock = @mutex_create[];
int write_lock = @mutex_create[];

fnc read_start[]::int {
    @mutex_lock[read_lock];
    readers++;
    if[readers == 1] {
        @mutex_lock[write_lock];
    }
    @mutex_unlock[read_lock];
    get[0];
}

fnc read_end[]::int {
    @mutex_lock[read_lock];
    readers--;
    if[readers == 0] {
        @mutex_unlock[write_lock];
    }
    @mutex_unlock[read_lock];
    get[0];
}

fnc write_start[]::int {
    @mutex_lock[write_lock];
    get[0];
}

fnc write_end[]::int {
    @mutex_unlock[write_lock];
    get[0];
}
```

### Barrier Synchronization

```ras
int barrier_count = 0;
int barrier_max = 4;
int barrier_lock = @mutex_create[];
int barrier_cond = @cond_create[];

fnc barrier_wait[]::int {
    @mutex_lock[barrier_lock];
    
    barrier_count++;
    if[barrier_count < barrier_max] {
        @cond_wait[barrier_cond, barrier_lock];
    } or {
        barrier_count = 0;
        @cond_broadcast[barrier_cond];
    }
    
    @mutex_unlock[barrier_lock];
    get[0];
}

fnc worker[id]::int {
    loop[int phase = 0; phase < 3; phase++] {
        show["Worker "];
        show[id];
        show[" phase "];
        show[phase];
        show["\n"];
        
        barrier_wait[];           // Wait for all
    }
    get[0];
}

fnc main[]::int {
    loop[int i = 0; i < 4; i++] {
        @spawn[worker, i];
    }
    get[0];
}
```

## Timing Patterns

### Timeout Simulation

```ras
fnc operation_with_timeout[timeout_ms]::int {
    int start = @clock[];
    
    loop[int ok = 1; ok == 1; ok = 0] {
        int elapsed = @clock[] - start;
        if[elapsed > timeout_ms] {
            show["Timeout\n"];
            ok = 0;
        } or {
            // Do work incrementally
            @sleep[10];
            
            if[work_done] {
                ok = 0;
            }
        }
    }
    
    get[0];
}
```

### Periodic Task

```ras
fnc periodic_task[interval_ms]::int {
    let[int last_run = @clock[]];
    
    loop[int ok = 1; ok == 1; ok = 0] {
        int now = @clock[];
        if[now - last_run >= interval_ms] {
            show["Task running\n"];
            last_run = now;
        }
        @sleep[10];
    }
    
    get[0];
}
```

## Error Handling with Threads

```ras
check {
    int tid = @spawn[worker, 0];
    int result = @join[tid];
} when: {
    show["Thread error\n"];
}
```

## Memory Safety

**Key rules:**
1. Each thread has its own stack
2. Global variables are shared (protect with mutexes)
3. Heap is shared (protect with mutexes)
4. Function parameters pass by value (safe)

```ras
int shared_data = 0;
int data_mutex = @mutex_create[];

fnc modify_shared[value]::int {
    @mutex_lock[data_mutex];
    shared_data = value;
    @mutex_unlock[data_mutex];
    get[0];
}

fnc read_shared[]::int {
    @mutex_lock[data_mutex];
    int val = shared_data;
    @mutex_unlock[data_mutex];
    get[val];
}
```

## Advanced Concurrency Patterns

### Reader-Writer Locks

For data that is read frequently but written rarely:

```ras
int lock = @rwlock_create[];

// Multiple readers can hold lock simultaneously
@rwlock_read[lock];
int val = shared_data;
@rwlock_read_unlock[lock];

// Exclusive write lock blocks all readers
@rwlock_write[lock];
shared_data = new_value;
@rwlock_write_unlock[lock];
```

### Barriers (Thread Synchronization Points)

Wait for multiple threads to reach same point:

```ras
int barrier = @barrier_create[4];  // Wait for 4 threads

fnc worker[id]::int {
    show["Thread "];
    show[id];
    show[" at barrier\n"];
    
    @barrier_wait[barrier];  // All threads block here
    
    show["Thread "];
    show[id];
    show[" continuing\n"];
    get[0];
}

// Spawn 4 threads, all will sync at barrier
@spawn[worker, 0];
@spawn[worker, 1];
@spawn[worker, 2];
@spawn[worker, 3];
```

### Event Objects

Signalable synchronization primitive:

```ras
int event = @event_create[];

// Thread 1: Wait for event
@event_wait[event];     // Blocks
show["Event received!\n"];

// Thread 2: Signal event
@event_signal[event];   // Wakes waiting thread
@event_reset[event];    // Reset for next use
```

### Thread Pool

Process multiple tasks with worker threads:

```ras
fnc task[data]::int {
    show["Processing: "];
    show[data];
    show["\n"];
    get[0];
}

int pool = @pool_create[4];      // 4 worker threads

@pool_submit[pool, task, 100];
@pool_submit[pool, task, 200];
@pool_submit[pool, task, 300];

@pool_wait[pool];                 // Wait for all tasks
@pool_destroy[pool];
```

### Channels (Message Passing)

Send data between threads safely:

```ras
int channel = @channel_create[10];  // Buffer 10 messages

// Sender thread
fnc sender[]::int {
    @channel_send[channel, 42];
    @channel_send[channel, 99];
    @channel_close[channel];
    get[0];
}

// Receiver thread
fnc receiver[]::int {
    int msg = @channel_recv[channel];
    show["Got message: "];
    show[msg];
    show["\n"];
    get[0];
}

@spawn[sender, 0];
@spawn[receiver, 0];
```

## Performance Considerations

1. **Lock contention:** More threads = more lock conflicts
2. **Cache coherence:** Shared data in different cache lines faster
3. **Context switching:** Too many threads hurts performance
4. **Lock-free better than locked** when possible

```ras
// Prefer atomic operations
@atomic_increment[counter];

// Over mutex (when possible)
// @mutex_lock[m];
// counter++;
// @mutex_unlock[m];
```

## Builtin Reference

| Builtin | Purpose | Args |
|---------|---------|------|
| @spawn | Create thread | fn, arg |
| @join | Wait thread | tid |
| @pid | Thread ID | none |
| @mutex_* | Mutual exclusion | various |
| @semaphore_* | Semaphore ops | various |
| @cond_* | Condition vars | various |
| @rwlock_* | R-W locks | various |
| @barrier_* | Thread barriers | various |
| @event_* | Events | various |
| @atomic_* | Atomic ops | various |
| @channel_* | Message passing | various |
| @pool_* | Thread pool | various |

## Related Documentation:
- [16-BUILTINS.md](16-BUILTINS.md) — Concurrency builtins reference
- [17-MEMORY-OPERATIONS.md](17-MEMORY-OPERATIONS.md) — Shared memory
- [12-TRY-CATCH.md](12-TRY-CATCH.md) — Error handling in threads
- [11-LOOPS.md](11-LOOPS.md) — Loop patterns
