/**
 * @file advanced_registration_example.cpp
 * @brief Advanced Driver Registration System Example
 * 
 * Demonstrates the enhanced driver registration system with:
 * - Multiple drivers per interface
 * - Priority-based selection
 * - Capability-based requirements
 * - Factory-based registration
 * - Fallback handling
 */

#include "flight/hal/core/driver_registry.hpp"
#include "flight/hal/core/driver_auto_registration.hpp"
#include "flight/hal/interfaces/memory.hpp"
#include <iostream>
#include <memory>
#include <vector>

using namespace flight::hal;

/**
 * @brief Mock platform memory driver (high performance)
 */
class PlatformMemoryDriver : public IMemoryInterface {
public:
    std::string_view get_interface_name() const override { return "IMemoryInterface"; }
    std::string_view get_driver_name() const override { return "PlatformMemoryDriver"; }
    int get_priority() const override { return static_cast<int>(DriverPriority::Platform); }
    std::string_view get_version() const override { return "2.1.0"; }
    
    // ICapabilityProvider implementation
    bool supports_capability(HALCapability capability) const override {
        return (capability == HALCapability::DMA || 
                capability == HALCapability::VirtualMemory ||
                capability == HALCapability::MemoryProtection);
    }
    
    uint32_t get_capability_mask() const override {
        return static_cast<uint32_t>(HALCapability::DMA) |
               static_cast<uint32_t>(HALCapability::VirtualMemory) |
               static_cast<uint32_t>(HALCapability::MemoryProtection);
    }
    
    std::vector<HALCapability> get_capabilities() const override {
        return {HALCapability::DMA, HALCapability::VirtualMemory, HALCapability::MemoryProtection};
    }
    
    PerformanceTier get_performance_tier() const override { return PerformanceTier::High; }
    
    const PlatformInfo& get_platform_info() const override {
        static PlatformInfo info{
            "Mock Platform", "x86_64", PerformanceTier::High, 
            1024 * 1024 * 1024, 8, true, true
        };
        return info;
    }
    
    bool has_fallback(HALCapability capability) const override { return false; }
    
    // IMemoryInterface implementation
    HALResult<MemoryAllocation> allocate(size_t size, MemoryAlignment alignment, MemoryFlags flags) override {
        std::cout << "PlatformMemoryDriver: Allocating " << size << " bytes with DMA support\n";
        MemoryAllocation alloc;
        alloc.ptr = malloc(size);
        alloc.size = size;
        alloc.alignment = alignment;
        alloc.flags = flags;
        return HALResult<MemoryAllocation>::success(alloc);
    }
    
    HALResult<void> deallocate(void* ptr) override {
        std::cout << "PlatformMemoryDriver: Deallocating memory\n";
        free(ptr);
        return HALResult<void>::success();
    }
    
    HALResult<MemoryAllocation> reallocate(void* ptr, size_t new_size) override {
        std::cout << "PlatformMemoryDriver: Reallocating to " << new_size << " bytes\n";
        MemoryAllocation alloc;
        alloc.ptr = realloc(ptr, new_size);
        alloc.size = new_size;
        return HALResult<MemoryAllocation>::success(alloc);
    }
    
    HALResult<MemoryStats> get_memory_stats() const override {
        MemoryStats stats;
        stats.total_bytes = 1024 * 1024 * 1024; // 1GB
        stats.used_bytes = 64 * 1024 * 1024;    // 64MB
        stats.free_bytes = stats.total_bytes - stats.used_bytes;
        stats.peak_used_bytes = 128 * 1024 * 1024;   // 128MB
        stats.allocation_count = 42;
        stats.fragmentation_pct = 5; // 5%
        return HALResult<MemoryStats>::success(stats);
    }
    
    bool supports_alignment(MemoryAlignment alignment) const override { return true; }
    bool supports_flags(MemoryFlags flags) const override { return true; }
    size_t get_max_allocation_size() const override { return 512 * 1024 * 1024; } // 512MB
    bool is_valid_pointer(void* ptr) const override { return ptr != nullptr; }
    HALResult<void> defragment() override { return HALResult<void>::success(); }
    
    // IHALInterface implementation
    HALResult<void> initialize() override {
        std::cout << "PlatformMemoryDriver: Initializing high-performance memory driver\n";
        active_ = true;
        return HALResult<void>::success();
    }
    
