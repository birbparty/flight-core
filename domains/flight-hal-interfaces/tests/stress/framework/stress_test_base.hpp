/**
 * @file stress_test_base.hpp
 * @brief Base Class for HAL Stress Testing Framework
 * 
 * Provides comprehensive stress testing infrastructure with configurable
 * test phases, resource monitoring, and performance metrics collection.
 */

#pragma once

#include "../../../include/flight/hal/core/hal_result.hpp"
#include "../../../include/flight/hal/core/hal_error.hpp"
#include "stress_metrics_collector.hpp"
#include <chrono>
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <functional>

namespace flight::hal::test::stress {

/**
 * @brief Stress test execution phases
 */
enum class StressPhase {
    Setup,          ///< Test initialization
    RampUp,         ///< Gradual load increase
    SustainedLoad,  ///< Steady-state stress
    PeakStress,     ///< Maximum stress condition
    Recovery,       ///< Post-stress recovery
    Teardown        ///< Test cleanup
};

/**
 * @brief Stress test severity levels
 */
enum class StressSeverity {
    Light,      ///< Development testing, quick feedback
    Standard,   ///< CI/CD integration, moderate duration
    Heavy,      ///< Pre-release validation, intensive scenarios
    Extreme,    ///< Maximum stress, long duration
    Custom      ///< User-defined parameters
};

/**
 * @brief Stress test configuration
 */
struct StressTestConfig {
    std::string test_name;
    StressSeverity severity = StressSeverity::Standard;
    
    // Duration settings
    std::chrono::seconds total_duration{30};
    std::chrono::seconds ramp_up_duration{5};
    std::chrono::seconds sustained_duration{15};
    std::chrono::seconds peak_duration{5};
    std::chrono::seconds recovery_duration{5};
    
    // Resource constraints
    size_t max_memory_mb = 64;
    uint32_t max_threads = std::thread::hardware_concurrency();
    double stress_intensity = 0.8;  // 0.0 to 1.0
    
    // Test behavior
    bool enable_recovery_testing = true;
    bool enable_performance_monitoring = true;
    bool fail_on_resource_exhaustion = false;
    bool continuous_validation = true;
    
    // Platform-specific settings
    bool embedded_mode = false;
    bool real_time_constraints = false;
    std::chrono::microseconds max_response_time{1000};
};

/**
 * @brief Stress test execution results
 */
struct StressTestResult {
    std::string test_name;
    bool success = false;
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point end_time;
    
    // Performance metrics
    StressMetrics initial_metrics;
    StressMetrics peak_metrics;
    StressMetrics final_metrics;
    
    // Failure information
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    size_t total_operations = 0;
    size_t failed_operations = 0;
    
    // Resource usage
    size_t peak_memory_usage = 0;
    uint32_t peak_thread_count = 0;
    double peak_cpu_usage = 0.0;
    
    // Recovery metrics
    std::chrono::milliseconds recovery_time{0};
    bool successful_recovery = false;
};

/**
 * @brief Base class for all HAL stress tests
 * 
 * Provides comprehensive stress testing framework with:
 * - Configurable test phases and duration
 * - Resource monitoring and constraint enforcement
 * - Performance metrics collection
 * - Automated recovery testing
 * - Detailed result reporting
 */
class StressTestBase {
public:
    explicit StressTestBase(const std::string& test_name) 
        : test_name_(test_name), running_(false), should_stop_(false) {
        config_.test_name = test_name;
        metrics_collector_ = std::make_unique<StressMetricsCollector>();
    }
    
    virtual ~StressTestBase() = default;
    
    /**
     * @brief Configure stress test parameters
     */
    void configure(const StressTestConfig& config) {
        config_ = config;
        config_.test_name = test_name_; // Preserve test name
    }
    
    /**
     * @brief Execute the complete stress test
     */
    StressTestResult execute() {
        result_ = StressTestResult{};
        result_.test_name = test_name_;
        result_.start_time = std::chrono::steady_clock::now();
        
        // Starting stress test
        
        try {
            running_ = true;
            should_stop_ = false;
            
            // Execute test phases
            if (!execute_phase(StressPhase::Setup)) {
                throw std::runtime_error("Setup phase failed");
            }
            
            // Start metrics collection
            metrics_collector_->start_collection();
            result_.initial_metrics = metrics_collector_->get_current_metrics();
            
            // Execute stress phases
            execute_phase(StressPhase::RampUp);
            execute_phase(StressPhase::SustainedLoad);
            execute_phase(StressPhase::PeakStress);
            
            // Capture peak metrics
            result_.peak_metrics = metrics_collector_->get_current_metrics();
            update_peak_usage();
            
            // Recovery phase
            if (config_.enable_recovery_testing) {
                auto recovery_start = std::chrono::steady_clock::now();
                execute_phase(StressPhase::Recovery);
                auto recovery_end = std::chrono::steady_clock::now();
                result_.recovery_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                    recovery_end - recovery_start);
                result_.successful_recovery = validate_recovery();
            }
            
            execute_phase(StressPhase::Teardown);
            
            // Final metrics
            result_.final_metrics = metrics_collector_->get_current_metrics();
            metrics_collector_->stop_collection();
            
            result_.success = (result_.errors.empty() || !config_.fail_on_resource_exhaustion);
            
        } catch (const std::exception& e) {
            result_.errors.push_back(std::string("Exception: ") + e.what());
            result_.success = false;
            // Stress test failed
        }
        
