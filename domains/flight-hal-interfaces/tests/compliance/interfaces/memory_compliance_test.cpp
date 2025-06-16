/**
 * @file memory_compliance_test.cpp
 * @brief Comprehensive Compliance Tests for Memory Interface
 * 
 * Tests memory interface implementation for compliance with specification,
 * performance requirements, and error handling across all target platforms.
 */

#include "../base/hal_interface_compliance_test.hpp"
#include "../../../include/flight/hal/interfaces/memory.hpp"
#include "../../../include/flight/hal/core/driver_registry.hpp"

#include <random>
#include <unordered_set>
#include <algorithm>
#include <cstring>

namespace flight::hal::testing {

/**
 * @brief Simple Mock Memory Driver for Testing
 * 
 * Provides a minimal memory interface implementation for testing compliance.
 */
class SimpleMockMemoryDriver : public IMemoryInterface {
public:
    HALResult<MemoryAllocation> allocate(const AllocationRequest& request) override {
        return allocate(request.size, request.alignment, request.flags);
    }
    
    HALResult<MemoryAllocation> allocate(size_t size, 
                                       MemoryAlignment alignment = MemoryAlignment::None, 
                                       MemoryFlags flags = MemoryFlags::None) override {
        if (size == 0 || size > 1024 * 1024 * 100) {  // Limit to 100MB
            return HALResult<MemoryAllocation>::error(errors::invalid_parameter(0));
        }
        
        void* ptr = std::aligned_alloc(static_cast<size_t>(alignment == MemoryAlignment::None ? 8 : static_cast<size_t>(alignment)), size);
        if (!ptr) {
            return HALResult<MemoryAllocation>::error(errors::resource_exhausted(0));
        }
        
        // Clear memory if Zero flag is set
        if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(MemoryFlags::Zero)) {
            std::memset(ptr, 0, size);
        }
        
        MemoryAllocation allocation;
        allocation.ptr = ptr;
        allocation.size = size;
        allocation.alignment = alignment;
        allocation.flags = flags;
        allocation.type = MemoryType::System;
        allocation.allocator = AllocatorType::System;
        
        allocations_[ptr] = allocation;
        stats_.used_bytes += size;
        stats_.allocation_count++;
        stats_.peak_used_bytes = std::max(stats_.peak_used_bytes, stats_.used_bytes);
        
        return HALResult<MemoryAllocation>::success(std::move(allocation));
    }
    
    HALResult<MemoryAllocation> allocate_typed(size_t size,
                                              MemoryType type,
                                              MemoryAlignment alignment = MemoryAlignment::None,
                                              MemoryFlags flags = MemoryFlags::None) override {
        auto result = allocate(size, alignment, flags);
        if (result.is_ok()) {
            auto allocation = result.value();
            allocation.type = type;
            allocations_[allocation.ptr] = allocation;
            return HALResult<MemoryAllocation>::success(std::move(allocation));
        }
        return result;
    }
    
    HALResult<void> deallocate(void* ptr) override {
        if (!ptr) {
            return HALResult<void>::error(errors::invalid_parameter(0));
        }
        
        auto it = allocations_.find(ptr);
        if (it == allocations_.end()) {
            return HALResult<void>::error(errors::invalid_parameter(0));
        }
        
        stats_.used_bytes -= it->second.size;
        stats_.allocation_count--;
        allocations_.erase(it);
        
        std::free(ptr);
        return HALResult<void>::success();
    }
    
    HALResult<MemoryAllocation> reallocate(void* ptr, size_t new_size) override {
        if (!ptr) {
            return allocate(new_size);
        }
        
        auto it = allocations_.find(ptr);
        if (it == allocations_.end()) {
            return HALResult<MemoryAllocation>::error(errors::invalid_parameter(0));
        }
        
        auto old_allocation = it->second;
        void* new_ptr = std::realloc(ptr, new_size);
        if (!new_ptr) {
            return HALResult<MemoryAllocation>::error(errors::resource_exhausted(0));
        }
        
        // Update statistics
        stats_.used_bytes = stats_.used_bytes - old_allocation.size + new_size;
        stats_.peak_used_bytes = std::max(stats_.peak_used_bytes, stats_.used_bytes);
        
        // Update allocation record
        allocations_.erase(it);
        MemoryAllocation new_allocation = old_allocation;
        new_allocation.ptr = new_ptr;
        new_allocation.size = new_size;
        allocations_[new_ptr] = new_allocation;
        
        return HALResult<MemoryAllocation>::success(std::move(new_allocation));
    }
    
