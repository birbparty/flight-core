/**
 * Flight-Core Component Model Integration Tests
 * Validates shared types integration with Component Model runtime
 */

#include <gtest/gtest.h>
#include "../../../bindings/cpp17/flight/flight_shared_types.hpp"

using namespace flight::shared_types;

class ComponentModelIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test session
        auto sessionResult = integration::FlightCoreIntegration::create_system_session("test-platform");
        ASSERT_TRUE(error::is_ok(sessionResult));
        testSession = error::unwrap(sessionResult);
    }
    
    session::SessionInfo testSession;
};

TEST_F(ComponentModelIntegrationTest, ComponentLifecycleManagement) {
    // Test runtime component creation
    auto runtimeResult = integration::FlightCoreIntegration::create_runtime_component("test-platform");
    ASSERT_TRUE(error::is_ok(runtimeResult));
    
    if (error::is_ok(runtimeResult)) {
        const auto& runtimeComponent = error::unwrap(runtimeResult);
        
        EXPECT_EQ(runtimeComponent.world, "flight:runtime-world");
        EXPECT_EQ(runtimeComponent.state, component::ComponentState::Running);
        EXPECT_TRUE(runtimeComponent.is_running());
        EXPECT_TRUE(runtimeComponent.is_healthy());
        
        // Verify Component Model metadata
        auto wasmEngine = std::find_if(runtimeComponent.metadata.begin(),
                                     runtimeComponent.metadata.end(),
                                     [](const auto& pair) {
                                         return pair.first == "wasm_engine";
                                     });
        ASSERT_NE(wasmEngine, runtimeComponent.metadata.end());
        EXPECT_EQ(wasmEngine->second, "uwrc");
    }
}

TEST_F(ComponentModelIntegrationTest, SessionComponentIntegration) {
    // Test session and component integration
    EXPECT_TRUE(testSession.is_active());
    EXPECT_FALSE(testSession.is_expired());
    
    // Test session age calculation
    auto age = testSession.age();
    EXPECT_GE(age.count(), 0.0);
    EXPECT_LT(age.count(), 1.0); // Should be less than 1 second old
    
    // Test session metadata
    auto platformMeta = std::find_if(testSession.metadata.begin(),
                                   testSession.metadata.end(),
                                   [](const auto& pair) {
                                       return pair.first == "platform";
                                   });
    // Session should have platform metadata if integrated properly
}

TEST_F(ComponentModelIntegrationTest, ErrorHandlingIntegration) {
    // Test Component Model error handling with shared types
    error::FlightError componentError{
        "component-test-error",
        error::ErrorSeverity::Warning,
        error::ErrorCategory::Component,
        "Component Model integration test error",
        error::ErrorContext{
            .source = "component-model-test",
            .operation = "test_error_integration",
            .session_id = testSession.id,
            .user_id = std::nullopt,
            .platform = testSession.platform,
            .metadata = {
                {"test_type", "integration"},
                {"component_model", "uwrc"},
                {"error_handling", "std::variant"}
            }
        },
        "Testing error propagation through Component Model",
        std::nullopt
    };
    
    // Test error recoverability
    EXPECT_TRUE(componentError.is_recoverable());
    
    // Test error string conversion
    std::string errorString = componentError.to_string();
    EXPECT_FALSE(errorString.empty());
    EXPECT_NE(errorString.find("component-test-error"), std::string::npos);
    
    // Test FlightResult with component error
    error::FlightResult<component::ComponentInfo> errorResult = componentError;
    EXPECT_TRUE(error::is_err(errorResult));
    EXPECT_FALSE(error::is_ok(errorResult));
}

