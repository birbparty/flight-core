/**
 * @file integration_test_base.hpp
 * @brief Base class and infrastructure for HAL integration tests
 * 
 * This file provides the foundational framework for integration testing across
 * multiple HAL drivers, enabling complex scenario testing, real-world simulation,
 * and comprehensive validation of cross-driver interactions.
 */

#pragma once

#include "../../../include/flight/hal/core/hal_result.hpp"
#include "../../../include/flight/hal/core/hal_error.hpp"
#include "../../../include/flight/hal/core/hal_logging.hpp"
#include "../../../include/flight/hal/coordination/resource_handle.hpp"
#include "../../../include/flight/hal/coordination/cross_driver_messenger.hpp"
#include "../../../include/flight/hal/coordination/deadlock_prevention.hpp"
#include "../../../include/flight/hal/core/platform_detection.hpp"
#include "../../../include/flight/hal/core/driver_registry.hpp"

#include <chrono>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <map>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace flight::hal::integration {

/**
 * @brief Test result status enumeration
 */
enum class TestStatus {
    NotRun,
    Running,
    Passed,
    Failed,
    Skipped,
    Timeout
};

/**
 * @brief Test execution metrics
 */
struct TestMetrics {
    std::chrono::milliseconds execution_time{0};
    std::chrono::milliseconds setup_time{0};
    std::chrono::milliseconds teardown_time{0};
    size_t memory_used_bytes{0};
    size_t peak_memory_bytes{0};
    uint32_t resource_acquisitions{0};
    uint32_t resource_conflicts{0};
    uint32_t messages_sent{0};
    uint32_t messages_received{0};
    uint32_t error_count{0};
    uint32_t warning_count{0};
    std::map<std::string, double> custom_metrics;
};

/**
 * @brief Test result information
 */
struct TestResult {
    TestStatus status{TestStatus::NotRun};
    std::string test_name;
    std::string description;
    std::string error_message;
    TestMetrics metrics;
    std::vector<std::string> log_messages;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    
    bool passed() const { return status == TestStatus::Passed; }
    bool failed() const { return status == TestStatus::Failed; }
    bool completed() const { return status == TestStatus::Passed || status == TestStatus::Failed; }
};

/**
 * @brief Driver state snapshot for test validation
 */
struct DriverState {
    std::string driver_id;
    std::map<std::string, std::string> properties;
    std::vector<std::string> held_resources;
    bool is_active{false};
    uint32_t message_queue_size{0};
    std::chrono::milliseconds last_activity{0};
};

/**
 * @brief Test scenario configuration
 */
struct TestScenario {
    std::string name;
    std::string description;
    std::vector<std::string> required_drivers;
    std::vector<std::string> required_resources;
    std::map<std::string, std::string> configuration;
    std::chrono::milliseconds timeout{30000}; // 30 second default
    bool requires_real_hardware{false};
    std::vector<std::string> platform_restrictions;
};

/**
 * @brief Multi-driver coordination context
 */
class DriverCoordinator {
public:
    DriverCoordinator() = default;
    ~DriverCoordinator() = default;
    
    /**
     * @brief Initialize coordination system
     */
    HALResult<void> initialize();
    
    /**
     * @brief Shutdown coordination system
     */
    void shutdown();
    
    /**
     * @brief Register a driver for coordination
     */
    HALResult<void> register_driver(const std::string& driver_id, 
                                   std::shared_ptr<coordination::IMessageHandler> handler);
    
    /**
     * @brief Unregister a driver
     */
    void unregister_driver(const std::string& driver_id);
    
    /**
     * @brief Send message between drivers
     */
    HALResult<void> send_message(const std::string& from_driver,
                                const std::string& to_driver,
                                std::unique_ptr<coordination::IMessagePayload> payload);
    
    /**
     * @brief Wait for all pending messages to be processed
     */
    HALResult<void> wait_for_message_processing(std::chrono::milliseconds timeout);
    
    /**
     * @brief Get current state of all registered drivers
     */
    std::vector<DriverState> get_driver_states();
    
    /**
     * @brief Check if system is in a stable state
     */
    bool is_system_stable();

private:
    std::map<std::string, std::shared_ptr<coordination::IMessageHandler>> drivers_;
    std::mutex coordinator_mutex_;
    bool initialized_{false};
};

/**
 * @brief Base class for all integration tests
 */
class IntegrationTestBase {
public:
    explicit IntegrationTestBase(const std::string& test_name);
    virtual ~IntegrationTestBase() = default;
    
    /**
     * @brief Run the integration test
     */
    TestResult run();
    