    HALResult<MemoryStats> get_memory_stats() const override {
        return HALResult<MemoryStats>::success(stats_);
    }
    
    HALResult<MemoryStats> get_memory_stats(MemoryType /*type*/) const override {
        return get_memory_stats();
    }
    
    HALResult<std::vector<MemoryRegion>> get_memory_regions() const override {
        std::vector<MemoryRegion> regions;
        return HALResult<std::vector<MemoryRegion>>::success(std::move(regions));
    }
    
    bool is_valid_pointer(void* ptr) const override {
        return allocations_.find(ptr) != allocations_.end();
    }
    
    HALResult<MemoryAllocation> get_allocation_info(void* ptr) const override {
        auto it = allocations_.find(ptr);
        if (it == allocations_.end()) {
            return HALResult<MemoryAllocation>::error(HALError::InvalidParameter);
        }
        return HALResult<MemoryAllocation>::success(it->second);
    }
    
    // Capability queries
    bool supports_alignment(MemoryAlignment /*alignment*/) const override { return true; }
    bool supports_flags(MemoryFlags /*flags*/) const override { return true; }
    bool supports_memory_type(MemoryType /*type*/) const override { return true; }
    bool supports_allocator_type(AllocatorType /*type*/) const override { return true; }
    size_t get_max_allocation_size() const override { return 1024 * 1024 * 100; }
    size_t get_max_allocation_size(MemoryType /*type*/) const override { return get_max_allocation_size(); }
    
    // Stub implementations for remaining methods
    HALResult<IMemoryAllocator*> get_allocator(AllocatorType /*type*/) override {
        return HALResult<IMemoryAllocator*>::error(HALError::NotSupported);
    }
    
    HALResult<AllocatorStats> get_allocator_stats(AllocatorType /*type*/) const override {
        return HALResult<AllocatorStats>::error(HALError::NotSupported);
    }
    
    std::vector<AllocatorType> get_available_allocators() const override {
        return {AllocatorType::System};
    }
    
    IMemoryPressureMonitor* get_pressure_monitor() override { return nullptr; }
    
    HALResult<uint32_t> register_pressure_callback(MemoryPressureLevel /*level*/,
                                                   MemoryPressureCallback /*callback*/) override {
        return HALResult<uint32_t>::error(HALError::NotSupported);
    }
    
    HALResult<void> unregister_pressure_callback(uint32_t /*callback_id*/) override {
        return HALResult<void>::error(HALError::NotSupported);
    }
    
    HALResult<void> defragment(DefragmentationCallback /*callback*/) override {
        return HALResult<void>::success();
    }
    
    HALResult<void> defragment_allocator(AllocatorType /*type*/, 
                                        DefragmentationCallback /*callback*/) override {
        return HALResult<void>::success();
    }
    
    HALResult<void> gc_hint() override {
        return HALResult<void>::success();
    }
    
    HALResult<size_t> trim_memory() override {
        return HALResult<size_t>::success(0);
    }

private:
    std::unordered_map<void*, MemoryAllocation> allocations_;
    MemoryStats stats_{};
};

/**
 * @brief Memory Interface Compliance Test Suite
 * 
 * Comprehensive test suite for validating memory interface implementations
 * across all supported platforms with emphasis on correctness, performance,
 * and robust error handling.
 */
class MemoryComplianceTest : public HALInterfaceComplianceTest<IMemoryInterface> {
protected:
    /**
     * @brief Create memory interface instance for testing
     * @return Unique pointer to memory interface
     */
    InterfacePtr create_interface() override {
        return std::make_unique<SimpleMockMemoryDriver>();
    }
    
    /**
     * @brief Get memory-specific test configuration
     * @return Test configuration optimized for memory testing
     */
    TestConfig get_test_config() override {
        TestConfig config = HALInterfaceComplianceTest::get_test_config();
        
        // Memory-specific configuration
        config.max_memory_usage = 1024 * 1024 * 50;  // 50MB for memory tests
        config.timeout = std::chrono::milliseconds{10000};  // Longer timeout for memory ops
        config.limits.max_allocation_size = 1024 * 1024 * 10;  // 10MB max
        config.limits.max_latency = std::chrono::microseconds{1000};
        
        return config;
    }
    
