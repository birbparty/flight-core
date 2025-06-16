/**
 * @file software_memory_driver.hpp
 * @brief Software-Only Memory Driver Reference Implementation
 * 
 * This driver demonstrates a complete, portable memory management implementation
 * using only standard C++ libraries. It serves as both a working fallback
 * implementation and an educational example of HAL driver patterns.
 * 
 * Key Features:
 * - Pure C++17 implementation (no platform dependencies)
 * - Comprehensive allocation tracking and debugging
 * - Multiple allocation strategies (standard, pool, linear)
 * - Memory pressure monitoring and management
 * - Full HAL interface compliance
 * - Performance monitoring and statistics
 */

#pragma once

#include "../../../../include/flight/hal/interfaces/memory.hpp"
#include "../../../../include/flight/hal/core/driver_auto_registration.hpp"
#include "../../../../include/flight/hal/core/hal_capabilities.hpp"
#include "../../../../include/flight/hal/allocators/pool_allocator.hpp"
#include "../../../../include/flight/hal/allocators/linear_allocator.hpp"

#include <memory>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace flight::hal::software {

/**
 * @brief Configuration for software memory driver
 */
struct SoftwareMemoryConfig {
    size_t max_allocation_size = 64 * 1024 * 1024;      // 64MB max single allocation
    size_t total_memory_limit = 512 * 1024 * 1024;      // 512MB total limit
    size_t pool_block_size = 4096;                      // 4KB pool blocks
    size_t linear_arena_size = 16 * 1024 * 1024;        // 16MB linear arena
    double memory_pressure_threshold = 0.85;            // 85% usage triggers pressure
    bool enable_debug_tracking = true;                  // Enable allocation tracking
    bool enable_memory_poisoning = false;               // Poison freed memory (debug)
    bool enable_leak_detection = true;                  // Track potential leaks
    uint32_t gc_threshold_ms = 5000;                    // GC hint threshold (5 seconds)
};

/**
 * @brief Software-only memory pressure monitor
 * 
 * Monitors memory usage and triggers callbacks when pressure levels change.
 * This implementation provides a template for platform-specific monitors.
 */
class SoftwareMemoryPressureMonitor : public IMemoryPressureMonitor {
public:
    explicit SoftwareMemoryPressureMonitor(const SoftwareMemoryConfig& config);
    ~SoftwareMemoryPressureMonitor() override = default;

    // IMemoryPressureMonitor implementation
    HALResult<uint32_t> register_callback(MemoryPressureLevel level, 
                                         MemoryPressureCallback callback) override;
    HALResult<void> unregister_callback(uint32_t callback_id) override;
    MemoryPressureInfo get_pressure_info() const override;
    HALResult<void> check_pressure() override;
    HALResult<void> set_emergency_reserve(size_t bytes) override;
    size_t get_emergency_reserve() const override;

    // Internal methods for driver use
    void update_memory_usage(size_t total_used, size_t total_available);
    void trigger_pressure_check();

private:
    struct PressureCallback {
        uint32_t id;
        MemoryPressureLevel level;
        MemoryPressureCallback callback;
    };

    void invoke_callbacks(MemoryPressureLevel new_level);
    MemoryPressureLevel calculate_pressure_level(double usage_ratio) const;

    mutable std::mutex callbacks_mutex_;
    std::vector<PressureCallback> callbacks_;
    std::atomic<uint32_t> next_callback_id_{1};
    
    mutable std::mutex pressure_mutex_;
    MemoryPressureInfo current_pressure_;
    size_t emergency_reserve_size_;
    
    const SoftwareMemoryConfig& config_;
};

/**
 * @brief Software-only memory allocator base class
 * 
 * Provides common functionality for different allocation strategies.
 */
class SoftwareAllocatorBase : public IMemoryAllocator {
public:
    explicit SoftwareAllocatorBase(AllocatorType type, std::string_view name);
    ~SoftwareAllocatorBase() override = default;

    // IMemoryAllocator implementation
    AllocatorType get_type() const override { return allocator_type_; }
    std::string_view get_name() const override { return allocator_name_; }
    
    bool supports_size(size_t size) const override;
    bool supports_alignment(size_t alignment) const override;

protected:
    // Statistics tracking
    void record_allocation(size_t size);
    void record_deallocation(size_t size);
    void update_fragmentation_ratio(double ratio);

