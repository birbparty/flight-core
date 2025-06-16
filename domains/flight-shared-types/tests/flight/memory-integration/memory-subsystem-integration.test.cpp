/**
 * Flight-Core Memory Subsystem Integration Tests
 * Validates shared types integration with Flight-Core memory management
 */

#include <gtest/gtest.h>
#include "../../../bindings/cpp17/flight/flight_shared_types.hpp"
#include <chrono>
#include <thread>

using namespace flight::shared_types;

class MemorySubsystemIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize memory monitoring
        startTime = std::chrono::steady_clock::now();
    }
    
    void TearDown() override {
        // Cleanup any allocated resources
    }
    
    std::chrono::steady_clock::time_point startTime;
};

TEST_F(MemorySubsystemIntegrationTest, PlatformMemoryAdaptation) {
    // Test memory adaptation across different platforms
    
    // Dreamcast memory profile
    auto dreamcastInfo = platform::PlatformDetector::get_dreamcast_info();
    EXPECT_EQ(dreamcastInfo.memory.total_memory.bytes, 16 * 1024 * 1024);
    EXPECT_EQ(dreamcastInfo.memory.available_memory.bytes, 12 * 1024 * 1024);
    EXPECT_EQ(dreamcastInfo.memory.heap_limit.bytes, 8 * 1024 * 1024);
    
    // PSP memory profile
    auto pspInfo = platform::PlatformDetector::get_psp_info();
    EXPECT_EQ(pspInfo.memory.total_memory.bytes, 32 * 1024 * 1024);
    EXPECT_GT(pspInfo.memory.available_memory.bytes, dreamcastInfo.memory.available_memory.bytes);
    
    // V6R memory profiles
    auto v6rSmall = platform::PlatformDetector::get_v6r_info("small");
    auto v6rMedium = platform::PlatformDetector::get_v6r_info("medium");
    auto v6rLarge = platform::PlatformDetector::get_v6r_info("large");
    
    EXPECT_EQ(v6rSmall.memory.total_memory.bytes, 512 * 1024 * 1024);
    EXPECT_EQ(v6rMedium.memory.total_memory.bytes, 1024 * 1024 * 1024);
    EXPECT_EQ(v6rLarge.memory.total_memory.bytes, 2048 * 1024 * 1024);
    
    // Verify memory scaling
    EXPECT_LT(dreamcastInfo.memory.total_memory.bytes, pspInfo.memory.total_memory.bytes);
    EXPECT_LT(pspInfo.memory.total_memory.bytes, v6rSmall.memory.total_memory.bytes);
    EXPECT_LT(v6rSmall.memory.total_memory.bytes, v6rMedium.memory.total_memory.bytes);
    EXPECT_LT(v6rMedium.memory.total_memory.bytes, v6rLarge.memory.total_memory.bytes);
}

TEST_F(MemorySubsystemIntegrationTest, MemoryUsageTracking) {
    // Test memory usage tracking integration
    
    // Create memory snapshots for different scenarios
    auto baselineSnapshot = memory::v6r::V6RMemoryUtils::create_snapshot(
        "baseline-test",
        "flight",
        memory::MemorySize::from_mb(10)
    );
    
    auto heavyUsageSnapshot = memory::v6r::V6RMemoryUtils::create_snapshot(
        "heavy-usage-test", 
        "flight",
        memory::MemorySize::from_mb(400)
    );
    
    // Test memory usage calculations
    EXPECT_LT(baselineSnapshot.usage_percentage(), 5.0); // Low usage
    EXPECT_GT(heavyUsageSnapshot.usage_percentage(), 75.0); // Heavy usage
    
    EXPECT_FALSE(baselineSnapshot.is_low_memory());
    EXPECT_TRUE(heavyUsageSnapshot.is_low_memory());
    
    // Test threshold checking
    EXPECT_FALSE(baselineSnapshot.exceeds_threshold(50.0));
    EXPECT_TRUE(heavyUsageSnapshot.exceeds_threshold(50.0));
}