    /**
     * @brief Get test scenario information
     */
    virtual TestScenario get_scenario() const = 0;
    
    /**
     * @brief Check if test can run on current platform
     */
    virtual bool can_run_on_platform() const;
    
    /**
     * @brief Get test name
     */
    const std::string& get_name() const { return test_name_; }
    
    /**
     * @brief Set test timeout
     */
    void set_timeout(std::chrono::milliseconds timeout) { timeout_ = timeout; }

protected:
    /**
     * @brief Setup test environment - override in derived classes
     */
    virtual HALResult<void> setup_test() { return HALResult<void>::success(); }
    
    /**
     * @brief Execute the actual test - must be implemented by derived classes
     */
    virtual HALResult<void> execute_test() = 0;
    
    /**
     * @brief Cleanup test environment - override in derived classes
     */
    virtual HALResult<void> teardown_test() { return HALResult<void>::success(); }
    
    /**
     * @brief Validate test preconditions
     */
    virtual HALResult<void> validate_preconditions();
    
    /**
     * @brief Validate test postconditions
     */
    virtual HALResult<void> validate_postconditions();
    
    // Utility methods for derived tests
    
    /**
     * @brief Wait for condition with timeout
     */
    bool wait_for_condition(std::function<bool()> condition, 
                           std::chrono::milliseconds timeout);
    
    /**
     * @brief Assert condition with custom message
     */
    void assert_condition(bool condition, const std::string& message);
    
    /**
     * @brief Record custom metric
     */
    void record_metric(const std::string& name, double value);
    
    /**
     * @brief Add log message
     */
    void log_message(const std::string& message);
    
    /**
     * @brief Get driver coordinator
     */
    DriverCoordinator& get_coordinator() { return coordinator_; }
    
    /**
     * @brief Get current test metrics
     */
    const TestMetrics& get_metrics() const { return current_result_.metrics; }

private:
    std::string test_name_;
    std::chrono::milliseconds timeout_{30000}; // 30 second default
    TestResult current_result_;
    DriverCoordinator coordinator_;
    std::atomic<bool> test_running_{false};
    std::mutex log_mutex_;
    
    void initialize_test_result();
    void finalize_test_result();
    HALResult<void> run_with_timeout();
};

/**
 * @brief Test registry for automatic test discovery
 */
class IntegrationTestRegistry {
public:
    static IntegrationTestRegistry& instance();
    
    /**
     * @brief Register a test factory function
     */
    void register_test(const std::string& test_name, 
                      std::function<std::unique_ptr<IntegrationTestBase>()> factory);
    
    /**
     * @brief Get all registered test names
     */
    std::vector<std::string> get_test_names() const;
    
    /**
     * @brief Create test instance by name
     */
    std::unique_ptr<IntegrationTestBase> create_test(const std::string& test_name);
    
    /**
     * @brief Run all registered tests
     */
    std::vector<TestResult> run_all_tests();
    
    /**
     * @brief Run tests matching pattern
     */
    std::vector<TestResult> run_tests_matching(const std::string& pattern);

private:
    IntegrationTestRegistry() = default;
    std::map<std::string, std::function<std::unique_ptr<IntegrationTestBase>()>> test_factories_;
    std::mutex registry_mutex_;
};

/**
 * @brief Macro for automatic test registration
 */
#define REGISTER_INTEGRATION_TEST(TestClass, TestName) \
    namespace { \
        struct TestClass##_registrar { \
            TestClass##_registrar() { \
                IntegrationTestRegistry::instance().register_test( \
                    TestName, \
                    []() -> std::unique_ptr<IntegrationTestBase> { \
                        return std::make_unique<TestClass>(); \
                    } \
                ); \
            } \
        }; \
        static TestClass##_registrar TestClass##_registrar_instance; \
    }

/**
 * @brief Test suite runner with reporting
 */
class IntegrationTestRunner {
public:
    IntegrationTestRunner() = default;
    
    /**
     * @brief Run specific test
     */
    TestResult run_test(const std::string& test_name);
    
    /**
     * @brief Run all tests
     */
    std::vector<TestResult> run_all_tests();
    
    /**
     * @brief Run test suite
     */
    std::vector<TestResult> run_test_suite(const std::vector<std::string>& test_names);
    
    /**
     * @brief Generate test report
     */
    std::string generate_report(const std::vector<TestResult>& results);
    
    /**
     * @brief Set verbose output
     */
    void set_verbose(bool verbose) { verbose_ = verbose; }

private:
    bool verbose_{false};
    void print_test_progress(const TestResult& result);
    std::string format_test_result(const TestResult& result);
};

} // namespace flight::hal::integration
