/**
 * @file comprehensive_time_example.cpp
 * @brief Comprehensive Timing Interface Example
 * 
 * Demonstrates the full capabilities of the Flight HAL timing interface
 * including high-precision timestamps, timers, stopwatches, frame timing,
 * and performance monitoring across different platforms.
 */

#include "../../include/flight/hal/interfaces/time.hpp"
#include "../../include/flight/hal/core/hal_logging.hpp"
#include <iostream>
#include <iomanip>
#include <vector>
#include <thread>

using namespace flight::hal;

// Forward declarations
void demonstrate_basic_timing();
void demonstrate_sleep_functions();
void demonstrate_timer_functionality();
void demonstrate_stopwatch_usage();
void demonstrate_frame_timing();
void demonstrate_performance_monitoring();
void demonstrate_timing_utilities();
void print_timing_capabilities(const ITimeInterface& time_interface);

/**
 * @brief Main timing demonstration
 */
int main() {
    std::cout << "=== Flight HAL Timing Interface Demonstration ===\n\n";
    
    // Note: In a real implementation, you would get the time interface from the HAL system
    // For this example, we'll demonstrate the API usage conceptually
    
    try {
        demonstrate_basic_timing();
        demonstrate_sleep_functions();
        demonstrate_timer_functionality();
        demonstrate_stopwatch_usage();
        demonstrate_frame_timing();
        demonstrate_performance_monitoring();
        demonstrate_timing_utilities();
        
        std::cout << "\n=== All timing demonstrations completed successfully ===\n";
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error during timing demonstration: " << e.what() << std::endl;
        return 1;
    }
}

/**
 * @brief Demonstrate basic timing operations
 */
void demonstrate_basic_timing() {
    std::cout << "--- Basic Timing Operations ---\n";
    
    // Create sample timestamps for demonstration
    Timestamp start = {0, ClockType::Monotonic, TimePrecision::Nanosecond};
    Timestamp end = {1000000000, ClockType::Monotonic, TimePrecision::Nanosecond}; // 1 second later
    
    std::cout << "Timestamp conversion examples:\n";
    std::cout << "Start timestamp: " << start.value << " ns\n";
    std::cout << "End timestamp: " << end.value << " ns\n";
    std::cout << "Start as seconds: " << start.to_seconds() << " s\n";
    std::cout << "End as milliseconds: " << end.to_milliseconds() << " ms\n";
    
    // Duration calculations
    Duration duration = Duration::from_nanoseconds(end.value - start.value);
    std::cout << "Duration: " << duration.to_seconds() << " seconds\n";
    std::cout << "Duration: " << duration.value << " nanoseconds\n";
    
    // Demonstrate different time precisions
    std::cout << "\nTime precision examples:\n";
    auto ms_duration = Duration::from_milliseconds(1500);
    auto us_duration = Duration::from_microseconds(1500000);
    auto s_duration = Duration::from_seconds(1.5);
    
    std::cout << "1.5 seconds as milliseconds: " << ms_duration.value << " (precision: " 
              << time::to_string(ms_duration.precision) << ")\n";
    std::cout << "1.5 seconds as microseconds: " << us_duration.value << " (precision: " 
              << time::to_string(us_duration.precision) << ")\n";
    std::cout << "1.5 seconds from double: " << s_duration.value << " ns (precision: " 
              << time::to_string(s_duration.precision) << ")\n";
    
    std::cout << "\n";
}

/**
 * @brief Demonstrate sleep functions
 */
void demonstrate_sleep_functions() {
    std::cout << "--- Sleep Functions ---\n";
    
    // Demonstrate different sleep modes and their use cases
    std::cout << "Sleep mode recommendations:\n";
    
    auto short_sleep = Duration::from_microseconds(10);
    auto medium_sleep = Duration::from_milliseconds(10);
    auto long_sleep = Duration::from_seconds(1.0);
    
    std::cout << "For " << short_sleep.to_seconds() * 1e6 << " μs sleep: Use " 
              << time::to_string(SleepMode::Busy) << " wait\n";
    std::cout << "For " << medium_sleep.to_seconds() * 1e3 << " ms sleep: Use " 
              << time::to_string(SleepMode::Yield) << " mode\n";
    std::cout << "For " << long_sleep.to_seconds() << " s sleep: Use " 
              << time::to_string(SleepMode::Block) << " mode\n";
    
    // Actual sleep demonstration (commented out to avoid delays in example)
    /*
    auto start_time = std::chrono::steady_clock::now();
    
    // This would use the actual time interface:
    // auto result = time_interface.sleep_milliseconds(100, SleepMode::Block);
    // if (result.is_ok()) {
    //     std::cout << "Successfully slept for 100ms\n";
    // }
    
    auto end_time = std::chrono::steady_clock::now();
    auto actual_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "Actual sleep duration: " << actual_duration.count() << " ms\n";
    */
    
    std::cout << "Sleep accuracy considerations:\n";
    std::cout << "- Busy wait: Highest precision, high CPU usage\n";
    std::cout << "- Yield: Good precision, moderate CPU usage\n";
    std::cout << "- Block: Lower precision, minimal CPU usage\n";
    std::cout << "- Adaptive: Automatically chooses based on duration\n";
    
    std::cout << "\n";
}

