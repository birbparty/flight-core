#include <catch2/catch_test_macros.hpp>
#include "flight/hal/macos/platform_coordinator.hpp"

using namespace flight::hal::macos;

TEST_CASE("PlatformCoordinator creation", "[unit][platform_coordinator]") {
    auto coordinator = PlatformCoordinator::create();
    REQUIRE(coordinator != nullptr);
}

TEST_CASE("Apple Silicon detection", "[unit][platform_coordinator]") {
    auto coordinator = PlatformCoordinator::create();
    REQUIRE(coordinator != nullptr);
    
    // Test Apple Silicon detection
    bool is_apple_silicon = coordinator->isAppleSilicon();
    
    SECTION("Apple Silicon detection works") {
        // This should work on any platform, returning true on Apple Silicon
        REQUIRE((is_apple_silicon == true || is_apple_silicon == false));
    }
    
    SECTION("M4 Max detection consistency") {
        bool is_m4_max = coordinator->isM4Max();
        
        // If it's M4 Max, it should also be Apple Silicon
        if (is_m4_max) {
            REQUIRE(is_apple_silicon == true);
        }
    }
}

TEST_CASE("System information retrieval", "[unit][platform_coordinator]") {
    auto coordinator = PlatformCoordinator::create();
    REQUIRE(coordinator != nullptr);
    
    auto system_info = coordinator->getSystemInfo();
    
    if (system_info.has_value()) {
        SECTION("Valid system information") {
            REQUIRE(!system_info->cpu_model.empty());
            REQUIRE(system_info->unified_memory_size > 0);
            
            // M4 Max should have specific characteristics
            if (coordinator->isM4Max()) {
                REQUIRE(system_info->performance_cores == 12);
                REQUIRE(system_info->efficiency_cores == 4);
                REQUIRE(system_info->gpu_cores == 40);
                REQUIRE(system_info->memory_bandwidth_gbps >= 500);
            }
        }
    }
}

TEST_CASE("Resource coordination", "[unit][platform_coordinator]") {
    auto coordinator = PlatformCoordinator::create();
    REQUIRE(coordinator != nullptr);
    
    SECTION("Apple Silicon optimizations") {
        auto result = coordinator->initializeAppleSiliconOptimizations();
        
        if (coordinator->isAppleSilicon()) {
            REQUIRE(result == CoordinationResult::Success);
        } else {
            REQUIRE(result == CoordinationResult::NotSupported);
        }
    }
    
    SECTION("CPU resource coordination") {
        auto result = coordinator->coordinateCPUResources();
        REQUIRE(result == CoordinationResult::Success);
    }
    
    SECTION("GPU resource coordination") {
        auto result = coordinator->coordinateGPUResources();
        REQUIRE(result == CoordinationResult::Success);
    }
    
    SECTION("Memory bandwidth optimization") {
        auto result = coordinator->optimizeMemoryBandwidth();
        REQUIRE(result == CoordinationResult::Success);
    }
}

TEST_CASE("Flight ecosystem coordination", "[unit][platform_coordinator]") {
    auto coordinator = PlatformCoordinator::create();
    REQUIRE(coordinator != nullptr);
    
    SECTION("CLI coordination") {
        auto result = coordinator->coordinateWithCLI();
        REQUIRE(result == CoordinationResult::Success);
    }
    
    SECTION("Runtime coordination") {
        auto result = coordinator->coordinateWithRuntime();
        REQUIRE(result == CoordinationResult::Success);
    }
    
    SECTION("Component flattening coordination") {
        auto result = coordinator->coordinateWithComponentFlattening();
        REQUIRE(result == CoordinationResult::Success);
    }
}

TEST_CASE("Performance monitoring", "[unit][platform_coordinator]") {
    auto coordinator = PlatformCoordinator::create();
    REQUIRE(coordinator != nullptr);
    
    SECTION("CPU utilization") {
        auto cpu_usage = coordinator->getCPUUtilization();
        
        if (coordinator->isM4Max()) {
            REQUIRE(cpu_usage.size() == 16); // 12 performance + 4 efficiency cores
        }
        
        // All values should be between 0.0 and 100.0
        for (const auto& usage : cpu_usage) {
            REQUIRE(usage >= 0.0f);
            REQUIRE(usage <= 100.0f);
        }
    }
    
    SECTION("GPU utilization") {
        auto gpu_usage = coordinator->getGPUUtilization();
        REQUIRE(gpu_usage >= 0.0f);
        REQUIRE(gpu_usage <= 100.0f);
    }
    
    SECTION("Memory pressure") {
        auto memory_pressure = coordinator->getMemoryPressure();
        REQUIRE(memory_pressure >= 0.0f);
        REQUIRE(memory_pressure <= 100.0f);
    }
    
    SECTION("Thermal state") {
        auto thermal_state = coordinator->getThermalState();
        REQUIRE(!thermal_state.empty());
    }
}
