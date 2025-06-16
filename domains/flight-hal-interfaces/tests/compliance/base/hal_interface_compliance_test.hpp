/**
 * @file hal_interface_compliance_test.hpp
 * @brief Base Template Class for HAL Interface Compliance Testing
 * 
 * Provides comprehensive testing framework for validating HAL interface
 * implementations across all target platforms with compliance, performance,
 * and error handling validation.
 */

#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <chrono>
#include <functional>
#include <vector>
#include <string>
#include <type_traits>
#include <thread>
#include <algorithm>

#include "../../../include/flight/hal/core/hal_result.hpp"
#include "../../../include/flight/hal/core/hal_error.hpp"
#include "../../../include/flight/hal/core/platform_detection.hpp"
#include "../../../include/flight/hal/core/platform_capabilities.hpp"

namespace flight::hal::testing {

/**
 * @brief Test configuration for platform-specific adaptations
 */
struct TestConfig {
    std::chrono::milliseconds timeout{5000};           ///< Test timeout
    size_t max_memory_usage{1024 * 1024 * 100};       ///< Max memory usage (100MB default)
    size_t max_iterations{10000};                      ///< Max test iterations
    bool enable_stress_testing{true};                  ///< Enable stress tests
    bool enable_performance_testing{true};             ///< Enable performance tests
    bool enable_error_injection{true};                 ///< Enable error injection
    std::string platform_name{"unknown"};              ///< Platform identifier
    
    // Platform-specific limits
    struct PlatformLimits {
        size_t max_allocation_size{1024 * 1024 * 10};  ///< Max single allocation
        std::chrono::microseconds max_latency{1000};    ///< Max acceptable latency
        double min_throughput{1000.0};                  ///< Min throughput (ops/sec)
        size_t max_concurrent_operations{100};          ///< Max concurrent ops
    } limits;
};

/**
 * @brief Performance measurement result
 */
struct PerformanceMetrics {
    std::chrono::nanoseconds min_latency{};
    std::chrono::nanoseconds max_latency{};
    std::chrono::nanoseconds avg_latency{};
    std::chrono::nanoseconds p95_latency{};
    std::chrono::nanoseconds p99_latency{};
    double throughput_ops_per_sec{0.0};
    size_t total_operations{0};
    size_t failed_operations{0};
    size_t memory_peak_usage{0};
    bool passed_sla{false};
};

/**
 * @brief Error injection types for testing error handling
 */
enum class ErrorInjectionType {
    None = 0,
    ResourceExhaustion,     ///< Simulate resource exhaustion
    MemoryPressure,         ///< Simulate memory pressure
    NetworkFailure,         ///< Simulate network failure
    HardwareFailure,        ///< Simulate hardware failure
    TimeoutFailure,         ///< Simulate timeout conditions
    InvalidParameter,       ///< Simulate invalid parameters
    ConcurrencyFailure      ///< Simulate concurrency issues
};

/**
 * @brief Base class for all HAL interface compliance tests
 * 
 * Template class that provides common testing infrastructure, utilities,
 * and patterns for validating HAL interface implementations.
 * 
 * @tparam InterfaceType The HAL interface type being tested
 */
template<typename InterfaceType>
class HALInterfaceComplianceTest : public ::testing::Test {
    static_assert(std::is_base_of_v<IHALInterface, InterfaceType>, 
                  "InterfaceType must inherit from IHALInterface");

protected:
    using Interface = InterfaceType;
    using InterfacePtr = std::unique_ptr<InterfaceType>;
    
    /**
     * @brief Test fixture setup
     */
    void SetUp() override {
        // Initialize platform detection
        auto platform_result = PlatformDetection::detect_current_platform();
        ASSERT_TRUE(platform_result.is_ok()) << "Failed to detect platform";
        current_platform_ = platform_result.value();
        
        // Get platform capabilities
        auto caps_result = PlatformCapabilities::get_capabilities(current_platform_);
        ASSERT_TRUE(caps_result.is_ok()) << "Failed to get platform capabilities";
        platform_capabilities_ = caps_result.value();
        
        // Configure test settings for platform
        test_config_ = get_test_config();
        test_config_.platform_name = to_string(current_platform_);
        
        // Apply platform-specific limits
        apply_platform_limits();
        
        // Create interface instance
        interface_ = create_interface();
        ASSERT_TRUE(interface_ != nullptr) << "Failed to create interface instance";
        
        // Initialize performance tracking
        performance_data_.clear();
        error_injection_active_ = false;
        
        // Record test start time
        test_start_time_ = std::chrono::high_resolution_clock::now();
    }
    