    HALResult<void> shutdown() override {
        std::cout << "PlatformMemoryDriver: Shutting down\n";
        active_ = false;
        return HALResult<void>::success();
    }
    
    bool is_active() const override { return active_; }
    bool is_available() const override { return true; }

private:
    bool active_ = false;
};

/**
 * @brief Basic memory driver (fallback)
 */
class BasicMemoryDriver : public IMemoryInterface {
public:
    std::string_view get_interface_name() const override { return "IMemoryInterface"; }
    std::string_view get_driver_name() const override { return "BasicMemoryDriver"; }
    int get_priority() const override { return static_cast<int>(DriverPriority::Low); }
    std::string_view get_version() const override { return "1.0.0"; }
    
    // ICapabilityProvider implementation
    bool supports_capability(HALCapability capability) const override { return false; }
    uint32_t get_capability_mask() const override { return 0; }
    std::vector<HALCapability> get_capabilities() const override { return {}; }
    PerformanceTier get_performance_tier() const override { return PerformanceTier::Limited; }
    
    const PlatformInfo& get_platform_info() const override {
        static PlatformInfo info{
            "Basic Platform", "generic", PerformanceTier::Limited, 
            16 * 1024 * 1024, 1, false, false
        };
        return info;
    }
    
    bool has_fallback(HALCapability capability) const override { return true; }
    
    // IMemoryInterface implementation
    HALResult<MemoryAllocation> allocate(size_t size, MemoryAlignment alignment, MemoryFlags flags) override {
        std::cout << "BasicMemoryDriver: Basic allocation of " << size << " bytes\n";
        MemoryAllocation alloc;
        alloc.ptr = malloc(size);
        alloc.size = size;
        alloc.alignment = alignment;
        alloc.flags = flags;
        return HALResult<MemoryAllocation>::success(alloc);
    }
    
    HALResult<void> deallocate(void* ptr) override {
        std::cout << "BasicMemoryDriver: Basic deallocation\n";
        free(ptr);
        return HALResult<void>::success();
    }
    
    HALResult<MemoryAllocation> reallocate(void* ptr, size_t new_size) override {
        std::cout << "BasicMemoryDriver: Basic reallocation to " << new_size << " bytes\n";
        MemoryAllocation alloc;
        alloc.ptr = realloc(ptr, new_size);
        alloc.size = new_size;
        return HALResult<MemoryAllocation>::success(alloc);
    }
    
    HALResult<MemoryStats> get_memory_stats() const override {
        MemoryStats stats;
        stats.total_bytes = 16 * 1024 * 1024;  // 16MB
        stats.used_bytes = 4 * 1024 * 1024;    // 4MB
        stats.free_bytes = stats.total_bytes - stats.used_bytes;
        stats.peak_used_bytes = 8 * 1024 * 1024;    // 8MB
        stats.allocation_count = 12;
        stats.fragmentation_pct = 15; // 15%
        return HALResult<MemoryStats>::success(stats);
    }
    
    bool supports_alignment(MemoryAlignment alignment) const override { 
        return alignment == MemoryAlignment::None; 
    }
    bool supports_flags(MemoryFlags flags) const override { 
        return flags == MemoryFlags::None; 
    }
    size_t get_max_allocation_size() const override { return 8 * 1024 * 1024; } // 8MB
    bool is_valid_pointer(void* ptr) const override { return ptr != nullptr; }
    HALResult<void> defragment() override { 
        std::cout << "BasicMemoryDriver: Basic defragmentation\n";
        return HALResult<void>::success(); 
    }
    
    // IHALInterface implementation
    HALResult<void> initialize() override {
        std::cout << "BasicMemoryDriver: Initializing basic memory driver\n";
        active_ = true;
        return HALResult<void>::success();
    }
    
    HALResult<void> shutdown() override {
        std::cout << "BasicMemoryDriver: Shutting down\n";
        active_ = false;
        return HALResult<void>::success();
    }
    
    bool is_active() const override { return active_; }
    bool is_available() const override { return true; }

private:
    bool active_ = false;
};

/**
 * @brief Factory function for creating memory drivers based on platform detection
 */
