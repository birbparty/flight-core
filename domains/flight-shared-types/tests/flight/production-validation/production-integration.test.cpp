/**
 * Production Integration Validation Tests
 * End-to-end validation of Flight-Core shared types integration readiness
 */

#include <gtest/gtest.h>
#include "../../../bindings/cpp17/flight/flight_shared_types.hpp"
#include <vector>
#include <thread>
#include <future>
#include <chrono>

using namespace flight::shared_types;

class ProductionIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize production-like environment
        setupTime = std::chrono::system_clock::now();
    }
    
    void TearDown() override {
        // Cleanup production test resources
        cleanup();
    }
    
    void cleanup() {
        // Cleanup test sessions and components
        for (const auto& sessionId : testSessions) {
            session::SessionOperations::terminate_session(sessionId);
        }
        testSessions.clear();
    }
    
    std::chrono::system_clock::time_point setupTime;
    std::vector<std::string> testSessions;
};

TEST_F(ProductionIntegrationTest, FullFlightCoreWorkflow) {
    // Test complete Flight-Core integration workflow
    
    // 1. Platform initialization
    auto platformResult = integration::FlightCoreIntegration::initialize_platform();
    ASSERT_TRUE(error::is_ok(platformResult));
    
    auto platform = error::unwrap(platformResult);
    
    // 2. System session creation
    auto sessionResult = integration::FlightCoreIntegration::create_system_session(platform.id);
    ASSERT_TRUE(error::is_ok(sessionResult));
    
    auto session = error::unwrap(sessionResult);
    testSessions.push_back(session.id);
    
    // 3. HAL component creation
    auto halResult = integration::FlightCoreIntegration::create_hal_component(platform.id);
    ASSERT_TRUE(error::is_ok(halResult));
    
    auto halComponent = error::unwrap(halResult);
    
    // 4. Runtime component creation
    auto runtimeResult = integration::FlightCoreIntegration::create_runtime_component(platform.id);
    ASSERT_TRUE(error::is_ok(runtimeResult));
    
    auto runtimeComponent = error::unwrap(runtimeResult);
    
    // 5. Memory management validation
    auto memoryResult = integration::FlightCoreIntegration::get_system_memory();
    ASSERT_TRUE(error::is_ok(memoryResult));
    
    auto memorySnapshot = error::unwrap(memoryResult);
    
    // Verify complete workflow
    EXPECT_TRUE(session.is_active());
    EXPECT_TRUE(halComponent.is_healthy());
    EXPECT_TRUE(runtimeComponent.is_running());
    EXPECT_FALSE(memorySnapshot.is_low_memory());
    
    // Verify components are associated with session
    EXPECT_EQ(halComponent.session_id, session.id);
    EXPECT_EQ(runtimeComponent.session_id, session.id);
    
    std::cout << "✅ Full Flight-Core workflow validated successfully" << std::endl;
}

TEST_F(ProductionIntegrationTest, V6RCloudDeploymentScenario) {
    // Test V6R cloud deployment scenario
    
    std::vector<std::string> vmSizes = {"small", "medium", "large"};
    
    for (const auto& vmSize : vmSizes) {
        // Create V6R session
        auto v6rSessionResult = integration::FlightCoreIntegration::create_v6r_session(
            vmSize, "production-user-" + vmSize);
        ASSERT_TRUE(error::is_ok(v6rSessionResult));
        
        auto v6rSession = error::unwrap(v6rSessionResult);
        testSessions.push_back(v6rSession.id);
        
        // Verify V6R session properties
        EXPECT_EQ(v6rSession.type, session::SessionType::User);
        EXPECT_TRUE(v6rSession.is_active());
        EXPECT_FALSE(v6rSession.is_expired());
        
        // Test V6R memory usage
        auto memoryUsageResult = integration::FlightCoreIntegration::get_v6r_memory_usage(
            v6rSession.id);
        ASSERT_TRUE(error::is_ok(memoryUsageResult));
        
        auto memoryUsage = error::unwrap(memoryUsageResult);
        
        // Verify memory scaling by VM size
        if (vmSize == "small") {
            EXPECT_EQ(memoryUsage.total.bytes, 512 * 1024 * 1024);
        } else if (vmSize == "medium") {
            EXPECT_EQ(memoryUsage.total.bytes, 1024 * 1024 * 1024);
        } else if (vmSize == "large") {
            EXPECT_EQ(memoryUsage.total.bytes, 2048 * 1024 * 1024);
        }
        
        EXPECT_LT(memoryUsage.usage_percentage(), 25.0); // Should start with low usage
        
        std::cout << "✅ V6R " << vmSize << " deployment scenario validated" << std::endl;
    }
}