    /**
     * @brief Cleanup memory test resources
     */
    void cleanup_test_resources() override {
        // Cleanup any allocated test memory
        for (auto* ptr : test_allocations_) {
            if (ptr && get_interface().is_valid_pointer(ptr)) {
                get_interface().deallocate(ptr);
            }
        }
        test_allocations_.clear();
    }
    
    /**
     * @brief Validate no memory leaks occurred
     */
    void validate_resource_cleanup() override {
        // Get final memory statistics
        auto stats_result = get_interface().get_memory_stats();
        if (stats_result.is_ok()) {
            auto stats = stats_result.value();
            
            // Ensure no significant memory leaks
            EXPECT_EQ(stats.allocation_count, 0) 
                << "Memory leak detected: " << stats.allocation_count << " allocations not freed";
            
            // Check that used memory is minimal
            EXPECT_LT(stats.used_bytes, 1024) 
                << "Excessive memory usage after cleanup: " << stats.used_bytes << " bytes";
        }
    }
    
    // === Test Utilities ===
    
    /**
     * @brief Allocate test memory and track it
     * @param size Size to allocate
     * @param alignment Alignment requirement
     * @return Allocated pointer
     */
    void* allocate_test_memory(size_t size, MemoryAlignment alignment = MemoryAlignment::None) {
        auto result = get_interface().allocate(size, alignment);
        if (result.is_ok()) {
            void* ptr = result.value().ptr;
            test_allocations_.insert(ptr);
            return ptr;
        }
        return nullptr;
    }
    
    /**
     * @brief Free test memory and untrack it
     * @param ptr Pointer to free
     */
    void free_test_memory(void* ptr) {
        auto result = get_interface().deallocate(ptr);
        if (result.is_ok()) {
            test_allocations_.erase(ptr);
        }
    }
    
    /**
     * @brief Generate random allocation pattern
     * @param count Number of allocations
     * @param min_size Minimum allocation size
     * @param max_size Maximum allocation size
     * @return Vector of allocation sizes
     */
    std::vector<size_t> generate_allocation_pattern(size_t count, size_t min_size, size_t max_size) {
        std::vector<size_t> sizes;
        sizes.reserve(count);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dis(min_size, max_size);
        
        for (size_t i = 0; i < count; ++i) {
            sizes.push_back(dis(gen));
        }
        
        return sizes;
    }

    // === Performance Operation Implementations ===
    
    void perform_BasicAllocation_operation() {
        void* ptr = allocate_test_memory(1024);
        if (ptr) {
            std::memset(ptr, 0x42, 1024);
            free_test_memory(ptr);
        }
    }
    
    void perform_AlignedAllocation_operation() {
        void* ptr = allocate_test_memory(1024, MemoryAlignment::Cache);
        if (ptr) {
            std::memset(ptr, 0x42, 1024);
            free_test_memory(ptr);
        }
    }
    
    void perform_ConcurrentAllocation_operation() {
        // Simple concurrent allocation pattern
        std::vector<void*> ptrs;
        for (int i = 0; i < 10; ++i) {
            void* ptr = allocate_test_memory(256);
            if (ptr) {
                ptrs.push_back(ptr);
            }
        }
        
        // Free all allocations
        for (void* ptr : ptrs) {
            free_test_memory(ptr);
        }
    }
    
    void perform_FragmentationPattern_operation() {
        // Allocation pattern designed to create fragmentation
        std::vector<void*> ptrs;
        
        // Allocate various sizes
        auto sizes = generate_allocation_pattern(20, 64, 2048);
        for (size_t size : sizes) {
            void* ptr = allocate_test_memory(size);
            if (ptr) {
                ptrs.push_back(ptr);
            }
        }
        
        // Free every other allocation
        for (size_t i = 1; i < ptrs.size(); i += 2) {
            free_test_memory(ptrs[i]);
            ptrs[i] = nullptr;
        }
        
        // Allocate again in the gaps
        for (size_t i = 1; i < ptrs.size(); i += 2) {
            if (!ptrs[i]) {
                void* ptr = allocate_test_memory(128);
                if (ptr) {
                    ptrs[i] = ptr;
                }
            }
        }
        
        // Clean up remaining allocations
        for (void* ptr : ptrs) {
            if (ptr) {
                free_test_memory(ptr);
            }
        }
    }
    