std::unique_ptr<IMemoryInterface> create_adaptive_memory_driver() {
    std::cout << "Factory: Creating adaptive memory driver based on platform\n";
    
    // Simulate platform detection
    bool has_dma_support = true; // Would be actual platform detection
    
    if (has_dma_support) {
        std::cout << "Factory: Platform supports DMA, creating PlatformMemoryDriver\n";
        return std::make_unique<PlatformMemoryDriver>();
    } else {
        std::cout << "Factory: Basic platform detected, creating BasicMemoryDriver\n";
        return std::make_unique<BasicMemoryDriver>();
    }
}

// Auto-register drivers with different priorities
REGISTER_HAL_DRIVER(IMemoryInterface, BasicMemoryDriver);

// Manual registration with comprehensive metadata
void register_advanced_drivers() {
    auto& registry = DriverRegistry::instance();
    
    // Register platform driver with comprehensive info
    DriverInfo platform_info;
    platform_info.name = "PlatformMemoryDriver";
    platform_info.version = Version{2, 1, 0};
    platform_info.priority = static_cast<int>(DriverPriority::Platform);
    platform_info.performance_tier = PerformanceTier::High;
    platform_info.capabilities = static_cast<uint32_t>(HALCapability::DMA) |
                                 static_cast<uint32_t>(HALCapability::VirtualMemory) |
                                 static_cast<uint32_t>(HALCapability::MemoryProtection);
    platform_info.supported_platforms = 0xFFFFFFFF;
    platform_info.memory_overhead = 2048; // 2KB overhead
    platform_info.supports_hot_swap = true;
    platform_info.description = "High-performance platform memory driver with DMA support";
    platform_info.dependencies = {};
    platform_info.fallback_drivers = {"BasicMemoryDriver"};
    platform_info.factory = []() -> std::unique_ptr<IHALInterface> {
        return std::make_unique<PlatformMemoryDriver>();
    };
    
    auto result = registry.register_driver_with_info<IMemoryInterface>(platform_info);
    if (result.is_success()) {
        std::cout << "Successfully registered PlatformMemoryDriver with comprehensive metadata\n";
    } else {
        std::cout << "Failed to register PlatformMemoryDriver: " << result.error_message() << "\n";
    }
    
    // Register factory-based adaptive driver
    DriverInfo adaptive_info;
    adaptive_info.name = "AdaptiveMemoryDriver";
    adaptive_info.version = Version{1, 5, 0};
    adaptive_info.priority = static_cast<int>(DriverPriority::High);
    adaptive_info.performance_tier = PerformanceTier::Standard;
    adaptive_info.capabilities = 0; // Will be determined at runtime
    adaptive_info.supported_platforms = 0xFFFFFFFF;
    adaptive_info.memory_overhead = 512; // 512B overhead
    adaptive_info.supports_hot_swap = false;
    adaptive_info.description = "Adaptive memory driver that selects implementation based on platform";
    adaptive_info.factory = []() -> std::unique_ptr<IHALInterface> {
        return create_adaptive_memory_driver();
    };
    
    result = registry.register_driver_factory<IMemoryInterface>(
        create_adaptive_memory_driver, adaptive_info);
    
    if (result.is_success()) {
        std::cout << "Successfully registered AdaptiveMemoryDriver factory\n";
    } else {
        std::cout << "Failed to register AdaptiveMemoryDriver: " << result.error_message() << "\n";
    }
}

