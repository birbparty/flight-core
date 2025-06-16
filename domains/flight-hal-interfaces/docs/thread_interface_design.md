# Flight HAL Threading Interface Design

## Overview

The Flight HAL Threading Interface provides a unified abstraction for threading and concurrency across platforms ranging from single-core retro systems (Dreamcast) to modern multi-core desktop environments. The interface supports graceful degradation, platform-adaptive execution models, and comprehensive synchronization primitives.

## Design Principles

### 1. Graceful Degradation
- **Single-threaded fallback**: All threading operations degrade gracefully to single-threaded execution on platforms without threading support
- **Cooperative scheduling**: PSP-like systems use cooperative threading with manual yield points
- **Adaptive work queues**: Automatically select appropriate execution strategy based on platform capabilities

### 2. Platform-Adaptive Architecture
- **Compile-time detection**: Uses platform detection macros for static capability determination
- **Runtime adaptation**: Dynamic capability probing for optimal configuration
- **Zero-overhead abstraction**: No runtime penalty on single-threaded platforms

### 3. Zero-Allocation Design
- **Stack-based synchronization**: All primitives use stack allocation where possible
- **Fixed-size work queues**: Bounded queue sizes for predictable memory usage
- **Memory pool integration**: Leverages HAL memory interface for efficient allocation

## Threading Models

### SingleThreaded (Dreamcast)
```cpp
ThreadingModel::SingleThreaded
- Max threads: 1
- Work queue mode: Immediate execution
- Synchronization: No-op implementations
- Cooperative scheduling: Manual process_pending() calls
```

### Cooperative (PSP)
```cpp
ThreadingModel::Cooperative
- Max threads: 4 (dual-core with limited threading)
- Work queue mode: Deferred execution
- Synchronization: Cooperative primitives with yield points
- Thread priorities: Basic support
```

### WebWorkers (Web Platforms)
```cpp
ThreadingModel::WebWorkers
- Max threads: 8 (reasonable browser limit)
- Work queue mode: Sequential/Parallel hybrid
- Synchronization: Limited (Mutex, BinarySemaphore only)
- Thread isolation: Full Web Worker isolation
```

### Preemptive (Desktop)
```cpp
ThreadingModel::Preemptive
- Max threads: Hardware concurrency
- Work queue mode: Full parallel execution
- Synchronization: Complete primitive set
- Thread priorities: Full OS-level support
```

## Core Interfaces

### IThreadInterface
Primary interface providing:
- Thread creation and management
- Synchronization primitive factories
- Work queue creation
- Platform capability queries
- Cooperative scheduling support

### IThread
Individual thread management:
- Start/join/detach operations
- Priority control
- State monitoring
- Performance statistics
- Completion callbacks

### Synchronization Primitives

#### IMutex
```cpp
- lock() / unlock()
- try_lock() / try_lock_for()
- Recursive mutex support
- Deadlock detection (debug builds)
```

#### ISemaphore
```cpp
- acquire() / release()
- Counting and binary variants
- Timeout support
- Resource counting
```

#### IConditionVariable
```cpp
- wait() / notify_one() / notify_all()
- Predicate-based waiting
- Spurious wakeup handling
```

### Work Queue System

#### IWorkQueue
Adaptive task scheduling:
- Priority-based work submission
- Dependency tracking
- Completion callbacks
- Performance metrics
- Platform-adaptive execution modes

#### IWorkItem
Individual work units:
- Execute() method for work execution
- Priority and dependency metadata
- Parallelization hints
- Execution time estimation

#### IThreadPool
High-level thread management:
- Dynamic thread pool sizing
- Work distribution
- Load balancing
- Thread monitoring

## Platform-Specific Optimizations

### Dreamcast (SH-4)
- **Cache-aware scheduling**: Work items scheduled to maximize cache locality
- **DMA coordination**: Integration with DMA transfers for audio/graphics
- **Interrupt handling**: Cooperative scheduling around hardware interrupts
- **Memory constraints**: 16MB RAM optimization strategies

### PSP (MIPS)
- **Dual-core utilization**: Media Engine and main CPU coordination
- **Power management**: Thread scheduling considering battery life
- **UMD access patterns**: Cooperative scheduling around disc I/O
- **32MB memory optimization**: Efficient stack and heap management

### Web Platforms
- **Main thread protection**: Avoid blocking main thread with heavy work
- **Worker communication**: Efficient message passing between workers
- **Memory transfer**: SharedArrayBuffer utilization where available
- **Browser compatibility**: Fallback strategies for older browsers

### Desktop Systems
- **NUMA awareness**: Thread affinity for multi-socket systems
- **CPU topology**: Scheduling based on CPU cache hierarchy
- **Power scaling**: Integration with OS power management
- **Real-time priorities**: Support for real-time thread scheduling

## Synchronization Strategies

### Single-Threaded
```cpp
// All synchronization operations are no-ops
mutex->lock();     // Returns immediately
semaphore->wait(); // Returns immediately
```

