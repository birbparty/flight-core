/**
 * @file gradual_exhaustion_test.cpp
 * @brief Memory Gradual Exhaustion Stress Test
 * 
 * Tests HAL memory interface behavior under gradually increasing memory pressure,
 * validating graceful degradation and proper error handling as memory becomes scarce.
 */

#include "../framework/stress_test_base.hpp"
#include "../../../include/flight/hal/interfaces/memory.hpp"
#include "../../mock/mock_drivers/test_memory_driver.hpp"
#include <vector>
#include <random>
#include <algorithm>

namespace flight::hal::test::stress {

/**
 * @brief Gradual memory exhaustion stress test
 * 
 * Gradually increases memory allocation pressure by:
 * 1. Starting with small allocations
 * 2. Progressively increasing allocation sizes and frequency
 * 3. Monitoring memory usage and system behavior
 * 4. Testing error handling when memory becomes scarce
 * 5. Validating recovery after releasing memory
 */
class GradualExhaustionTest : public StressTestBase {
public:
    explicit GradualExhaustionTest() : StressTestBase("MemoryGradualExhaustion") {
        // Configure for memory-focused testing
        auto config = config_presets::standard_stress("MemoryGradualExhaustion");
        config.max_memory_mb = 32; // Limit to 32MB for testing
        config.fail_on_resource_exhaustion = false; // Allow graceful degradation
        configure(config);
    }

protected:
    bool setup_test() override {
        // Initialize mock memory driver with gradual exhaustion behavior
        mock_driver_ = std::make_unique<TestMemoryDriver>();
        
        // Configure progressive memory reduction
        mock_driver_->get_behavior_config().set_method_behavior("allocate", 
            create_progressive_exhaustion_behavior());
        
        auto init_result = mock_driver_->initialize();
        if (!init_result) {
            record_error("Failed to initialize mock memory driver: " + 
                        std::string(init_result.error().message()));
            return false;
        }
        
        // Clear allocation tracking
        allocations_.clear();
        total_allocated_ = 0;
        allocation_failures_ = 0;
        
        return true;
    }
    
    bool teardown_test() override {
        // Release all allocations
        for (auto& alloc : allocations_) {
            if (alloc.ptr) {
                mock_driver_->deallocate(alloc.ptr);
            }
        }
        allocations_.clear();
        
        if (mock_driver_) {
            mock_driver_->shutdown();
        }
        
        return true;
    }
    
    bool execute_stress_iteration(double intensity) override {
        // Calculate allocation parameters based on intensity
        size_t base_allocation_size = static_cast<size_t>(1024 * intensity); // 0-1KB base
        size_t max_allocation_size = static_cast<size_t>(1024 * 1024 * intensity); // 0-1MB max
        size_t allocation_count = static_cast<size_t>(10 * intensity); // 0-10 allocations per iteration
        
        bool iteration_success = true;
        
        // Perform allocations
        for (size_t i = 0; i < allocation_count; ++i) {
            size_t allocation_size = generate_allocation_size(base_allocation_size, max_allocation_size);
            
            auto start_time = std::chrono::high_resolution_clock::now();
            auto result = mock_driver_->allocate(allocation_size, MemoryAlignment::SIMD, MemoryFlags::None);
            auto end_time = std::chrono::high_resolution_clock::now();
            
            // Record performance measurement
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
            get_metrics_collector()->record_performance_measurement(duration);
            
            if (result) {
                // Successful allocation
                AllocationInfo alloc_info;
                alloc_info.ptr = result.value().ptr;
                alloc_info.size = allocation_size;
                alloc_info.timestamp = std::chrono::steady_clock::now();
                allocations_.push_back(alloc_info);
                
                total_allocated_ += allocation_size;
                get_metrics_collector()->record_allocation(allocation_size);
                
                // Write pattern to memory to ensure it's usable
                if (!write_test_pattern(alloc_info.ptr, allocation_size)) {
                    record_warning("Memory write test failed for allocation");
                    iteration_success = false;
                }
                
                record_operation(true);
            } else {
                // Allocation failed - this is expected under memory pressure
                allocation_failures_++;
                record_operation(false);
                
                // Log the failure reason for analysis
                record_warning("Memory allocation failed: " + std::string(result.error().message()));
            }
        }
        
        // Occasionally deallocate some memory to test recovery
        if (intensity > 0.5 && should_deallocate()) {
            perform_random_deallocation();
        }
        
        // Monitor memory fragmentation
        check_memory_fragmentation();
        
        return iteration_success;
    }
    
