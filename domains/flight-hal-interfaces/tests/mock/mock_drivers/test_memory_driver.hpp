/**
 * @file test_memory_driver.hpp
 * @brief Testing-Focused Mock Memory Driver
 * 
 * Comprehensive mock memory driver specifically designed for unit testing
 * with configurable behavior, error injection, and state verification.
 */

#pragma once

#include "../mock_driver_base.hpp"
#include "../../../include/flight/hal/interfaces/memory.hpp"
#include "../../../include/flight/hal/allocators/linear_allocator.hpp"
#include "../../../include/flight/hal/allocators/pool_allocator.hpp"
#include "../../../include/flight/hal/core/driver_auto_registration.hpp"
#include <unordered_map>
#include <memory>
#include <atomic>

namespace flight::hal::test {

/**
 * @brief Mock allocation entry for tracking
 */
struct MockAllocation {
    size_t size;
    MemoryAlignment alignment;
    MemoryFlags flags;
    MemoryType type;
    void* ptr;
    std::string debug_name;
    std::chrono::high_resolution_clock::time_point created_at;
    bool valid;
    
    MockAllocation() = default;
    MockAllocation(size_t s, MemoryAlignment a, MemoryFlags f, MemoryType t, void* p, const std::string& name)
        : size(s), alignment(a), flags(f), type(t), ptr(p), debug_name(name),
          created_at(std::chrono::high_resolution_clock::now()), valid(true) {}
};

/**
 * @brief Testing-focused mock memory driver
 * 
 * Provides comprehensive memory interface simulation with:
 * - Configurable allocation behavior
 * - Memory pressure simulation
 * - Allocator lifecycle tracking
 * - Error injection capabilities
 * - Performance simulation
 */
class TestMemoryDriver : public MockDriverBase<IMemoryInterface>, public IMemoryPressureMonitor {
public:
    TestMemoryDriver() : MockDriverBase("TestMemoryDriver"), next_allocation_id_(1) {
        // Set up memory-specific capabilities
        add_capability(HALCapability::DMA);
        add_capability(HALCapability::VirtualMemory);
        
        // Initialize memory constraints (simulate limited embedded system)
        total_memory_ = 16 * 1024 * 1024; // 16MB
        available_memory_ = total_memory_;
        max_allocation_size_ = 4 * 1024 * 1024; // 4MB
        
        // Initialize allocator availability
        available_allocators_ = {
            AllocatorType::System,
            AllocatorType::Linear,
            AllocatorType::Pool
        };
        
        // Set up realistic memory behavior
        configure_memory_behavior();
    }
    
    ~TestMemoryDriver() override = default;
    
    // === Core Allocation Methods ===
    
    HALResult<MemoryAllocation> allocate(const AllocationRequest& request) override {
        MOCK_METHOD_WITH_PARAMS("allocate", MemoryAllocation,
            TRACK_PARAM("size", request.size);
            TRACK_PARAM("alignment", static_cast<int>(request.alignment));
            TRACK_PARAM("flags", static_cast<int>(request.flags));
            TRACK_PARAM("preferred_type", static_cast<int>(request.preferred_type));,
            
            auto result = simulate_allocation(request.size, request.alignment, request.flags, request.preferred_type, "allocation_request");
            if (result) {
                TRACK_RETURN(result.value());
            }
            return result;
        );
    }
    
    HALResult<MemoryAllocation> allocate(size_t size, MemoryAlignment alignment, MemoryFlags flags) override {
        auto result = simulate_allocation(size, alignment, flags, MemoryType::System, "default_allocation");
        return result;
    }
    
    HALResult<MemoryAllocation> allocate_typed(size_t size, MemoryType type, 
                                              MemoryAlignment alignment, MemoryFlags flags) override {
        auto result = simulate_allocation(size, alignment, flags, type, "typed_allocation");
        return result;
    }
    
    HALResult<void> deallocate(void* ptr) override {
        MOCK_METHOD_WITH_PARAMS("deallocate", void,
            TRACK_PARAM("ptr", ptr);,
            
            auto it = allocations_.find(ptr);
            if (it == allocations_.end()) {
                auto error = errors::invalid_parameter(1, "Invalid pointer");
                TRACK_ERROR(error.message());
                return HALResult<void>::error(error);
            }
            
            auto& allocation = it->second;
            if (!allocation.valid) {
                auto error = errors::invalid_parameter(2, "Double free detected");
                TRACK_ERROR(error.message());
                return HALResult<void>::error(error);
            }
            
            // Return memory to available pool
            available_memory_ += allocation.size;
            allocation.valid = false;
            
            MOCK_RESOURCE_DESTROY("Memory", reinterpret_cast<uintptr_t>(ptr));
            
            allocations_.erase(it);
            return HALResult<void>::success();
        );
    }
    