    /**
     * @brief Test fixture teardown
     */
    void TearDown() override {
        // Clean up resources
        cleanup_test_resources();
        
        // Validate no resource leaks
        validate_resource_cleanup();
        
        // Record test end time
        auto test_end_time = std::chrono::high_resolution_clock::now();
        auto test_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            test_end_time - test_start_time_);
        
        // Ensure test didn't exceed timeout
        EXPECT_LT(test_duration, test_config_.timeout) 
            << "Test exceeded configured timeout";
        
        // Reset interface
        interface_.reset();
    }
    
    // === Virtual methods for derived classes to implement ===
    
    /**
     * @brief Create interface instance for testing
     * Must be implemented by derived test classes
     * @return Unique pointer to interface instance
     */
    virtual InterfacePtr create_interface() = 0;
    
    /**
     * @brief Get test configuration for this interface
     * Can be overridden by derived classes for custom configuration
     * @return Test configuration structure
     */
    virtual TestConfig get_test_config() {
        return TestConfig{};  // Default configuration
    }
    
    /**
     * @brief Cleanup test-specific resources
     * Can be overridden by derived classes
     */
    virtual void cleanup_test_resources() {
        // Default: no additional cleanup needed
    }
    
    /**
     * @brief Validate resource cleanup
     * Can be overridden by derived classes
     */
    virtual void validate_resource_cleanup() {
        // Default: no additional validation needed
    }
    
    // === Common test utilities ===
    
    /**
     * @brief Measure operation latency
     * @param operation Function to measure
     * @return Latency in nanoseconds
     */
    template<typename Func>
    std::chrono::nanoseconds measure_latency(Func&& operation) {
        auto start = std::chrono::high_resolution_clock::now();
        operation();
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    }
    
    /**
     * @brief Measure operation throughput
     * @param operation Function to measure
     * @param iterations Number of iterations
     * @return Performance metrics
     */
    template<typename Func>
    PerformanceMetrics measure_throughput(Func&& operation, size_t iterations = 1000) {
        std::vector<std::chrono::nanoseconds> latencies;
        latencies.reserve(iterations);
        
        size_t successful_ops = 0;
        size_t failed_ops = 0;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < iterations; ++i) {
            try {
                auto latency = measure_latency(operation);
                latencies.push_back(latency);
                ++successful_ops;
            } catch (...) {
                ++failed_ops;
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto total_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end_time - start_time);
        
        // Calculate metrics
        PerformanceMetrics metrics;
        metrics.total_operations = iterations;
        metrics.failed_operations = failed_ops;
        
        if (!latencies.empty()) {
            std::sort(latencies.begin(), latencies.end());
            
            metrics.min_latency = latencies.front();
            metrics.max_latency = latencies.back();
            
            // Calculate average
            auto total_latency = std::chrono::nanoseconds::zero();
            for (const auto& lat : latencies) {
                total_latency += lat;
            }
            metrics.avg_latency = total_latency / latencies.size();
            
            // Calculate percentiles
            auto p95_idx = static_cast<size_t>(latencies.size() * 0.95);
            auto p99_idx = static_cast<size_t>(latencies.size() * 0.99);
            metrics.p95_latency = latencies[std::min(p95_idx, latencies.size() - 1)];
            metrics.p99_latency = latencies[std::min(p99_idx, latencies.size() - 1)];
            
            // Calculate throughput
            if (total_duration.count() > 0) {
                double duration_seconds = total_duration.count() / 1e9;
                metrics.throughput_ops_per_sec = successful_ops / duration_seconds;
            }
            
            // Check SLA compliance
            metrics.passed_sla = (metrics.p95_latency <= test_config_.limits.max_latency) &&
                                (metrics.throughput_ops_per_sec >= test_config_.limits.min_throughput);
        }
        
        performance_data_.push_back(metrics);
        return metrics;
    }
    