void demonstrate_driver_selection() {
    auto& registry = DriverRegistry::instance();
    
    std::cout << "\n=== Driver Selection Demonstration ===\n";
    
    // Show all registered drivers
    std::cout << "Total registered drivers: " << registry.driver_count() << "\n";
    std::cout << "Memory interface drivers: " << registry.interface_driver_count<IMemoryInterface>() << "\n";
    
    auto driver_info = registry.get_driver_info<IMemoryInterface>();
    std::cout << "\nRegistered memory drivers:\n";
    for (const auto& info : driver_info) {
        std::cout << "  - " << info.name << " v" << info.version.to_string() 
                  << " (priority: " << info.priority << ")\n";
    }
    
    // Test 1: Get best available driver (highest priority)
    std::cout << "\n--- Test 1: Get best available driver ---\n";
    auto best_driver = registry.get_interface<IMemoryInterface>();
    if (best_driver) {
        std::cout << "Selected driver: " << best_driver->get_driver_name() 
                  << " (priority: " << best_driver->get_priority() << ")\n";
        
        // Initialize and test the driver
        auto init_result = best_driver->initialize();
        if (init_result.is_success()) {
            auto alloc_result = best_driver->allocate(1024);
            if (alloc_result.is_success()) {
                std::cout << "Successfully allocated 1024 bytes\n";
                best_driver->deallocate(alloc_result.value().ptr);
            }
        }
    }
    
    // Test 2: Get driver with specific capabilities
    std::cout << "\n--- Test 2: Get driver with DMA capability ---\n";
    CapabilityRequirements dma_requirements;
    dma_requirements.required_capabilities = static_cast<uint32_t>(HALCapability::DMA);
    dma_requirements.minimum_performance = PerformanceTier::Standard;
    
    auto dma_driver = registry.get_interface_with_requirements<IMemoryInterface>(dma_requirements);
    if (dma_driver) {
        std::cout << "Selected DMA-capable driver: " << dma_driver->get_driver_name() << "\n";
    } else {
        std::cout << "No driver found with DMA capability\n";
    }
    
    // Test 3: Get specific driver by name
    std::cout << "\n--- Test 3: Get specific driver by name ---\n";
    auto basic_driver = registry.get_interface_by_name<IMemoryInterface>("BasicMemoryDriver");
    if (basic_driver) {
        std::cout << "Found BasicMemoryDriver by name\n";
        auto init_result = basic_driver->initialize();
        if (init_result.is_success()) {
            auto stats_result = basic_driver->get_memory_stats();
            if (stats_result.is_success()) {
                const auto& stats = stats_result.value();
                std::cout << "Memory stats - Total: " << stats.total_bytes 
                          << ", Used: " << stats.used_bytes << "\n";
            }
        }
    }
    
    // Test 4: Get all drivers for comparison
    std::cout << "\n--- Test 4: Get all memory drivers ---\n";
    auto all_drivers = registry.get_all_interfaces<IMemoryInterface>();
    std::cout << "Retrieved " << all_drivers.size() << " memory drivers:\n";
    for (auto& driver : all_drivers) {
        std::cout << "  - " << driver->get_driver_name() 
                  << " (tier: " << static_cast<int>(driver->get_performance_tier()) << ")\n";
    }
}

void demonstrate_capability_filtering() {
    auto& registry = DriverRegistry::instance();
    
    std::cout << "\n=== Capability-Based Selection ===\n";
    
    // Test different capability requirements
    std::vector<std::pair<std::string, CapabilityRequirements>> test_cases = {
        {
            "High-performance with DMA",
            {
                static_cast<uint32_t>(HALCapability::DMA),
                static_cast<uint32_t>(HALCapability::VirtualMemory),
                PerformanceTier::High,
                SIZE_MAX,
                false
            }
        },
        {
            "Low memory overhead",
            {
                0, // No required capabilities
                0,
                PerformanceTier::Minimal,
                1024, // Max 1KB overhead
                false
            }
        },
        {
            "Hot-swappable driver",
            {
                0,
                0,
                PerformanceTier::Minimal,
                SIZE_MAX,
                true
            }
        }
    };
    
    for (const auto& [description, requirements] : test_cases) {
        std::cout << "\n--- Testing: " << description << " ---\n";
        auto driver = registry.get_interface_with_requirements<IMemoryInterface>(requirements);
        if (driver) {
            std::cout << "Found suitable driver: " << driver->get_driver_name() << "\n";
        } else {
            std::cout << "No driver meets the requirements\n";
        }
    }
}

int main() {
    std::cout << "=== Enhanced Driver Registration System Demo ===\n\n";
    
    // Register advanced drivers
    register_advanced_drivers();
    
    // Initialize all drivers
    auto& registry = DriverRegistry::instance();
    auto init_result = registry.initialize_all();
    if (init_result.is_success()) {
        std::cout << "All drivers initialized successfully\n";
    } else {
        std::cout << "Driver initialization failed: " << init_result.error_message() << "\n";
    }
    
    // Demonstrate driver selection
    demonstrate_driver_selection();
    
    // Demonstrate capability-based filtering
    demonstrate_capability_filtering();
    
    // Cleanup
    std::cout << "\n=== Cleanup ===\n";
    registry.shutdown_all();
    std::cout << "All drivers shut down\n";
    
    return 0;
}