TEST_F(ProductionIntegrationTest, ConstrainedPlatformProductionReadiness) {
    // Test production readiness on constrained platforms
    
    std::vector<std::string> constrainedPlatforms = {"dreamcast", "psp"};
    
    for (const auto& platformId : constrainedPlatforms) {
        // Initialize constrained platform
        auto platformInfo = (platformId == "dreamcast") 
            ? platform::PlatformDetector::get_dreamcast_info()
            : platform::PlatformDetector::get_psp_info();
        
        EXPECT_TRUE(platformInfo.is_constrained());
        EXPECT_FALSE(platformInfo.supports_threading());
        
        // Create production-appropriate session
        auto sessionResult = session::SessionOperations::create_session(
            session::SessionType::Component,
            platformId,
            std::nullopt
        );
        ASSERT_TRUE(error::is_ok(sessionResult));
        
        auto session = error::unwrap(sessionResult);
        testSessions.push_back(session.id);
        
        // Test constrained memory allocation
        auto allocationResult = memory::MemoryOperations::create_allocation(
            session.id,
            memory::MemorySize::from_mb(2), // Conservative allocation
            memory::MemoryPurpose::ComponentStack
        );
        EXPECT_TRUE(memory::is_ok(allocationResult));
        
        // Test memory pressure monitoring
        auto pressureResult = memory::MemoryOperations::get_memory_pressure(session.id);
        if (memory::is_ok(pressureResult)) {
            auto pressure = memory::unwrap(pressureResult);
            EXPECT_NE(pressure, memory::MemoryPressure::Critical);
        }
        
        // Test component creation within constraints
        auto componentResult = component::ComponentOperations::create_component(
            "constrained-component-" + platformId,
            "flight:constrained-world",
            platformId
        );
        EXPECT_TRUE(error::is_ok(componentResult));
        
        if (error::is_ok(componentResult)) {
            auto component = error::unwrap(componentResult);
            
            // Verify memory usage is appropriate for platform
            if (platformId == "dreamcast") {
                EXPECT_LE(component.memory_usage.used.bytes, 4 * 1024 * 1024); // Max 4MB
            } else if (platformId == "psp") {
                EXPECT_LE(component.memory_usage.used.bytes, 8 * 1024 * 1024); // Max 8MB
            }
        }
        
        std::cout << "✅ " << platformId << " production readiness validated" << std::endl;
    }
}

TEST_F(ProductionIntegrationTest, ConcurrentSessionHandling) {
    // Test handling of concurrent sessions (production load)
    
    const int maxConcurrentSessions = 50;
    std::vector<std::future<error::FlightResult<session::SessionInfo>>> sessionFutures;
    
    // Create sessions concurrently
    for (int i = 0; i < maxConcurrentSessions; ++i) {
        auto future = std::async(std::launch::async, [i]() {
            return session::SessionOperations::create_session(
                session::SessionType::Component,
                "production-platform",
                "concurrent-user-" + std::to_string(i)
            );
        });
        sessionFutures.push_back(std::move(future));
    }
    
    // Collect results
    std::vector<session::SessionInfo> sessions;
    for (auto& future : sessionFutures) {
        auto result = future.get();
        if (error::is_ok(result)) {
            auto session = error::unwrap(result);
            sessions.push_back(session);
            testSessions.push_back(session.id);
        }
    }
    
    // Verify all sessions were created successfully
    EXPECT_EQ(sessions.size(), maxConcurrentSessions);
    
    // Test concurrent operations on sessions
    std::vector<std::future<void>> operationFutures;
    
    for (const auto& session : sessions) {
        auto future = std::async(std::launch::async, [&session]() {
            // Perform typical session operations
            auto memorySnapshot = memory::v6r::V6RMemoryUtils::create_snapshot(
                session.id,
                "production-platform",
                memory::MemorySize::from_mb(10)
            );
            
            auto componentResult = component::ComponentOperations::create_component(
                "concurrent-component-" + session.id,
                "concurrent:world",
                "production-platform"
            );
            
            auto sessionInfo = session::SessionOperations::get_session(session.id);
        });
        operationFutures.push_back(std::move(future));
    }
    
    // Wait for all operations to complete
    for (auto& future : operationFutures) {
        future.wait();
    }
    
    std::cout << "✅ Concurrent session handling (" << maxConcurrentSessions 
              << " sessions) validated" << std::endl;
}