        running_ = false;
        result_.end_time = std::chrono::steady_clock::now();
        
        // Stress test completed
        return result_;
    }
    
    /**
     * @brief Stop the stress test gracefully
     */
    void stop() {
        should_stop_ = true;
    }
    
    /**
     * @brief Check if test is currently running
     */
    bool is_running() const {
        return running_;
    }
    
    /**
     * @brief Get current test configuration
     */
    const StressTestConfig& get_config() const {
        return config_;
    }
    
    /**
     * @brief Get test results (available during and after execution)
     */
    const StressTestResult& get_result() const {
        return result_;
    }

protected:
    /**
     * @brief Execute a specific test phase (override in derived classes)
     */
    virtual bool execute_phase(StressPhase phase) {
        switch (phase) {
            case StressPhase::Setup:
                return setup_test();
            case StressPhase::RampUp:
                return execute_ramp_up();
            case StressPhase::SustainedLoad:
                return execute_sustained_load();
            case StressPhase::PeakStress:
                return execute_peak_stress();
            case StressPhase::Recovery:
                return execute_recovery();
            case StressPhase::Teardown:
                return teardown_test();
        }
        return false;
    }
    
    /**
     * @brief Test-specific setup (override in derived classes)
     */
    virtual bool setup_test() = 0;
    
    /**
     * @brief Test-specific teardown (override in derived classes)
     */
    virtual bool teardown_test() = 0;
    
    /**
     * @brief Execute ramp-up phase (override for custom behavior)
     */
    virtual bool execute_ramp_up() {
        return execute_timed_phase("RampUp", config_.ramp_up_duration, 
                                 [this](double progress) {
                                     return execute_stress_iteration(progress * config_.stress_intensity);
                                 });
    }
    
    /**
     * @brief Execute sustained load phase (override for custom behavior)
     */
    virtual bool execute_sustained_load() {
        return execute_timed_phase("SustainedLoad", config_.sustained_duration,
                                 [this](double progress) {
                                     return execute_stress_iteration(config_.stress_intensity);
                                 });
    }
    
    /**
     * @brief Execute peak stress phase (override for custom behavior)
     */
    virtual bool execute_peak_stress() {
        return execute_timed_phase("PeakStress", config_.peak_duration,
                                 [this](double progress) {
                                     return execute_stress_iteration(1.0);
                                 });
    }
    
    /**
     * @brief Execute recovery phase (override for custom behavior)
     */
    virtual bool execute_recovery() {
        return execute_timed_phase("Recovery", config_.recovery_duration,
                                 [this](double progress) {
                                     return execute_stress_iteration((1.0 - progress) * 0.1);
                                 });
    }
    
    /**
     * @brief Execute a single stress iteration (must override in derived classes)
     * @param intensity Stress intensity from 0.0 to 1.0
     */
    virtual bool execute_stress_iteration(double intensity) = 0;
    
    /**
     * @brief Validate system recovery after stress (override for custom validation)
     */
    virtual bool validate_recovery() {
        // Default implementation checks if metrics returned to near-initial values
        auto current = metrics_collector_->get_current_metrics();
        
        // Memory should be within 10% of initial
        double memory_ratio = static_cast<double>(current.memory_usage) / 
                             std::max(result_.initial_metrics.memory_usage, 1UL);
        if (memory_ratio > 1.1) {
            result_.warnings.push_back("Memory usage didn't recover to initial levels");
            return false;
        }
        
        return true;
    }
    
    /**
     * @brief Check if test should continue running
     */
    bool should_continue() const {
        return !should_stop_ && running_;
    }
    
    /**
     * @brief Record an error
     */
    void record_error(const std::string& error) {
        result_.errors.push_back(error);
        // Stress test error recorded
    }
    
    /**
     * @brief Record a warning
     */
    void record_warning(const std::string& warning) {
        result_.warnings.push_back(warning);
        // Stress test warning recorded
    }
    
    /**
     * @brief Increment operation counters
     */
    void record_operation(bool success) {
        result_.total_operations++;
        if (!success) {
            result_.failed_operations++;
        }
    }
    