### Cooperative
```cpp
// Synchronization with yield points
while (!try_acquire_resource()) {
    thread_interface->yield_current_thread();
}
```

### Preemptive
```cpp
// Full OS-level synchronization
std::unique_lock<std::mutex> lock(mutex);
condition.wait(lock, [&] { return resource_available; });
```

## Work Queue Execution Modes

### Immediate Mode (Single-threaded)
- Work executed immediately in calling thread
- Zero queuing overhead
- Deterministic execution order

### Deferred Mode (Cooperative)
- Work queued for later execution
- Manual processing via process_pending()
- Cooperative scheduling integration

### Parallel Mode (Preemptive)
- Work distributed across thread pool
- Automatic load balancing
- Priority-based scheduling

### Adaptive Mode
- Automatically selects best mode for platform
- Runtime capability detection
- Performance optimization

## Performance Characteristics

### Memory Overhead
```cpp
// Per-thread overhead by platform
Dreamcast:    0 bytes (no threading)
PSP:          512 bytes (minimal cooperative state)
Web:          8KB (Web Worker overhead)
Desktop:      2KB (OS thread control block)
```

### Context Switch Performance
```cpp
// Context switch overhead (nanoseconds)
Dreamcast:    0 (no context switches)
PSP:          1000 (cooperative yield)
Web:          Variable (browser dependent)
Desktop:      500-2000 (OS scheduler dependent)
```

### Synchronization Overhead
```cpp
// Synchronization operation overhead (nanoseconds)
Dreamcast:    0 (no-op)
PSP:          100 (cooperative check)
Web:          1000-5000 (worker communication)
Desktop:      50-200 (kernel synchronization)
```

## Usage Examples

### Basic Threading
```cpp
auto thread_interface = registry.get_interface<IThreadInterface>();

// Create thread with platform-appropriate configuration
auto config = ThreadConfig::create_default("WorkerThread");
auto thread_result = thread_interface->create_thread(config);

if (thread_result.is_success()) {
    auto thread = std::move(thread_result.value());
    thread->start([]() {
        // Thread work here
    });
    thread->join();
}
```

### Work Queue Usage
```cpp
// Create adaptive work queue
auto queue_result = thread_interface->create_work_queue(
    WorkQueueMode::Adaptive, 100, "MainWorkQueue");

auto queue = std::move(queue_result.value());

// Submit work
auto work_handle = queue->submit([]() {
    // Work function
}, WorkPriority::Normal, "ExampleWork");

// Process work (required for single-threaded/cooperative)
if (thread_interface->get_threading_model() != ThreadingModel::Preemptive) {
    queue->process_pending();
}
```

### Synchronization
```cpp
// Create mutex (fails gracefully on single-threaded)
auto mutex_result = thread_interface->create_mutex("ResourceMutex");

if (mutex_result.is_success()) {
    auto mutex = std::move(mutex_result.value());
    
    // Critical section
    mutex->lock();
    // Protected operations
    mutex->unlock();
}
```

### Cooperative Scheduling
```cpp
// For platforms supporting cooperative scheduling
void long_running_task() {
    for (int i = 0; i < large_number; ++i) {
        // Do work
        process_item(i);
        
        // Yield periodically for cooperative platforms
        if (i % 100 == 0) {
            thread_interface->cooperative_tick();
        }
    }
}
```

## Error Handling

### Graceful Failures
- Thread creation failures fall back to single-threaded execution
- Synchronization primitive creation returns appropriate errors
- Work queue overflow handled with backpressure

### Platform Limitations
```cpp
// Handle platform-specific limitations
auto thread_result = thread_interface->create_thread(config);
if (thread_result.is_error()) {
    if (thread_result.error().code() == HALErrorCode::NotSupported) {
        // Fall back to single-threaded execution
        execute_work_directly();
    }
}
```

## Testing Strategy

### Mock Implementation
- Comprehensive mock driver supporting all threading models
- Platform simulation for testing fallback behaviors
- Performance characteristic simulation
- Deterministic behavior for unit testing

### Cross-Platform Validation
- Automated testing across all target platforms
- Performance regression testing
- Memory usage validation
- Deadlock detection and prevention testing

## Future Extensions

### Planned Features
- **Lock-free collections**: High-performance concurrent data structures
- **Work stealing**: Advanced work distribution algorithms
- **Thread-local storage**: Platform-adaptive TLS support
- **Fiber support**: User-space threading for high-concurrency scenarios

### Advanced Synchronization
- **Read-write locks**: Shared/exclusive access patterns
- **Barriers**: Multi-thread synchronization points
- **Events**: Windows-style event objects
- **Futex**: Linux-style fast userspace mutexes

This comprehensive threading interface design provides a robust foundation for cross-platform concurrency while maintaining the Flight HAL principles of zero-allocation, graceful degradation, and platform optimization.