    // === Error Handling Test Implementations ===
    
    void test_InvalidSize_error_handling() {
        // Test allocation with invalid sizes
        auto result1 = get_interface().allocate(0);
        EXPECT_TRUE(result1.is_err()) << "Zero-size allocation should fail";
        
        auto result2 = get_interface().allocate(SIZE_MAX);
        EXPECT_TRUE(result2.is_err()) << "Maximum size allocation should fail";
        
        // Test deallocation with invalid pointer
        auto result3 = get_interface().deallocate(nullptr);
        EXPECT_TRUE(result3.is_err()) << "Null pointer deallocation should fail";
        
        auto result4 = get_interface().deallocate(reinterpret_cast<void*>(0xDEADBEEF));
        EXPECT_TRUE(result4.is_err()) << "Invalid pointer deallocation should fail";
    }
    
    void test_ResourceExhaustion_error_handling() {
        // Attempt to exhaust memory resources
        std::vector<void*> allocations;
        
        // Allocate until failure
        const size_t large_size = get_config().limits.max_allocation_size / 2;
        for (int i = 0; i < 100; ++i) {  // Reasonable limit to prevent infinite loop
            auto result = get_interface().allocate(large_size);
            if (result.is_err()) {
                // Expected failure due to resource exhaustion
                EXPECT_EQ(result.error(), HALError::ResourceExhausted);
                break;
            } else {
                allocations.push_back(result.value().ptr);
            }
        }
        
        // Clean up allocations
        for (void* ptr : allocations) {
            get_interface().deallocate(ptr);
        }
        
        // Verify system recovers after cleanup
        auto recovery_result = get_interface().allocate(1024);
        EXPECT_TRUE(recovery_result.is_ok()) << "System should recover after cleanup";
        
        if (recovery_result.is_ok()) {
            get_interface().deallocate(recovery_result.value().ptr);
        }
    }

private:
    std::unordered_set<void*> test_allocations_;
};

// === Basic Compliance Tests ===

HAL_COMPLIANCE_TEST(Memory, BasicAllocation) {
    // Test basic allocation and deallocation
    const size_t test_size = 1024;
    
    auto result = get_interface().allocate(test_size);
    validate_hal_result(result, "basic allocation");
    
    ASSERT_TRUE(result.is_ok());
    auto allocation = result.value();
    
    // Validate allocation properties
    EXPECT_NE(allocation.ptr, nullptr);
    EXPECT_GE(allocation.size, test_size);
    EXPECT_EQ(allocation.type, MemoryType::System);  // Default type
    
    // Test memory is writable
    std::memset(allocation.ptr, 0xAA, test_size);
    EXPECT_EQ(static_cast<uint8_t*>(allocation.ptr)[0], 0xAA);
    EXPECT_EQ(static_cast<uint8_t*>(allocation.ptr)[test_size - 1], 0xAA);
    
    // Free the memory
    auto free_result = get_interface().deallocate(allocation.ptr);
    validate_hal_result(free_result, "basic deallocation");
}

HAL_COMPLIANCE_TEST(Memory, AlignmentCompliance) {
    // Test various alignment requirements
    const std::vector<MemoryAlignment> alignments = {
        MemoryAlignment::None,
        MemoryAlignment::Word,
        MemoryAlignment::Cache,
        MemoryAlignment::SIMD,
        MemoryAlignment::GPU
    };
    
    for (auto alignment : alignments) {
        // Skip unsupported alignments
        if (!get_interface().supports_alignment(alignment)) {
            continue;
        }
        
        auto result = get_interface().allocate(256, alignment);
        validate_hal_result(result, "aligned allocation");
        
        if (result.is_ok()) {
            auto allocation = result.value();
            
            // Verify alignment
            auto alignment_value = static_cast<size_t>(alignment);
            auto ptr_value = reinterpret_cast<uintptr_t>(allocation.ptr);
            EXPECT_EQ(ptr_value % alignment_value, 0) 
                << "Allocation not properly aligned to " << alignment_value << " bytes";
            
            EXPECT_EQ(allocation.alignment, alignment);
            
            // Cleanup
            get_interface().deallocate(allocation.ptr);
        }
    }
}