TEST_F(MemorySubsystemIntegrationTest, ConstrainedPlatformMemoryManagement) {
    // Test memory management on constrained platforms
    
    // Dreamcast memory constraints
    auto dreamcastSnapshot = memory::v6r::V6RMemoryUtils::create_snapshot(
        "dreamcast-memory-test",
        "dreamcast",
        memory::MemorySize::from_mb(8) // 8MB usage out of 16MB total
    );
    
    // Should be within acceptable range for Dreamcast
    EXPECT_LE(dreamcastSnapshot.usage_percentage(), 60.0);
    EXPECT_FALSE(dreamcastSnapshot.is_low_memory());
    
    // Test near-limit scenario
    auto nearLimitSnapshot = memory::v6r::V6RMemoryUtils::create_snapshot(
        "dreamcast-near-limit",
        "dreamcast", 
        memory::MemorySize::from_mb(14) // 14MB usage - approaching limit
    );
    
    EXPECT_GT(nearLimitSnapshot.usage_percentage(), 85.0);
    EXPECT_TRUE(nearLimitSnapshot.is_low_memory());
    
    // Test PSP memory management
    auto pspSnapshot = memory::v6r::V6RMemoryUtils::create_snapshot(
        "psp-memory-test",
        "psp",
        memory::MemorySize::from_mb(16) // 16MB usage out of 32MB
    );
    
    EXPECT_LE(pspSnapshot.usage_percentage(), 55.0);
    EXPECT_FALSE(pspSnapshot.is_low_memory());
}

TEST_F(MemorySubsystemIntegrationTest, V6RMemoryIntegration) {
    // Test V6R memory configuration validation
    
    memory::v6r::V6RMemoryConfig validConfig{
        .vm_size = "medium",
        .session_id = "integration-test-session",
        .user_id = "integration-test-user"
    };
    
    memory::v6r::V6RMemoryConfig invalidConfig{
        .vm_size = "invalid-size",
        .session_id = "",
        .user_id = "test-user"
    };
    
    EXPECT_TRUE(memory::v6r::V6RMemoryUtils::validate_config(validConfig));
    EXPECT_FALSE(memory::v6r::V6RMemoryUtils::validate_config(invalidConfig));
    
    // Test VM memory limit calculation
    auto smallLimit = memory::v6r::V6RMemoryUtils::get_vm_memory_limit("small");
    auto mediumLimit = memory::v6r::V6RMemoryUtils::get_vm_memory_limit("medium");
    auto largeLimit = memory::v6r::V6RMemoryUtils::get_vm_memory_limit("large");
    
    EXPECT_EQ(smallLimit.bytes, 512 * 1024 * 1024);
    EXPECT_EQ(mediumLimit.bytes, 1024 * 1024 * 1024);
    EXPECT_EQ(largeLimit.bytes, 2048 * 1024 * 1024);
    
    EXPECT_LT(smallLimit.bytes, mediumLimit.bytes);
    EXPECT_LT(mediumLimit.bytes, largeLimit.bytes);
}

TEST_F(MemorySubsystemIntegrationTest, MemoryErrorHandling) {
    // Test memory error handling integration
    
    error::FlightError memoryError{
        "memory-integration-error",
        error::ErrorSeverity::Critical,
        error::ErrorCategory::Memory,
        "Memory allocation failed in integration test",
        error::ErrorContext{
            .source = "memory-subsystem-test",
            .operation = "allocate_component_memory",
            .session_id = "memory-test-session",
            .user_id = std::nullopt,
            .platform = "test-platform",
            .metadata = {
                {"requested_bytes", "8388608"},
                {"available_bytes", "4194304"},
                {"memory_type", "component_heap"},
                {"allocator", "flight"}
            }
        },
        "Insufficient memory for component allocation",
        "heap-fragmentation"
    };
    
    // Test memory error result
    memory::MemoryResult<memory::MemorySize> memoryResult = memoryError;
    EXPECT_TRUE(std::holds_alternative<error::FlightError>(memoryResult));
    
    if (std::holds_alternative<error::FlightError>(memoryResult)) {
        const auto& error = std::get<error::FlightError>(memoryResult);
        EXPECT_EQ(error.category(), error::ErrorCategory::Memory);
        EXPECT_TRUE(error.is_recoverable()); // Memory errors often recoverable
    }
    
    // Test successful memory result
    memory::MemoryResult<memory::MemorySize> successResult = memory::MemorySize::from_mb(8);
    EXPECT_TRUE(std::holds_alternative<memory::MemorySize>(successResult));
    
    if (std::holds_alternative<memory::MemorySize>(successResult)) {
        const auto& size = std::get<memory::MemorySize>(successResult);
        EXPECT_EQ(size.bytes, 8 * 1024 * 1024);
        EXPECT_EQ(size.human_readable, "8.0MB");
    }
}