TEST_F(ProductionIntegrationTest, HighFrequencyOperations) {
    // Test high-frequency operations (1000+ ops/sec requirement)
    
    const int operationsPerSecond = 1000;
    const int testDurationSeconds = 5;
    const int totalOperations = operationsPerSecond * testDurationSeconds;
    
    // Create test session
    auto sessionResult = session::SessionOperations::create_session(
        session::SessionType::Development,
        "high-frequency-platform",
        "high-freq-user"
    );
    ASSERT_TRUE(error::is_ok(sessionResult));
    
    auto session = error::unwrap(sessionResult);
    testSessions.push_back(session.id);
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Perform high-frequency memory operations
    for (int i = 0; i < totalOperations; ++i) {
        // Memory size creation (very frequent operation)
        auto size = memory::MemorySize::from_bytes(i * 1024);
        
        // Memory snapshot creation (frequent operation)
        if (i % 10 == 0) {
            auto snapshot = memory::v6r::V6RMemoryUtils::create_snapshot(
                session.id,
                "high-frequency-platform",
                size
            );
        }
        
        // Error creation (occasional operation)
        if (i % 100 == 0) {
            error::FlightError testError{
                "high-freq-error-" + std::to_string(i),
                error::ErrorSeverity::Info,
                error::ErrorCategory::Performance,
                "High frequency test error",
                error::ErrorContext{
                    .source = "high-frequency-test",
                    .operation = "high_frequency_operation",
                    .session_id = session.id,
                    .user_id = std::nullopt,
                    .platform = "high-frequency-platform",
                    .metadata = {{"iteration", std::to_string(i)}}
                },
                "Testing high frequency error handling",
                std::nullopt
            };
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    double actualOpsPerSecond = (totalOperations * 1000.0) / duration.count();
    
    // Verify we met the performance requirement
    EXPECT_GE(actualOpsPerSecond, operationsPerSecond);
    
    std::cout << "✅ High-frequency operations: " << actualOpsPerSecond 
              << " ops/sec (target: " << operationsPerSecond << " ops/sec)" << std::endl;
}

TEST_F(ProductionIntegrationTest, ErrorRecoveryScenarios) {
    // Test production error recovery scenarios
    
    // Create test session
    auto sessionResult = session::SessionOperations::create_session(
        session::SessionType::System,
        "error-recovery-platform",
        std::nullopt
    );
    ASSERT_TRUE(error::is_ok(sessionResult));
    
    auto session = error::unwrap(sessionResult);
    testSessions.push_back(session.id);
    
    // Test memory exhaustion recovery
    {
        auto exhaustionError = error::ErrorOperations::create_platform_error(
            error::PlatformErrorCode::InsufficientPlatformMemory,
            "Simulated memory exhaustion",
            "error-recovery-platform",
            "memory_allocation"
        );
        
        EXPECT_EQ(exhaustionError.category(), error::ErrorCategory::Platform);
        EXPECT_FALSE(exhaustionError.is_recoverable()); // Memory exhaustion not recoverable
        
        // Test error recovery suggestions
        auto suggestions = error::ErrorOperations::get_recovery_suggestions(exhaustionError);
        EXPECT_FALSE(suggestions.empty());
        
        bool hasMemoryRelatedSuggestion = false;
        for (const auto& suggestion : suggestions) {
            if (suggestion.action_type == error::RecoveryActionType::ReduceResources ||
                suggestion.action_type == error::RecoveryActionType::UpgradeResources) {
                hasMemoryRelatedSuggestion = true;
                break;
            }
        }
        EXPECT_TRUE(hasMemoryRelatedSuggestion);
    }
    
    // Test component failure recovery
    {
        auto componentError = error::ErrorOperations::create_error(
            error::ErrorSeverity::Critical,
            error::ErrorCategory::Component,
            "Component initialization failed",
            error::ErrorContext{
                .source = "component-manager",
                .operation = "initialize_component",
                .session_id = session.id,
                .user_id = std::nullopt,
                .platform = "error-recovery-platform",
                .metadata = {
                    {"component_id", "failed-component"},
                    {"failure_type", "initialization"},
                    {"retry_count", "3"}
                }
            }
        );
        
        EXPECT_TRUE(componentError.is_recoverable()); // Component errors often recoverable
        
        // Test error chaining
        auto rootCause = error::ErrorOperations::create_simple_error(
            error::ErrorSeverity::Error,
            error::ErrorCategory::Memory,
            "Insufficient memory for component",
            "memory-allocator",
            "allocate_component_memory"
        );
        
        auto chainedError = error::ErrorOperations::chain_error(componentError, rootCause);
        EXPECT_TRUE(chainedError.cause().has_value());
    }
    
    // Test network error recovery
    {
        auto networkError = error::ErrorOperations::create_network_error(
            error::NetworkErrorCode::ConnectionTimeout,
            "V6R service connection timeout",
            "v6r-api.example.com"
        );
        
        EXPECT_EQ(networkError.category(), error::ErrorCategory::Network);
        EXPECT_TRUE(networkError.is_recoverable()); // Network errors often recoverable
        
        auto suggestions = error::ErrorOperations::get_recovery_suggestions(networkError);
        bool hasRetryAction = false;
        for (const auto& suggestion : suggestions) {
            if (suggestion.action_type == error::RecoveryActionType::Retry ||
                suggestion.action_type == error::RecoveryActionType::WaitRetry) {
                hasRetryAction = true;
                break;
            }
        }
        EXPECT_TRUE(hasRetryAction);
    }
    
    std::cout << "✅ Error recovery scenarios validated" << std::endl;
}

TEST_F(ProductionIntegrationTest, ResourceCleanupValidation) {
    // Test proper resource cleanup in production scenarios
    
    std::vector<std::string> createdSessions;
    std::vector<std::string> createdComponents;
    std::vector<std::string> createdAllocations;
    
    // Create multiple resources
    for (int i = 0; i < 10; ++i) {
        // Create session
        auto sessionResult = session::SessionOperations::create_session(
            session::SessionType::Component,
            "cleanup-platform",
            "cleanup-user-" + std::to_string(i)
        );
        
        if (error::is_ok(sessionResult)) {
            auto session = error::unwrap(sessionResult);
            createdSessions.push_back(session.id);
            
            // Create component in session
            auto componentResult = component::ComponentOperations::create_component(
                "cleanup-component-" + std::to_string(i),
                "cleanup:world",
                "cleanup-platform"
            );
            
            if (error::is_ok(componentResult)) {
                auto component = error::unwrap(componentResult);
                createdComponents.push_back(component.id);
            }
            
            // Create memory allocation
            auto allocationResult = memory::MemoryOperations::create_allocation(
                session.id,
                memory::MemorySize::from_mb(5),
                memory::MemoryPurpose::ComponentStack
            );
            
            if (memory::is_ok(allocationResult)) {
                auto allocation = memory::unwrap(allocationResult);
                createdAllocations.push_back(allocation.id);
            }
        }
    }
    
    // Verify resources were created
    EXPECT_EQ(createdSessions.size(), 10);
    EXPECT_EQ(createdComponents.size(), 10);
    EXPECT_EQ(createdAllocations.size(), 10);
    
    // Test cleanup process
    
    // 1. Free memory allocations
    for (const auto& allocationId : createdAllocations) {
        auto freeResult = memory::MemoryOperations::free_allocation(allocationId);
        EXPECT_TRUE(memory::is_ok(freeResult));
    }
    
    // 2. Terminate components
    for (const auto& componentId : createdComponents) {
        auto terminateResult = component::ComponentOperations::update_component_state(
            componentId, component::ComponentState::Terminated);
        EXPECT_TRUE(error::is_ok(terminateResult));
    }
    
    // 3. Terminate sessions
    for (const auto& sessionId : createdSessions) {
        auto terminateResult = session::SessionOperations::terminate_session(sessionId);
        EXPECT_TRUE(error::is_ok(terminateResult));
    }
    
    // Verify cleanup by checking resource states
    for (const auto& sessionId : createdSessions) {
        auto sessionResult = session::SessionOperations::get_session(sessionId);
        if (error::is_ok(sessionResult)) {
            auto session = error::unwrap(sessionResult);
            EXPECT_EQ(session.state, session::SessionState::Terminated);
        }
    }
    
    std::cout << "✅ Resource cleanup validation completed" << std::endl;
}

TEST_F(ProductionIntegrationTest, ProductionReadinessCertification) {
    // Final production readiness certification
    
    struct ReadinessCheck {
        std::string category;
        std::function<bool()> check;
        bool required;
    };
    
    std::vector<ReadinessCheck> readinessChecks = {
        {
            "Platform Detection",
            []() {
                auto result = platform::PlatformDetector::detect_current_platform();
                return error::is_ok(result);
            },
            true
        },
        {
            "Session Management",
            []() {
                auto result = session::SessionOperations::create_session(
                    session::SessionType::System, "certification-platform", std::nullopt);
                return error::is_ok(result);
            },
            true
        },
        {
            "Component Operations",
            []() {
                auto result = component::ComponentOperations::create_component(
                    "certification-component", "cert:world", "certification-platform");
                return error::is_ok(result);
            },
            true
        },
        {
            "Memory Management",
            []() {
                auto result = memory::MemoryOperations::create_allocation(
                    "certification-session",
                    memory::MemorySize::from_mb(1),
                    memory::MemoryPurpose::VmHeap
                );
                return memory::is_ok(result);
            },
            true
        },
        {
            "Error Handling",
            []() {
                error::FlightError testError{
                    "certification-error",
                    error::ErrorSeverity::Info,
                    error::ErrorCategory::Validation,
                    "Certification test error",
                    error::ErrorContext{
                        .source = "certification-test",
                        .operation = "readiness_check",
                        .session_id = "certification-session",
                        .user_id = std::nullopt,
                        .platform = "certification-platform",
                        .metadata = {{"check_type", "production_readiness"}}
                    },
                    "Testing error handling for certification",
                    std::nullopt
                };
                return !testError.to_string().empty();
            },
            true
        },
        {
            "V6R Integration",
            []() {
                memory::v6r::V6RMemoryConfig config{
                    .vm_size = "medium",
                    .session_id = "certification-v6r-session",
                    .user_id = "certification-user"
                };
                return memory::v6r::V6RMemoryUtils::validate_config(config);
            },
            true
        }
    };
    
    int passedChecks = 0;
    int requiredChecks = 0;
    int totalChecks = readinessChecks.size();
    
    std::cout << "\n🔍 Production Readiness Certification\n" << std::endl;
    
    for (const auto& check : readinessChecks) {
        if (check.required) requiredChecks++;
        
        bool passed = false;
        try {
            passed = check.check();
        } catch (const std::exception& e) {
            passed = false;
        }
        
        if (passed) {
            passedChecks++;
            std::cout << "✅ " << check.category << ": PASS" << std::endl;
        } else {
            std::cout << "❌ " << check.category << ": FAIL" 
                      << (check.required ? " (REQUIRED)" : " (OPTIONAL)") << std::endl;
        }
    }
    
    double passRate = (passedChecks * 100.0) / totalChecks;
    bool certificationPassed = (passedChecks >= requiredChecks) && (passRate >= 90.0);
    
    std::cout << "\n📊 Certification Results:" << std::endl;
    std::cout << "   Passed: " << passedChecks << "/" << totalChecks 
              << " (" << passRate << "%)" << std::endl;
    std::cout << "   Required: " << requiredChecks << " (all must pass)" << std::endl;
    
    if (certificationPassed) {
        std::cout << "\n🎉 PRODUCTION READINESS: CERTIFIED ✅" << std::endl;
        std::cout << "   Flight Shared Types are ready for production use" << std::endl;
        std::cout << "   V6R integration can proceed with confidence" << std::endl;
    } else {
        std::cout << "\n⚠️ PRODUCTION READINESS: NOT CERTIFIED ❌" << std::endl;
        std::cout << "   Address failed checks before production deployment" << std::endl;
    }
    
    // The test assertion
    EXPECT_TRUE(certificationPassed) << "Production readiness certification failed";
}
