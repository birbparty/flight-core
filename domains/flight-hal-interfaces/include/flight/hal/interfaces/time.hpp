/**
 * @file time.hpp
 * @brief Flight HAL Elite Timing and Clock Interface
 * 
 * High-precision timing system providing microsecond resolution timestamps,
 * performance counters, frame timing utilities, and sleep functions across
 * platforms from Dreamcast to modern systems with monotonic clock guarantees.
 */

#pragma once

#include "../core/driver_registry.hpp"
#include "../core/hal_result.hpp"
#include "../core/hal_capabilities.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>
#include <string_view>
#include <chrono>

namespace flight::hal {

/**
 * @brief Clock type classification for different timing use cases
 */
enum class ClockType : uint8_t {
    Monotonic = 0,      ///< Always increasing, system-independent (preferred)
    Realtime,           ///< Wall clock time, can jump backwards
    Process,            ///< Process execution time
    Thread,             ///< Thread execution time
    HighResolution,     ///< Platform's highest resolution clock
    GameTimer,          ///< Game-specific timer (paused during game pause)
    ProfileTimer        ///< High-precision profiling timer
};

/**
 * @brief Time precision levels available on different platforms
 */
enum class TimePrecision : uint8_t {
    Nanosecond = 0,     ///< 1e-9 seconds (modern platforms)
    Microsecond,        ///< 1e-6 seconds (most platforms)
    Millisecond,        ///< 1e-3 seconds (legacy systems)
    Centisecond,        ///< 1e-2 seconds (very limited systems)
    Second              ///< 1 second (fallback only)
};

/**
 * @brief Sleep mode strategies for different precision/performance trade-offs
 */
enum class SleepMode : uint8_t {
    Busy = 0,           ///< Busy wait (highest precision, high CPU usage)
    Yield,              ///< Yield to scheduler (medium precision)
    Block,              ///< Block thread (lowest precision, lowest CPU usage)
    Adaptive            ///< Adaptive based on sleep duration
};

/**
 * @brief Timer state for pausable/resumable timers
 */
enum class TimerState : uint8_t {
    Running = 0,        ///< Timer is actively running
    Paused,             ///< Timer is paused
    Stopped,            ///< Timer is stopped and reset
    Expired             ///< Timer has reached its target
};

/**
 * @brief Frame timing mode for game loops
 */
enum class FrameTimingMode : uint8_t {
    Variable = 0,       ///< Variable timestep (delta time)
    Fixed,              ///< Fixed timestep with accumulation
    SemiFixed,          ///< Semi-fixed with maximum frame time
    Adaptive            ///< Adaptive based on performance
};

/**
 * @brief High-precision timestamp structure
 * 
 * Stores a timestamp with metadata about its source and precision.
 * Designed for zero-allocation timing operations.
 */
struct Timestamp {
    uint64_t value;             ///< Raw timestamp value
    ClockType source;           ///< Clock that generated this timestamp
    TimePrecision precision;    ///< Actual precision of the timestamp
    
    /**
     * @brief Convert timestamp to seconds
     * @return Time in seconds as double
     */
    double to_seconds() const;
    
    /**
     * @brief Convert timestamp to milliseconds
     * @return Time in milliseconds
     */
    uint64_t to_milliseconds() const;
    
    /**
     * @brief Convert timestamp to microseconds
     * @return Time in microseconds
     */
    uint64_t to_microseconds() const;
    
    /**
     * @brief Convert timestamp to nanoseconds
     * @return Time in nanoseconds
     */
    uint64_t to_nanoseconds() const;
};

/**
 * @brief Time duration structure for intervals and delays
 */
struct Duration {
    uint64_t value;             ///< Duration value
    TimePrecision precision;    ///< Precision of the duration
    
    /**
     * @brief Create duration from seconds
     * @param seconds Duration in seconds
     * @return Duration structure
     */
    static Duration from_seconds(double seconds);
    
    /**
     * @brief Create duration from milliseconds
     * @param milliseconds Duration in milliseconds
     * @return Duration structure
     */
    static Duration from_milliseconds(uint64_t milliseconds);
    
    /**
     * @brief Create duration from microseconds
     * @param microseconds Duration in microseconds
     * @return Duration structure
     */
    static Duration from_microseconds(uint64_t microseconds);
    