    HALResult<MemoryAllocation> reallocate(void* ptr, size_t new_size) override {
        MOCK_METHOD_WITH_PARAMS("reallocate", MemoryAllocation,
            TRACK_PARAM("ptr", ptr);
            TRACK_PARAM("new_size", new_size);,
            
            if (ptr == nullptr) {
                return allocate(new_size);
            }
            
            auto it = allocations_.find(ptr);
            if (it == allocations_.end()) {
                auto error = errors::invalid_parameter(1, "Invalid pointer");
                TRACK_ERROR(error.message());
                return HALResult<MemoryAllocation>::error(error);
            }
            
            auto old_allocation = it->second;
            
            // Simulate reallocation by deallocating and allocating new
            auto dealloc_result = deallocate(ptr);
            if (!dealloc_result) {
                return HALResult<MemoryAllocation>::error(dealloc_result.error());
            }
            
            auto new_result = simulate_allocation(new_size, old_allocation.alignment, 
                                                old_allocation.flags, old_allocation.type, 
                                                old_allocation.debug_name);
            
            if (new_result) {
                TRACK_RETURN(new_result.value());
            }
            return new_result;
        );
    }
    
    // === Memory Information Methods ===
    
    HALResult<MemoryStats> get_memory_stats() const override {
        MOCK_METHOD_IMPL("get_memory_stats", MemoryStats,
            MemoryStats stats;
            stats.total_memory = total_memory_;
            stats.available_memory = available_memory_;
            stats.used_memory = total_memory_ - available_memory_;
            stats.allocation_count = allocations_.size();
            stats.peak_memory_usage = total_memory_ - min_available_memory_;
            stats.fragmentation_ratio = calculate_fragmentation();
            
            TRACK_RETURN(stats);
            return HALResult<MemoryStats>::success(stats);
        );
    }
    
    HALResult<MemoryStats> get_memory_stats(MemoryType type) const override {
        MOCK_METHOD_WITH_PARAMS("get_memory_stats_typed", MemoryStats,
            TRACK_PARAM("type", static_cast<int>(type));,
            
            MemoryStats stats;
            stats.total_memory = total_memory_;
            stats.available_memory = available_memory_;
            
            // Calculate type-specific usage
            size_t type_usage = 0;
            size_t type_count = 0;
            for (const auto& pair : allocations_) {
                if (pair.second.type == type) {
                    type_usage += pair.second.size;
                    type_count++;
                }
            }
            
            stats.used_memory = type_usage;
            stats.allocation_count = type_count;
            stats.peak_memory_usage = type_usage; // Simplified
            stats.fragmentation_ratio = 0.0;
            
            TRACK_RETURN(stats);
            return HALResult<MemoryStats>::success(stats);
        );
    }
    
    HALResult<std::vector<MemoryRegion>> get_memory_regions() const override {
        MOCK_METHOD_IMPL("get_memory_regions", std::vector<MemoryRegion>,
            std::vector<MemoryRegion> regions;
            
            // Simulate memory regions
            MemoryRegion main_region;
            main_region.base_address = reinterpret_cast<void*>(0x80000000);
            main_region.size = total_memory_;
            main_region.type = MemoryType::Default;
            main_region.flags = MemoryFlags::ReadWrite;
            main_region.name = "Main Memory";
            regions.push_back(main_region);
            
            TRACK_RETURN(regions);
            return HALResult<std::vector<MemoryRegion>>::success(regions);
        );
    }
    
    bool is_valid_pointer(void* ptr) const override {
        TRACK_CALL(const_cast<MockStateTracker&>(get_state_tracker()), "is_valid_pointer");
        TRACK_PARAM("ptr", ptr);
        
        auto it = allocations_.find(ptr);
        bool valid = (it != allocations_.end()) && it->second.valid;
        
        TRACK_RETURN(valid);
        return valid;
    }
    