    /**
     * @brief Enable error injection for testing error handling
     * @param type Type of error to inject
     */
    void enable_error_injection(ErrorInjectionType type) {
        if (test_config_.enable_error_injection) {
            error_injection_type_ = type;
            error_injection_active_ = true;
        }
    }
    
    /**
     * @brief Disable error injection
     */
    void disable_error_injection() {
        error_injection_active_ = false;
        error_injection_type_ = ErrorInjectionType::None;
    }
    
    /**
     * @brief Check if error injection is active
     * @return true if error injection is enabled
     */
    bool is_error_injection_active() const {
        return error_injection_active_;
    }
    
    /**
     * @brief Get current error injection type
     * @return Current error injection type
     */
    ErrorInjectionType get_error_injection_type() const {
        return error_injection_type_;
    }
    
    /**
     * @brief Validate HALResult compliance
     * @param result HALResult to validate
     * @param operation_name Name of operation for error reporting
     */
    template<typename T, typename E>
    void validate_hal_result(const HALResult<T, E>& result, 
                           const std::string& operation_name) {
        // Check that result is in valid state
        EXPECT_TRUE(result.is_ok() || result.is_err()) 
            << operation_name << ": HALResult must be either ok() or err()";
        
        // If error injection is active, we might expect errors
        if (error_injection_active_) {
            // Allow both success and failure during error injection
            return;
        }
        
        // Otherwise, expect success for basic operations
        if (result.is_err()) {
            GTEST_NONFATAL_FAILURE_(("Operation failed: " + operation_name + 
                                   " - Error: " + std::to_string(static_cast<int>(result.error()))).c_str());
        }
    }
    
    /**
     * @brief Run stress test with concurrent operations
     * @param operation Function to run concurrently
     * @param num_threads Number of concurrent threads
     * @param iterations_per_thread Iterations per thread
     */
    template<typename Func>
    void run_stress_test(Func&& operation, 
                        size_t num_threads = 4, 
                        size_t iterations_per_thread = 100) {
        if (!test_config_.enable_stress_testing) {
            GTEST_SKIP() << "Stress testing disabled";
            return;
        }
        
        std::vector<std::thread> threads;
        std::vector<std::exception_ptr> exceptions(num_threads);
        
        // Launch threads
        for (size_t i = 0; i < num_threads; ++i) {
            threads.emplace_back([this, &operation, &exceptions, i, iterations_per_thread]() {
                try {
                    for (size_t j = 0; j < iterations_per_thread; ++j) {
                        operation();
                    }
                } catch (...) {
                    exceptions[i] = std::current_exception();
                }
            });
        }
        
        // Wait for completion
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Check for exceptions
        for (size_t i = 0; i < num_threads; ++i) {
            if (exceptions[i]) {
                GTEST_NONFATAL_FAILURE_(("Thread " + std::to_string(i) + " threw exception").c_str());
            }
        }
    }
    
    // === Protected member access ===
    
    /**
     * @brief Get interface instance
     * @return Reference to interface instance
     */
    InterfaceType& get_interface() {
        return *interface_;
    }
    
    /**
     * @brief Get test configuration
     * @return Reference to test configuration
     */
    const TestConfig& get_config() const {
        return test_config_;
    }
    
    /**
     * @brief Get current platform
     * @return Current platform type
     */
    PlatformType get_platform() const {
        return current_platform_;
    }
    
    /**
     * @brief Get platform capabilities
     * @return Platform capabilities
     */
    const PlatformCapabilities& get_platform_capabilities() const {
        return platform_capabilities_;
    }
    
    /**
     * @brief Get performance data
     * @return Vector of performance metrics
     */
    const std::vector<PerformanceMetrics>& get_performance_data() const {
        return performance_data_;
    }

private:
    InterfacePtr interface_;
    TestConfig test_config_;
    PlatformType current_platform_;
    PlatformCapabilities platform_capabilities_;
    