HAL_COMPLIANCE_TEST(Memory, MemoryTypes) {
    // Test different memory types
    const std::vector<MemoryType> types = {
        MemoryType::System,
        MemoryType::Video,
        MemoryType::Audio,
        MemoryType::Network,
        MemoryType::Temporary
    };
    
    for (auto type : types) {
        // Skip unsupported types
        if (!get_interface().supports_memory_type(type)) {
            continue;
        }
        
        auto result = get_interface().allocate_typed(1024, type);
        validate_hal_result(result, "typed allocation");
        
        if (result.is_ok()) {
            auto allocation = result.value();
            EXPECT_EQ(allocation.type, type);
            EXPECT_NE(allocation.ptr, nullptr);
            
            // Test memory is accessible
            std::memset(allocation.ptr, 0x55, 100);
            EXPECT_EQ(static_cast<uint8_t*>(allocation.ptr)[0], 0x55);
            
            // Cleanup
            get_interface().deallocate(allocation.ptr);
        }
    }
}

HAL_COMPLIANCE_TEST(Memory, MemoryFlags) {
    // Test memory allocation flags
    const std::vector<MemoryFlags> flag_combinations = {
        MemoryFlags::None,
        MemoryFlags::Cacheable,
        MemoryFlags::Zero,
        MemoryFlags::Cacheable | MemoryFlags::Zero,
        MemoryFlags::DMACapable,
        MemoryFlags::Persistent
    };
    
    for (auto flags : flag_combinations) {
        // Skip unsupported flags
        if (!get_interface().supports_flags(flags)) {
            continue;
        }
        
        auto result = get_interface().allocate(512, MemoryAlignment::None, flags);
        validate_hal_result(result, "flags allocation");
        
        if (result.is_ok()) {
            auto allocation = result.value();
            
            // Check flags are applied
            EXPECT_TRUE((allocation.flags & flags) == flags);
            
            // If Zero flag is set, memory should be zeroed
            if ((flags & MemoryFlags::Zero) == MemoryFlags::Zero) {
                auto* bytes = static_cast<uint8_t*>(allocation.ptr);
                bool all_zero = true;
                for (size_t i = 0; i < 512; ++i) {
                    if (bytes[i] != 0) {
                        all_zero = false;
                        break;
                    }
                }
                EXPECT_TRUE(all_zero) << "Memory not zeroed despite Zero flag";
            }
            
            // Cleanup
            get_interface().deallocate(allocation.ptr);
        }
    }
}

HAL_COMPLIANCE_TEST(Memory, AllocationInfo) {
    // Test allocation information retrieval
    const size_t test_size = 2048;
    
    auto result = get_interface().allocate(test_size);
    ASSERT_TRUE(result.is_ok());
    auto allocation = result.value();
    
    // Test pointer validation
    EXPECT_TRUE(get_interface().is_valid_pointer(allocation.ptr));
    EXPECT_FALSE(get_interface().is_valid_pointer(nullptr));
    EXPECT_FALSE(get_interface().is_valid_pointer(reinterpret_cast<void*>(0xDEADBEEF)));
    
    // Test allocation info retrieval
    auto info_result = get_interface().get_allocation_info(allocation.ptr);
    validate_hal_result(info_result, "allocation info");
    
    if (info_result.is_ok()) {
        auto info = info_result.value();
        EXPECT_EQ(info.ptr, allocation.ptr);
        EXPECT_GE(info.size, test_size);
        EXPECT_EQ(info.type, allocation.type);
        EXPECT_EQ(info.flags, allocation.flags);
    }
    
    // Cleanup
    get_interface().deallocate(allocation.ptr);
}

// === Performance Compliance Tests ===

HAL_PERFORMANCE_TEST(Memory, BasicAllocation, 1000.0) {
    perform_BasicAllocation_operation();
}

HAL_PERFORMANCE_TEST(Memory, AlignedAllocation, 500.0) {
    perform_AlignedAllocation_operation();
}

// === Error Handling Tests ===