    HALResult<MemoryAllocation> get_allocation_info(void* ptr) const override {
        MOCK_METHOD_WITH_PARAMS("get_allocation_info", MemoryAllocation,
            TRACK_PARAM("ptr", ptr);,
            
            auto it = allocations_.find(ptr);
            if (it == allocations_.end() || !it->second.valid) {
                auto error = errors::invalid_parameter(1, "Invalid pointer");
                TRACK_ERROR(error.message());
                return HALResult<MemoryAllocation>::error(error);
            }
            
            auto& mock_alloc = it->second;
            MemoryAllocation alloc;
            alloc.ptr = mock_alloc.ptr;
            alloc.size = mock_alloc.size;
            alloc.alignment = mock_alloc.alignment;
            alloc.flags = mock_alloc.flags;
            alloc.type = mock_alloc.type;
            alloc.debug_name = mock_alloc.debug_name;
            
            TRACK_RETURN(alloc);
            return HALResult<MemoryAllocation>::success(alloc);
        );
    }
    
    // === Allocator Management ===
    
    HALResult<IMemoryAllocator*> get_allocator(AllocatorType type) override {
        MOCK_METHOD_WITH_PARAMS("get_allocator", IMemoryAllocator*,
            TRACK_PARAM("type", static_cast<int>(type));,
            
            auto it = std::find(available_allocators_.begin(), available_allocators_.end(), type);
            if (it == available_allocators_.end()) {
                auto error = errors::feature_not_supported(1, "Allocator type not supported");
                TRACK_ERROR(error.message());
                return HALResult<IMemoryAllocator*>::error(error);
            }
            
            // Return mock allocator pointer (simplified)
            auto* allocator = reinterpret_cast<IMemoryAllocator*>(0x12345678 + static_cast<int>(type));
            TRACK_RETURN(allocator);
            return HALResult<IMemoryAllocator*>::success(allocator);
        );
    }
    
    HALResult<AllocatorStats> get_allocator_stats(AllocatorType type) const override {
        MOCK_METHOD_WITH_PARAMS("get_allocator_stats", AllocatorStats,
            TRACK_PARAM("type", static_cast<int>(type));,
            
            AllocatorStats stats;
            stats.total_size = total_memory_ / 4; // Simplified allocation per allocator
            stats.used_size = stats.total_size / 2;
            stats.available_size = stats.total_size - stats.used_size;
            stats.allocation_count = 10; // Mock value
            stats.fragmentation_ratio = 0.1;
            
            TRACK_RETURN(stats);
            return HALResult<AllocatorStats>::success(stats);
        );
    }
    
    std::vector<AllocatorType> get_available_allocators() const override {
        TRACK_CALL(const_cast<MockStateTracker&>(get_state_tracker()), "get_available_allocators");
        TRACK_RETURN(available_allocators_);
        return available_allocators_;
    }
    
    // === Memory Pressure Management ===
    
    IMemoryPressureMonitor* get_pressure_monitor() override {
        TRACK_CALL(get_state_tracker(), "get_pressure_monitor");
        TRACK_RETURN(static_cast<IMemoryPressureMonitor*>(this));
        return this;
    }
    
    HALResult<uint32_t> register_pressure_callback(MemoryPressureLevel level,
                                                   MemoryPressureCallback callback) override {
        MOCK_METHOD_WITH_PARAMS("register_pressure_callback", uint32_t,
            TRACK_PARAM("level", static_cast<int>(level));,
            
            uint32_t callback_id = next_callback_id_++;
            pressure_callbacks_[callback_id] = {level, callback};
            
            TRACK_RETURN(callback_id);
            return HALResult<uint32_t>::success(callback_id);
        );
    }
    
    HALResult<void> unregister_pressure_callback(uint32_t callback_id) override {
        MOCK_METHOD_WITH_PARAMS("unregister_pressure_callback", void,
            TRACK_PARAM("callback_id", callback_id);,
            
            auto it = pressure_callbacks_.find(callback_id);
            if (it == pressure_callbacks_.end()) {
                auto error = errors::invalid_parameter(1, "Invalid callback ID");
                TRACK_ERROR(error.message());
                return HALResult<void>::error(error);
            }
            
            pressure_callbacks_.erase(it);
            return HALResult<void>::success();
        );
    }
    
    // === Capability Queries ===
    
    bool supports_alignment(MemoryAlignment alignment) const override {
        return static_cast<size_t>(alignment) <= 64; // Mock constraint
    }
    
    bool supports_flags(MemoryFlags flags) const override {
        // Support most common flags
        return (static_cast<uint32_t>(flags) & 0xFF) == static_cast<uint32_t>(flags);
    }
    
