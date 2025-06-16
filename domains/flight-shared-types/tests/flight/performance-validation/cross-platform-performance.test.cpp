/**
 * Cross-Platform Performance Validation Tests
 * Ensures shared types maintain performance across all Flight-Core platforms
 */

#include <gtest/gtest.h>
#include <benchmark/benchmark.h>
#include "../../../bindings/cpp17/flight/flight_shared_types.hpp"
#include <chrono>
#include <random>

using namespace flight::shared_types;

class CrossPlatformPerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup random number generation for realistic data
        generator.seed(std::chrono::system_clock::now().time_since_epoch().count());
        distribution = std::uniform_int_distribution<>(1024, 1024*1024*1024); // 1KB to 1GB
    }
    
    std::mt19937 generator;
    std::uniform_int_distribution<> distribution;
};

// Benchmark memory size operations
static void BM_MemorySizeCreation(benchmark::State& state) {
    for (auto _ : state) {
        auto size = memory::MemorySize::from_bytes(state.range(0));
        benchmark::DoNotOptimize(size);
    }
}
BENCHMARK(BM_MemorySizeCreation)->Range(1024, 2048*1024*1024);

static void BM_MemoryUsageCalculation(benchmark::State& state) {
    auto snapshot = memory::v6r::V6RMemoryUtils::create_snapshot(
        "benchmark-session",
        "benchmark-platform", 
        memory::MemorySize::from_bytes(state.range(0))
    );
    
    for (auto _ : state) {
        auto percentage = snapshot.usage_percentage();
        benchmark::DoNotOptimize(percentage);
    }
}
BENCHMARK(BM_MemoryUsageCalculation)->Range(1024*1024, 1024*1024*1024);

static void BM_ErrorCreation(benchmark::State& state) {
    for (auto _ : state) {
        error::FlightError testError{
            "benchmark-error",
            error::ErrorSeverity::Warning,
            error::ErrorCategory::Performance,
            "Performance benchmark error",
            error::ErrorContext{
                .source = "performance-test",
                .operation = "benchmark_error_creation",
                .session_id = "benchmark-session",
                .user_id = std::nullopt,
                .platform = "benchmark-platform",
                .metadata = {
                    {"benchmark", "true"},
                    {"iteration", std::to_string(state.iterations())}
                }
            },
            "Testing error creation performance",
            std::nullopt
        };
        benchmark::DoNotOptimize(testError);
    }
}
BENCHMARK(BM_ErrorCreation);

static void BM_ComponentCreation(benchmark::State& state) {
    for (auto _ : state) {
        auto result = component::ComponentOperations::create_component(
            "benchmark-component-" + std::to_string(state.iterations()),
            "benchmark:world",
            "benchmark-platform"
        );
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ComponentCreation);

static void BM_PlatformDetection(benchmark::State& state) {
    for (auto _ : state) {
        auto result = platform::PlatformDetector::detect_current_platform();
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_PlatformDetection);

static void BM_SessionCreation(benchmark::State& state) {
    for (auto _ : state) {
        auto result = session::SessionOperations::create_session(
            session::SessionType::Component,
            "benchmark-platform",
            "benchmark-user"
        );
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_SessionCreation);

TEST_F(CrossPlatformPerformanceTest, MemoryOperationPerformance) {
    // Test memory operation performance requirements
    const int iterations = 10000;
    
    // Test MemorySize creation performance
    auto startTime = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        volatile auto size = memory::MemorySize::from_bytes(distribution(generator));
    }
    auto endTime = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime);
    double avgTimePerOp = duration.count() / static_cast<double>(iterations);
    
    // Should be extremely fast - less than 100ns per operation
    EXPECT_LT(avgTimePerOp, 100.0);
    
    std::cout << "MemorySize creation: " << avgTimePerOp << " ns per operation" << std::endl;
}

TEST_F(CrossPlatformPerformanceTest, ErrorHandlingPerformance) {
    // Test error handling performance
    const int iterations = 1000;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        error::FlightError testError{
            "perf-test-" + std::to_string(i),
            error::ErrorSeverity::Info,
            error::ErrorCategory::Performance,
            "Performance test error",
            error::ErrorContext{
                .source = "performance-test",
                .operation = "error_performance_test",
                .session_id = "perf-session",
                .user_id = std::nullopt,
                .platform = "perf-platform",
                .metadata = {{"iteration", std::to_string(i)}}
            },
            "Testing error creation performance",
            std::nullopt
        };
        
        // Test error operations
        volatile bool recoverable = testError.is_recoverable();
        volatile auto errorString = testError.to_string();
    }
    auto endTime = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    double avgTimePerOp = duration.count() / static_cast<double>(iterations);
    
    // Error creation should be fast - less than 10μs per operation
    EXPECT_LT(avgTimePerOp, 10.0);
    
    std::cout << "Error handling: " << avgTimePerOp << " μs per operation" << std::endl;
}

