/**
 * Flight-Core HAL Platform Integration Tests
 * Validates shared types integration with Flight-Core HAL layer
 */

#include <gtest/gtest.h>
#include "../../../bindings/cpp17/flight/flight_shared_types.hpp"
#include <memory>
#include <vector>

using namespace flight::shared_types;

class HALPlatformIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize platform detection
        auto platformResult = integration::FlightCoreIntegration::initialize_platform();
        ASSERT_TRUE(error::is_ok(platformResult));
        
        if (error::is_ok(platformResult)) {
            currentPlatform = error::unwrap(platformResult);
        }
    }
    
    platform::PlatformInfo currentPlatform;
};

TEST_F(HALPlatformIntegrationTest, DreamcastHALIntegration) {
    // Test Dreamcast HAL component creation
    auto dreamcastInfo = platform::PlatformDetector::get_dreamcast_info();
    
    // Verify Dreamcast platform characteristics
    EXPECT_EQ(dreamcastInfo.id, "dreamcast");
    EXPECT_EQ(dreamcastInfo.type, platform::PlatformType::Dreamcast);
    EXPECT_EQ(dreamcastInfo.capability, platform::PlatformCapability::Minimal);
    EXPECT_EQ(dreamcastInfo.memory.total_memory.bytes, 16 * 1024 * 1024); // 16MB
    
    // Test HAL component creation for Dreamcast
    auto halResult = integration::FlightCoreIntegration::create_hal_component("dreamcast");
    ASSERT_TRUE(error::is_ok(halResult));
    
    if (error::is_ok(halResult)) {
        const auto& halComponent = error::unwrap(halResult);
        
        EXPECT_EQ(halComponent.id, "flight-hal-dreamcast");
        EXPECT_EQ(halComponent.platform, "dreamcast");
        EXPECT_EQ(halComponent.world, "flight:hal-world");
        EXPECT_EQ(halComponent.state, component::ComponentState::Instantiated);
        
        // Verify C++17 metadata integration
        auto cppStandard = std::find_if(halComponent.metadata.begin(), 
                                      halComponent.metadata.end(),
                                      [](const auto& pair) { 
                                          return pair.first == "cpp_standard"; 
                                      });
        ASSERT_NE(cppStandard, halComponent.metadata.end());
        EXPECT_EQ(cppStandard->second, "C++17");
        
        // Verify memory constraints are respected
        EXPECT_LE(halComponent.memory_usage.used.bytes, 
                 dreamcastInfo.memory.available_memory.bytes);
    }
}

TEST_F(HALPlatformIntegrationTest, PSPHALIntegration) {
    // Test PSP HAL component creation
    auto pspInfo = platform::PlatformDetector::get_psp_info();
    
    // Verify PSP platform characteristics
    EXPECT_EQ(pspInfo.id, "psp");
    EXPECT_EQ(pspInfo.type, platform::PlatformType::Psp);
    EXPECT_EQ(pspInfo.capability, platform::PlatformCapability::Basic);
    EXPECT_EQ(pspInfo.memory.total_memory.bytes, 32 * 1024 * 1024); // 32MB
    
    // Test HAL component creation for PSP
    auto halResult = integration::FlightCoreIntegration::create_hal_component("psp");
    ASSERT_TRUE(error::is_ok(halResult));
    
    if (error::is_ok(halResult)) {
        const auto& halComponent = error::unwrap(halResult);
        
        EXPECT_EQ(halComponent.id, "flight-hal-psp");
        EXPECT_EQ(halComponent.platform, "psp");
        EXPECT_TRUE(halComponent.is_healthy());
        
        // Verify memory usage is appropriate for PSP
        EXPECT_GT(halComponent.memory_usage.available.bytes, 
                 halComponent.memory_usage.used.bytes);
    }
}

TEST_F(HALPlatformIntegrationTest, ModernPlatformHALIntegration) {
    // Test modern platform HAL integration (Linux, macOS, Windows)
    std::vector<std::string> modernPlatforms = {
        "linux-native", "macos-native", "windows-native"
    };
    
    for (const auto& platformId : modernPlatforms) {
        auto halResult = integration::FlightCoreIntegration::create_hal_component(platformId);
        ASSERT_TRUE(error::is_ok(halResult)) << "Failed to create HAL for " << platformId;
        
        if (error::is_ok(halResult)) {
            const auto& halComponent = error::unwrap(halResult);
            
            EXPECT_EQ(halComponent.platform, platformId);
            EXPECT_TRUE(halComponent.is_running() || halComponent.is_healthy());
            
            // Modern platforms should have more memory available
            EXPECT_GE(halComponent.memory_usage.total.bytes, 512 * 1024 * 1024); // >= 512MB
        }
    }
}

TEST_F(HALPlatformIntegrationTest, PlatformCapabilityDetection) {
    // Test platform capability detection using shared types
    auto dreamcast = platform::PlatformDetector::get_dreamcast_info();
    auto psp = platform::PlatformDetector::get_psp_info();
    
    // Test constrained platform detection
    EXPECT_TRUE(dreamcast.is_constrained());
    EXPECT_TRUE(psp.is_constrained());
    
    // Test capability-based feature detection
    EXPECT_FALSE(dreamcast.supports_threading()); // Single-core SH4
    EXPECT_FALSE(psp.supports_threading());       // Limited threading
    
    // Test network capability detection
    EXPECT_TRUE(dreamcast.supports_networking());  // Has ethernet
    EXPECT_TRUE(psp.supports_networking());        // Has WiFi
    
    // Test platform-specific features
    EXPECT_TRUE(platform::PlatformDetector::has_feature(dreamcast, "dma"));
    EXPECT_TRUE(platform::PlatformDetector::has_feature(psp, "wifi"));
}