/**
 * @brief Demonstrate timer functionality
 */
void demonstrate_timer_functionality() {
    std::cout << "--- Timer Functionality ---\n";
    
    // Timer configuration examples
    auto short_timer_config = TimerConfig::from_milliseconds(500, false);
    auto repeating_timer_config = TimerConfig::from_seconds(1.0, true);
    
    std::cout << "Timer configuration examples:\n";
    std::cout << "Short timer: " << short_timer_config.target_duration.to_seconds() 
              << "s, auto-reset: " << (short_timer_config.auto_reset ? "yes" : "no") << "\n";
    std::cout << "Repeating timer: " << repeating_timer_config.target_duration.to_seconds() 
              << "s, auto-reset: " << (repeating_timer_config.auto_reset ? "yes" : "no") << "\n";
    
    std::cout << "\nTimer usage pattern:\n";
    std::cout << "1. Create timer with configuration\n";
    std::cout << "2. Set callback (optional)\n";
    std::cout << "3. Start timer\n";
    std::cout << "4. Check state: " << time::to_string(TimerState::Running) << "\n";
    std::cout << "5. Pause/resume as needed\n";
    std::cout << "6. Timer expires -> state: " << time::to_string(TimerState::Expired) << "\n";
    std::cout << "7. Auto-reset (if configured) or manual reset\n";
    
    // Timer callback example
    auto timer_callback = [](uint32_t timer_id, const Duration& elapsed) {
        std::cout << "Timer " << timer_id << " expired after " 
                  << elapsed.to_seconds() << " seconds\n";
    };
    
    std::cout << "\nTimer states:\n";
    std::cout << "- " << time::to_string(TimerState::Stopped) << ": Initial state, timer not running\n";
    std::cout << "- " << time::to_string(TimerState::Running) << ": Timer is actively counting\n";
    std::cout << "- " << time::to_string(TimerState::Paused) << ": Timer paused, can be resumed\n";
    std::cout << "- " << time::to_string(TimerState::Expired) << ": Timer reached target duration\n";
    
    std::cout << "\n";
}

/**
 * @brief Demonstrate stopwatch usage
 */
void demonstrate_stopwatch_usage() {
    std::cout << "--- Stopwatch Usage ---\n";
    
    std::cout << "Stopwatch operation sequence:\n";
    std::cout << "1. Create stopwatch with preferred clock type\n";
    std::cout << "2. Start timing\n";
    std::cout << "3. Lap timing (get elapsed without stopping)\n";
    std::cout << "4. Stop and get final elapsed time\n";
    std::cout << "5. Reset for next measurement\n";
    
    std::cout << "\nStopwatch use cases:\n";
    std::cout << "- Performance profiling\n";
    std::cout << "- Function execution timing\n";
    std::cout << "- Game loop timing\n";
    std::cout << "- Algorithm benchmarking\n";
    
    std::cout << "\nClock type recommendations:\n";
    std::cout << "- " << time::to_string(ClockType::HighResolution) 
              << ": For maximum precision\n";
    std::cout << "- " << time::to_string(ClockType::Monotonic) 
              << ": For stable, system-independent timing\n";
    std::cout << "- " << time::to_string(ClockType::Process) 
              << ": For process-specific timing\n";
    
    // Simulate stopwatch measurements
    std::cout << "\nSimulated measurement sequence:\n";
    std::cout << "Stopwatch start -> Lap 1: 0.1s -> Lap 2: 0.25s -> Stop: 0.5s\n";
    
    std::cout << "\n";
}

/**
 * @brief Demonstrate frame timing for game loops
 */