    bool supports_memory_type(MemoryType type) const override {
        return type == MemoryType::Default || type == MemoryType::DMA || type == MemoryType::VideoMemory;
    }
    
    bool supports_allocator_type(AllocatorType type) const override {
        auto it = std::find(available_allocators_.begin(), available_allocators_.end(), type);
        return it != available_allocators_.end();
    }
    
    size_t get_max_allocation_size() const override {
        return max_allocation_size_;
    }
    
    size_t get_max_allocation_size(MemoryType type) const override {
        // Simulate type-specific limits
        switch (type) {
            case MemoryType::VideoMemory:
                return max_allocation_size_ / 2;
            case MemoryType::DMA:
                return max_allocation_size_ / 4;
            default:
                return max_allocation_size_;
        }
    }
    
    // === Memory Management Operations ===
    
    HALResult<void> defragment(DefragmentationCallback callback) override {
        MOCK_VOID_METHOD("defragment",
            // Simulate defragmentation
            if (callback) {
                callback(50); // 50% progress
                callback(100); // Complete
            }
            return HALResult<void>::success();
        );
    }
    
    HALResult<void> defragment_allocator(AllocatorType type, DefragmentationCallback callback) override {
        MOCK_METHOD_WITH_PARAMS("defragment_allocator", void,
            TRACK_PARAM("type", static_cast<int>(type));,
            
            if (!supports_allocator_type(type)) {
                auto error = errors::feature_not_supported(1, "Allocator type not supported");
                TRACK_ERROR(error.message());
                return HALResult<void>::error(error);
            }
            
            // Simulate allocator-specific defragmentation
            if (callback) {
                callback(25);
                callback(75);
                callback(100);
            }
            return HALResult<void>::success();
        );
    }
    
    HALResult<void> gc_hint() override {
        MOCK_VOID_METHOD("gc_hint",
            // Simulate garbage collection hint
            return HALResult<void>::success();
        );
    }
    
    HALResult<size_t> trim_memory() override {
        MOCK_METHOD_IMPL("trim_memory", size_t,
            // Simulate memory trimming
            size_t trimmed = available_memory_ / 10;
            
            TRACK_RETURN(trimmed);
            return HALResult<size_t>::success(trimmed);
        );
    }
    
    // === IMemoryPressureMonitor Implementation ===
    
    HALResult<uint32_t> register_callback(MemoryPressureLevel level, MemoryPressureCallback callback) override {
        return register_pressure_callback(level, callback);
    }
    
    HALResult<void> unregister_callback(uint32_t callback_id) override {
        return unregister_pressure_callback(callback_id);
    }
    
    MemoryPressureInfo get_pressure_info() const override {
        TRACK_CALL(const_cast<MockStateTracker&>(get_state_tracker()), "get_pressure_info");
        
        MemoryPressureInfo info;
        info.current_level = calculate_pressure_level();
        info.available_memory = available_memory_;
        info.total_memory = total_memory_;
        info.pressure_threshold_low = total_memory_ * 0.8;
        info.pressure_threshold_medium = total_memory_ * 0.9;
        info.pressure_threshold_high = total_memory_ * 0.95;
        
        TRACK_RETURN(info);
        return info;
    }
    
    HALResult<void> check_pressure() override {
        MOCK_VOID_METHOD("check_pressure",
            auto level = calculate_pressure_level();
            if (level != MemoryPressureLevel::None) {
                // Trigger callbacks
                for (const auto& pair : pressure_callbacks_) {
                    if (pair.second.first == level) {
                        pair.second.second(get_pressure_info());
                    }
                }
            }
            return HALResult<void>::success();
        );
    }
    
    HALResult<void> set_emergency_reserve(size_t bytes) override {
        MOCK_METHOD_WITH_PARAMS("set_emergency_reserve", void,
            TRACK_PARAM("bytes", bytes);,
            
            emergency_reserve_ = bytes;
            return HALResult<void>::success();
        );
    }
    
    size_t get_emergency_reserve() const override {
        return emergency_reserve_;
    }
    
    // === Testing Interface ===
    
    /**
     * @brief Simulate memory pressure by reducing available memory
     */
    void simulate_memory_pressure(double pressure_ratio) {
        available_memory_ = static_cast<size_t>(total_memory_ * (1.0 - pressure_ratio));
        min_available_memory_ = std::min(min_available_memory_, available_memory_);
    }
    
    /**
     * @brief Get current allocation count
     */
    size_t get_allocation_count() const {
        return allocations_.size();
    }
    