    /**
     * @brief Check resource constraints
     */
    bool check_resource_constraints() {
        auto metrics = metrics_collector_->get_current_metrics();
        
        // Check memory limit
        if (metrics.memory_usage > config_.max_memory_mb * 1024 * 1024) {
            if (config_.fail_on_resource_exhaustion) {
                record_error("Memory limit exceeded");
                return false;
            } else {
                record_warning("Memory limit exceeded");
            }
        }
        
        // Check thread count
        if (metrics.thread_count > config_.max_threads) {
            if (config_.fail_on_resource_exhaustion) {
                record_error("Thread limit exceeded");
                return false;
            } else {
                record_warning("Thread limit exceeded");
            }
        }
        
        // Check response time (if real-time constraints enabled)
        if (config_.real_time_constraints && 
            metrics.avg_response_time > config_.max_response_time) {
            record_error("Real-time constraint violation");
            return false;
        }
        
        return true;
    }
    
    /**
     * @brief Get current metrics collector
     */
    StressMetricsCollector* get_metrics_collector() {
        return metrics_collector_.get();
    }

private:
    /**
     * @brief Execute a timed test phase
     */
    bool execute_timed_phase(const std::string& phase_name, 
                           std::chrono::seconds duration,
                           std::function<bool(double)> phase_function) {
        // Executing phase
        
        auto start_time = std::chrono::steady_clock::now();
        auto end_time = start_time + duration;
        
        while (std::chrono::steady_clock::now() < end_time && should_continue()) {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = current_time - start_time;
            double progress = static_cast<double>(elapsed.count()) / duration.count();
            
            if (!phase_function(progress)) {
                record_error(phase_name + " phase iteration failed");
                return false;
            }
            
            if (config_.continuous_validation && !check_resource_constraints()) {
                return false;
            }
            
            // Small delay to prevent excessive CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        return true;
    }
    
    /**
     * @brief Update peak resource usage tracking
     */
    void update_peak_usage() {
        auto metrics = metrics_collector_->get_current_metrics();
        result_.peak_memory_usage = std::max(result_.peak_memory_usage, metrics.memory_usage);
        result_.peak_thread_count = std::max(result_.peak_thread_count, metrics.thread_count);
        result_.peak_cpu_usage = std::max(result_.peak_cpu_usage, metrics.cpu_usage);
    }
    
    std::string test_name_;
    StressTestConfig config_;
    StressTestResult result_;
    std::unique_ptr<StressMetricsCollector> metrics_collector_;
    
    std::atomic<bool> running_;
    std::atomic<bool> should_stop_;
};

/**
 * @brief Helper function to create stress test configurations
 */
namespace config_presets {

/**
 * @brief Light stress test configuration for development
 */
inline StressTestConfig light_stress(const std::string& test_name) {
    StressTestConfig config;
    config.test_name = test_name;
    config.severity = StressSeverity::Light;
    config.total_duration = std::chrono::seconds{10};
    config.ramp_up_duration = std::chrono::seconds{2};
    config.sustained_duration = std::chrono::seconds{5};
    config.peak_duration = std::chrono::seconds{2};
    config.recovery_duration = std::chrono::seconds{1};
    config.stress_intensity = 0.5;
    return config;
}

/**
 * @brief Standard stress test configuration for CI/CD
 */
inline StressTestConfig standard_stress(const std::string& test_name) {
    StressTestConfig config;
    config.test_name = test_name;
    config.severity = StressSeverity::Standard;
    return config; // Uses default values
}

/**
 * @brief Heavy stress test configuration for validation
 */
inline StressTestConfig heavy_stress(const std::string& test_name) {
    StressTestConfig config;
    config.test_name = test_name;
    config.severity = StressSeverity::Heavy;
    config.total_duration = std::chrono::seconds{120};
    config.ramp_up_duration = std::chrono::seconds{20};
    config.sustained_duration = std::chrono::seconds{60};
    config.peak_duration = std::chrono::seconds{30};
    config.recovery_duration = std::chrono::seconds{10};
    config.stress_intensity = 0.9;
    return config;
}

/**
 * @brief Embedded system stress test configuration
 */
inline StressTestConfig embedded_stress(const std::string& test_name) {
    StressTestConfig config;
    config.test_name = test_name;
    config.severity = StressSeverity::Standard;
    config.embedded_mode = true;
    config.max_memory_mb = 8; // Limited memory
    config.max_threads = 2;   // Limited threads
    config.real_time_constraints = true;
    config.max_response_time = std::chrono::microseconds{100};
    config.fail_on_resource_exhaustion = true;
    return config;
}

} // namespace config_presets

} // namespace flight::hal::test::stress