    bool validate_recovery() override {
        // Test that memory can be allocated again after stress
        const size_t test_allocation_size = 1024;
        auto result = mock_driver_->allocate(test_allocation_size, MemoryAlignment::SIMD, MemoryFlags::None);
        
        if (result) {
            // Successful recovery allocation
            mock_driver_->deallocate(result.value().ptr);
            return true;
        } else {
            record_error("Memory recovery validation failed: " + std::string(result.error().message()));
            return false;
        }
    }

private:
    struct AllocationInfo {
        void* ptr = nullptr;
        size_t size = 0;
        std::chrono::steady_clock::time_point timestamp;
    };
    
    /**
     * @brief Create behavior that progressively reduces available memory
     */
    MethodBehaviorConfig create_progressive_exhaustion_behavior() {
        MethodBehaviorConfig config;
        config.resource_mode = ResourceMode::Exhaustible;
        config.max_resources = 32 * 1024 * 1024; // 32MB total
        config.resource_per_call = 1; // Will be set dynamically
        
        // Custom failure function that increases failure rate over time
        config.failure_mode = FailureMode::Custom;
        config.custom_failure_fn = [this](uint32_t call_count) -> bool {
            // Calculate failure probability based on memory pressure
            double memory_pressure = static_cast<double>(total_allocated_) / (32 * 1024 * 1024);
            double failure_probability = std::max(0.0, (memory_pressure - 0.7) * 10.0); // Start failing at 70% usage
            
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            return dist(rng_) < failure_probability;
        };
        
        // Realistic timing that increases with memory pressure
        config.performance_mode = PerformanceMode::Custom;
        config.custom_timing_fn = [this](uint32_t call_count) -> std::chrono::microseconds {
            double memory_pressure = static_cast<double>(total_allocated_) / (32 * 1024 * 1024);
            auto base_time = std::chrono::microseconds{10};
            auto pressure_multiplier = 1.0 + (memory_pressure * 5.0); // Up to 6x slower under pressure
            return std::chrono::microseconds{static_cast<long>(base_time.count() * pressure_multiplier)};
        };
        
        return config;
    }
    
    /**
     * @brief Generate allocation size with some randomness
     */
    size_t generate_allocation_size(size_t base_size, size_t max_size) {
        if (base_size >= max_size) {
            return base_size;
        }
        
        std::uniform_int_distribution<size_t> dist(base_size, max_size);
        return dist(rng_);
    }
    
    /**
     * @brief Write test pattern to allocated memory
     */
    bool write_test_pattern(void* ptr, size_t size) {
        if (!ptr || size == 0) {
            return false;
        }
        
        try {
            uint8_t* bytes = static_cast<uint8_t*>(ptr);
            uint8_t pattern = static_cast<uint8_t>(size & 0xFF);
            
            // Write pattern to first and last few bytes
            size_t test_bytes = std::min(size, static_cast<size_t>(64));
            for (size_t i = 0; i < test_bytes; ++i) {
                bytes[i] = pattern;
                if (size > test_bytes) {
                    bytes[size - 1 - i] = pattern;
                }
            }
            
            // Verify the pattern
            for (size_t i = 0; i < test_bytes; ++i) {
                if (bytes[i] != pattern) {
                    return false;
                }
                if (size > test_bytes && bytes[size - 1 - i] != pattern) {
                    return false;
                }
            }
            
            return true;
        } catch (...) {
            return false;
        }
    }
    
    /**
     * @brief Decide whether to deallocate memory this iteration
     */
    bool should_deallocate() {
        if (allocations_.empty()) {
            return false;
        }
        
        // Deallocate with 10% probability, higher if memory pressure is high
        double memory_pressure = static_cast<double>(total_allocated_) / (32 * 1024 * 1024);
        double dealloc_probability = 0.1 + (memory_pressure * 0.2); // 10-30% chance
        
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        return dist(rng_) < dealloc_probability;
    }
    