void demonstrate_frame_timing() {
    std::cout << "--- Frame Timing for Game Loops ---\n";
    
    std::cout << "Frame timing modes:\n";
    std::cout << "- " << time::to_string(FrameTimingMode::Variable) 
              << ": Delta time varies with actual frame duration\n";
    std::cout << "- " << time::to_string(FrameTimingMode::Fixed) 
              << ": Fixed timestep with accumulation\n";
    std::cout << "- " << time::to_string(FrameTimingMode::SemiFixed) 
              << ": Fixed timestep with maximum frame time cap\n";
    std::cout << "- " << time::to_string(FrameTimingMode::Adaptive) 
              << ": Adapts based on performance\n";
    
    // Simulate frame timing data
    FrameTiming simulated_timing = {};
    simulated_timing.frame_start = {100000000, ClockType::Monotonic, TimePrecision::Nanosecond};
    simulated_timing.frame_end = {116666667, ClockType::Monotonic, TimePrecision::Nanosecond};
    simulated_timing.frame_duration = Duration::from_nanoseconds(16666667); // ~60 FPS
    simulated_timing.delta_seconds = 0.01666667;
    simulated_timing.raw_delta_seconds = 0.01666667;
    simulated_timing.fps = 60.0;
    simulated_timing.average_fps = 59.8;
    simulated_timing.frame_number = 1000;
    simulated_timing.dropped_frames = 0;
    simulated_timing.vsync_enabled = true;
    
    std::cout << "\nSimulated frame timing data:\n";
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Frame #" << simulated_timing.frame_number << "\n";
    std::cout << "Duration: " << simulated_timing.delta_seconds << " seconds\n";
    std::cout << "Current FPS: " << simulated_timing.fps << "\n";
    std::cout << "Average FPS: " << simulated_timing.average_fps << "\n";
    std::cout << "Dropped frames: " << simulated_timing.dropped_frames << "\n";
    std::cout << "VSync: " << (simulated_timing.vsync_enabled ? "enabled" : "disabled") << "\n";
    
    // Frame timing best practices
    std::cout << "\nFrame timing best practices:\n";
    std::cout << "1. Use monotonic clocks for consistent timing\n";
    std::cout << "2. Apply delta time smoothing to reduce jitter\n";
    std::cout << "3. Clamp delta time to prevent spiral of death\n";
    std::cout << "4. Monitor frame drops and performance metrics\n";
    std::cout << "5. Choose appropriate timing mode for your game type\n";
    
    // Delta time smoothing example
    double current_delta = 0.0180;  // 18ms frame
    double previous_delta = 0.0166; // 16.6ms frame
    double smoothing_factor = 0.1;
    double smoothed = time::smooth_delta_time(current_delta, previous_delta, smoothing_factor);
    
    std::cout << "\nDelta time smoothing example:\n";
    std::cout << "Current: " << current_delta * 1000 << "ms, Previous: " << previous_delta * 1000 << "ms\n";
    std::cout << "Smoothed: " << smoothed * 1000 << "ms (factor: " << smoothing_factor << ")\n";
    
    std::cout << "\n";
}

/**
 * @brief Demonstrate performance monitoring
 */
void demonstrate_performance_monitoring() {
    std::cout << "--- Performance Monitoring ---\n";
    
    // Simulate performance metrics
    PerformanceMetrics metrics = {};
    metrics.min_frame_time = Duration::from_milliseconds(15);
    metrics.max_frame_time = Duration::from_milliseconds(25);
    metrics.avg_frame_time = Duration::from_milliseconds(17);
    metrics.frame_time_stddev = Duration::from_microseconds(1500);
    metrics.total_frames = 10000;
    metrics.dropped_frames = 25;
    metrics.late_frames = 150;
    metrics.frame_time_variance = 0.05;
    metrics.stability_ratio = 0.92;
    
    std::cout << "Performance metrics example:\n";
    std::cout << "Total frames: " << metrics.total_frames << "\n";
    std::cout << "Dropped frames: " << metrics.dropped_frames << "\n";
    std::cout << "Late frames: " << metrics.late_frames << "\n";
    std::cout << "Min frame time: " << metrics.min_frame_time.to_seconds() * 1000 << " ms\n";
    std::cout << "Max frame time: " << metrics.max_frame_time.to_seconds() * 1000 << " ms\n";
    std::cout << "Avg frame time: " << metrics.avg_frame_time.to_seconds() * 1000 << " ms\n";
    std::cout << "Frame time std dev: " << metrics.frame_time_stddev.to_seconds() * 1000 << " ms\n";
    std::cout << "Stability ratio: " << metrics.stability_ratio << "\n";
    std::cout << "Efficiency: " << metrics.get_efficiency() << "%\n";
    
    std::cout << "\nPerformance monitoring use cases:\n";
    std::cout << "- Real-time performance analysis\n";
    std::cout << "- Automatic quality adjustment\n";
    std::cout << "- Performance regression detection\n";
    std::cout << "- System load monitoring\n";
    
    std::cout << "\nMonitoring callback pattern:\n";
    std::cout << "1. Register performance callback with update interval\n";
    std::cout << "2. Callback receives metrics periodically\n";
    std::cout << "3. Application can adjust settings based on metrics\n";
    std::cout << "4. Unregister callback when done\n";
    
    std::cout << "\n";
}