TEST_F(CrossPlatformPerformanceTest, PlatformDetectionPerformance) {
    // Test platform detection performance
    const int iterations = 100;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        // Test different platform info getters
        volatile auto dreamcast = platform::PlatformDetector::get_dreamcast_info();
        volatile auto psp = platform::PlatformDetector::get_psp_info();
        volatile auto v6r = platform::PlatformDetector::get_v6r_info("medium");
    }
    auto endTime = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    double avgTimePerOp = duration.count() / static_cast<double>(iterations * 3);
    
    // Platform detection should be very fast - less than 1μs per operation
    EXPECT_LT(avgTimePerOp, 1.0);
    
    std::cout << "Platform detection: " << avgTimePerOp << " μs per operation" << std::endl;
}

TEST_F(CrossPlatformPerformanceTest, ComponentOperationPerformance) {
    // Test component operation performance
    const int iterations = 100;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    std::vector<std::string> componentIds;
    
    // Create components
    for (int i = 0; i < iterations; ++i) {
        auto result = component::ComponentOperations::create_component(
            "perf-test-" + std::to_string(i),
            "performance:world",
            "performance-platform"
        );
        
        if (error::is_ok(result)) {
            componentIds.push_back(error::unwrap(result).id);
        }
    }
    
    // Get component info
    for (const auto& id : componentIds) {
        volatile auto result = component::ComponentOperations::get_component(id);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    double avgTimePerOp = duration.count() / static_cast<double>(iterations * 2);
    
    // Component operations should be reasonably fast - less than 100μs per operation
    EXPECT_LT(avgTimePerOp, 100.0);
    
    std::cout << "Component operations: " << avgTimePerOp << " μs per operation" << std::endl;
}

TEST_F(CrossPlatformPerformanceTest, SessionManagementPerformance) {
    // Test session management performance
    const int iterations = 50;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    std::vector<std::string> sessionIds;
    
    // Create sessions
    for (int i = 0; i < iterations; ++i) {
        auto result = session::SessionOperations::create_session(
            session::SessionType::Component,
            "performance-platform",
            "perf-user-" + std::to_string(i)
        );
        
        if (error::is_ok(result)) {
            sessionIds.push_back(error::unwrap(result).id);
        }
    }
    
    // Get session info
    for (const auto& id : sessionIds) {
        volatile auto result = session::SessionOperations::get_session(id);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    double avgTimePerOp = duration.count() / static_cast<double>(iterations * 2);
    
    // Session operations should be fast - less than 200μs per operation
    EXPECT_LT(avgTimePerOp, 200.0);
    
    std::cout << "Session management: " << avgTimePerOp << " μs per operation" << std::endl;
}

TEST_F(CrossPlatformPerformanceTest, ConstrainedPlatformPerformance) {
    // Test performance on constrained platforms (Dreamcast/PSP)
    
    // Simulate Dreamcast constraints
    const uint64_t dreamcastMemoryLimit = 16 * 1024 * 1024; // 16MB
    const int dreamcastIterations = 100; // Fewer iterations for constrained platform
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < dreamcastIterations; ++i) {
        // Small memory operations suitable for Dreamcast
        auto size = memory::MemorySize::from_kb(64); // 64KB
        auto snapshot = memory::v6r::V6RMemoryUtils::create_snapshot(
            "dreamcast-perf-test",
            "dreamcast",
            size
        );
        
        volatile auto percentage = snapshot.usage_percentage();
        volatile bool lowMemory = snapshot.is_low_memory();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    double avgTimePerOp = duration.count() / static_cast<double>(dreamcastIterations);
    
    // Even on constrained platforms, operations should be fast - less than 50μs per operation
    EXPECT_LT(avgTimePerOp, 50.0);
    
    std::cout << "Dreamcast performance: " << avgTimePerOp << " μs per operation" << std::endl;
}

TEST_F(CrossPlatformPerformanceTest, V6RCloudPerformance) {
    // Test performance on V6R cloud platforms
    
    const int v6rIterations = 1000; // More iterations for cloud platform
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < v6rIterations; ++i) {
        // Larger operations suitable for cloud platforms
        auto size = memory::MemorySize::from_mb(64); // 64MB
        auto snapshot = memory::v6r::V6RMemoryUtils::create_snapshot(
            "v6r-perf-test",
            "v6r-large",
            size
        );
        
        volatile auto percentage = snapshot.usage_percentage();
        volatile bool lowMemory = snapshot.is_low_memory();
        
        // Test V6R-specific operations
        memory::v6r::V6RMemoryConfig config{
            .vm_size = "large",
            .session_id = "v6r-perf-session",
            .user_id = "v6r-perf-user"
        };
        volatile bool valid = memory::v6r::V6RMemoryUtils::validate_config(config);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    double avgTimePerOp = duration.count() / static_cast<double>(v6rIterations);
    
    // V6R operations should be very fast - less than 10μs per operation
    EXPECT_LT(avgTimePerOp, 10.0);
    
    std::cout << "V6R cloud performance: " << avgTimePerOp << " μs per operation" << std::endl;
}

TEST_F(CrossPlatformPerformanceTest, MemoryPressureDetectionPerformance) {
    // Test memory pressure detection performance
    const int iterations = 1000;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        auto snapshot = memory::v6r::V6RMemoryUtils::create_snapshot(
            "pressure-perf-test",
            "performance-platform",
            memory::MemorySize::from_mb(i % 100 + 1) // Variable memory usage
        );
        
        volatile auto percentage = snapshot.usage_percentage();
        volatile bool lowMemory = snapshot.is_low_memory();
        volatile bool exceeds = snapshot.exceeds_threshold(75.0);
        volatile bool fragmented = snapshot.is_fragmented();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime);
    double avgTimePerOp = duration.count() / static_cast<double>(iterations);
    
    // Memory pressure detection should be extremely fast - less than 200ns per operation
    EXPECT_LT(avgTimePerOp, 200.0);
    
    std::cout << "Memory pressure detection: " << avgTimePerOp << " ns per operation" << std::endl;
}

TEST_F(CrossPlatformPerformanceTest, ConcurrentOperationPerformance) {
    // Test performance under concurrent operations (simulating multiple sessions)
    const int sessionCount = 50;
    const int operationsPerSession = 20;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Simulate concurrent session operations
    std::vector<std::string> sessionIds;
    for (int s = 0; s < sessionCount; ++s) {
        auto sessionResult = session::SessionOperations::create_session(
            session::SessionType::Component,
            "concurrent-platform",
            "concurrent-user-" + std::to_string(s)
        );
        
        if (error::is_ok(sessionResult)) {
            sessionIds.push_back(error::unwrap(sessionResult).id);
        }
    }
    
    // Perform operations for each session
    for (const auto& sessionId : sessionIds) {
        for (int op = 0; op < operationsPerSession; ++op) {
            // Memory snapshot
            auto snapshot = memory::v6r::V6RMemoryUtils::create_snapshot(
                sessionId,
                "concurrent-platform",
                memory::MemorySize::from_mb(op + 1)
            );
            
            // Component creation
            auto componentResult = component::ComponentOperations::create_component(
                "concurrent-component-" + sessionId + "-" + std::to_string(op),
                "concurrent:world",
                "concurrent-platform"
            );
            
            // Error handling
            if (op % 5 == 0) { // Create some errors
                error::FlightError testError{
                    "concurrent-error",
                    error::ErrorSeverity::Info,
                    error::ErrorCategory::Component,
                    "Concurrent operation test error",
                    error::ErrorContext{
                        .source = "concurrent-test",
                        .operation = "concurrent_operation",
                        .session_id = sessionId,
                        .user_id = std::nullopt,
                        .platform = "concurrent-platform",
                        .metadata = {{"operation", std::to_string(op)}}
                    },
                    "Testing concurrent error handling",
                    std::nullopt
                };
            }
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    double totalOperations = sessionCount * operationsPerSession * 3; // 3 operations per iteration
    double avgTimePerOp = duration.count() / totalOperations;
    
    // Concurrent operations should maintain good performance - less than 1ms per operation
    EXPECT_LT(avgTimePerOp, 1.0);
    
    std::cout << "Concurrent operations (" << sessionCount << " sessions): " 
              << avgTimePerOp << " ms per operation" << std::endl;
    std::cout << "Total operations: " << totalOperations << std::endl;
    std::cout << "Total time: " << duration.count() << " ms" << std::endl;
}

TEST_F(CrossPlatformPerformanceTest, OverheadValidation) {
    // Validate that shared types add minimal overhead
    
    // Baseline: raw operations without shared types
    const int iterations = 10000;
    
    auto baselineStart = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        volatile uint64_t rawMemory = i * 1024;
        volatile double rawPercentage = (rawMemory * 100.0) / (1024 * 1024 * 1024);
        volatile bool rawCheck = rawPercentage > 50.0;
    }
    auto baselineEnd = std::chrono::high_resolution_clock::now();
    
    // Shared types: same operations with shared types
    auto sharedTypesStart = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto size = memory::MemorySize::from_bytes(i * 1024);
        auto snapshot = memory::v6r::V6RMemoryUtils::create_snapshot(
            "overhead-test",
            "test-platform",
            size
        );
        volatile auto percentage = snapshot.usage_percentage();
        volatile bool check = percentage > 50.0;
    }
    auto sharedTypesEnd = std::chrono::high_resolution_clock::now();
    
    auto baselineDuration = std::chrono::duration_cast<std::chrono::microseconds>(
        baselineEnd - baselineStart);
    auto sharedTypesDuration = std::chrono::duration_cast<std::chrono::microseconds>(
        sharedTypesEnd - sharedTypesStart);
    
    // Calculate overhead percentage
    double overhead = ((sharedTypesDuration.count() - baselineDuration.count()) * 100.0) 
                     / baselineDuration.count();
    
    // Overhead should be minimal - less than 1% as specified in requirements
    EXPECT_LT(overhead, 1.0);
    
    std::cout << "Baseline time: " << baselineDuration.count() << " μs" << std::endl;
    std::cout << "Shared types time: " << sharedTypesDuration.count() << " μs" << std::endl;
    std::cout << "Overhead: " << overhead << "%" << std::endl;
}

// Google Benchmark main function (for when running as standalone benchmark)
BENCHMARK_MAIN();