    AllocatorType allocator_type_;
    std::string allocator_name_;
    
    // Performance statistics
    std::atomic<size_t> total_allocated_{0};
    std::atomic<size_t> allocation_count_{0};
    std::atomic<double> fragmentation_ratio_{0.0};
    
    static constexpr size_t MAX_ALLOCATION_SIZE = 64 * 1024 * 1024;  // 64MB
    static constexpr size_t MAX_ALIGNMENT = 4096;                    // 4KB page alignment
};

/**
 * @brief Standard allocator using malloc/free
 * 
 * Simple wrapper around standard allocation functions with tracking.
 */
class SoftwareStandardAllocator : public SoftwareAllocatorBase {
public:
    SoftwareStandardAllocator();
    ~SoftwareStandardAllocator() override;

    // IMemoryAllocator implementation
    HALResult<void*> allocate(size_t size, size_t alignment = alignof(std::max_align_t)) override;
    HALResult<void> deallocate(void* ptr) override;
    HALResult<void*> reallocate(void* ptr, size_t new_size) override;
    AllocatorStats get_stats() const override;
    bool owns_pointer(void* ptr) const override;
    HALResult<void> reset() override;
    HALResult<void> defragment(DefragmentationCallback callback = nullptr) override;

private:
    struct AllocationInfo {
        size_t size;
        size_t alignment;
        std::chrono::steady_clock::time_point allocated_at;
    };

    mutable std::mutex allocations_mutex_;
    std::unordered_map<void*, AllocationInfo> allocations_;
};

/**
 * @brief Complete software memory driver implementation
 * 
 * This driver provides a reference implementation demonstrating:
 * - Multiple allocator strategies
 * - Comprehensive error handling
 * - Memory pressure monitoring
 * - Performance tracking
 * - Resource management
 * - Platform capability reporting
 */
class SoftwareMemoryDriver : public IMemoryInterface {
public:
    // Driver metadata
    static constexpr int PRIORITY = 5;  // Medium priority (fallback implementation)
    static constexpr const char* DRIVER_NAME = "Software Memory Driver";
    static constexpr const char* DRIVER_VERSION = "1.0.0";

    explicit SoftwareMemoryDriver(const SoftwareMemoryConfig& config = SoftwareMemoryConfig{});
    ~SoftwareMemoryDriver() override;

    // === Core Memory Interface Implementation ===

    HALResult<MemoryAllocation> allocate(const AllocationRequest& request) override;
    
    HALResult<MemoryAllocation> allocate(size_t size, 
                                        MemoryAlignment alignment = MemoryAlignment::None, 
                                        MemoryFlags flags = MemoryFlags::None) override;
    
    HALResult<MemoryAllocation> allocate_typed(size_t size,
                                              MemoryType type,
                                              MemoryAlignment alignment = MemoryAlignment::None,
                                              MemoryFlags flags = MemoryFlags::None) override;
    
    HALResult<void> deallocate(void* ptr) override;
    
    HALResult<MemoryAllocation> reallocate(void* ptr, size_t new_size) override;

    // === Memory Information Methods ===

    HALResult<MemoryStats> get_memory_stats() const override;
    HALResult<MemoryStats> get_memory_stats(MemoryType type) const override;
    HALResult<std::vector<MemoryRegion>> get_memory_regions() const override;
    bool is_valid_pointer(void* ptr) const override;
    HALResult<MemoryAllocation> get_allocation_info(void* ptr) const override;

    // === Allocator Management ===

    HALResult<IMemoryAllocator*> get_allocator(AllocatorType type) override;
    HALResult<AllocatorStats> get_allocator_stats(AllocatorType type) const override;
    std::vector<AllocatorType> get_available_allocators() const override;

    // === Memory Pressure Management ===

    IMemoryPressureMonitor* get_pressure_monitor() override;
    HALResult<uint32_t> register_pressure_callback(MemoryPressureLevel level,
                                                   MemoryPressureCallback callback) override;
    HALResult<void> unregister_pressure_callback(uint32_t callback_id) override;

    // === Capability Queries ===