HAL_ERROR_TEST(Memory, InvalidSize, InvalidParameter) {
    test_InvalidSize_error_handling();
}

HAL_ERROR_TEST(Memory, ResourceExhaustion, ResourceExhaustion) {
    test_ResourceExhaustion_error_handling();
}

// === Stress Tests ===

HAL_STRESS_TEST(Memory, ConcurrentAllocation) {
    perform_ConcurrentAllocation_operation();
}

HAL_STRESS_TEST(Memory, FragmentationPattern) {
    perform_FragmentationPattern_operation();
}

// === Advanced Compliance Tests ===

HAL_COMPLIANCE_TEST(Memory, ReallocCompliance) {
    // Test reallocation functionality
    const size_t initial_size = 1024;
    const size_t new_size = 2048;
    
    auto result = get_interface().allocate(initial_size);
    ASSERT_TRUE(result.is_ok());
    auto allocation = result.value();
    
    // Fill with test pattern
    auto* bytes = static_cast<uint8_t*>(allocation.ptr);
    for (size_t i = 0; i < initial_size; ++i) {
        bytes[i] = static_cast<uint8_t>(i & 0xFF);
    }
    
    // Reallocate to larger size
    auto realloc_result = get_interface().reallocate(allocation.ptr, new_size);
    validate_hal_result(realloc_result, "reallocation");
    
    if (realloc_result.is_ok()) {
        auto new_allocation = realloc_result.value();
        EXPECT_GE(new_allocation.size, new_size);
        
        // Verify data preservation
        auto* new_bytes = static_cast<uint8_t*>(new_allocation.ptr);
        for (size_t i = 0; i < initial_size; ++i) {
            EXPECT_EQ(new_bytes[i], static_cast<uint8_t>(i & 0xFF)) 
                << "Data corruption during reallocation at index " << i;
        }
        
        // Cleanup
        get_interface().deallocate(new_allocation.ptr);
    } else {
        // If realloc failed, original pointer should still be valid
        EXPECT_TRUE(get_interface().is_valid_pointer(allocation.ptr));
        get_interface().deallocate(allocation.ptr);
    }
}

HAL_COMPLIANCE_TEST(Memory, MemoryStatistics) {
    // Test memory statistics accuracy
    auto initial_stats_result = get_interface().get_memory_stats();
    ASSERT_TRUE(initial_stats_result.is_ok());
    auto initial_stats = initial_stats_result.value();
    
    // Allocate some memory
    const size_t alloc_size = 4096;
    auto result = get_interface().allocate(alloc_size);
    ASSERT_TRUE(result.is_ok());
    auto allocation = result.value();
    
    // Check updated statistics
    auto updated_stats_result = get_interface().get_memory_stats();
    ASSERT_TRUE(updated_stats_result.is_ok());
    auto updated_stats = updated_stats_result.value();
    
    // Used memory should have increased
    EXPECT_GT(updated_stats.used_bytes, initial_stats.used_bytes);
    EXPECT_GT(updated_stats.allocation_count, initial_stats.allocation_count);
    EXPECT_LT(updated_stats.free_bytes, initial_stats.free_bytes);
    
    // Peak usage should be updated
    EXPECT_GE(updated_stats.peak_used_bytes, updated_stats.used_bytes);
    
    // Cleanup
    get_interface().deallocate(allocation.ptr);
    
    // Verify statistics return to baseline
    auto final_stats_result = get_interface().get_memory_stats();
    if (final_stats_result.is_ok()) {
        auto final_stats = final_stats_result.value();
        EXPECT_LE(final_stats.used_bytes, initial_stats.used_bytes + 100);  // Allow small overhead
        EXPECT_EQ(final_stats.allocation_count, initial_stats.allocation_count);
    }
}

HAL_COMPLIANCE_TEST(Memory, PressureHandling) {
    // Test memory pressure monitoring
    auto* monitor = get_interface().get_pressure_monitor();
    if (!monitor) {
        GTEST_SKIP() << "Memory pressure monitoring not supported";
    }
    
    // Register pressure callback
    bool callback_invoked = false;
    auto callback = [&callback_invoked](MemoryPressureLevel level, const MemoryPressureInfo& info) {
        callback_invoked = true;
        EXPECT_NE(level, MemoryPressureLevel::None);
        EXPECT_GT(info.pressure_ratio, 0.0);
    };
    
    auto register_result = monitor->register_callback(MemoryPressureLevel::Medium, callback);
    if (register_result.is_ok()) {
        auto callback_id = register_result.value();
        
        // Force pressure check
        monitor->check_pressure();
        
        // Unregister callback
        monitor->unregister_callback(callback_id);
    }
}

