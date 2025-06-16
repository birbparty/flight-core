/**
 * @file integration_test_base.cpp
 * @brief Implementation of integration test base framework
 */

#include "integration_test_base.hpp"
#include "../../../include/flight/hal/core/platform_detection.hpp"

#include <regex>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <future>
#include <iostream>

namespace flight::hal::integration {

// DriverCoordinator Implementation

HALResult<void> DriverCoordinator::initialize() {
    std::lock_guard<std::mutex> lock(coordinator_mutex_);
    
    if (initialized_) {
        return HALResult<void>::success();
    }
    
    // Initialize the cross-driver messenger
    auto& messenger = coordination::CrossDriverMessenger::instance();
    auto messenger_init = messenger.initialize();
    if (!messenger_init) {
        return HALResult<void>::error(errors::internal_error(1, "Failed to initialize cross-driver messenger"));
    }
    
    // Initialize deadlock prevention
    auto& deadlock_prevention = coordination::DeadlockPrevention::instance();
    auto deadlock_init = deadlock_prevention.initialize();
    if (!deadlock_init) {
        return HALResult<void>::error(errors::internal_error(2, "Failed to initialize deadlock prevention"));
    }
    
    initialized_ = true;
    return HALResult<void>::success();
}

void DriverCoordinator::shutdown() {
    std::lock_guard<std::mutex> lock(coordinator_mutex_);
    
    if (!initialized_) {
        return;
    }
    
    // Unregister all drivers
    drivers_.clear();
    
    // Shutdown coordination systems
    coordination::CrossDriverMessenger::instance().shutdown();
    coordination::DeadlockPrevention::instance().shutdown();
    
    initialized_ = false;
}

HALResult<void> DriverCoordinator::register_driver(const std::string& driver_id,
                                                  std::shared_ptr<coordination::IMessageHandler> handler) {
    std::lock_guard<std::mutex> lock(coordinator_mutex_);
    
    if (!initialized_) {
        return HALResult<void>::error(errors::internal_error(1, "Coordinator not initialized"));
    }
    
    if (drivers_.find(driver_id) != drivers_.end()) {
        return HALResult<void>::error(errors::validation_failed(1, "Driver already registered"));
    }
    
    drivers_[driver_id] = handler;
    
    // Register with cross-driver messenger
    auto& messenger = coordination::CrossDriverMessenger::instance();
    messenger.register_handler(driver_id, handler);
    
    return HALResult<void>::success();
}

void DriverCoordinator::unregister_driver(const std::string& driver_id) {
    std::lock_guard<std::mutex> lock(coordinator_mutex_);
    
    auto it = drivers_.find(driver_id);
    if (it != drivers_.end()) {
        // Unregister from cross-driver messenger
        auto& messenger = coordination::CrossDriverMessenger::instance();
        messenger.unregister_handler(driver_id);
        
        drivers_.erase(it);
    }
}

HALResult<void> DriverCoordinator::send_message(const std::string& from_driver,
                                               const std::string& to_driver,
                                               std::unique_ptr<coordination::IMessagePayload> payload) {
    std::lock_guard<std::mutex> lock(coordinator_mutex_);
    
    if (!initialized_) {
        return HALResult<void>::error(errors::internal_error(1, "Coordinator not initialized"));
    }
    
    // Create message
    coordination::MessageHeader header;
    header.type = coordination::MessageType::Request;
    header.sender_id = from_driver;
    header.recipient_id = to_driver;
    header.priority = coordination::MessagePriority::Normal;
    
    coordination::Message message(header, std::move(payload));
    
    // Send through messenger
    auto& messenger = coordination::CrossDriverMessenger::instance();
    auto result = messenger.send_request(std::move(message), std::chrono::milliseconds(5000));
    
    if (!result) {
        return HALResult<void>::error(result.error());
    }
    
    return HALResult<void>::success();
}

HALResult<void> DriverCoordinator::wait_for_message_processing(std::chrono::milliseconds timeout) {
    auto start_time = std::chrono::steady_clock::now();
    
    while (std::chrono::steady_clock::now() - start_time < timeout) {
        if (is_system_stable()) {
            return HALResult<void>::success();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return HALResult<void>::error(errors::network_timeout(1, "Timeout waiting for message processing"));
}

std::vector<DriverState> DriverCoordinator::get_driver_states() {
    std::lock_guard<std::mutex> lock(coordinator_mutex_);
    
    std::vector<DriverState> states;
    for (const auto& [driver_id, handler] : drivers_) {
        DriverState state;
        state.driver_id = driver_id;
        state.is_active = true; // Assume active if registered
        state.message_queue_size = 0; // Would need to query actual queue size
        state.last_activity = std::chrono::milliseconds(0);
        states.push_back(state);
    }
    
    return states;
}

bool DriverCoordinator::is_system_stable() {
    std::lock_guard<std::mutex> lock(coordinator_mutex_);
    
    // Check if all drivers are in stable state
    // This is a simplified check - would need more sophisticated logic
    auto& messenger = coordination::CrossDriverMessenger::instance();
    auto stats = messenger.get_stats();
    
    // System is considered stable if no messages are pending
    return (stats.messages_sent == stats.messages_received);
}

// IntegrationTestBase Implementation

IntegrationTestBase::IntegrationTestBase(const std::string& test_name) 
    : test_name_(test_name) {
    initialize_test_result();
}

TestResult IntegrationTestBase::run() {
    current_result_.start_time = std::chrono::system_clock::now();
    current_result_.status = TestStatus::Running;
    test_running_ = true;
    
    try {
        // Check if test can run on current platform
        if (!can_run_on_platform()) {
            current_result_.status = TestStatus::Skipped;
            current_result_.error_message = "Test cannot run on current platform";
            finalize_test_result();
            return current_result_;
        }
        
        // Run test with timeout
        auto result = run_with_timeout();
        
        if (result) {
            current_result_.status = TestStatus::Passed;
        } else {
            current_result_.status = TestStatus::Failed;
            current_result_.error_message = result.error().message();
        }
        
    } catch (const std::exception& e) {
        current_result_.status = TestStatus::Failed;
        current_result_.error_message = std::string("Exception: ") + e.what();
    } catch (...) {
        current_result_.status = TestStatus::Failed;
        current_result_.error_message = "Unknown exception";
    }
    
    test_running_ = false;
    finalize_test_result();
    return current_result_;
}

bool IntegrationTestBase::can_run_on_platform() const {
    auto scenario = get_scenario();
    
    if (scenario.platform_restrictions.empty()) {
        return true; // No restrictions
    }
    
    auto platform_info = flight::hal::RuntimePlatformDetector::detect_platform_info();
    std::string current_platform = platform_info.platform_name;
    
    // Check if current platform is in allowed list
    return std::find(scenario.platform_restrictions.begin(), 
                    scenario.platform_restrictions.end(), 
                    current_platform) != scenario.platform_restrictions.end();
}

HALResult<void> IntegrationTestBase::validate_preconditions() {
    auto scenario = get_scenario();
    
    // Check required drivers are available
    for (const auto& driver_id : scenario.required_drivers) {
        // This would check if driver is available in the system
        // For now, just assume all required drivers are available
        (void)driver_id; // Suppress unused variable warning
    }
    
    // Check required resources are available
    for (const auto& resource_name : scenario.required_resources) {
        auto resource_result = coordination::ResourceRegistry::instance().find_resource(resource_name);
        if (!resource_result) {
            return HALResult<void>::error(errors::validation_failed(1, "Required resource not available"));
        }
    }
    
    return HALResult<void>::success();
}

HALResult<void> IntegrationTestBase::validate_postconditions() {
    // Check system is in stable state
    if (!coordinator_.is_system_stable()) {
        return HALResult<void>::error(errors::validation_failed(1, "System not in stable state after test"));
    }
    
    return HALResult<void>::success();
}

bool IntegrationTestBase::wait_for_condition(std::function<bool()> condition, 
                                           std::chrono::milliseconds timeout) {
    auto start_time = std::chrono::steady_clock::now();
    
    while (std::chrono::steady_clock::now() - start_time < timeout) {
        if (condition()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return false;
}

void IntegrationTestBase::assert_condition(bool condition, const std::string& message) {
    if (!condition) {
        current_result_.metrics.error_count++;
        log_message("ASSERTION FAILED: " + message);
        throw std::runtime_error("Assertion failed: " + message);
    }
}

void IntegrationTestBase::record_metric(const std::string& name, double value) {
    current_result_.metrics.custom_metrics[name] = value;
}

void IntegrationTestBase::log_message(const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%H:%M:%S") << " - " << message;
    
    current_result_.log_messages.push_back(oss.str());
}

void IntegrationTestBase::initialize_test_result() {
    current_result_.test_name = test_name_;
    current_result_.description = get_scenario().description;
    current_result_.status = TestStatus::NotRun;
    current_result_.log_messages.clear();
    current_result_.metrics = TestMetrics{};
}

void IntegrationTestBase::finalize_test_result() {
    current_result_.end_time = std::chrono::system_clock::now();
    current_result_.metrics.execution_time = 
        std::chrono::duration_cast<std::chrono::milliseconds>(
            current_result_.end_time - current_result_.start_time);
}

HALResult<void> IntegrationTestBase::run_with_timeout() {
    auto scenario = get_scenario();
    
    // Use timeout from scenario or default
    auto timeout = scenario.timeout > std::chrono::milliseconds(0) ? 
                   scenario.timeout : timeout_;
    
    // Run test in separate thread with timeout
    std::promise<HALResult<void>> promise;
    auto future = promise.get_future();
    
    std::thread test_thread([this, &promise]() {
        try {
            // Initialize coordinator
            auto init_result = coordinator_.initialize();
            if (!init_result) {
                promise.set_value(init_result);
                return;
            }
            
            // Validate preconditions
            auto precond_result = validate_preconditions();
            if (!precond_result) {
                promise.set_value(precond_result);
                return;
            }
            
            // Setup test
            auto setup_start = std::chrono::steady_clock::now();
            auto setup_result = setup_test();
            auto setup_end = std::chrono::steady_clock::now();
            
            current_result_.metrics.setup_time = 
                std::chrono::duration_cast<std::chrono::milliseconds>(setup_end - setup_start);
            
            if (!setup_result) {
                promise.set_value(setup_result);
                return;
            }
            
            // Execute test
            auto execute_result = execute_test();
            if (!execute_result) {
                promise.set_value(execute_result);
                return;
            }
            
            // Teardown test
            auto teardown_start = std::chrono::steady_clock::now();
            auto teardown_result = teardown_test();
            auto teardown_end = std::chrono::steady_clock::now();
            
            current_result_.metrics.teardown_time = 
                std::chrono::duration_cast<std::chrono::milliseconds>(teardown_end - teardown_start);
            
            if (!teardown_result) {
                promise.set_value(teardown_result);
                return;
            }
            
            // Validate postconditions
            auto postcond_result = validate_postconditions();
            if (!postcond_result) {
                promise.set_value(postcond_result);
                return;
            }
            
            // Shutdown coordinator
            coordinator_.shutdown();
            
            promise.set_value(HALResult<void>::success());
            
        } catch (const std::exception& e) {
            promise.set_value(HALResult<void>::error(errors::internal_error(1, "Test execution exception")));
        } catch (...) {
            promise.set_value(HALResult<void>::error(errors::internal_error(2, "Unknown test execution exception")));
        }
    });
    
    // Wait for completion or timeout
    if (future.wait_for(timeout) == std::future_status::timeout) {
        test_thread.detach(); // Let thread finish in background
        current_result_.status = TestStatus::Timeout;
        return HALResult<void>::error(errors::network_timeout(1, "Test execution timeout"));
    }
    
    test_thread.join();
    return future.get();
}

// IntegrationTestRegistry Implementation

IntegrationTestRegistry& IntegrationTestRegistry::instance() {
    static IntegrationTestRegistry instance;
    return instance;
}

void IntegrationTestRegistry::register_test(const std::string& test_name,
                                          std::function<std::unique_ptr<IntegrationTestBase>()> factory) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    test_factories_[test_name] = factory;
}

std::vector<std::string> IntegrationTestRegistry::get_test_names() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(registry_mutex_));
    
    std::vector<std::string> names;
    for (const auto& [name, factory] : test_factories_) {
        names.push_back(name);
    }
    
    std::sort(names.begin(), names.end());
    return names;
}

std::unique_ptr<IntegrationTestBase> IntegrationTestRegistry::create_test(const std::string& test_name) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    auto it = test_factories_.find(test_name);
    if (it != test_factories_.end()) {
        return it->second();
    }
    
    return nullptr;
}

std::vector<TestResult> IntegrationTestRegistry::run_all_tests() {
    auto test_names = get_test_names();
    std::vector<TestResult> results;
    
    for (const auto& test_name : test_names) {
        auto test = create_test(test_name);
        if (test) {
            results.push_back(test->run());
        }
    }
    
    return results;
}

std::vector<TestResult> IntegrationTestRegistry::run_tests_matching(const std::string& pattern) {
    auto test_names = get_test_names();
    std::vector<TestResult> results;
    
    std::regex pattern_regex(pattern);
    
    for (const auto& test_name : test_names) {
        if (std::regex_match(test_name, pattern_regex)) {
            auto test = create_test(test_name);
            if (test) {
                results.push_back(test->run());
            }
        }
    }
    
    return results;
}

// IntegrationTestRunner Implementation

TestResult IntegrationTestRunner::run_test(const std::string& test_name) {
    auto test = IntegrationTestRegistry::instance().create_test(test_name);
    if (!test) {
        TestResult result;
        result.test_name = test_name;
        result.status = TestStatus::Failed;
        result.error_message = "Test not found: " + test_name;
        return result;
    }
    
    if (verbose_) {
        std::cout << "Running test: " << test_name << std::endl;
    }
    
    auto result = test->run();
    
    if (verbose_) {
        print_test_progress(result);
    }
    
    return result;
}

std::vector<TestResult> IntegrationTestRunner::run_all_tests() {
    return IntegrationTestRegistry::instance().run_all_tests();
}

std::vector<TestResult> IntegrationTestRunner::run_test_suite(const std::vector<std::string>& test_names) {
    std::vector<TestResult> results;
    
    for (const auto& test_name : test_names) {
        results.push_back(run_test(test_name));
    }
    
    return results;
}

std::string IntegrationTestRunner::generate_report(const std::vector<TestResult>& results) {
    std::ostringstream report;
    
    // Summary
    size_t passed = 0, failed = 0, skipped = 0, timeout = 0;
    for (const auto& result : results) {
        switch (result.status) {
            case TestStatus::Passed: passed++; break;
            case TestStatus::Failed: failed++; break;
            case TestStatus::Skipped: skipped++; break;
            case TestStatus::Timeout: timeout++; break;
            default: break;
        }
    }
    
    report << "=== Integration Test Report ===" << std::endl;
    report << "Total tests: " << results.size() << std::endl;
    report << "Passed: " << passed << std::endl;
    report << "Failed: " << failed << std::endl;
    report << "Skipped: " << skipped << std::endl;
    report << "Timeout: " << timeout << std::endl;
    report << std::endl;
    
    // Detailed results
    for (const auto& result : results) {
        report << format_test_result(result) << std::endl;
    }
    
    return report.str();
}

void IntegrationTestRunner::print_test_progress(const TestResult& result) {
    std::string status_str;
    switch (result.status) {
        case TestStatus::Passed: status_str = "PASSED"; break;
        case TestStatus::Failed: status_str = "FAILED"; break;
        case TestStatus::Skipped: status_str = "SKIPPED"; break;
        case TestStatus::Timeout: status_str = "TIMEOUT"; break;
        default: status_str = "UNKNOWN"; break;
    }
    
    std::cout << "[" << status_str << "] " << result.test_name 
              << " (" << result.metrics.execution_time.count() << "ms)" << std::endl;
    
    if (result.failed() && !result.error_message.empty()) {
        std::cout << "  Error: " << result.error_message << std::endl;
    }
}

std::string IntegrationTestRunner::format_test_result(const TestResult& result) {
    std::ostringstream oss;
    
    oss << "Test: " << result.test_name << std::endl;
    oss << "Status: ";
    
    switch (result.status) {
        case TestStatus::Passed: oss << "PASSED"; break;
        case TestStatus::Failed: oss << "FAILED"; break;
        case TestStatus::Skipped: oss << "SKIPPED"; break;
        case TestStatus::Timeout: oss << "TIMEOUT"; break;
        default: oss << "UNKNOWN"; break;
    }
    
    oss << std::endl;
    oss << "Execution time: " << result.metrics.execution_time.count() << "ms" << std::endl;
    
    if (!result.error_message.empty()) {
        oss << "Error: " << result.error_message << std::endl;
    }
    
    if (!result.metrics.custom_metrics.empty()) {
        oss << "Metrics:" << std::endl;
        for (const auto& [name, value] : result.metrics.custom_metrics) {
            oss << "  " << name << ": " << value << std::endl;
        }
    }
    
    oss << "---" << std::endl;
    
    return oss.str();
}

} // namespace flight::hal::integration