TEST_F(ComponentModelIntegrationTest, Cpp17FeatureIntegration) {
    // Test C++17 feature integration in Component Model
    
    // Test std::optional usage
    std::optional<component::ComponentInfo> optionalComponent = std::nullopt;
    EXPECT_FALSE(optionalComponent.has_value());
    
    // Create component with std::optional fields
    component::ComponentInfo testComponent{
        .id = "cpp17-test-component",
        .name = "C++17 Test Component",
        .version = "1.0.0",
        .state = component::ComponentState::Instantiated,
        .world = "test:cpp17-world",
        .platform = "test-platform",
        .session_id = testSession.id, // std::optional<std::string>
        .created_at = std::chrono::system_clock::now(),
        .last_activity = std::chrono::system_clock::now(),
        .memory_usage = memory::v6r::V6RMemoryUtils::create_snapshot(
            "cpp17-test", "test-platform", memory::MemorySize::from_mb(1)),
        .metadata = {
            {"cpp_standard", "C++17"},
            {"features", "std::optional,std::variant"},
            {"test_case", "feature_integration"}
        }
    };
    
    optionalComponent = testComponent;
    EXPECT_TRUE(optionalComponent.has_value());
    
    if (optionalComponent) {
        EXPECT_EQ(optionalComponent->id, "cpp17-test-component");
        EXPECT_TRUE(optionalComponent->session_id.has_value());
        EXPECT_EQ(optionalComponent->session_id.value(), testSession.id);
    }
    
    // Test std::variant usage with FlightResult
    error::FlightResult<component::ComponentInfo> variantResult = testComponent;
    EXPECT_TRUE(error::is_ok(variantResult));
    
    if (error::is_ok(variantResult)) {
        const auto& component = error::unwrap(variantResult);
        EXPECT_EQ(component.id, "cpp17-test-component");
    }
}

TEST_F(ComponentModelIntegrationTest, ComponentInstantiationPatterns) {
    // Test different component instantiation patterns
    
    // Single-threaded component
    auto singleThreadedResult = component::ComponentOperations::create_component(
        "single-threaded-test", "flight:single-world", "test-platform");
    ASSERT_TRUE(error::is_ok(singleThreadedResult));
    
    // Multi-threaded component  
    auto multiThreadedResult = component::ComponentOperations::create_component(
        "multi-threaded-test", "flight:multi-world", "test-platform");
    ASSERT_TRUE(error::is_ok(multiThreadedResult));
    
    // Real-time component
    auto realTimeResult = component::ComponentOperations::create_component(
        "real-time-test", "flight:rt-world", "test-platform");
    ASSERT_TRUE(error::is_ok(realTimeResult));
    
    // Verify different execution contexts
    if (error::is_ok(singleThreadedResult) && error::is_ok(multiThreadedResult)) {
        auto singleContext = component::ComponentOperations::get_execution_context(
            error::unwrap(singleThreadedResult).id);
        auto multiContext = component::ComponentOperations::get_execution_context(
            error::unwrap(multiThreadedResult).id);
        
        if (error::is_ok(singleContext) && error::is_ok(multiContext)) {
            // Different execution modes should have different contexts
            EXPECT_NE(error::unwrap(singleContext).execution_mode, 
                     error::unwrap(multiContext).execution_mode);
        }
    }
}

TEST_F(ComponentModelIntegrationTest, MemoryIntegratedComponentManagement) {
    // Test component management with memory integration
    
    auto componentResult = component::ComponentOperations::create_component(
        "memory-aware-component", "flight:memory-world", "dreamcast");
    ASSERT_TRUE(error::is_ok(componentResult));
    
    if (error::is_ok(componentResult)) {
        const auto& component = error::unwrap(componentResult);
        
        // Component should respect Dreamcast memory constraints
        EXPECT_LE(component.memory_usage.used.bytes, 16 * 1024 * 1024); // 16MB limit
        EXPECT_GE(component.memory_usage.available.bytes, 0);
        
        // Memory usage should be realistic for Dreamcast
        EXPECT_LE(component.memory_usage.usage_percentage(), 50.0);
        
        // Test component state updates with memory tracking
        auto stateUpdateResult = component::ComponentOperations::update_component_state(
            component.id, component::ComponentState::Running);
        EXPECT_TRUE(error::is_ok(stateUpdateResult));
        
        // Get updated component info
        auto updatedComponentResult = component::ComponentOperations::get_component(component.id);
        if (error::is_ok(updatedComponentResult)) {
            const auto& updatedComponent = error::unwrap(updatedComponentResult);
            EXPECT_EQ(updatedComponent.state, component::ComponentState::Running);
            
            // Memory usage should be tracked across state changes
            EXPECT_GT(updatedComponent.last_activity, component.created_at);
        }
    }
}

