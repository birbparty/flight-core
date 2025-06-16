#ifndef FLIGHT_HAL_THREAD_DRIVER_HPP
#define FLIGHT_HAL_THREAD_DRIVER_HPP

#include "driver.hpp"
#include "result.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <chrono>

namespace flight
{
    namespace hal
    {

        // Thread handle
        struct ThreadHandle
        {
            uint32_t id;
            bool operator==(const ThreadHandle &other) const { return id == other.id; }
            bool operator!=(const ThreadHandle &other) const { return id != other.id; }
        };

        constexpr ThreadHandle INVALID_THREAD_HANDLE = {0};

        // Mutex handle
        struct MutexHandle
        {
            uint32_t id;
            bool operator==(const MutexHandle &other) const { return id == other.id; }
            bool operator!=(const MutexHandle &other) const { return id != other.id; }
        };

        constexpr MutexHandle INVALID_MUTEX_HANDLE = {0};

        // Condition variable handle
        struct CondVarHandle
        {
            uint32_t id;
            bool operator==(const CondVarHandle &other) const { return id == other.id; }
            bool operator!=(const CondVarHandle &other) const { return id != other.id; }
        };

        constexpr CondVarHandle INVALID_CONDVAR_HANDLE = {0};

        // Thread priority levels
        enum class ThreadPriority : uint32_t
        {
            Lowest,
            BelowNormal,
            Normal,
            AboveNormal,
            Highest,
            TimeCritical
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

        // Thread function type
        using ThreadFunction = std::function<void()>;

        // Thread creation options
        struct ThreadOptions
        {
            const char *name;        // Thread name for debugging
            size_t stack_size;       // 0 = default
            ThreadPriority priority; // Thread priority
            uint32_t cpu_affinity;   // CPU core mask (0 = any core)
        };

        // Threading capabilities
        struct ThreadingCapabilities
        {
            bool has_threading;        // Platform supports threading at all
            bool has_preemptive;       // True preemptive multitasking
            bool has_priorities;       // Thread priorities supported
            bool has_affinity;         // CPU affinity supported
            bool has_tls;              // Thread-local storage
            bool has_atomics;          // Atomic operations
            bool has_barriers;         // Memory barriers
            uint32_t hardware_threads; // Number of hardware threads
            uint32_t max_threads;      // Maximum software threads
            size_t default_stack_size; // Default thread stack size
            size_t min_stack_size;     // Minimum thread stack size
        };

        // Thread driver interface (RetroArch pattern)
        class ThreadDriver : public Driver
        {
        public:
            // Driver interface
            DriverType type() const override { return DriverType::Thread; }

            // Capability queries
            virtual ThreadingCapabilities capabilities() const = 0;
            virtual bool is_threading_available() const = 0;

            // Thread management
            virtual Result<ThreadHandle> create_thread(
                ThreadFunction func,
                const ThreadOptions &options = {}) = 0;

            virtual Result<void> join_thread(ThreadHandle handle) = 0;
            virtual Result<void> detach_thread(ThreadHandle handle) = 0;
            virtual Result<bool> is_thread_joinable(ThreadHandle handle) const = 0;

            // Thread control
            virtual Result<void> set_thread_priority(ThreadHandle handle, ThreadPriority priority) = 0;
            virtual Result<ThreadPriority> get_thread_priority(ThreadHandle handle) const = 0;
            virtual Result<void> set_thread_affinity(ThreadHandle handle, uint32_t cpu_mask) = 0;
            virtual Result<void> yield_thread() = 0;

            // Thread identification
            virtual ThreadHandle get_current_thread() const = 0;
            virtual uint32_t get_thread_id(ThreadHandle handle) const = 0;
            virtual Result<const char *> get_thread_name(ThreadHandle handle) const = 0;
            virtual Result<void> set_thread_name(ThreadHandle handle, const char *name) = 0;

            // Thread state
            virtual Result<ThreadState> get_thread_state(ThreadHandle handle) const = 0;

            // Sleep functions
            virtual void sleep_for(std::chrono::milliseconds duration) = 0;
            virtual void sleep_until(std::chrono::steady_clock::time_point time_point) = 0;

            // Mutex operations
            virtual Result<MutexHandle> create_mutex(bool recursive = false) = 0;
            virtual Result<void> destroy_mutex(MutexHandle handle) = 0;
            virtual Result<void> lock_mutex(MutexHandle handle) = 0;
            virtual Result<bool> try_lock_mutex(MutexHandle handle) = 0;
            virtual Result<bool> try_lock_mutex_for(MutexHandle handle, std::chrono::milliseconds timeout) = 0;
            virtual Result<void> unlock_mutex(MutexHandle handle) = 0;

            // Condition variable operations
            virtual Result<CondVarHandle> create_condition_variable() = 0;
            virtual Result<void> destroy_condition_variable(CondVarHandle handle) = 0;
            virtual Result<void> wait_condition_variable(CondVarHandle cv, MutexHandle mutex) = 0;
            virtual Result<bool> wait_condition_variable_for(
                CondVarHandle cv,
                MutexHandle mutex,
                std::chrono::milliseconds timeout) = 0;
            virtual Result<void> notify_one_condition_variable(CondVarHandle handle) = 0;
            virtual Result<void> notify_all_condition_variable(CondVarHandle handle) = 0;