    /**
     * @brief Create duration from nanoseconds
     * @param nanoseconds Duration in nanoseconds
     * @return Duration structure
     */
    static Duration from_nanoseconds(uint64_t nanoseconds);
    
    /**
     * @brief Convert to seconds
     * @return Duration in seconds
     */
    double to_seconds() const;
    
    /**
     * @brief Add another duration
     * @param other Duration to add
     * @return New duration
     */
    Duration operator+(const Duration& other) const;
    
    /**
     * @brief Subtract another duration
     * @param other Duration to subtract
     * @return New duration
     */
    Duration operator-(const Duration& other) const;
    
    /**
     * @brief Compare with another duration
     * @param other Duration to compare
     * @return true if this duration is less than other
     */
    bool operator<(const Duration& other) const;
    
    /**
     * @brief Compare with another duration
     * @param other Duration to compare
     * @return true if this duration is greater than other
     */
    bool operator>(const Duration& other) const;
};

/**
 * @brief Frame timing information for game loops
 */
struct FrameTiming {
    Timestamp frame_start;      ///< Frame start timestamp
    Timestamp frame_end;        ///< Frame end timestamp
    Duration frame_duration;    ///< Actual frame duration
    double delta_seconds;       ///< Frame time in seconds (smoothed)
    double raw_delta_seconds;   ///< Raw frame time without smoothing
    double fps;                 ///< Current frames per second
    double average_fps;         ///< Average FPS over time window
    uint64_t frame_number;      ///< Monotonic frame counter
    uint32_t dropped_frames;    ///< Number of dropped frames since last call
    bool vsync_enabled;         ///< Whether VSync is active
    
    /**
     * @brief Check if frame timing is valid
     * @return true if timing data is valid
     */
    bool is_valid() const;
};

/**
 * @brief Performance timing metrics for profiling
 */
struct PerformanceMetrics {
    Duration min_frame_time;        ///< Minimum frame time observed
    Duration max_frame_time;        ///< Maximum frame time observed
    Duration avg_frame_time;        ///< Average frame time
    Duration frame_time_stddev;     ///< Standard deviation of frame times
    uint32_t total_frames;          ///< Total frames processed
    uint32_t dropped_frames;        ///< Total dropped frames
    uint32_t late_frames;           ///< Frames that exceeded target time
    double frame_time_variance;     ///< Variance in frame times
    double stability_ratio;         ///< Frame time stability (0.0-1.0)
    
    /**
     * @brief Reset all metrics
     */
    void reset();
    
    /**
     * @brief Get efficiency percentage
     * @return Efficiency as percentage (0-100)
     */
    double get_efficiency() const;
};

/**
 * @brief Timer configuration for pausable/resumable timers
 */
struct TimerConfig {
    Duration target_duration;       ///< Target duration for the timer
    bool auto_reset;               ///< Whether to auto-reset when expired
    bool high_precision;           ///< Use high-precision timing
    ClockType preferred_clock;     ///< Preferred clock type
    
    /**
     * @brief Create timer config from seconds
     * @param seconds Target duration in seconds
     * @param auto_reset Whether to auto-reset
     * @return Timer configuration
     */
    static TimerConfig from_seconds(double seconds, bool auto_reset = false);
    
    /**
     * @brief Create timer config from milliseconds
     * @param milliseconds Target duration in milliseconds
     * @param auto_reset Whether to auto-reset
     * @return Timer configuration
     */
    static TimerConfig from_milliseconds(uint64_t milliseconds, bool auto_reset = false);
};

/**
 * @brief Clock capability information
 */
struct ClockCapabilities {
    ClockType type;                 ///< Clock type
    TimePrecision precision;        ///< Available precision
    bool is_monotonic;             ///< Whether clock is monotonic
    bool is_steady;                ///< Whether clock is steady (not affected by system clock adjustments)
    bool supports_high_res;        ///< Whether high-resolution mode is available
    uint64_t frequency;            ///< Clock frequency (ticks per second)
    Duration resolution;           ///< Minimum resolution
    std::string_view name;         ///< Human-readable name
};

/**
 * @brief Platform timing capabilities
 */
struct PlatformTimingCapabilities {
    std::vector<ClockCapabilities> available_clocks;  ///< Available clock types
    TimePrecision best_precision;                     ///< Best available precision
    ClockType preferred_clock;                        ///< Preferred clock for general use
    ClockType preferred_game_clock;                   ///< Preferred clock for game timing
    bool supports_nanosleep;                          ///< Whether nanosleep is available
    bool supports_busy_wait;                          ///< Whether busy waiting is efficient
    bool supports_yield;                              ///< Whether thread yielding is available
    bool supports_vsync_detection;                    ///< Whether VSync detection is possible
    uint32_t timer_overhead_ns;                       ///< Timer call overhead in nanoseconds
};

// Forward declarations
class ITimer;
class IStopwatch;
class IFrameTimer;

/**
 * @brief Timer completion callback function type
 */
using TimerCallback = std::function<void(uint32_t timer_id, const Duration& elapsed)>;

/**
 * @brief Frame timing callback for game loops
 */
using FrameCallback = std::function<void(const FrameTiming& timing)>;

/**
 * @brief Performance monitoring callback
 */
using PerformanceCallback = std::function<void(const PerformanceMetrics& metrics)>;

/**
 * @brief Individual timer interface for pausable/resumable timers
 */
class ITimer {
public:
    virtual ~ITimer() = default;
    