    /**
     * @brief Check if pointer was allocated by this driver
     */
    bool has_allocation(void* ptr) const {
        return allocations_.find(ptr) != allocations_.end();
    }
    
    /**
     * @brief Force fragmentation for testing
     */
    void force_fragmentation() {
        fragmentation_factor_ = 0.5; // 50% fragmentation
    }

private:
    /**
     * @brief Configure realistic memory behavior
     */
    void configure_memory_behavior() {
        using namespace behavior_presets;
        
        // Configure allocation to occasionally fail under pressure
        configure_method("allocate", fail_randomly(5.0, HALErrorCategory::Resource, 1, "Memory allocation failed"));
        
        // Configure deallocate with minimal delay
        configure_method("deallocate", fixed_timing(std::chrono::microseconds(1)));
        
        // Configure memory stats with realistic timing
        configure_method("get_memory_stats", realistic_hardware());
    }
    
    /**
     * @brief Simulate memory allocation
     */
    HALResult<MemoryAllocation> simulate_allocation(size_t size, MemoryAlignment alignment, 
                                                   MemoryFlags flags, MemoryType type,
                                                   const std::string& debug_name) {
        // Check size constraints
        if (size == 0) {
            return HALResult<MemoryAllocation>::error(errors::invalid_parameter(1, "Zero size allocation"));
        }
        
        if (size > get_max_allocation_size(type)) {
            return HALResult<MemoryAllocation>::error(errors::parameter_out_of_range(1, "Allocation too large"));
        }
        
        // Check alignment support
        if (!supports_alignment(alignment)) {
            return HALResult<MemoryAllocation>::error(errors::feature_not_supported(1, "Alignment not supported"));
        }
        
        // Check available memory
        if (size > available_memory_) {
            return HALResult<MemoryAllocation>::error(errors::out_of_memory(1, "Insufficient memory"));
        }
        
        // Simulate allocation
        void* ptr = reinterpret_cast<void*>(0x80000000 + next_allocation_id_);
        uint32_t allocation_id = next_allocation_id_++;
        
        MockAllocation mock_alloc(size, alignment, flags, type, ptr, debug_name);
        allocations_[ptr] = mock_alloc;
        
        available_memory_ -= size;
        min_available_memory_ = std::min(min_available_memory_, available_memory_);
        
        MOCK_RESOURCE_CREATE("Memory", allocation_id, size, debug_name);
        
        MemoryAllocation result;
        result.ptr = ptr;
        result.size = size;
        result.alignment = alignment;
        result.flags = flags;
        result.type = type;
        result.debug_name = debug_name;
        
        return HALResult<MemoryAllocation>::success(result);
    }
    
    /**
     * @brief Calculate current memory pressure level
     */
    MemoryPressureLevel calculate_pressure_level() const {
        double usage_ratio = 1.0 - (static_cast<double>(available_memory_) / total_memory_);
        
        if (usage_ratio > 0.95) return MemoryPressureLevel::Critical;
        if (usage_ratio > 0.85) return MemoryPressureLevel::High;
        if (usage_ratio > 0.70) return MemoryPressureLevel::Medium;
        if (usage_ratio > 0.50) return MemoryPressureLevel::Low;
        return MemoryPressureLevel::None;
    }
    
    /**
     * @brief Calculate memory fragmentation ratio
     */
    double calculate_fragmentation() const {
        return fragmentation_factor_;
    }
    
    // Memory state
    std::atomic<uint32_t> next_allocation_id_;
    std::unordered_map<void*, MockAllocation> allocations_;
    
    size_t total_memory_;
    std::atomic<size_t> available_memory_;
    std::atomic<size_t> min_available_memory_{SIZE_MAX};
    size_t max_allocation_size_;
    size_t emergency_reserve_ = 0;
    double fragmentation_factor_ = 0.1;
    
    std::vector<AllocatorType> available_allocators_;
    
    // Pressure monitoring
    std::atomic<uint32_t> next_callback_id_{1};
    std::unordered_map<uint32_t, std::pair<MemoryPressureLevel, MemoryPressureCallback>> pressure_callbacks_;
};

} // namespace flight::hal::test

// Auto-register the test memory driver when compiled for testing
#ifdef FLIGHT_ENABLE_TEST_DRIVERS
#include "../../../include/flight/hal/core/driver_auto_registration.hpp"
REGISTER_HAL_DRIVER(flight::hal::IMemoryInterface, flight::hal::test::TestMemoryDriver);
#endif
