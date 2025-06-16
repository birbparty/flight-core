#ifndef FLIGHT_HAL_THREAD_HPP
#define FLIGHT_HAL_THREAD_HPP

#include "platform.hpp"
#include "result.hpp"
#include <cstdint>
#include <memory>
#include <functional>

namespace flight
{
    namespace hal
    {

        // Thread priority levels
        enum class ThreadPriority : uint32_t
        {
            Lowest = 0,
            Low = 1,
            Normal = 2,
            High = 3,
            Highest = 4,
            RealTime = 5 // May not be supported on all platforms
        };

        // Thread state
        enum class ThreadState : uint32_t
        {
            Created,
            Running,
            Suspended,
            Waiting,
            Terminated
        };

        // Thread ID type (platform-specific internally)
        using ThreadId = uint64_t;

        // Abstract thread interface
        class Thread
        {
        public:
            virtual ~Thread() = default;

            // Thread control
            virtual void join() = 0;
            virtual bool joinable() const = 0;
            virtual void detach() = 0;

            // Thread properties
            virtual ThreadId get_id() const = 0;
            virtual ThreadState get_state() const = 0;

            // Priority management
            virtual Result<void> set_priority(ThreadPriority priority) = 0;
            virtual Result<ThreadPriority> get_priority() const = 0;

            // Thread naming (for debugging)
            virtual Result<void> set_name(const char *name) = 0;
            virtual Result<std::string> get_name() const = 0;

            // CPU affinity (optional feature)
            virtual Result<void> set_affinity(uint32_t cpu_index) = 0;
            virtual Result<uint32_t> get_affinity() const = 0;
        };

        // Mutex interface
        class Mutex
        {
        public:
            virtual ~Mutex() = default;

            virtual void lock() = 0;
            virtual void unlock() = 0;
            virtual bool try_lock() = 0;
        };

        // RAII lock guard
        template <typename MutexType>
        class LockGuard
        {
        public:
            explicit LockGuard(MutexType &mutex) : mutex_(mutex)
            {
                mutex_.lock();
            }

            ~LockGuard()
            {
                mutex_.unlock();
            }

            // Non-copyable, non-movable
            LockGuard(const LockGuard &) = delete;
            LockGuard &operator=(const LockGuard &) = delete;
            LockGuard(LockGuard &&) = delete;
            LockGuard &operator=(LockGuard &&) = delete;

        private:
            MutexType &mutex_;
        };

        // Condition variable interface
        class ConditionVariable
        {
        public:
            virtual ~ConditionVariable() = default;

            // Wait indefinitely
            virtual void wait(Mutex &mutex) = 0;

            // Wait with timeout (returns true if signaled, false if timed out)
            virtual bool wait_for(Mutex &mutex, uint64_t timeout_ms) = 0;

            // Wake one waiting thread
            virtual void notify_one() = 0;

            // Wake all waiting threads
            virtual void notify_all() = 0;
        };

        // Semaphore interface
        class Semaphore
        {
        public:
            virtual ~Semaphore() = default;

            // Acquire (decrement)
            virtual void acquire() = 0;

            // Try acquire with timeout (returns true if acquired)
            virtual bool try_acquire_for(uint64_t timeout_ms) = 0;

            // Release (increment)
            virtual void release() = 0;

            // Get current count
            virtual uint32_t count() const = 0;
        };

        // Thread factory functions

        // Create a new thread
        // Returns nullptr on platforms without threading support
        std::unique_ptr<Thread> create_thread(std::function<void()> func);

        // Create a mutex
        std::unique_ptr<Mutex> create_mutex();

        // Create a condition variable
        std::unique_ptr<ConditionVariable> create_condition_variable();

        // Create a semaphore
        std::unique_ptr<Semaphore> create_semaphore(uint32_t initial_count);

        // Thread utilities

        // Get current thread ID
        ThreadId get_current_thread_id();

        // Yield execution to other threads
        void yield();

        // Sleep current thread
        void sleep_for_ms(uint64_t milliseconds);

        // Get number of hardware threads (CPU cores)
        uint32_t get_hardware_thread_count();

        // Thread local storage (TLS) support
        template <typename T>
        class ThreadLocal
        {
        public:
            ThreadLocal() = default;
            ~ThreadLocal() = default;

            // Get value for current thread
            T *get();
            const T *get() const;

            // Set value for current thread
            void set(std::unique_ptr<T> value);

            // Reset value for current thread
            void reset();

        private:
            // Platform-specific implementation
            // Will be specialized per platform
            void *impl_;
        };

        // Platform-specific threading capabilities
        namespace thread_capabilities
        {
            constexpr bool is_supported() noexcept
            {
#if FLIGHT_HAS_THREADS
                return true;
#else
                return false;
#endif
            }

            constexpr bool has_priority_support() noexcept
            {
#if defined(FLIGHT_PLATFORM_MACOS) || defined(FLIGHT_PLATFORM_PSP)
                return true;
#else
                return false;
#endif
            }

            constexpr bool has_affinity_support() noexcept
            {
#if defined(FLIGHT_PLATFORM_MACOS)
                return true;
#else
                return false;
#endif
            }

            constexpr bool has_thread_naming() noexcept
            {
#if defined(FLIGHT_PLATFORM_MACOS)
                return true;
#else
                return false;
#endif
            }

            constexpr bool is_cooperative() noexcept
            {
#if defined(FLIGHT_PLATFORM_PSP)
                return true; // PSP uses cooperative threading
#else
                return false;
#endif
            }
        }

        // Helper for platforms without threading
        // Returns a no-op implementation
        namespace no_thread
        {
            class NoOpThread : public Thread
            {
            public:
                void join() override {}
                bool joinable() const override { return false; }
                void detach() override {}
                ThreadId get_id() const override { return 0; }
                ThreadState get_state() const override { return ThreadState::Terminated; }
                Result<void> set_priority(ThreadPriority) override { return ErrorCode::NotSupported; }
                Result<ThreadPriority> get_priority() const override { return ErrorCode::NotSupported; }
                Result<void> set_name(const char *) override { return ErrorCode::NotSupported; }
                Result<std::string> get_name() const override { return ErrorCode::NotSupported; }
                Result<void> set_affinity(uint32_t) override { return ErrorCode::NotSupported; }
                Result<uint32_t> get_affinity() const override { return ErrorCode::NotSupported; }
            };

            class NoOpMutex : public Mutex
            {
            public:
                void lock() override {}
                void unlock() override {}
                bool try_lock() override { return true; }
            };
        }

    } // namespace hal
} // namespace flight

#endif // FLIGHT_HAL_THREAD_HPP
