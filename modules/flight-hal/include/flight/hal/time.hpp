#ifndef FLIGHT_HAL_TIME_HPP
#define FLIGHT_HAL_TIME_HPP

#include "platform.hpp"
#include "result.hpp"
#include <cstdint>
#include <chrono>
#include <string>
#include <vector>

namespace flight
{
    namespace hal
    {

        // Time point type using steady clock
        using TimePoint = std::chrono::steady_clock::time_point;

        // Duration types
        using Nanoseconds = std::chrono::nanoseconds;
        using Microseconds = std::chrono::microseconds;
        using Milliseconds = std::chrono::milliseconds;
        using Seconds = std::chrono::seconds;

        // Clock interface for custom implementations
        class Clock
        {
        public:
            virtual ~Clock() = default;

            // Get current time point
            virtual TimePoint now() const = 0;

            // Check if clock is steady (monotonic)
            virtual bool is_steady() const = 0;

            // Get clock resolution in nanoseconds
            virtual uint64_t resolution_ns() const = 0;
        };

        // Time functions

        // Get current time point from system steady clock
        TimePoint now();

        // Sleep for specified duration
        void sleep_for(const Nanoseconds &duration);

        // Sleep until specific time point
        void sleep_until(const TimePoint &time_point);

        // High-resolution performance counter
        // These are platform-specific and may have different epochs
        uint64_t get_performance_counter();
        uint64_t get_performance_frequency();

        // Convert performance counter to nanoseconds
        inline uint64_t performance_counter_to_ns(uint64_t counter)
        {
            uint64_t freq = get_performance_frequency();
            if (freq == 0)
                return 0;
            return (counter * 1000000000ULL) / freq;
        }

        // Convert nanoseconds to performance counter
        inline uint64_t ns_to_performance_counter(uint64_t ns)
        {
            uint64_t freq = get_performance_frequency();
            if (freq == 0)
                return 0;
            return (ns * freq) / 1000000000ULL;
        }

        // Timer class for measuring elapsed time
        class Timer
        {
        public:
            Timer() : start_time_(now()) {}

            // Reset the timer
            void reset()
            {
                start_time_ = now();
            }

            // Get elapsed time since construction or last reset
            template <typename Duration = Milliseconds>
            typename Duration::rep elapsed() const
            {
                auto elapsed_time = now() - start_time_;
                return std::chrono::duration_cast<Duration>(elapsed_time).count();
            }

            // Get elapsed time as floating point seconds
            double elapsed_seconds() const
            {
                auto elapsed_time = now() - start_time_;
                return std::chrono::duration<double>(elapsed_time).count();
            }

        private:
            TimePoint start_time_;
        };

        // High-resolution timer using performance counters
        class HighResTimer
        {
        public:
            HighResTimer() : start_counter_(get_performance_counter()) {}

            // Reset the timer
            void reset()
            {
                start_counter_ = get_performance_counter();
            }

            // Get elapsed nanoseconds
            uint64_t elapsed_ns() const
            {
                uint64_t current = get_performance_counter();
                uint64_t elapsed = current - start_counter_;
                return performance_counter_to_ns(elapsed);
            }

            // Get elapsed microseconds
            uint64_t elapsed_us() const
            {
                return elapsed_ns() / 1000;
            }

            // Get elapsed milliseconds
            uint64_t elapsed_ms() const
            {
                return elapsed_ns() / 1000000;
            }

            // Get elapsed seconds as floating point
            double elapsed_seconds() const
            {
                return static_cast<double>(elapsed_ns()) / 1000000000.0;
            }

        private:
            uint64_t start_counter_;
        };

        // Scoped timer - measures time until destruction
        template <typename Callback>
        class ScopedTimer
        {
        public:
            explicit ScopedTimer(Callback callback)
                : callback_(std::move(callback)), timer_() {}

            ~ScopedTimer()
            {
                callback_(timer_.elapsed_seconds());
            }

            // Non-copyable, non-movable
            ScopedTimer(const ScopedTimer &) = delete;
            ScopedTimer &operator=(const ScopedTimer &) = delete;
            ScopedTimer(ScopedTimer &&) = delete;
            ScopedTimer &operator=(ScopedTimer &&) = delete;

        private:
            Callback callback_;
            Timer timer_;
        };

        // Helper to create scoped timer
        template <typename Callback>
        ScopedTimer<Callback> make_scoped_timer(Callback callback)
        {
            return ScopedTimer<Callback>(std::move(callback));
        }

        // Platform-specific time information
        struct TimeInfo
        {
            // System uptime in seconds (if available)
            uint64_t uptime_seconds;

            // Unix timestamp (seconds since epoch)
            int64_t unix_timestamp;

            // Timezone offset from UTC in minutes
            int32_t timezone_offset_minutes;

            // Is daylight saving time active
            bool is_dst;

            // Clock resolution in nanoseconds
            uint64_t clock_resolution_ns;

            // Performance counter frequency
            uint64_t perf_counter_frequency;
        };

        // Get system time information
        TimeInfo get_time_info();

        // Time formatting utilities

        // Format duration as human-readable string
        // Example: 1h 23m 45s 678ms
        std::string format_duration(const Nanoseconds &duration);

        // Parse duration from string
        // Supports: 1h30m, 45s, 1000ms, etc.
        Result<Nanoseconds> parse_duration(const char *str);

        // Platform time capabilities
        namespace time_capabilities
        {
            // Check if high-resolution timers are available
            constexpr bool has_high_resolution() noexcept
            {
#if defined(FLIGHT_PLATFORM_MACOS) || defined(FLIGHT_PLATFORM_EMSCRIPTEN)
                return true;
#else
                return false;
#endif
            }

            // Check if system has monotonic clock
            constexpr bool has_monotonic_clock() noexcept
            {
#if defined(FLIGHT_PLATFORM_DREAMCAST)
                return false; // Dreamcast clock can be changed
#else
                return true;
#endif
            }

            // Get expected timer resolution in nanoseconds
            constexpr uint64_t expected_resolution_ns() noexcept
            {
#if defined(FLIGHT_PLATFORM_MACOS)
                return 1; // 1ns resolution on macOS
#elif defined(FLIGHT_PLATFORM_DREAMCAST) || defined(FLIGHT_PLATFORM_PSP)
                return 1000000; // 1ms resolution on embedded
#else
                return 1000; // 1μs default
#endif
            }
        }

    } // namespace hal
} // namespace flight

#endif // FLIGHT_HAL_TIME_HPP