TEST_F(MemorySubsystemIntegrationTest, PerformanceValidation) {
    // Test memory operation performance
    
    const int iterations = 1000;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Perform memory size calculations
    for (int i = 0; i < iterations; ++i) {
        auto size = memory::MemorySize::from_bytes(i * 1024);
        volatile auto percentage = (size.bytes * 100) / (16 * 1024 * 1024); // Prevent optimization
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    
    // Performance should be excellent - less than 1ms for 1000 operations
    EXPECT_LT(duration.count(), 1000); // Less than 1000 microseconds
    
    double avgTimePerOp = duration.count() / static_cast<double>(iterations);
    EXPECT_LT(avgTimePerOp, 1.0); // Less than 1 microsecond per operation
    
    std::cout << "Memory operations performance: " 
              << avgTimePerOp << " μs per operation" << std::endl;
}

TEST_F(MemorySubsystemIntegrationTest, MemoryPressureDetection) {
    // Test memory pressure detection across platforms
    
    struct PlatformTestCase {
        std::string platform;
        uint64_t totalMemoryMB;
        uint64_t usedMemoryMB;
        memory::MemoryPressure expectedPressure;
    };
    
    std::vector<PlatformTestCase> testCases = {
        {"dreamcast", 16, 4, memory::MemoryPressure::Low},      // 25% usage
        {"dreamcast", 16, 12, memory::MemoryPressure::High},    // 75% usage
        {"dreamcast", 16, 15, memory::MemoryPressure::Critical}, // 94% usage
        {"psp", 32, 8, memory::MemoryPressure::Low},            // 25% usage
        {"psp", 32, 24, memory::MemoryPressure::High},          // 75% usage
        {"v6r-medium", 1024, 256, memory::MemoryPressure::Low}, // 25% usage
        {"v6r-medium", 1024, 768, memory::MemoryPressure::Medium}, // 75% usage
    };
    
    for (const auto& testCase : testCases) {
        auto snapshot = memory::v6r::V6RMemoryUtils::create_snapshot(
            "pressure-test-" + testCase.platform,
            testCase.platform,
            memory::MemorySize::from_mb(testCase.usedMemoryMB)
        );
        
        auto pressureResult = memory::MemoryOperations::get_memory_pressure(
            "pressure-test-" + testCase.platform);
        
        if (memory::is_ok(pressureResult)) {
            auto pressure = memory::unwrap(pressureResult);
            EXPECT_EQ(pressure, testCase.expectedPressure) 
                << "Platform: " << testCase.platform 
                << ", Used: " << testCase.usedMemoryMB << "MB";
        }
    }
}

TEST_F(MemorySubsystemIntegrationTest, MemoryAllocationPatterns) {
    // Test different memory allocation patterns
    
    // Small frequent allocations (typical for component overhead)
    std::vector<memory::MemoryAllocation> smallAllocations;
    for (int i = 0; i < 100; ++i) {
        auto allocationResult = memory::MemoryOperations::create_allocation(
            "pattern-test-session",
            memory::MemorySize::from_kb(4), // 4KB each
            memory::MemoryPurpose::ComponentStack
        );
        
        if (memory::is_ok(allocationResult)) {
            smallAllocations.push_back(memory::unwrap(allocationResult));
        }
    }
    
    EXPECT_EQ(smallAllocations.size(), 100);
    
    // Medium allocations (typical for asset loading)
    std::vector<memory::MemoryAllocation> mediumAllocations;
    for (int i = 0; i < 10; ++i) {
        auto allocationResult = memory::MemoryOperations::create_allocation(
            "pattern-test-session",
            memory::MemorySize::from_mb(1), // 1MB each
            memory::MemoryPurpose::AssetCache
        );
        
        if (memory::is_ok(allocationResult)) {
            mediumAllocations.push_back(memory::unwrap(allocationResult));
        }
    }
    
    EXPECT_EQ(mediumAllocations.size(), 10);
    
    // Large allocation (typical for WASM linear memory)
    auto largeAllocationResult = memory::MemoryOperations::create_allocation(
        "pattern-test-session",
        memory::MemorySize::from_mb(64), // 64MB
        memory::MemoryPurpose::WasmLinear
    );
    
    EXPECT_TRUE(memory::is_ok(largeAllocationResult));
    
    // Test allocation listing
    auto allocationListResult = memory::MemoryOperations::list_allocations("pattern-test-session");
    if (memory::is_ok(allocationListResult)) {
        const auto& allocations = memory::unwrap(allocationListResult);
        EXPECT_GE(allocations.size(), 111); // At least our test allocations
        
        // Verify allocation purposes are tracked
        int componentCount = 0, assetCount = 0, wasmCount = 0;
        for (const auto& allocation : allocations) {
            if (allocation.purpose == memory::MemoryPurpose::ComponentStack) componentCount++;
            if (allocation.purpose == memory::MemoryPurpose::AssetCache) assetCount++;
            if (allocation.purpose == memory::MemoryPurpose::WasmLinear) wasmCount++;
        }
        
        EXPECT_GE(componentCount, 100);
        EXPECT_GE(assetCount, 10);
        EXPECT_GE(wasmCount, 1);
    }
    
    // Cleanup - free all allocations
    for (const auto& allocation : smallAllocations) {
        memory::MemoryOperations::free_allocation(allocation.id);
    }
    for (const auto& allocation : mediumAllocations) {
        memory::MemoryOperations::free_allocation(allocation.id);
    }
    if (memory::is_ok(largeAllocationResult)) {
        memory::MemoryOperations::free_allocation(memory::unwrap(largeAllocationResult).id);
    }
}

TEST_F(MemorySubsystemIntegrationTest, MemoryLimitEnforcement) {
    // Test memory limit enforcement across platforms
    
    // Set Dreamcast-appropriate limits
    memory::MemoryLimits dreamcastLimits{
        .heap_max = memory::MemorySize::from_mb(8),
        .stack_max = memory::MemorySize::from_mb(1),
        .cache_max = memory::MemorySize::from_mb(4),
        .soft_limit = memory::MemorySize::from_mb(12),
        .hard_limit = memory::MemorySize::from_mb(14)
    };
    
    auto limitsResult = memory::MemoryOperations::set_memory_limits(
        "dreamcast-test-session", dreamcastLimits);
    EXPECT_TRUE(memory::is_ok(limitsResult));
    
    // Test allocation within limits
    auto validAllocationResult = memory::MemoryOperations::create_allocation(
        "dreamcast-test-session",
        memory::MemorySize::from_mb(4), // Within heap limit
        memory::MemoryPurpose::VmHeap
    );
    EXPECT_TRUE(memory::is_ok(validAllocationResult));
    
    // Test allocation exceeding limits
    auto invalidAllocationResult = memory::MemoryOperations::create_allocation(
        "dreamcast-test-session",
        memory::MemorySize::from_mb(16), // Exceeds platform limits
        memory::MemoryPurpose::VmHeap
    );
    EXPECT_TRUE(memory::is_err(invalidAllocationResult));
    
    if (memory::is_err(invalidAllocationResult)) {
        const auto& error = memory::unwrap_err(invalidAllocationResult);
        EXPECT_EQ(error.code(), memory::MemoryError::Code::LimitExceeded);
    }
    
    // Test allocation validation
    auto validationResult = memory::MemoryOperations::validate_allocation_request(
        "dreamcast-test-session",
        memory::MemorySize::from_mb(2),
        memory::MemoryPurpose::ComponentStack
    );
    EXPECT_TRUE(memory::is_ok(validationResult));
    
    auto invalidValidationResult = memory::MemoryOperations::validate_allocation_request(
        "dreamcast-test-session",
        memory::MemorySize::from_mb(20),
        memory::MemoryPurpose::ComponentStack
    );
    EXPECT_TRUE(memory::is_err(invalidValidationResult));
}

TEST_F(MemorySubsystemIntegrationTest, FragmentationMonitoring) {
    // Test memory fragmentation monitoring
    
    // Create fragmented allocation pattern
    std::vector<memory::MemoryAllocation> fragmentingAllocations;
    
    // Allocate many small blocks
    for (int i = 0; i < 50; ++i) {
        auto allocationResult = memory::MemoryOperations::create_allocation(
            "fragmentation-test-session",
            memory::MemorySize::from_kb(8), // 8KB each
            memory::MemoryPurpose::Temporary
        );
        
        if (memory::is_ok(allocationResult)) {
            fragmentingAllocations.push_back(memory::unwrap(allocationResult));
        }
    }
    
    // Free every other allocation to create holes
    for (size_t i = 0; i < fragmentingAllocations.size(); i += 2) {
        memory::MemoryOperations::free_allocation(fragmentingAllocations[i].id);
    }
    
    // Check memory snapshot for fragmentation
    auto snapshotResult = memory::MemoryOperations::get_memory_snapshot("fragmentation-test-session");
    if (memory::is_ok(snapshotResult)) {
        const auto& snapshot = memory::unwrap(snapshotResult);
        
        // After creating holes, fragmentation should be detected
        EXPECT_GT(snapshot.fragmentation_ratio, 0.1); // More than 10% fragmented
        EXPECT_TRUE(snapshot.is_fragmented());
    }
    
    // Cleanup remaining allocations
    for (size_t i = 1; i < fragmentingAllocations.size(); i += 2) {
        memory::MemoryOperations::free_allocation(fragmentingAllocations[i].id);
    }
}