            // Platform-specific features

            // Thread-local storage (if supported)
            virtual Result<uint32_t> create_tls_key() { return ErrorCode::NotSupported; }
            virtual Result<void> destroy_tls_key(uint32_t key) { return ErrorCode::NotSupported; }
            virtual Result<void> set_tls_value(uint32_t key, void *value) { return ErrorCode::NotSupported; }
            virtual Result<void *> get_tls_value(uint32_t key) { return ErrorCode::NotSupported; }

            // PSP cooperative threading
            virtual Result<void> change_thread_priority(ThreadHandle handle, int delta) { return ErrorCode::NotSupported; }
            virtual Result<void> rotate_thread_ready_queue(ThreadPriority priority) { return ErrorCode::NotSupported; }

            // Debug/profiling
            virtual uint32_t get_thread_count() const = 0;
            virtual Result<uint64_t> get_thread_cpu_time(ThreadHandle handle) const = 0;
        };

        // RAII wrappers for easier use

        // Scoped lock
        class ScopedLock
        {
        private:
            ThreadDriver *driver_;
            MutexHandle mutex_;
            bool locked_;

        public:
            ScopedLock(ThreadDriver *driver, MutexHandle mutex)
                : driver_(driver), mutex_(mutex), locked_(false)
            {
                if (driver_ && mutex_ != INVALID_MUTEX_HANDLE)
                {
                    auto result = driver_->lock_mutex(mutex_);
                    locked_ = result.is_ok();
                }
            }

            ~ScopedLock()
            {
                if (locked_ && driver_)
                {
                    driver_->unlock_mutex(mutex_);
                }
            }

            // Non-copyable, movable
            ScopedLock(const ScopedLock &) = delete;
            ScopedLock &operator=(const ScopedLock &) = delete;
            ScopedLock(ScopedLock &&other) noexcept
                : driver_(other.driver_), mutex_(other.mutex_), locked_(other.locked_)
            {
                other.locked_ = false;
            }
            ScopedLock &operator=(ScopedLock &&other) noexcept
            {
                if (this != &other)
                {
                    if (locked_ && driver_)
                    {
                        driver_->unlock_mutex(mutex_);
                    }
                    driver_ = other.driver_;
                    mutex_ = other.mutex_;
                    locked_ = other.locked_;
                    other.locked_ = false;
                }
                return *this;
            }

            bool is_locked() const { return locked_; }
        };

        // Platform-specific threading defaults
        namespace threading_defaults
        {
            // Desktop defaults (full threading)
            constexpr ThreadingCapabilities DESKTOP_CAPABILITIES = {
                .has_threading = true,
                .has_preemptive = true,
                .has_priorities = true,
                .has_affinity = true,
                .has_tls = true,
                .has_atomics = true,
                .has_barriers = true,
                .hardware_threads = 8, // Typical modern CPU
                .max_threads = 1024,
                .default_stack_size = 1024 * 1024, // 1MB
                .min_stack_size = 16 * 1024        // 16KB
            };

            // PSP defaults (cooperative threading)
            constexpr ThreadingCapabilities PSP_CAPABILITIES = {
                .has_threading = true,
                .has_preemptive = false, // Cooperative only
                .has_priorities = true,
                .has_affinity = false,
                .has_tls = true,
                .has_atomics = true,
                .has_barriers = true,
                .hardware_threads = 1,
                .max_threads = 64,
                .default_stack_size = 64 * 1024, // 64KB
                .min_stack_size = 4 * 1024       // 4KB
            };

            // Dreamcast defaults (no threading)
            constexpr ThreadingCapabilities DREAMCAST_CAPABILITIES = {
                .has_threading = false,
                .has_preemptive = false,
                .has_priorities = false,
                .has_affinity = false,
                .has_tls = false,
                .has_atomics = true, // Basic atomics
                .has_barriers = true,
                .hardware_threads = 1,
                .max_threads = 1,
                .default_stack_size = 0,
                .min_stack_size = 0};

            // PlayStation 1 defaults (no threading)
            constexpr ThreadingCapabilities PSX_CAPABILITIES = {
                .has_threading = false,
                .has_preemptive = false,
                .has_priorities = false,
                .has_affinity = false,
                .has_tls = false,
                .has_atomics = false,
                .has_barriers = false,
                .hardware_threads = 1,
                .max_threads = 1,
                .default_stack_size = 0,
                .min_stack_size = 0};

            // Web/Emscripten defaults (Web Workers)
            constexpr ThreadingCapabilities WEB_CAPABILITIES = {
                .has_threading = true,
                .has_preemptive = true,
                .has_priorities = false,
                .has_affinity = false,
                .has_tls = true,
                .has_atomics = true,
                .has_barriers = true,
                .hardware_threads = 4, // Browser dependent
                .max_threads = 256,
                .default_stack_size = 512 * 1024, // 512KB
                .min_stack_size = 64 * 1024       // 64KB
            };
        }

        // Helper to check if threading is available at compile time
        constexpr bool has_threading_support()
        {
#if defined(FLIGHT_PLATFORM_DREAMCAST) || defined(FLIGHT_PLATFORM_PSX)
            return false;
#else
            return true;
#endif
        }

    } // namespace hal
} // namespace flight

#endif // FLIGHT_HAL_THREAD_DRIVER_HPP
