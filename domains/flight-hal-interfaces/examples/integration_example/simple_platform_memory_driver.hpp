/**
 * @file simple_platform_memory_driver.hpp
 * @brief Integration Guidelines Example: Simple Platform Memory Driver
 * 
 * This file demonstrates how to implement a basic HAL memory driver following
 * the Flight HAL Integration Guidelines. Use this as a template for your
 * platform-specific implementations.
 * 
 * Reference: docs/integration_guidelines.md
 */

#pragma once

#include "../../include/flight/hal/interfaces/memory.hpp"
#include "../../include/flight/hal/core/driver_auto_registration.hpp"
#include "../../include/flight/hal/core/hal_capabilities.hpp"
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <chrono>

namespace flight::hal::examples::integration {

/**
 * @brief Simple platform memory driver following integration guidelines
 * 
 * This driver demonstrates:
 * - Universal driver implementation patterns
 * - Platform-specific capability detection
 * - HALResult error handling patterns
 * - Resource management best practices
 * - Performance monitoring integration
 */
class SimplePlatformMemoryDriver : public IMemoryInterface {
public:
    // Driver metadata (Integration Guidelines Section 1.1)
    static constexpr int PRIORITY = 10;
    static constexpr const char* DRIVER_NAME = "Simple Platform Memory Driver";
    static constexpr const char* DRIVER_VERSION = "1.0.0";
    
    SimplePlatformMemoryDriver();
    ~SimplePlatformMemoryDriver() override;
    
    // === Core Memory Interface Implementation ===
    
    HALResult<MemoryAllocation> allocate(const AllocationRequest& request) override;
    
    HALResult<MemoryAllocation> allocate(size_t size, 
                                        MemoryAlignment alignment = MemoryAlignment::None, 
                                        MemoryFlags flags = MemoryFlags::None) override;
    
    HALResult<void> deallocate(void* ptr) override;
    
    HALResult<MemoryAllocation> reallocate(void* ptr, size_t new_size) override;
    
    // === Memory Information Methods ===
    
    HALResult<MemoryStats> get_memory_stats() const override;
    
    bool is_valid_pointer(void* ptr) const override;
    
    HALResult<MemoryAllocation> get_allocation_info(void* ptr) const override;
    
    // === Capability Queries ===
    
    bool supports_alignment(MemoryAlignment alignment) const override;
    
    bool supports_flags(MemoryFlags flags) const override;
    
    size_t get_max_allocation_size() const override;
    
    // === Memory Management Operations ===
    
    HALResult<void> gc_hint() override;
    
    HALResult<size_t> trim_memory() override;
    
    // === IHALInterface Implementation (Integration Guidelines Section 1.1) ===
    
    std::string_view get_driver_name() const override {
        return DRIVER_NAME;
    }
    
    int get_priority() const override {
        return PRIORITY;
    }
    
    HALResult<void> initialize() override;
    
    HALResult<void> shutdown() override;
    
    bool is_active() const override {
        return initialized_ && active_;
    }
    
    bool is_available() const override {
        return platform_is_available();
    }
    
    std::string_view get_version() const override {
        return DRIVER_VERSION;
    }
    
    // === ICapabilityProvider Implementation (Integration Guidelines Section 1.4) ===
    
    bool supports_capability(HALCapability capability) const override {
        return (capability_mask_ & static_cast<uint32_t>(capability)) != 0;
    }
    
    uint32_t get_capability_mask() const override {
        return capability_mask_;
    }
    
    std::vector<HALCapability> get_capabilities() const override;
    
    PerformanceTier get_performance_tier() const override {
        return performance_tier_;
    }
    
    const PlatformInfo& get_platform_info() const override {
        return *platform_info_;
    }
    
    bool has_fallback(HALCapability capability) const override {
        // Simple implementation - no fallbacks for basic driver
        return false;
    }

private:
    // === Allocation Tracking (Integration Guidelines Section 1.5) ===
    
    struct AllocationEntry {
        size_t size;
        MemoryAlignment alignment;
        MemoryFlags flags;
        std::chrono::steady_clock::time_point allocated_at;
        bool valid;
        
        AllocationEntry(size_t s, MemoryAlignment a, MemoryFlags f)
            : size(s), alignment(a), flags(f)
            , allocated_at(std::chrono::steady_clock::now())
            , valid(true) {}
    };
    
    // === Platform Detection (Integration Guidelines Section 2) ===
    
    /**
     * @brief Initialize platform-specific capabilities
     * 
     * This method demonstrates the capability detection pattern from
     * Integration Guidelines Section 1.4
     */
    void init_platform_capabilities();
    
    /**
     * @brief Initialize platform information structure
     */
    void init_platform_info();
    
    /**
     * @brief Check if platform is available for this driver
     */
    bool platform_is_available() const;
    
    // === Platform-Specific Allocation (Integration Guidelines Section 2) ===
    
    /**
     * @brief Platform-specific memory allocation
     * 
     * Platform teams should replace this with their native allocation API:
     * - macOS: mmap/malloc with madvise hints
     * - Dreamcast: KallistiOS allocation with cache alignment
     * - PSP: PSPSDK allocation with Media Engine consideration
     * - Web: Emscripten allocation with browser limits
     */
    void* platform_allocate(size_t size, MemoryAlignment alignment);
    
    /**
     * @brief Platform-specific memory deallocation
     */
    void platform_deallocate(void* ptr);
    
    /**
     * @brief Get system memory size using platform APIs
     */
    size_t get_system_memory_size() const;
    
    /**
     * @brief Check if allocation would exceed platform limits
     */
    bool would_exceed_platform_limits(size_t size) const;
    
    /**
     * @brief Calculate aligned size for allocation
     */
    size_t calculate_aligned_size(size_t size, MemoryAlignment alignment) const;
    
    // === Thread-Safe State Management ===
    
    mutable std::mutex allocations_mutex_;
    std::unordered_map<void*, AllocationEntry> allocations_;
    
    // === Performance Statistics (Integration Guidelines Section 4) ===
    
    std::atomic<size_t> total_allocated_{0};
    std::atomic<size_t> peak_allocated_{0};
    std::atomic<size_t> allocation_count_{0};
    std::atomic<size_t> deallocation_count_{0};
    
    // === Driver State ===
    
    bool initialized_ = false;
    bool active_ = false;
    
    // === Platform Capabilities ===
    
    uint32_t capability_mask_;
    PerformanceTier performance_tier_;
    std::unique_ptr<PlatformInfo> platform_info_;
    
    // === Platform Constraints ===
    
    size_t max_allocation_size_;
    size_t total_system_memory_;
    size_t memory_pressure_threshold_;
};

} // namespace flight::hal::examples::integration

// === Auto-Registration (Integration Guidelines Section 1.2) ===

#ifdef FLIGHT_ENABLE_INTEGRATION_EXAMPLES
REGISTER_HAL_DRIVER(flight::hal::IMemoryInterface, 
                   flight::hal::examples::integration::SimplePlatformMemoryDriver);
#endif