    /**
     * @brief Start the timer
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> start() = 0;
    
    /**
     * @brief Pause the timer
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> pause() = 0;
    
    /**
     * @brief Resume the timer
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> resume() = 0;
    
    /**
     * @brief Stop and reset the timer
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> stop() = 0;
    
    /**
     * @brief Reset the timer to zero
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> reset() = 0;
    
    /**
     * @brief Get current elapsed time
     * @return HALResult containing elapsed duration
     */
    virtual HALResult<Duration> get_elapsed() const = 0;
    
    /**
     * @brief Get remaining time (if target duration is set)
     * @return HALResult containing remaining duration
     */
    virtual HALResult<Duration> get_remaining() const = 0;
    
    /**
     * @brief Get current timer state
     * @return Current timer state
     */
    virtual TimerState get_state() const = 0;
    
    /**
     * @brief Check if timer has expired
     * @return true if timer has reached target duration
     */
    virtual bool has_expired() const = 0;
    
    /**
     * @brief Get timer configuration
     * @return Timer configuration
     */
    virtual const TimerConfig& get_config() const = 0;
    
    /**
     * @brief Set timer callback
     * @param callback Callback to invoke when timer expires
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_callback(TimerCallback callback) = 0;
};

/**
 * @brief High-precision stopwatch interface
 */
class IStopwatch {
public:
    virtual ~IStopwatch() = default;
    
    /**
     * @brief Start the stopwatch
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> start() = 0;
    
    /**
     * @brief Stop the stopwatch
     * @return HALResult containing elapsed duration
     */
    virtual HALResult<Duration> stop() = 0;
    
    /**
     * @brief Reset the stopwatch to zero
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> reset() = 0;
    
    /**
     * @brief Get elapsed time without stopping
     * @return HALResult containing elapsed duration
     */
    virtual HALResult<Duration> lap() = 0;
    
    /**
     * @brief Check if stopwatch is running
     * @return true if running
     */
    virtual bool is_running() const = 0;
    
    /**
     * @brief Get total elapsed time
     * @return Total elapsed duration
     */
    virtual Duration get_total_elapsed() const = 0;
};

/**
 * @brief Frame timer interface for game loops
 */
class IFrameTimer {
public:
    virtual ~IFrameTimer() = default;
    
    /**
     * @brief Begin a new frame
     * @return HALResult containing frame timing info
     */
    virtual HALResult<FrameTiming> begin_frame() = 0;
    
    /**
     * @brief End the current frame
     * @return HALResult containing frame timing info
     */
    virtual HALResult<FrameTiming> end_frame() = 0;
    
    /**
     * @brief Set target frame rate
     * @param fps Target frames per second
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_target_fps(double fps) = 0;
    
    /**
     * @brief Get current frame timing
     * @return Current frame timing info
     */
    virtual const FrameTiming& get_current_timing() const = 0;
    
    /**
     * @brief Get performance metrics
     * @return Performance metrics
     */
    virtual const PerformanceMetrics& get_performance_metrics() const = 0;
    