// === Performance Operation Implementations ===

private:
    void perform_BasicAllocation_operation() {
        void* ptr = allocate_test_memory(1024);
        if (ptr) {
            std::memset(ptr, 0x42, 1024);
            free_test_memory(ptr);
        }
    }
    
    void perform_AlignedAllocation_operation() {
        void* ptr = allocate_test_memory(1024, MemoryAlignment::Cache);
        if (ptr) {
            std::memset(ptr, 0x42, 1024);
            free_test_memory(ptr);
        }
    }
    
    void perform_ConcurrentAllocation_operation() {
        // Simple concurrent allocation pattern
        std::vector<void*> ptrs;
        for (int i = 0; i < 10; ++i) {
            void* ptr = allocate_test_memory(256);
            if (ptr) {
                ptrs.push_back(ptr);
            }
        }
        
        // Free all allocations
        for (void* ptr : ptrs) {
            free_test_memory(ptr);
        }
    }
    
    void perform_FragmentationPattern_operation() {
        // Allocation pattern designed to create fragmentation
        std::vector<void*> ptrs;
        
        // Allocate various sizes
        auto sizes = generate_allocation_pattern(20, 64, 2048);
        for (size_t size : sizes) {
            void* ptr = allocate_test_memory(size);
            if (ptr) {
                ptrs.push_back(ptr);
            }
        }
        
        // Free every other allocation
        for (size_t i = 1; i < ptrs.size(); i += 2) {
            free_test_memory(ptrs[i]);
            ptrs[i] = nullptr;
        }
        
        // Allocate again in the gaps
        for (size_t i = 1; i < ptrs.size(); i += 2) {
            if (!ptrs[i]) {
                void* ptr = allocate_test_memory(128);
                if (ptr) {
                    ptrs[i] = ptr;
                }
            }
        }
        
        // Clean up remaining allocations
        for (void* ptr : ptrs) {
            if (ptr) {
                free_test_memory(ptr);
            }
        }
    }
    
    // === Error Handling Test Implementations ===
    
    void test_InvalidSize_error_handling() {
        // Test allocation with invalid sizes
        auto result1 = get_interface().allocate(0);
        EXPECT_TRUE(result1.is_err()) << "Zero-size allocation should fail";
        
        auto result2 = get_interface().allocate(SIZE_MAX);
        EXPECT_TRUE(result2.is_err()) << "Maximum size allocation should fail";
        
        // Test deallocation with invalid pointer
        auto result3 = get_interface().deallocate(nullptr);
        EXPECT_TRUE(result3.is_err()) << "Null pointer deallocation should fail";
        
        auto result4 = get_interface().deallocate(reinterpret_cast<void*>(0xDEADBEEF));
        EXPECT_TRUE(result4.is_err()) << "Invalid pointer deallocation should fail";
    }
    
    void test_ResourceExhaustion_error_handling() {
        // Attempt to exhaust memory resources
        std::vector<void*> allocations;
        
        // Allocate until failure
        const size_t large_size = get_config().limits.max_allocation_size / 2;
        for (int i = 0; i < 100; ++i) {  // Reasonable limit to prevent infinite loop
            auto result = get_interface().allocate(large_size);
            if (result.is_err()) {
                // Expected failure due to resource exhaustion
                EXPECT_EQ(result.error(), HALError::ResourceExhausted);
                break;
            } else {
                allocations.push_back(result.value().ptr);
            }
        }
        
        // Clean up allocations
        for (void* ptr : allocations) {
            get_interface().deallocate(ptr);
        }
        
        // Verify system recovers after cleanup
        auto recovery_result = get_interface().allocate(1024);
        EXPECT_TRUE(recovery_result.is_ok()) << "System should recover after cleanup";
        
        if (recovery_result.is_ok()) {
            get_interface().deallocate(recovery_result.value().ptr);
        }
    }
};

} // namespace flight::hal::testing