TEST_F(HALPlatformIntegrationTest, MemoryConstraintEnforcement) {
    // Test memory constraint enforcement in HAL layer
    auto dreamcastInfo = platform::PlatformDetector::get_dreamcast_info();
    
    // Simulate memory allocation within platform limits
    auto memorySnapshot = memory::v6r::V6RMemoryUtils::create_snapshot(
        "hal-memory-test",
        "dreamcast",
        memory::MemorySize::from_mb(4) // 4MB usage
    );
    
    // Verify usage is within Dreamcast limits
    EXPECT_LE(memorySnapshot.used.bytes, dreamcastInfo.memory.available_memory.bytes);
    EXPECT_FALSE(memorySnapshot.is_low_memory()); // 4MB of 12MB available is fine
    
    // Test memory pressure detection
    auto highUsageSnapshot = memory::v6r::V6RMemoryUtils::create_snapshot(
        "hal-pressure-test",
        "dreamcast",
        memory::MemorySize::from_mb(14) // 14MB usage - high pressure
    );
    
    EXPECT_TRUE(highUsageSnapshot.is_low_memory());
    EXPECT_GT(highUsageSnapshot.usage_percentage(), 85.0);
}

TEST_F(HALPlatformIntegrationTest, V6RCloudHALIntegration) {
    // Test V6R cloud platform HAL integration
    std::vector<std::string> v6rSizes = {"small", "medium", "large"};
    
    for (const auto& vmSize : v6rSizes) {
        auto v6rInfo = platform::PlatformDetector::get_v6r_info(vmSize);
        auto halResult = integration::FlightCoreIntegration::create_hal_component("v6r-" + vmSize);
        
        ASSERT_TRUE(error::is_ok(halResult)) << "Failed to create HAL for V6R " << vmSize;
        
        if (error::is_ok(halResult)) {
            const auto& halComponent = error::unwrap(halResult);
            
            EXPECT_EQ(halComponent.platform, "v6r-" + vmSize);
            EXPECT_TRUE(halComponent.is_cloud_platform());
            EXPECT_EQ(halComponent.world, "flight:hal-world");
            
            // Verify V6R-specific metadata
            auto vmSizeMeta = std::find_if(halComponent.metadata.begin(),
                                         halComponent.metadata.end(),
                                         [](const auto& pair) {
                                             return pair.first == "vm_size";
                                         });
            ASSERT_NE(vmSizeMeta, halComponent.metadata.end());
            EXPECT_EQ(vmSizeMeta->second, vmSize);
            
            // Verify memory scaling
            if (vmSize == "small") {
                EXPECT_EQ(halComponent.memory_usage.total.bytes, 512 * 1024 * 1024);
            } else if (vmSize == "medium") {
                EXPECT_EQ(halComponent.memory_usage.total.bytes, 1024 * 1024 * 1024);
            } else if (vmSize == "large") {
                EXPECT_EQ(halComponent.memory_usage.total.bytes, 2048 * 1024 * 1024);
            }
        }
    }
}

TEST_F(HALPlatformIntegrationTest, HALComponentErrorHandling) {
    // Test HAL component error handling
    auto invalidHalResult = integration::FlightCoreIntegration::create_hal_component("invalid-platform");
    EXPECT_TRUE(error::is_err(invalidHalResult));
    
    if (error::is_err(invalidHalResult)) {
        const auto& error = error::unwrap_err(invalidHalResult);
        EXPECT_EQ(error.category(), error::ErrorCategory::Platform);
        EXPECT_TRUE(error.is_recoverable());
    }
    
    // Test memory exhaustion scenario
    auto exhaustionResult = integration::FlightCoreIntegration::create_hal_component("dreamcast-exhausted");
    if (error::is_err(exhaustionResult)) {
        const auto& error = error::unwrap_err(exhaustionResult);
        EXPECT_EQ(error.category(), error::ErrorCategory::Memory);
        EXPECT_FALSE(error.is_recoverable()); // Memory exhaustion not recoverable
    }
}

TEST_F(HALPlatformIntegrationTest, CrossPlatformHALCompatibility) {
    // Test cross-platform HAL compatibility
    std::vector<std::pair<std::string, platform::PlatformCapability>> platforms = {
        {"dreamcast", platform::PlatformCapability::Minimal},
        {"psp", platform::PlatformCapability::Basic},
        {"vita", platform::PlatformCapability::Standard},
        {"v6r-small", platform::PlatformCapability::Enhanced},
        {"v6r-medium", platform::PlatformCapability::Full},
        {"v6r-large", platform::PlatformCapability::Unlimited}
    };
    
    for (const auto& [platformId, expectedCapability] : platforms) {
        auto halResult = integration::FlightCoreIntegration::create_hal_component(platformId);
        
        if (error::is_ok(halResult)) {
            const auto& halComponent = error::unwrap(halResult);
            
            // Verify capability mapping
            auto capabilityMeta = std::find_if(halComponent.metadata.begin(),
                                             halComponent.metadata.end(),
                                             [](const auto& pair) {
                                                 return pair.first == "platform_capability";
                                             });
            ASSERT_NE(capabilityMeta, halComponent.metadata.end());
            
            // All HAL components should have consistent interface
            EXPECT_TRUE(halComponent.world.starts_with("flight:"));
            EXPECT_TRUE(halComponent.id.starts_with("flight-hal-"));
            
            // Memory usage should be proportional to capability
            if (expectedCapability == platform::PlatformCapability::Minimal) {
                EXPECT_LE(halComponent.memory_usage.total.bytes, 32 * 1024 * 1024);
            } else if (expectedCapability == platform::PlatformCapability::Unlimited) {
                EXPECT_GE(halComponent.memory_usage.total.bytes, 512 * 1024 * 1024);
            }
        }
    }
}