    /**
     * @brief Set frame timing mode
     * @param mode Frame timing mode
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_timing_mode(FrameTimingMode mode) = 0;
    
    /**
     * @brief Set delta time smoothing factor
     * @param factor Smoothing factor (0.0-1.0)
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_smoothing_factor(double factor) = 0;
    
    /**
     * @brief Reset performance metrics
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> reset_metrics() = 0;
};

/**
 * @brief Enhanced timing interface for high-precision timing operations
 * 
 * Comprehensive timing system supporting multiple clock types, high-precision
 * timestamps, sleep functions, frame timing, and performance monitoring
 * across platforms from retro consoles to modern systems.
 */
class ITimeInterface : public IHALInterface {
public:
    virtual ~ITimeInterface() = default;
    
    // === Timestamp and Clock Methods ===
    
    /**
     * @brief Get current timestamp from preferred clock
     * @return HALResult containing current timestamp
     */
    virtual HALResult<Timestamp> now() = 0;
    
    /**
     * @brief Get current timestamp from specific clock type
     * @param clock_type Clock type to query
     * @return HALResult containing current timestamp
     */
    virtual HALResult<Timestamp> now(ClockType clock_type) = 0;
    
    /**
     * @brief Get elapsed time between two timestamps
     * @param start Start timestamp
     * @param end End timestamp
     * @return HALResult containing elapsed duration
     */
    virtual HALResult<Duration> elapsed(const Timestamp& start, const Timestamp& end) = 0;
    
    /**
     * @brief Get platform timing capabilities
     * @return Platform timing capabilities
     */
    virtual const PlatformTimingCapabilities& get_capabilities() const = 0;
    
    /**
     * @brief Get clock capabilities for specific type
     * @param clock_type Clock type to query
     * @return HALResult containing clock capabilities
     */
    virtual HALResult<ClockCapabilities> get_clock_capabilities(ClockType clock_type) const = 0;
    
    // === Sleep and Delay Methods ===
    
    /**
     * @brief Sleep for specified duration
     * @param duration Duration to sleep
     * @param mode Sleep mode (busy, yield, block)
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> sleep(const Duration& duration, SleepMode mode = SleepMode::Adaptive) = 0;
    
    /**
     * @brief Sleep for specified duration in seconds
     * @param seconds Duration in seconds
     * @param mode Sleep mode
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> sleep_seconds(double seconds, SleepMode mode = SleepMode::Adaptive) = 0;
    
    /**
     * @brief Sleep for specified duration in milliseconds
     * @param milliseconds Duration in milliseconds
     * @param mode Sleep mode
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> sleep_milliseconds(uint64_t milliseconds, SleepMode mode = SleepMode::Adaptive) = 0;
    
    /**
     * @brief Sleep for specified duration in microseconds
     * @param microseconds Duration in microseconds
     * @param mode Sleep mode
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> sleep_microseconds(uint64_t microseconds, SleepMode mode = SleepMode::Adaptive) = 0;
    
    /**
     * @brief Sleep until specific timestamp
     * @param target_time Target timestamp to sleep until
     * @param mode Sleep mode
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> sleep_until(const Timestamp& target_time, SleepMode mode = SleepMode::Adaptive) = 0;
    
    // === Timer Creation and Management ===
    
    /**
     * @brief Create a new timer
     * @param config Timer configuration
     * @return HALResult containing timer interface
     */
    virtual HALResult<std::unique_ptr<ITimer>> create_timer(const TimerConfig& config) = 0;
    
    /**
     * @brief Create a high-precision stopwatch
     * @param clock_type Preferred clock type for the stopwatch
     * @return HALResult containing stopwatch interface
     */
    virtual HALResult<std::unique_ptr<IStopwatch>> create_stopwatch(ClockType clock_type = ClockType::HighResolution) = 0;
    
    /**
     * @brief Create a frame timer for game loops
     * @param target_fps Target frames per second
     * @param mode Frame timing mode
     * @return HALResult containing frame timer interface
     */
    virtual HALResult<std::unique_ptr<IFrameTimer>> create_frame_timer(double target_fps = 60.0, 
                                                                      FrameTimingMode mode = FrameTimingMode::Variable) = 0;
    
    // === Performance Monitoring ===
    
    /**
     * @brief Start performance monitoring
     * @param callback Callback for performance updates
     * @param update_interval Update interval for callbacks
     * @return HALResult with monitoring ID
     */
    virtual HALResult<uint32_t> start_performance_monitoring(PerformanceCallback callback, 
                                                            const Duration& update_interval) = 0;
    