    bool supports_alignment(MemoryAlignment alignment) const override;
    bool supports_flags(MemoryFlags flags) const override;
    bool supports_memory_type(MemoryType type) const override;
    bool supports_allocator_type(AllocatorType type) const override;
    size_t get_max_allocation_size() const override;
    size_t get_max_allocation_size(MemoryType type) const override;

    // === Memory Management Operations ===

    HALResult<void> defragment(DefragmentationCallback callback = nullptr) override;
    HALResult<void> defragment_allocator(AllocatorType type, 
                                        DefragmentationCallback callback = nullptr) override;
    HALResult<void> gc_hint() override;
    HALResult<size_t> trim_memory() override;

    // === IHALInterface Implementation ===

    std::string_view get_driver_name() const override { return DRIVER_NAME; }
    int get_priority() const override { return PRIORITY; }
    HALResult<void> initialize() override;
    HALResult<void> shutdown() override;
    bool is_active() const override { return initialized_ && active_; }
    bool is_available() const override;
    std::string_view get_version() const override { return DRIVER_VERSION; }

    // === ICapabilityProvider Implementation ===

    bool supports_capability(HALCapability capability) const override;
    uint32_t get_capability_mask() const override { return capability_mask_; }
    std::vector<HALCapability> get_capabilities() const override;
    PerformanceTier get_performance_tier() const override { return performance_tier_; }
    const PlatformInfo& get_platform_info() const override { return *platform_info_; }
    bool has_fallback(HALCapability capability) const override;

private:
    // === Initialization Methods ===
    void init_platform_capabilities();
    void init_platform_info();
    void init_allocators();
    void init_pressure_monitor();

    // === Allocation Management ===
    struct AllocationEntry {
        size_t size;
        MemoryAlignment alignment;
        MemoryFlags flags;
        MemoryType type;
        AllocatorType allocator;
        std::chrono::steady_clock::time_point allocated_at;
        bool valid;

        AllocationEntry(size_t s, MemoryAlignment a, MemoryFlags f, MemoryType t, AllocatorType alloc)
            : size(s), alignment(a), flags(f), type(t), allocator(alloc)
            , allocated_at(std::chrono::steady_clock::now()), valid(true) {}
    };

    HALResult<MemoryAllocation> allocate_internal(const AllocationRequest& request);
    IMemoryAllocator* select_allocator(const AllocationRequest& request);
    bool validate_allocation_request(const AllocationRequest& request) const;
    void track_allocation(void* ptr, const AllocationEntry& entry);
    void untrack_allocation(void* ptr);
    void update_memory_pressure();

    // === Memory Poisoning (Debug) ===
    void poison_memory(void* ptr, size_t size);
    void check_memory_poison(void* ptr, size_t size);

    // === Configuration ===
    SoftwareMemoryConfig config_;

    // === Driver State ===
    std::atomic<bool> initialized_{false};
    std::atomic<bool> active_{false};

    // === Platform Capabilities ===
    uint32_t capability_mask_;
    PerformanceTier performance_tier_;
    std::unique_ptr<PlatformInfo> platform_info_;

    // === Allocators ===
    std::unique_ptr<SoftwareStandardAllocator> standard_allocator_;
    std::unique_ptr<flight::hal::allocators::PoolAllocator> pool_allocator_;
    std::unique_ptr<flight::hal::allocators::LinearAllocator> linear_allocator_;

    // === Memory Pressure Monitoring ===
    std::unique_ptr<SoftwareMemoryPressureMonitor> pressure_monitor_;

    // === Allocation Tracking ===
    mutable std::mutex allocations_mutex_;
    std::unordered_map<void*, AllocationEntry> allocations_;

    // === Performance Statistics ===
    std::atomic<size_t> total_allocated_{0};
    std::atomic<size_t> peak_allocated_{0};
    std::atomic<size_t> allocation_count_{0};
    std::atomic<size_t> deallocation_count_{0};
    std::atomic<uint64_t> last_gc_hint_time_{0};

    // === Debug Features ===
    std::atomic<bool> leak_detection_enabled_{true};
    static constexpr uint8_t POISON_BYTE = 0xDD;  // Debug poison pattern
};

// === Auto-Registration ===
REGISTER_HAL_DRIVER(flight::hal::IMemoryInterface, SoftwareMemoryDriver);

} // namespace flight::hal::software