    std::vector<PerformanceMetrics> performance_data_;
    bool error_injection_active_{false};
    ErrorInjectionType error_injection_type_{ErrorInjectionType::None};
    
    std::chrono::high_resolution_clock::time_point test_start_time_;
    
    /**
     * @brief Apply platform-specific test limits
     */
    void apply_platform_limits() {
        switch (current_platform_) {
            case PlatformType::Dreamcast:
                // Dreamcast: Limited memory, relaxed timing
                test_config_.limits.max_allocation_size = 1024 * 1024;  // 1MB
                test_config_.limits.max_latency = std::chrono::microseconds{5000};
                test_config_.limits.min_throughput = 100.0;
                test_config_.limits.max_concurrent_operations = 4;
                break;
                
            case PlatformType::PSP:
                // PSP: Limited memory, moderate performance
                test_config_.limits.max_allocation_size = 1024 * 1024 * 2;  // 2MB
                test_config_.limits.max_latency = std::chrono::microseconds{2000};
                test_config_.limits.min_throughput = 500.0;
                test_config_.limits.max_concurrent_operations = 8;
                break;
                
            case PlatformType::Web:
                // Web: Variable performance, security restrictions
                test_config_.limits.max_allocation_size = 1024 * 1024 * 50;  // 50MB
                test_config_.limits.max_latency = std::chrono::microseconds{10000};
                test_config_.limits.min_throughput = 200.0;
                test_config_.limits.max_concurrent_operations = 16;
                break;
                
            case PlatformType::macOS:
            case PlatformType::Linux:
            case PlatformType::Windows:
                // Desktop: High performance expectations
                test_config_.limits.max_allocation_size = 1024 * 1024 * 100;  // 100MB
                test_config_.limits.max_latency = std::chrono::microseconds{1000};
                test_config_.limits.min_throughput = 10000.0;
                test_config_.limits.max_concurrent_operations = 64;
                break;
                
            default:
                // Unknown platform: Conservative limits
                test_config_.limits.max_allocation_size = 1024 * 1024 * 10;  // 10MB
                test_config_.limits.max_latency = std::chrono::microseconds{5000};
                test_config_.limits.min_throughput = 1000.0;
                test_config_.limits.max_concurrent_operations = 16;
                break;
        }
    }
};

// === Compliance Test Macros ===

/**
 * @brief Macro for defining interface compliance tests
 */
#define HAL_COMPLIANCE_TEST(interface_name, test_name) \
    TEST_F(interface_name##ComplianceTest, Compliance_##test_name)

/**
 * @brief Macro for defining performance tests with SLA validation
 */
#define HAL_PERFORMANCE_TEST(interface_name, test_name, expected_throughput) \
    TEST_F(interface_name##ComplianceTest, Performance_##test_name) { \
        if (!get_config().enable_performance_testing) { \
            GTEST_SKIP() << "Performance testing disabled"; \
        } \
        auto metrics = measure_throughput([this]() { \
            perform_##test_name##_operation(); \
        }); \
        EXPECT_GE(metrics.throughput_ops_per_sec, expected_throughput) \
            << "Performance below expected threshold"; \
        EXPECT_TRUE(metrics.passed_sla) << "Performance SLA not met"; \
    }

/**
 * @brief Macro for defining error handling tests
 */
#define HAL_ERROR_TEST(interface_name, test_name, error_type) \
    TEST_F(interface_name##ComplianceTest, ErrorHandling_##test_name) { \
        if (!get_config().enable_error_injection) { \
            GTEST_SKIP() << "Error injection disabled"; \
        } \
        enable_error_injection(ErrorInjectionType::error_type); \
        test_##test_name##_error_handling(); \
        disable_error_injection(); \
    }

/**
 * @brief Macro for defining stress tests
 */
#define HAL_STRESS_TEST(interface_name, test_name) \
    TEST_F(interface_name##ComplianceTest, Stress_##test_name) { \
        if (!get_config().enable_stress_testing) { \
            GTEST_SKIP() << "Stress testing disabled"; \
        } \
        run_stress_test([this]() { \
            perform_##test_name##_operation(); \
        }); \
    }

} // namespace flight::hal::testing
