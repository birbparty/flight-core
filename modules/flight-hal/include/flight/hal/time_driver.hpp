#ifndef FLIGHT_HAL_TIME_DRIVER_HPP
#define FLIGHT_HAL_TIME_DRIVER_HPP

#include "driver.hpp"
#include "result.hpp"
#include <cstdint>
#include <chrono>
#include <string>

namespace flight
{
    namespace hal
    {

        // Time capabilities
        struct TimeCapabilities
        {
            bool has_monotonic_clock;     // Clock that never goes backwards
            bool has_high_resolution;     // Sub-millisecond precision
            bool has_performance_counter; // Hardware performance counter
            bool has_real_time_clock;     // Battery-backed RTC
            bool can_set_system_time;     // Can modify system time
            uint64_t resolution_ns;       // Clock resolution in nanoseconds
            uint64_t perf_counter_freq;   // Performance counter frequency
            uint32_t sleep_precision_ms;  // Sleep precision in milliseconds
        };

        // Time point using steady clock
        using TimePoint = std::chrono::steady_clock::time_point;
        using SystemTimePoint = std::chrono::system_clock::time_point;

        // Duration types
        using Nanoseconds = std::chrono::nanoseconds;
        using Microseconds = std::chrono::microseconds;
        using Milliseconds = std::chrono::milliseconds;
        using Seconds = std::chrono::seconds;

        // System time information
        struct SystemTimeInfo
        {
            int64_t unix_timestamp;          // Seconds since Unix epoch
            uint32_t milliseconds;           // Milliseconds within second
            int32_t timezone_offset_minutes; // Offset from UTC
            bool is_dst;                     // Daylight saving time active
            uint64_t uptime_ms;              // System uptime in milliseconds
        };

        // Date/time components
        struct DateTime
        {
            uint16_t year;
            uint8_t month;        // 1-12
            uint8_t day;          // 1-31
            uint8_t hour;         // 0-23
            uint8_t minute;       // 0-59
            uint8_t second;       // 0-59
            uint16_t millisecond; // 0-999
            int8_t day_of_week;   // 0-6 (Sunday = 0)
        };

        // Time driver interface (RetroArch pattern)
        class TimeDriver : public Driver
        {
        public:
            // Driver interface
            DriverType type() const override { return DriverType::Time; }

            // Capability queries
            virtual TimeCapabilities capabilities() const = 0;

            // Monotonic time (for measuring intervals)
            virtual TimePoint now() const = 0;
            virtual uint64_t monotonic_ms() const = 0;
            virtual uint64_t monotonic_us() const = 0;

            // System time (wall clock)
            virtual SystemTimePoint system_now() const = 0;
            virtual Result<SystemTimeInfo> get_system_time() const = 0;
            virtual Result<DateTime> get_date_time() const = 0;

            // Sleep functions
            virtual void sleep_ms(uint32_t milliseconds) = 0;
            virtual void sleep_us(uint32_t microseconds) = 0;
            virtual void sleep_until(TimePoint time_point) = 0;

            // High-resolution performance counter
            virtual uint64_t performance_counter() const = 0;
            virtual uint64_t performance_frequency() const = 0;

            // Timer utilities
            virtual Result<uint64_t> get_uptime_ms() const = 0;
            virtual Result<uint64_t> get_process_time_ms() const = 0;
            virtual Result<uint64_t> get_thread_time_ms() const = 0;

            // Platform-specific operations

            // Set system time (if supported)
            virtual Result<void> set_system_time(const SystemTimeInfo &time)
            {
                return ErrorCode::NotSupported;
            }

            // Get timezone information
            virtual Result<std::string> get_timezone_name() const
            {
                return ErrorCode::NotSupported;
            }

            // RTC operations (for platforms with battery-backed RTC)
            virtual Result<DateTime> read_rtc() const
            {
                return ErrorCode::NotSupported;
            }

            virtual Result<void> write_rtc(const DateTime &dt)
            {
                return ErrorCode::NotSupported;
            }

            // Platform-specific timing adjustments
            virtual void calibrate_timers() {}
        };

        // Helper functions for time conversions
        inline uint64_t performance_counter_to_ns(uint64_t counter, uint64_t frequency)
        {
            if (frequency == 0)
                return 0;
            return (counter * 1000000000ULL) / frequency;
        }

        inline uint64_t ns_to_performance_counter(uint64_t ns, uint64_t frequency)
        {
            if (frequency == 0)
                return 0;
            return (ns * frequency) / 1000000000ULL;
        }

        // Timer class for measuring elapsed time
        class Timer
        {
        private:
            TimeDriver *driver_;
            TimePoint start_time_;

        public:
            explicit Timer(TimeDriver *driver)
                : driver_(driver), start_time_(driver->now()) {}

            void reset()
            {
                start_time_ = driver_->now();
            }

            template <typename Duration = Milliseconds>
            typename Duration::rep elapsed() const
            {
                auto elapsed_time = driver_->now() - start_time_;
                return std::chrono::duration_cast<Duration>(elapsed_time).count();
            }

            double elapsed_seconds() const
            {
                auto elapsed_time = driver_->now() - start_time_;
                return std::chrono::duration<double>(elapsed_time).count();
            }