    /**
     * @brief Stop performance monitoring
     * @param monitor_id Monitor ID returned from start_performance_monitoring
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> stop_performance_monitoring(uint32_t monitor_id) = 0;
    
    /**
     * @brief Get current performance metrics
     * @return HALResult containing performance metrics
     */
    virtual HALResult<PerformanceMetrics> get_performance_metrics() = 0;
    
    // === Utility Methods ===
    
    /**
     * @brief Calibrate timer overhead
     * @return HALResult containing calibration results
     */
    virtual HALResult<Duration> calibrate_overhead() = 0;
    
    /**
     * @brief Check if specific clock type is supported
     * @param clock_type Clock type to check
     * @return true if supported
     */
    virtual bool supports_clock_type(ClockType clock_type) const = 0;
    
    /**
     * @brief Check if specific precision is supported
     * @param precision Precision to check
     * @return true if supported
     */
    virtual bool supports_precision(TimePrecision precision) const = 0;
    
    /**
     * @brief Check if specific sleep mode is supported
     * @param mode Sleep mode to check
     * @return true if supported
     */
    virtual bool supports_sleep_mode(SleepMode mode) const = 0;
    
    /**
     * @brief Get recommended sleep mode for duration
     * @param duration Duration to sleep
     * @return Recommended sleep mode
     */
    virtual SleepMode get_recommended_sleep_mode(const Duration& duration) const = 0;
    
    /**
     * @brief Convert between different time units
     * @param value Value to convert
     * @param from_precision Source precision
     * @param to_precision Target precision
     * @return Converted value
     */
    virtual uint64_t convert_time_units(uint64_t value, TimePrecision from_precision, TimePrecision to_precision) const = 0;
    
    // === IHALInterface implementation ===
    std::string_view get_interface_name() const override {
        return "time";
    }
};

// === Utility Functions ===

/**
 * @brief Timing utility functions
 */
namespace time {

/**
 * @brief Convert clock type to string
 * @param type Clock type to convert
 * @return String representation
 */
const char* to_string(ClockType type);

/**
 * @brief Convert time precision to string
 * @param precision Time precision to convert
 * @return String representation
 */
const char* to_string(TimePrecision precision);

/**
 * @brief Convert sleep mode to string
 * @param mode Sleep mode to convert
 * @return String representation
 */
const char* to_string(SleepMode mode);

/**
 * @brief Convert timer state to string
 * @param state Timer state to convert
 * @return String representation
 */
const char* to_string(TimerState state);

/**
 * @brief Convert frame timing mode to string
 * @param mode Frame timing mode to convert
 * @return String representation
 */
const char* to_string(FrameTimingMode mode);

/**
 * @brief Calculate frames per second from frame duration
 * @param frame_duration Frame duration
 * @return FPS value
 */
double calculate_fps(const Duration& frame_duration);

/**
 * @brief Calculate frame duration from FPS
 * @param fps Frames per second
 * @return Frame duration
 */
Duration calculate_frame_duration(double fps);

/**
 * @brief Apply delta time smoothing
 * @param current_delta Current delta time
 * @param previous_delta Previous delta time
 * @param smoothing_factor Smoothing factor (0.0-1.0)
 * @return Smoothed delta time
 */
double smooth_delta_time(double current_delta, double previous_delta, double smoothing_factor);

/**
 * @brief Clamp delta time to reasonable bounds
 * @param delta_time Delta time to clamp
 * @param min_delta Minimum delta time
 * @param max_delta Maximum delta time
 * @return Clamped delta time
 */
double clamp_delta_time(double delta_time, double min_delta = 0.001, double max_delta = 0.1);

/**
 * @brief Check if two timestamps are from compatible clocks
 * @param a First timestamp
 * @param b Second timestamp
 * @return true if timestamps can be compared
 */
bool are_timestamps_compatible(const Timestamp& a, const Timestamp& b);

/**
 * @brief Get precision multiplier for time unit conversion
 * @param precision Time precision
 * @return Multiplier for converting to nanoseconds
 */
uint64_t get_precision_multiplier(TimePrecision precision);

/**
 * @brief Get precision divisor for time unit conversion
 * @param precision Time precision
 * @return Divisor for converting from nanoseconds
 */
uint64_t get_precision_divisor(TimePrecision precision);

} // namespace time

} // namespace flight::hal