/**
 * @brief Demonstrate timing utility functions
 */
void demonstrate_timing_utilities() {
    std::cout << "--- Timing Utilities ---\n";
    
    // FPS calculations
    auto frame_60fps = time::calculate_frame_duration(60.0);
    auto frame_30fps = time::calculate_frame_duration(30.0);
    
    std::cout << "Frame duration calculations:\n";
    std::cout << "60 FPS frame duration: " << frame_60fps.to_seconds() * 1000 << " ms\n";
    std::cout << "30 FPS frame duration: " << frame_30fps.to_seconds() * 1000 << " ms\n";
    
    auto fps_from_16ms = time::calculate_fps(Duration::from_milliseconds(16));
    auto fps_from_33ms = time::calculate_fps(Duration::from_milliseconds(33));
    
    std::cout << "FPS from frame duration:\n";
    std::cout << "16ms frame -> " << fps_from_16ms << " FPS\n";
    std::cout << "33ms frame -> " << fps_from_33ms << " FPS\n";
    
    // Delta time clamping
    double unclamped_delta = 0.25; // 250ms frame (very bad)
    double clamped_delta = time::clamp_delta_time(unclamped_delta, 0.001, 0.1);
    
    std::cout << "\nDelta time clamping:\n";
    std::cout << "Unclamped: " << unclamped_delta * 1000 << " ms\n";
    std::cout << "Clamped (1-100ms): " << clamped_delta * 1000 << " ms\n";
    
    // Timestamp compatibility
    Timestamp mono1 = {1000, ClockType::Monotonic, TimePrecision::Nanosecond};
    Timestamp mono2 = {2000, ClockType::Monotonic, TimePrecision::Nanosecond};
    Timestamp real1 = {1000, ClockType::Realtime, TimePrecision::Nanosecond};
    
    std::cout << "\nTimestamp compatibility:\n";
    std::cout << "Monotonic + Monotonic: " << (time::are_timestamps_compatible(mono1, mono2) ? "compatible" : "incompatible") << "\n";
    std::cout << "Monotonic + Realtime: " << (time::are_timestamps_compatible(mono1, real1) ? "compatible" : "incompatible") << "\n";
    
    // Precision conversion
    std::cout << "\nPrecision conversion utilities:\n";
    std::cout << "Nanosecond multiplier: " << time::get_precision_multiplier(TimePrecision::Nanosecond) << "\n";
    std::cout << "Microsecond multiplier: " << time::get_precision_multiplier(TimePrecision::Microsecond) << "\n";
    std::cout << "Millisecond multiplier: " << time::get_precision_multiplier(TimePrecision::Millisecond) << "\n";
    
    std::cout << "\n";
}

/**
 * @brief Print timing capabilities (conceptual)
 */
void print_timing_capabilities(const ITimeInterface& time_interface) {
    std::cout << "--- Platform Timing Capabilities ---\n";
    
    // This would be implemented with actual interface calls
    std::cout << "Available clock types:\n";
    std::cout << "- " << time::to_string(ClockType::Monotonic) << ": " 
              << "✓" << "\n"; // time_interface.supports_clock_type(ClockType::Monotonic)
    std::cout << "- " << time::to_string(ClockType::HighResolution) << ": " 
              << "✓" << "\n";
    std::cout << "- " << time::to_string(ClockType::Realtime) << ": " 
              << "✓" << "\n";
    
    std::cout << "\nSupported precisions:\n";
    std::cout << "- " << time::to_string(TimePrecision::Nanosecond) << ": " 
              << "✓" << "\n";
    std::cout << "- " << time::to_string(TimePrecision::Microsecond) << ": " 
              << "✓" << "\n";
    std::cout << "- " << time::to_string(TimePrecision::Millisecond) << ": " 
              << "✓" << "\n";
    
    std::cout << "\nSupported sleep modes:\n";
    std::cout << "- " << time::to_string(SleepMode::Busy) << ": " 
              << "✓" << "\n";
    std::cout << "- " << time::to_string(SleepMode::Yield) << ": " 
              << "✓" << "\n";
    std::cout << "- " << time::to_string(SleepMode::Block) << ": " 
              << "✓" << "\n";
    
    std::cout << "\n";
}