            uint64_t elapsed_ms() const
            {
                return elapsed<Milliseconds>();
            }

            uint64_t elapsed_us() const
            {
                return elapsed<Microseconds>();
            }
        };

        // High-resolution timer using performance counters
        class HighResTimer
        {
        private:
            TimeDriver *driver_;
            uint64_t start_counter_;

        public:
            explicit HighResTimer(TimeDriver *driver)
                : driver_(driver), start_counter_(driver->performance_counter()) {}

            void reset()
            {
                start_counter_ = driver_->performance_counter();
            }

            uint64_t elapsed_ns() const
            {
                uint64_t current = driver_->performance_counter();
                uint64_t elapsed = current - start_counter_;
                return performance_counter_to_ns(elapsed, driver_->performance_frequency());
            }

            uint64_t elapsed_us() const
            {
                return elapsed_ns() / 1000;
            }

            uint64_t elapsed_ms() const
            {
                return elapsed_ns() / 1000000;
            }

            double elapsed_seconds() const
            {
                return static_cast<double>(elapsed_ns()) / 1000000000.0;
            }
        };

        // Scoped timer - measures time until destruction
        template <typename Callback>
        class ScopedTimer
        {
        private:
            Callback callback_;
            Timer timer_;

        public:
            ScopedTimer(TimeDriver *driver, Callback callback)
                : callback_(std::move(callback)), timer_(driver) {}

            ~ScopedTimer()
            {
                callback_(timer_.elapsed_seconds());
            }

            // Non-copyable, non-movable
            ScopedTimer(const ScopedTimer &) = delete;
            ScopedTimer &operator=(const ScopedTimer &) = delete;
            ScopedTimer(ScopedTimer &&) = delete;
            ScopedTimer &operator=(ScopedTimer &&) = delete;
        };

        // Platform-specific time defaults
        namespace time_defaults
        {
            // Desktop defaults (high precision)
            constexpr TimeCapabilities DESKTOP_CAPABILITIES = {
                .has_monotonic_clock = true,
                .has_high_resolution = true,
                .has_performance_counter = true,
                .has_real_time_clock = true,
                .can_set_system_time = true,
                .resolution_ns = 1,              // 1ns resolution
                .perf_counter_freq = 1000000000, // 1GHz typical
                .sleep_precision_ms = 1          // 1ms sleep precision
            };

            // Dreamcast defaults (RTC, millisecond precision)
            constexpr TimeCapabilities DREAMCAST_CAPABILITIES = {
                .has_monotonic_clock = false, // Clock can be changed
                .has_high_resolution = false,
                .has_performance_counter = true,
                .has_real_time_clock = true,
                .can_set_system_time = true,
                .resolution_ns = 1000000,    // 1ms resolution
                .perf_counter_freq = 100000, // 100KHz timer
                .sleep_precision_ms = 10     // 10ms sleep precision
            };

            // PlayStation 1 defaults (basic timing)
            constexpr TimeCapabilities PSX_CAPABILITIES = {
                .has_monotonic_clock = true,
                .has_high_resolution = false,
                .has_performance_counter = true,
                .has_real_time_clock = false, // No RTC
                .can_set_system_time = false,
                .resolution_ns = 1000000,      // 1ms resolution
                .perf_counter_freq = 33868800, // GPU clock
                .sleep_precision_ms = 16       // ~16ms (1 frame)
            };

            // PSP defaults (good timing support)
            constexpr TimeCapabilities PSP_CAPABILITIES = {
                .has_monotonic_clock = true,
                .has_high_resolution = true,
                .has_performance_counter = true,
                .has_real_time_clock = true,
                .can_set_system_time = false,
                .resolution_ns = 1000,          // 1μs resolution
                .perf_counter_freq = 333000000, // 333MHz CPU
                .sleep_precision_ms = 1         // 1ms sleep precision
            };

            // Web/Emscripten defaults (browser dependent)
            constexpr TimeCapabilities WEB_CAPABILITIES = {
                .has_monotonic_clock = true,
                .has_high_resolution = true,
                .has_performance_counter = true,
                .has_real_time_clock = true,
                .can_set_system_time = false,
                .resolution_ns = 1000,        // 1μs (performance.now())
                .perf_counter_freq = 1000000, // μs precision
                .sleep_precision_ms = 4       // Browser throttling
            };
        }

        // Time formatting utilities

        // Format duration as human-readable string
        inline std::string format_duration(uint64_t milliseconds)
        {
            uint64_t ms = milliseconds % 1000;
            uint64_t seconds = (milliseconds / 1000) % 60;
            uint64_t minutes = (milliseconds / 60000) % 60;
            uint64_t hours = milliseconds / 3600000;

            std::string result;
            if (hours > 0)
                result += std::to_string(hours) + "h ";
            if (minutes > 0)
                result += std::to_string(minutes) + "m ";
            if (seconds > 0)
                result += std::to_string(seconds) + "s ";
            if (ms > 0 || result.empty())
                result += std::to_string(ms) + "ms";

            // Remove trailing space if any
            if (!result.empty() && result.back() == ' ')
                result.pop_back();

            return result;
        }

    } // namespace hal
} // namespace flight

#endif // FLIGHT_HAL_TIME_DRIVER_HPP
