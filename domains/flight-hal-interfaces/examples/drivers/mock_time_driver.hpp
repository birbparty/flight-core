/**
 * @file mock_time_driver.hpp
 * @brief Mock Time Driver for Flight HAL Testing
 * 
 * Mock implementation of the timing interface for testing, development,
 * and demonstration purposes. Provides simulated high-precision timing
 * with configurable behavior.
 */

#pragma once

#include "../../include/flight/hal/interfaces/time.hpp"
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <map>

namespace flight::hal::mock {

/**
 * @brief Mock timer implementation for testing
 */
class MockTimer : public ITimer {
public:
    MockTimer(const TimerConfig& config);
    ~MockTimer() override = default;
    
    // ITimer implementation
    HALResult<void> start() override;
    HALResult<void> pause() override;
    HALResult<void> resume() override;
    HALResult<void> stop() override;
    HALResult<void> reset() override;
    HALResult<Duration> get_elapsed() const override;
    HALResult<Duration> get_remaining() const override;
    TimerState get_state() const override;
    bool has_expired() const override;
    const TimerConfig& get_config() const override;
    HALResult<void> set_callback(TimerCallback callback) override;

private:
    TimerConfig config_;
    mutable std::atomic<TimerState> state_;
    std::chrono::steady_clock::time_point start_time_;
    std::chrono::steady_clock::time_point pause_time_;
    std::chrono::nanoseconds accumulated_time_;
    TimerCallback callback_;
    mutable std::mutex mutex_;
    uint32_t timer_id_;
    static std::atomic<uint32_t> next_timer_id_;
    
    void check_expiration() const;
};

/**
 * @brief Mock stopwatch implementation for testing
 */
class MockStopwatch : public IStopwatch {
public:
    explicit MockStopwatch(ClockType clock_type);
    ~MockStopwatch() override = default;
    
    // IStopwatch implementation
    HALResult<void> start() override;
    HALResult<Duration> stop() override;
    HALResult<void> reset() override;
    HALResult<Duration> lap() override;
    bool is_running() const override;
    Duration get_total_elapsed() const override;

private:
    ClockType clock_type_;
    std::atomic<bool> running_;
    std::chrono::steady_clock::time_point start_time_;
    std::chrono::nanoseconds total_elapsed_;
    mutable std::mutex mutex_;
    
    Duration get_current_elapsed() const;
};

/**
 * @brief Mock frame timer implementation for testing
 */
class MockFrameTimer : public IFrameTimer {
public:
    MockFrameTimer(double target_fps, FrameTimingMode mode);
    ~MockFrameTimer() override = default;
    
    // IFrameTimer implementation
    HALResult<FrameTiming> begin_frame() override;
    HALResult<FrameTiming> end_frame() override;
    HALResult<void> set_target_fps(double fps) override;
    const FrameTiming& get_current_timing() const override;
    const PerformanceMetrics& get_performance_metrics() const override;
    HALResult<void> set_timing_mode(FrameTimingMode mode) override;
    HALResult<void> set_smoothing_factor(double factor) override;
    HALResult<void> reset_metrics() override;

private:
    double target_fps_;
    FrameTimingMode timing_mode_;
    double smoothing_factor_;
    FrameTiming current_timing_;
    PerformanceMetrics metrics_;
    std::chrono::steady_clock::time_point last_frame_time_;
    bool frame_in_progress_;
    std::mutex mutex_;
    
    void update_metrics(const Duration& frame_duration);
    void calculate_frame_timing();
};

/**
 * @brief Mock time interface implementation for testing
 * 
 * Provides a complete mock implementation of the timing interface
 * with configurable simulation parameters for testing various
 * timing scenarios and platform behaviors.
 */
class MockTimeInterface : public ITimeInterface {
public:
    MockTimeInterface();
    ~MockTimeInterface() override = default;
    
    // Configuration methods for testing
    void set_simulated_precision(TimePrecision precision);
    void set_timer_overhead(uint32_t overhead_ns);
    void enable_clock_drift(bool enabled, double drift_rate = 0.001);
    void simulate_sleep_inaccuracy(bool enabled, double inaccuracy_factor = 0.1);
    
    // ITimeInterface implementation
    HALResult<Timestamp> now() override;
    HALResult<Timestamp> now(ClockType clock_type) override;
    HALResult<Duration> elapsed(const Timestamp& start, const Timestamp& end) override;
    const PlatformTimingCapabilities& get_capabilities() const override;
    HALResult<ClockCapabilities> get_clock_capabilities(ClockType clock_type) const override;
    
    HALResult<void> sleep(const Duration& duration, SleepMode mode = SleepMode::Adaptive) override;
    HALResult<void> sleep_seconds(double seconds, SleepMode mode = SleepMode::Adaptive) override;
    HALResult<void> sleep_milliseconds(uint64_t milliseconds, SleepMode mode = SleepMode::Adaptive) override;
    HALResult<void> sleep_microseconds(uint64_t microseconds, SleepMode mode = SleepMode::Adaptive) override;
    HALResult<void> sleep_until(const Timestamp& target_time, SleepMode mode = SleepMode::Adaptive) override;
    
    HALResult<std::unique_ptr<ITimer>> create_timer(const TimerConfig& config) override;
    HALResult<std::unique_ptr<IStopwatch>> create_stopwatch(ClockType clock_type = ClockType::HighResolution) override;
    HALResult<std::unique_ptr<IFrameTimer>> create_frame_timer(double target_fps = 60.0, 
                                                              FrameTimingMode mode = FrameTimingMode::Variable) override;
    
