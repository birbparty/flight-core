/**
 * @file simple_integration_test.cpp
 * @brief Simple integration test that doesn't depend on complex mock drivers
 */

#include "../framework/integration_test_base.hpp"
#include <memory>

namespace flight::hal::integration {

/**
 * @brief Simple integration test for basic framework functionality
 */
class SimpleIntegrationTest : public IntegrationTestBase {
public:
    SimpleIntegrationTest() : IntegrationTestBase("SimpleIntegrationTest") {}

protected:
    TestScenario get_scenario() const override {
        TestScenario scenario;
        scenario.description = "Simple integration test for framework validation";
        scenario.required_drivers = {}; // No drivers required
        scenario.required_resources = {}; // No resources required  
        scenario.platform_restrictions = {}; // Can run on any platform
        scenario.timeout = std::chrono::seconds(30);
        return scenario;
    }

    HALResult<void> setup_test() override {
        log_message("Setting up simple integration test");
        record_metric("setup_operations", 1.0);
        return HALResult<void>::success();
    }

    HALResult<void> execute_test() override {
        log_message("Executing simple integration test");
        
        // Test basic framework functionality
        assert_condition(true, "Basic assertion should pass");
        
        // Test timing functionality
        auto start_time = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto end_time = std::chrono::steady_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        assert_condition(elapsed.count() >= 90, "Sleep should take at least 90ms");
        
        record_metric("sleep_time_ms", static_cast<double>(elapsed.count()));
        
        // Test wait_for_condition functionality
        bool condition_met = wait_for_condition([]() { return true; }, std::chrono::milliseconds(100));
        assert_condition(condition_met, "Immediate condition should be met");
        
        record_metric("test_operations", 3.0);
        log_message("Simple integration test completed successfully");
        
        return HALResult<void>::success();
    }

    HALResult<void> teardown_test() override {
        log_message("Tearing down simple integration test");
        record_metric("teardown_operations", 1.0);
        return HALResult<void>::success();
    }
};

// Register the test
namespace {
    bool register_simple_test = []() {
        IntegrationTestRegistry::instance().register_test(
            "SimpleIntegrationTest",
            []() -> std::unique_ptr<IntegrationTestBase> {
                return std::make_unique<SimpleIntegrationTest>();
            }
        );
        return true;
    }();
}

} // namespace flight::hal::integration
