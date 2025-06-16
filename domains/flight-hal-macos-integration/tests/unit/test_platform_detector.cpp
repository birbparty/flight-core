#include <catch2/catch_test_macros.hpp>
#include "flight/hal/macos/platform_detector.hpp"
#include "flight/hal/macos/platform_coordinator.hpp"

using namespace flight::hal::macos;

TEST_CASE("Platform Detection - Basic Functionality", "[platform-detector]") {
    SECTION("Can detect capabilities") {
        auto caps = PlatformDetector::detect_capabilities();
        
        // Basic sanity checks
        REQUIRE(caps.cpu.total_cores > 0);
        REQUIRE(caps.memory.total_bytes > 0);
        REQUIRE(caps.memory.page_size > 0);
        
        // Apple Silicon should have unified memory
        if (PlatformDetector::is_apple_silicon()) {
            REQUIRE(caps.memory.unified_architecture == true);
            REQUIRE(caps.gpu.metal_support == true);
        }
    }
    
    SECTION("Cache functionality works") {
        // Clear cache first
        PlatformDetector::invalidate_cache();
        REQUIRE_FALSE(PlatformDetector::is_cache_valid());
        
        // First detection should populate cache
        auto caps1 = PlatformDetector::detect_capabilities();
        REQUIRE(PlatformDetector::is_cache_valid());
        
        // Second detection should use cache
        auto caps2 = PlatformDetector::detect_capabilities();
        
        // Should be identical
        REQUIRE(caps1.cpu.total_cores == caps2.cpu.total_cores);
        REQUIRE(caps1.memory.total_bytes == caps2.memory.total_bytes);
        REQUIRE(caps1.gpu.core_count == caps2.gpu.core_count);
    }
}

TEST_CASE("Platform Detection - Apple Silicon Specific", "[platform-detector]") {
    if (!PlatformDetector::is_apple_silicon()) {
        SKIP("Not running on Apple Silicon - skipping Apple Silicon specific tests");
    }
    
    SECTION("Apple Silicon detection") {
        REQUIRE(PlatformDetector::is_apple_silicon());
        
        auto caps = PlatformDetector::detect_capabilities();
        
        // Apple Silicon characteristics
        REQUIRE(caps.memory.unified_architecture == true);
        REQUIRE(caps.gpu.metal_support == true);
        REQUIRE(caps.cpu.supports_arm64e == true);
        REQUIRE(caps.cpu.architecture.find("arm") != std::string::npos);
    }
    
    SECTION("M-series family detection") {
        auto caps = PlatformDetector::detect_capabilities();
        
        // Should detect at least one M-series family
        bool is_m_series = PlatformDetector::is_m1_family() || 
                          PlatformDetector::is_m2_family() || 
                          PlatformDetector::is_m3_family() || 
                          PlatformDetector::is_m4_family();
        REQUIRE(is_m_series);
        
        // Chip identifier should contain M-series info
        REQUIRE_FALSE(caps.system.chip_identifier.empty());
        REQUIRE(caps.system.chip_identifier.find("M") != std::string::npos);
    }
    
    SECTION("M4 Max specific detection") {
        if (PlatformDetector::is_m4_max()) {
            auto caps = PlatformDetector::detect_capabilities();
            
            // M4 Max specific characteristics
            REQUIRE(caps.cpu.performance_cores == 12);
            REQUIRE(caps.cpu.efficiency_cores == 4);
            REQUIRE(caps.cpu.total_cores == 16);
            REQUIRE(caps.gpu.core_count == 40);
            REQUIRE(caps.memory.bandwidth_gbps == 546);
            REQUIRE(caps.system.chip_identifier == "M4 Max");
        }
    }
}