    HALResult<uint32_t> start_performance_monitoring(PerformanceCallback callback, 
                                                    const Duration& update_interval) override;
    HALResult<void> stop_performance_monitoring(uint32_t monitor_id) override;
    HALResult<PerformanceMetrics> get_performance_metrics() override;
    
    HALResult<Duration> calibrate_overhead() override;
    bool supports_clock_type(ClockType clock_type) const override;
    bool supports_precision(TimePrecision precision) const override;
    bool supports_sleep_mode(SleepMode mode) const override;
    SleepMode get_recommended_sleep_mode(const Duration& duration) const override;
    uint64_t convert_time_units(uint64_t value, TimePrecision from_precision, TimePrecision to_precision) const override;

private:
    PlatformTimingCapabilities capabilities_;
    TimePrecision simulated_precision_;
    uint32_t timer_overhead_ns_;
    bool clock_drift_enabled_;
    double clock_drift_rate_;
    bool sleep_inaccuracy_enabled_;
    double sleep_inaccuracy_factor_;
    std::chrono::steady_clock::time_point simulation_start_;
    
    // Performance monitoring
    std::map<uint32_t, std::pair<PerformanceCallback, Duration>> performance_monitors_;
    std::atomic<uint32_t> next_monitor_id_;
    PerformanceMetrics global_metrics_;
    std::thread monitor_thread_;
    std::atomic<bool> monitoring_active_;
    std::mutex monitors_mutex_;
    
    void initialize_capabilities();
    uint64_t get_raw_timestamp(ClockType clock_type) const;
    void apply_simulated_effects(uint64_t& timestamp) const;
    void performance_monitor_thread();
    HALResult<void> sleep_internal(std::chrono::nanoseconds duration, SleepMode mode);
};

// === Mock Timer Implementation ===

std::atomic<uint32_t> MockTimer::next_timer_id_{1};

MockTimer::MockTimer(const TimerConfig& config)
    : config_(config)
    , state_(TimerState::Stopped)
    , accumulated_time_(0)
    , timer_id_(next_timer_id_++) {}

HALResult<void> MockTimer::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == TimerState::Running) {
        return HALResult<void>::error(errors::invalid_state(1, "Timer already running"));
    }
    
    state_.store(TimerState::Running);
    start_time_ = std::chrono::steady_clock::now();
    return HALResult<void>::success();
}

HALResult<void> MockTimer::pause() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ != TimerState::Running) {
        return HALResult<void>::error(errors::invalid_state(2, "Timer not running"));
    }
    
    state_.store(TimerState::Paused);
    pause_time_ = std::chrono::steady_clock::now();
    accumulated_time_ += pause_time_ - start_time_;
    return HALResult<void>::success();
}

HALResult<void> MockTimer::resume() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ != TimerState::Paused) {
        return HALResult<void>::error(errors::invalid_state(3, "Timer not paused"));
    }
    
    state_.store(TimerState::Running);
    start_time_ = std::chrono::steady_clock::now();
    return HALResult<void>::success();
}

HALResult<void> MockTimer::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    state_.store(TimerState::Stopped);
    accumulated_time_ = std::chrono::nanoseconds(0);
    return HALResult<void>::success();
}

HALResult<void> MockTimer::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    accumulated_time_ = std::chrono::nanoseconds(0);
    if (state_ == TimerState::Running) {
        start_time_ = std::chrono::steady_clock::now();
    }
    return HALResult<void>::success();
}

HALResult<Duration> MockTimer::get_elapsed() const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto total_elapsed = accumulated_time_;
    
    if (state_ == TimerState::Running) {
        auto now = std::chrono::steady_clock::now();
        total_elapsed += now - start_time_;
    }
    
    return HALResult<Duration>::success(
        Duration::from_nanoseconds(total_elapsed.count())
    );
}

HALResult<Duration> MockTimer::get_remaining() const {
    auto elapsed_result = get_elapsed();
    if (!elapsed_result.is_ok()) {
        return HALResult<Duration>::error(elapsed_result.error());
    }
    
    auto target_ns = config_.target_duration.value;
    auto elapsed_ns = elapsed_result.value().value;
    
    if (elapsed_ns >= target_ns) {
        return HALResult<Duration>::success(Duration::from_nanoseconds(0));
    }
    
    return HALResult<Duration>::success(
        Duration::from_nanoseconds(target_ns - elapsed_ns)
    );
}

TimerState MockTimer::get_state() const {
    check_expiration();
    return state_.load();
}

bool MockTimer::has_expired() const {
    check_expiration();
    return state_.load() == TimerState::Expired;
}

const TimerConfig& MockTimer::get_config() const {
    return config_;
}

HALResult<void> MockTimer::set_callback(TimerCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callback_ = std::move(callback);
    return HALResult<void>::success();
}

void MockTimer::check_expiration() const {
    if (state_.load() != TimerState::Running) return;
    
    auto elapsed_result = get_elapsed();
    if (!elapsed_result.is_ok()) return;
    
    if (elapsed_result.value().value >= config_.target_duration.value) {
        state_ = TimerState::Expired;
        
        if (callback_) {
            callback_(timer_id_, elapsed_result.value());
        }
        
        if (config_.auto_reset) {
            const_cast<MockTimer*>(this)->reset();
            const_cast<MockTimer*>(this)->start();
        }
    }
}

} // namespace flight::hal::mock
