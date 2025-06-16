/**
 * @file time.cpp
 * @brief Flight HAL Timing Interface Implementation
 * 
 * Core implementation of timing utilities and helper functions for
 * the Flight HAL timing interface.
 */

#include "../../include/flight/hal/interfaces/time.hpp"
#include "../../include/flight/hal/core/hal_error.hpp"
#include <algorithm>
#include <cmath>

namespace flight::hal {

// === Timestamp Implementation ===

double Timestamp::to_seconds() const {
    switch (precision) {
        case TimePrecision::Nanosecond:
            return static_cast<double>(value) / 1e9;
        case TimePrecision::Microsecond:
            return static_cast<double>(value) / 1e6;
        case TimePrecision::Millisecond:
            return static_cast<double>(value) / 1e3;
        case TimePrecision::Centisecond:
            return static_cast<double>(value) / 1e2;
        case TimePrecision::Second:
            return static_cast<double>(value);
        default:
            return 0.0;
    }
}

uint64_t Timestamp::to_milliseconds() const {
    switch (precision) {
        case TimePrecision::Nanosecond:
            return value / 1000000;
        case TimePrecision::Microsecond:
            return value / 1000;
        case TimePrecision::Millisecond:
            return value;
        case TimePrecision::Centisecond:
            return value * 10;
        case TimePrecision::Second:
            return value * 1000;
        default:
            return 0;
    }
}

uint64_t Timestamp::to_microseconds() const {
    switch (precision) {
        case TimePrecision::Nanosecond:
            return value / 1000;
        case TimePrecision::Microsecond:
            return value;
        case TimePrecision::Millisecond:
            return value * 1000;
        case TimePrecision::Centisecond:
            return value * 10000;
        case TimePrecision::Second:
            return value * 1000000;
        default:
            return 0;
    }
}

uint64_t Timestamp::to_nanoseconds() const {
    switch (precision) {
        case TimePrecision::Nanosecond:
            return value;
        case TimePrecision::Microsecond:
            return value * 1000;
        case TimePrecision::Millisecond:
            return value * 1000000;
        case TimePrecision::Centisecond:
            return value * 10000000;
        case TimePrecision::Second:
            return value * 1000000000;
        default:
            return 0;
    }
}

// === Duration Implementation ===

Duration Duration::from_seconds(double seconds) {
    // Use nanoseconds for maximum precision
    uint64_t nanoseconds = static_cast<uint64_t>(seconds * 1e9);
    return {nanoseconds, TimePrecision::Nanosecond};
}

Duration Duration::from_milliseconds(uint64_t milliseconds) {
    return {milliseconds * 1000000, TimePrecision::Nanosecond};
}

Duration Duration::from_microseconds(uint64_t microseconds) {
    return {microseconds * 1000, TimePrecision::Nanosecond};
}

Duration Duration::from_nanoseconds(uint64_t nanoseconds) {
    return {nanoseconds, TimePrecision::Nanosecond};
}

double Duration::to_seconds() const {
    switch (precision) {
        case TimePrecision::Nanosecond:
            return static_cast<double>(value) / 1e9;
        case TimePrecision::Microsecond:
            return static_cast<double>(value) / 1e6;
        case TimePrecision::Millisecond:
            return static_cast<double>(value) / 1e3;
        case TimePrecision::Centisecond:
            return static_cast<double>(value) / 1e2;
        case TimePrecision::Second:
            return static_cast<double>(value);
        default:
            return 0.0;
    }
}

Duration Duration::operator+(const Duration& other) const {
    // Convert both to nanoseconds for addition
    uint64_t this_ns = 0;
    uint64_t other_ns = 0;
    
    switch (precision) {
        case TimePrecision::Nanosecond: this_ns = value; break;
        case TimePrecision::Microsecond: this_ns = value * 1000; break;
        case TimePrecision::Millisecond: this_ns = value * 1000000; break;
        case TimePrecision::Centisecond: this_ns = value * 10000000; break;
        case TimePrecision::Second: this_ns = value * 1000000000; break;
    }
    
    switch (other.precision) {
        case TimePrecision::Nanosecond: other_ns = other.value; break;
        case TimePrecision::Microsecond: other_ns = other.value * 1000; break;
        case TimePrecision::Millisecond: other_ns = other.value * 1000000; break;
        case TimePrecision::Centisecond: other_ns = other.value * 10000000; break;
        case TimePrecision::Second: other_ns = other.value * 1000000000; break;
    }
    
    return {this_ns + other_ns, TimePrecision::Nanosecond};
}

Duration Duration::operator-(const Duration& other) const {
    // Convert both to nanoseconds for subtraction
    uint64_t this_ns = 0;
    uint64_t other_ns = 0;
    
    switch (precision) {
        case TimePrecision::Nanosecond: this_ns = value; break;
        case TimePrecision::Microsecond: this_ns = value * 1000; break;
        case TimePrecision::Millisecond: this_ns = value * 1000000; break;
        case TimePrecision::Centisecond: this_ns = value * 10000000; break;
        case TimePrecision::Second: this_ns = value * 1000000000; break;
    }
    
    switch (other.precision) {
        case TimePrecision::Nanosecond: other_ns = other.value; break;
        case TimePrecision::Microsecond: other_ns = other.value * 1000; break;
        case TimePrecision::Millisecond: other_ns = other.value * 1000000; break;
        case TimePrecision::Centisecond: other_ns = other.value * 10000000; break;
        case TimePrecision::Second: other_ns = other.value * 1000000000; break;
    }
    
    if (this_ns >= other_ns) {
        return {this_ns - other_ns, TimePrecision::Nanosecond};
    } else {
        return {0, TimePrecision::Nanosecond};
    }
}

bool Duration::operator<(const Duration& other) const {
    // Convert both to nanoseconds for comparison
    uint64_t this_ns = 0;
    uint64_t other_ns = 0;
    
    switch (precision) {
        case TimePrecision::Nanosecond: this_ns = value; break;
        case TimePrecision::Microsecond: this_ns = value * 1000; break;
        case TimePrecision::Millisecond: this_ns = value * 1000000; break;
        case TimePrecision::Centisecond: this_ns = value * 10000000; break;
        case TimePrecision::Second: this_ns = value * 1000000000; break;
    }
    
    switch (other.precision) {
        case TimePrecision::Nanosecond: other_ns = other.value; break;
        case TimePrecision::Microsecond: other_ns = other.value * 1000; break;
        case TimePrecision::Millisecond: other_ns = other.value * 1000000; break;
        case TimePrecision::Centisecond: other_ns = other.value * 10000000; break;
        case TimePrecision::Second: other_ns = other.value * 1000000000; break;
    }
    
    return this_ns < other_ns;
}

bool Duration::operator>(const Duration& other) const {
    return other < *this;
}

// === FrameTiming Implementation ===

bool FrameTiming::is_valid() const {
    return frame_start.value != 0 && 
           frame_end.value >= frame_start.value &&
           delta_seconds > 0.0 &&
           fps >= 0.0;
}

// === PerformanceMetrics Implementation ===

void PerformanceMetrics::reset() {
    min_frame_time = Duration::from_nanoseconds(UINT64_MAX);
    max_frame_time = Duration::from_nanoseconds(0);
    avg_frame_time = Duration::from_nanoseconds(0);
    frame_time_stddev = Duration::from_nanoseconds(0);
    total_frames = 0;
    dropped_frames = 0;
    late_frames = 0;
    frame_time_variance = 0.0;
    stability_ratio = 0.0;
}

double PerformanceMetrics::get_efficiency() const {
    if (total_frames == 0) return 0.0;
    
    double success_rate = 1.0 - (static_cast<double>(dropped_frames + late_frames) / total_frames);
    return std::max(0.0, std::min(100.0, success_rate * 100.0));
}

// === TimerConfig Implementation ===

TimerConfig TimerConfig::from_seconds(double seconds, bool auto_reset) {
    TimerConfig config;
    config.target_duration = Duration::from_seconds(seconds);
    config.auto_reset = auto_reset;
    config.high_precision = seconds < 0.001; // Use high precision for sub-millisecond timers
    config.preferred_clock = ClockType::Monotonic;
    return config;
}

TimerConfig TimerConfig::from_milliseconds(uint64_t milliseconds, bool auto_reset) {
    TimerConfig config;
    config.target_duration = Duration::from_milliseconds(milliseconds);
    config.auto_reset = auto_reset;
    config.high_precision = milliseconds < 10; // Use high precision for sub-10ms timers
    config.preferred_clock = ClockType::Monotonic;
    return config;
}

// === Utility Functions ===

namespace time {

const char* to_string(ClockType type) {
    switch (type) {
        case ClockType::Monotonic: return "Monotonic";
        case ClockType::Realtime: return "Realtime";
        case ClockType::Process: return "Process";
        case ClockType::Thread: return "Thread";
        case ClockType::HighResolution: return "HighResolution";
        case ClockType::GameTimer: return "GameTimer";
        case ClockType::ProfileTimer: return "ProfileTimer";
        default: return "Unknown";
    }
}

const char* to_string(TimePrecision precision) {
    switch (precision) {
        case TimePrecision::Nanosecond: return "Nanosecond";
        case TimePrecision::Microsecond: return "Microsecond";
        case TimePrecision::Millisecond: return "Millisecond";
        case TimePrecision::Centisecond: return "Centisecond";
        case TimePrecision::Second: return "Second";
        default: return "Unknown";
    }
}

const char* to_string(SleepMode mode) {
    switch (mode) {
        case SleepMode::Busy: return "Busy";
        case SleepMode::Yield: return "Yield";
        case SleepMode::Block: return "Block";
        case SleepMode::Adaptive: return "Adaptive";
        default: return "Unknown";
    }
}

const char* to_string(TimerState state) {
    switch (state) {
        case TimerState::Running: return "Running";
        case TimerState::Paused: return "Paused";
        case TimerState::Stopped: return "Stopped";
        case TimerState::Expired: return "Expired";
        default: return "Unknown";
    }
}

const char* to_string(FrameTimingMode mode) {
    switch (mode) {
        case FrameTimingMode::Variable: return "Variable";
        case FrameTimingMode::Fixed: return "Fixed";
        case FrameTimingMode::SemiFixed: return "SemiFixed";
        case FrameTimingMode::Adaptive: return "Adaptive";
        default: return "Unknown";
    }
}

double calculate_fps(const Duration& frame_duration) {
    double seconds = frame_duration.to_seconds();
    return (seconds > 0.0) ? (1.0 / seconds) : 0.0;
}

Duration calculate_frame_duration(double fps) {
    if (fps <= 0.0) {
        return Duration::from_seconds(1.0 / 60.0); // Default to 60 FPS
    }
    return Duration::from_seconds(1.0 / fps);
}

double smooth_delta_time(double current_delta, double previous_delta, double smoothing_factor) {
    if (smoothing_factor <= 0.0) return current_delta;
    if (smoothing_factor >= 1.0) return previous_delta;
    
    return previous_delta * smoothing_factor + current_delta * (1.0 - smoothing_factor);
}

double clamp_delta_time(double delta_time, double min_delta, double max_delta) {
    return std::max(min_delta, std::min(max_delta, delta_time));
}

bool are_timestamps_compatible(const Timestamp& a, const Timestamp& b) {
    // Timestamps are compatible if they're from the same clock type
    // or if both are monotonic clocks
    return a.source == b.source || 
           (a.source == ClockType::Monotonic && b.source == ClockType::Monotonic) ||
           (a.source == ClockType::HighResolution && b.source == ClockType::HighResolution);
}

uint64_t get_precision_multiplier(TimePrecision precision) {
    switch (precision) {
        case TimePrecision::Nanosecond: return 1;
        case TimePrecision::Microsecond: return 1000;
        case TimePrecision::Millisecond: return 1000000;
        case TimePrecision::Centisecond: return 10000000;
        case TimePrecision::Second: return 1000000000;
        default: return 1;
    }
}

uint64_t get_precision_divisor(TimePrecision precision) {
    return get_precision_multiplier(precision);
}

} // namespace time

} // namespace flight::hal