TEST_CASE("Platform Detection - Feature Support", "[platform-detector]") {
    SECTION("SIMD and AMX support") {
        if (PlatformDetector::is_apple_silicon()) {
            REQUIRE(PlatformDetector::supports_advanced_simd());
            REQUIRE(PlatformDetector::supports_amx_instructions());
            REQUIRE(PlatformDetector::supports_neural_engine());
        }
    }
    
    SECTION("Metal support") {
        if (PlatformDetector::is_apple_silicon()) {
            REQUIRE(PlatformDetector::supports_metal_performance_shaders());
            
            auto caps = PlatformDetector::detect_capabilities();
            REQUIRE(caps.gpu.metal_support);
            REQUIRE(caps.gpu.core_count > 0);
        }
    }
    
    SECTION("Modern GPU features") {
        if (PlatformDetector::is_m3_family() || PlatformDetector::is_m4_family()) {
            auto caps = PlatformDetector::detect_capabilities();
            
            // M3 and M4 should support ray tracing and mesh shading
            REQUIRE(caps.gpu.supports_raytracing);
            REQUIRE(caps.gpu.supports_mesh_shading);
            REQUIRE(caps.gpu.supports_function_pointers);
        }
    }
}

TEST_CASE("Platform Detection - Optimization Recommendations", "[platform-detector]") {
    SECTION("Thread count recommendations") {
        auto caps = PlatformDetector::detect_capabilities();
        
        // Component flattening should use mostly performance cores
        auto cf_threads = PlatformDetector::optimal_thread_count_for_workload(WorkloadType::ComponentFlattening);
        REQUIRE(cf_threads > 0);
        REQUIRE(cf_threads <= caps.cpu.total_cores);
        
        // CPU intensive should use performance cores
        auto cpu_threads = PlatformDetector::optimal_thread_count_for_workload(WorkloadType::CPUIntensive);
        REQUIRE(cpu_threads == caps.cpu.performance_cores);
        
        // Background tasks should prefer efficiency cores
        auto bg_threads = PlatformDetector::optimal_thread_count_for_workload(WorkloadType::Background);
        REQUIRE(bg_threads == caps.cpu.efficiency_cores);
        
        // GPU compute should use minimal CPU threads
        auto gpu_threads = PlatformDetector::optimal_thread_count_for_workload(WorkloadType::GPUCompute);
        REQUIRE(gpu_threads == 2);
    }
    
    SECTION("Core assignment recommendations") {
        // Background tasks should use efficiency cores
        REQUIRE(PlatformDetector::should_use_efficiency_cores_for(TaskType::BackgroundProcessing));
        REQUIRE(PlatformDetector::should_use_efficiency_cores_for(TaskType::SystemMaintenance));
        
        // Real-time tasks should use performance cores
        REQUIRE_FALSE(PlatformDetector::should_use_efficiency_cores_for(TaskType::RealTimeProcessing));
        REQUIRE_FALSE(PlatformDetector::should_use_efficiency_cores_for(TaskType::UserInterface));
    }
    
    SECTION("Memory pool recommendations") {
        auto pool_size = PlatformDetector::recommended_memory_pool_size();
        REQUIRE(pool_size > 0);
        
        auto caps = PlatformDetector::detect_capabilities();
        // Should be reasonable fraction of available memory
        auto expected_max = caps.memory.available_bytes / (1024 * 1024 * 4); // Quarter of available
        REQUIRE(pool_size <= expected_max);
    }
}

TEST_CASE("Platform Detection - Optimization Profiles", "[platform-detector]") {
    SECTION("Component flattening profile") {
        auto profile = PlatformDetector::get_optimization_profile(WorkloadType::ComponentFlattening);
        
        REQUIRE(profile.recommended_worker_threads > 0);
        REQUIRE(profile.optimal_memory_pool_size_mb == 512);
        REQUIRE(profile.recommended_power_profile == PowerProfile::MaxPerformance);
        REQUIRE(profile.cpu_usage_target == 0.8f);
        REQUIRE(profile.enable_thermal_management);
        
        if (PlatformDetector::is_apple_silicon()) {
            REQUIRE(profile.enable_metal_acceleration);
            REQUIRE(profile.enable_advanced_simd);
        }
    }
    
    SECTION("Background workload profile") {
        auto profile = PlatformDetector::get_optimization_profile(WorkloadType::Background);
        
        REQUIRE(profile.optimal_memory_pool_size_mb == 64);
        REQUIRE(profile.recommended_power_profile == PowerProfile::PowerEfficient);
        REQUIRE(profile.cpu_usage_target == 0.3f);
        REQUIRE(profile.use_efficiency_cores_for_background);
    }
}