TEST_F(ComponentModelIntegrationTest, CrossPlatformComponentCompatibility) {
    // Test component compatibility across platforms
    std::vector<std::pair<std::string, std::string>> platformWorldPairs = {
        {"dreamcast", "flight:hal-world"},
        {"psp", "flight:portable-world"},
        {"vita", "flight:enhanced-world"},
        {"v6r-small", "flight:cloud-world"},
        {"v6r-medium", "flight:scalable-world"},
        {"v6r-large", "flight:enterprise-world"}
    };
    
    for (const auto& [platform, world] : platformWorldPairs) {
        auto componentResult = component::ComponentOperations::create_component(
            "cross-platform-test-" + platform, world, platform);
        
        if (error::is_ok(componentResult)) {
            const auto& component = error::unwrap(componentResult);
            
            EXPECT_EQ(component.platform, platform);
            EXPECT_EQ(component.world, world);
            EXPECT_EQ(component.state, component::ComponentState::Instantiated);
            
            // Verify platform-appropriate memory usage
            if (platform == "dreamcast") {
                EXPECT_LE(component.memory_usage.total.bytes, 16 * 1024 * 1024);
            } else if (platform == "v6r-large") {
                EXPECT_GE(component.memory_usage.total.bytes, 2048 * 1024 * 1024);
            }
            
            // All components should have consistent metadata structure
            auto platformMeta = std::find_if(component.metadata.begin(),
                                           component.metadata.end(),
                                           [](const auto& pair) {
                                               return pair.first == "target_platform";
                                           });
            EXPECT_NE(platformMeta, component.metadata.end());
        }
    }
}

TEST_F(ComponentModelIntegrationTest, ComponentResourceManagement) {
    // Test component resource management and cleanup
    
    std::vector<component::ComponentInfo> components;
    const int componentCount = 10;
    
    // Create multiple components
    for (int i = 0; i < componentCount; ++i) {
        auto componentResult = component::ComponentOperations::create_component(
            "resource-test-" + std::to_string(i), 
            "flight:resource-world", 
            "test-platform");
        
        if (error::is_ok(componentResult)) {
            auto componentInfo = component::ComponentOperations::get_component(
                error::unwrap(componentResult).id);
            if (error::is_ok(componentInfo)) {
                components.push_back(error::unwrap(componentInfo));
            }
        }
    }
    
    EXPECT_EQ(components.size(), componentCount);
    
    // Test component listing
    auto listResult = component::ComponentOperations::list_components("test-platform");
    if (error::is_ok(listResult)) {
        const auto& componentList = error::unwrap(listResult);
        EXPECT_GE(componentList.size(), componentCount);
    }
    
    // Test filtered listing by state
    auto runningListResult = component::ComponentOperations::list_components(
        "test-platform", component::ComponentState::Running);
    if (error::is_ok(runningListResult)) {
        const auto& runningComponents = error::unwrap(runningListResult);
        // Not all components may be running initially
        EXPECT_LE(runningComponents.size(), componentCount);
    }
    
    // Cleanup - terminate all test components
    for (const auto& component : components) {
        auto terminateResult = component::ComponentOperations::update_component_state(
            component.id, component::ComponentState::Terminated);
        EXPECT_TRUE(error::is_ok(terminateResult));
    }
}