    /**
     * @brief Randomly deallocate some memory
     */
    void perform_random_deallocation() {
        if (allocations_.empty()) {
            return;
        }
        
        // Deallocate 10-30% of current allocations
        std::uniform_real_distribution<double> percent_dist(0.1, 0.3);
        size_t dealloc_count = static_cast<size_t>(allocations_.size() * percent_dist(rng_));
        dealloc_count = std::max(static_cast<size_t>(1), dealloc_count);
        
        std::vector<size_t> indices_to_remove;
        std::uniform_int_distribution<size_t> index_dist(0, allocations_.size() - 1);
        
        for (size_t i = 0; i < dealloc_count; ++i) {
            size_t index = index_dist(rng_);
            if (std::find(indices_to_remove.begin(), indices_to_remove.end(), index) == indices_to_remove.end()) {
                indices_to_remove.push_back(index);
            }
        }
        
        // Sort indices in descending order to safely remove from vector
        std::sort(indices_to_remove.rbegin(), indices_to_remove.rend());
        
        for (size_t index : indices_to_remove) {
            if (index < allocations_.size()) {
                auto& alloc = allocations_[index];
                if (alloc.ptr) {
                    mock_driver_->deallocate(alloc.ptr);
                    total_allocated_ -= alloc.size;
                    get_metrics_collector()->record_deallocation(alloc.size);
                }
                allocations_.erase(allocations_.begin() + index);
            }
        }
    }
    
    /**
     * @brief Check for memory fragmentation patterns
     */
    void check_memory_fragmentation() {
        // Simple fragmentation check: try to allocate a large block
        const size_t large_block_size = 1024 * 1024; // 1MB
        auto result = mock_driver_->allocate(large_block_size, MemoryAlignment::SIMD, MemoryFlags::None);
        
        if (!result && total_allocated_ < (16 * 1024 * 1024)) { // Less than 50% memory used
            // Could indicate fragmentation
            record_warning("Potential memory fragmentation detected");
        } else if (result) {
            // Release the test allocation immediately
            mock_driver_->deallocate(result.value().ptr);
        }
    }
    
    std::unique_ptr<TestMemoryDriver> mock_driver_;
    std::vector<AllocationInfo> allocations_;
    std::atomic<size_t> total_allocated_{0};
    std::atomic<size_t> allocation_failures_{0};
    mutable std::mt19937 rng_{std::random_device{}()};
};

} // namespace flight::hal::test::stress

// Test execution function
int main() {
    using namespace flight::hal::test::stress;
    
    // Test with different configurations
    std::vector<StressTestConfig> test_configs = {
        config_presets::light_stress("MemoryGradualExhaustion_Light"),
        config_presets::standard_stress("MemoryGradualExhaustion_Standard"),
        config_presets::embedded_stress("MemoryGradualExhaustion_Embedded")
    };
    
    int failed_tests = 0;
    
    for (const auto& config : test_configs) {
        GradualExhaustionTest test;
        test.configure(config);
        
        auto result = test.execute();
        
        // Print results
        printf("Test: %s\n", result.test_name.c_str());
        printf("Success: %s\n", result.success ? "true" : "false");
        printf("Total Operations: %zu\n", result.total_operations);
        printf("Failed Operations: %zu\n", result.failed_operations);
        printf("Peak Memory Usage: %zu KB\n", result.peak_memory_usage / 1024);
        printf("Recovery: %s\n", result.successful_recovery ? "successful" : "failed");
        
        if (!result.errors.empty()) {
            printf("Errors:\n");
            for (const auto& error : result.errors) {
                printf("  - %s\n", error.c_str());
            }
        }
        
        if (!result.warnings.empty()) {
            printf("Warnings:\n");
            for (const auto& warning : result.warnings) {
                printf("  - %s\n", warning.c_str());
            }
        }
        
        printf("\n");
        
        if (!result.success) {
            failed_tests++;
        }
    }
    
    return failed_tests;
}