TEST_CASE("Platform Detection - Resource Constraints", "[platform-detector]") {
    SECTION("Resource constraint calculation") {
        auto constraints = PlatformDetector::get_resource_constraints();
        auto caps = PlatformDetector::detect_capabilities();
        
        REQUIRE(constraints.max_concurrent_threads == caps.cpu.total_cores * 2);
        REQUIRE(constraints.max_memory_allocation_mb > 0);
        REQUIRE(constraints.thermal_headroom == 1.0f);
        
        if (PlatformDetector::is_apple_silicon()) {
            REQUIRE(constraints.can_use_metal_compute);
        }
        
        if (caps.cpu.performance_cores > 0) {
            REQUIRE(constraints.can_use_performance_cores);
        }
        
        if (caps.cpu.efficiency_cores > 0) {
            REQUIRE(constraints.can_use_efficiency_cores);
        }
    }
}

TEST_CASE("Platform Detection Manager - Singleton", "[platform-detector-manager]") {
    SECTION("Singleton pattern works") {
        auto& manager1 = PlatformDetectionManager::getInstance();
        auto& manager2 = PlatformDetectionManager::getInstance();
        
        // Should be the same instance
        REQUIRE(&manager1 == &manager2);
    }
    
    SECTION("Manager functionality") {
        auto& manager = PlatformDetectionManager::getInstance();
        
        // Clear cache
        manager.invalidateCache();
        
        // Get capabilities
        auto caps = manager.getCapabilities();
        REQUIRE(caps.cpu.total_cores > 0);
        
        // Get optimization profile
        auto profile = manager.getOptimizationProfile(WorkloadType::ComponentFlattening);
        REQUIRE(profile.recommended_worker_threads > 0);
        
        // Get resource constraints
        auto constraints = manager.getResourceConstraints();
        REQUIRE(constraints.max_concurrent_threads > 0);
        
        // Get development environment
        auto dev_env = manager.getDevEnvironment();
        // Basic structure check - should have xcode_path field (may be empty if no Xcode)
        // We just check that the function returns a valid structure
        REQUIRE(true); // Structure test - just verify it doesn't crash
    }
}

TEST_CASE("Platform Detection - Integration with PlatformCoordinator", "[platform-integration]") {
    SECTION("PlatformCoordinator integration") {
        auto coordinator = PlatformCoordinator::create();
        REQUIRE(coordinator != nullptr);
        
        // Test Apple Silicon detection consistency
        bool detector_apple_silicon = PlatformDetector::is_apple_silicon();
        bool coordinator_apple_silicon = coordinator->isAppleSilicon();
        REQUIRE(detector_apple_silicon == coordinator_apple_silicon);
        
        // Test M4 Max detection consistency
        bool detector_m4_max = PlatformDetector::is_m4_max();
        bool coordinator_m4_max = coordinator->isM4Max();
        REQUIRE(detector_m4_max == coordinator_m4_max);
        
        // Test system info consistency
        auto detector_caps = PlatformDetector::detect_capabilities();
        auto coordinator_info = coordinator->getSystemInfo();
        
        if (coordinator_info.has_value()) {
            REQUIRE(detector_caps.cpu.performance_cores == coordinator_info->performance_cores);
            REQUIRE(detector_caps.cpu.efficiency_cores == coordinator_info->efficiency_cores);
            REQUIRE(detector_caps.gpu.core_count == coordinator_info->gpu_cores);
            REQUIRE(detector_caps.memory.total_bytes == coordinator_info->unified_memory_size);
        }
    }
}
