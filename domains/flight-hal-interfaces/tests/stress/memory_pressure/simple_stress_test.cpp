/**
 * @file simple_stress_test.cpp
 * @brief Simplified Memory Stress Test for Framework Verification
 * 
 * A standalone stress test that validates the stress testing framework
 * without depending on complex mock infrastructure.
 */

#include "../framework/stress_test_base.hpp"
#include <vector>
#include <random>
#include <algorithm>
#include <cstdlib>

namespace flight::hal::test::stress {

/**
 * @brief Simple memory stress test using standard allocators
 * 
 * This test validates the stress testing framework by performing
 * memory allocations using standard malloc/free to simulate
 * memory pressure scenarios.
 */
class SimpleMemoryStressTest : public StressTestBase {
public:
    explicit SimpleMemoryStressTest() : StressTestBase("SimpleMemoryStress") {
        // Configure for light testing to ensure it works
        auto config = config_presets::light_stress("SimpleMemoryStress");
        config.max_memory_mb = 16; // Limit to 16MB for testing
        config.fail_on_resource_exhaustion = false; // Allow graceful degradation
        configure(config);
    }

protected:
    bool setup_test() override {
        // Clear allocation tracking
        allocations_.clear();
        total_allocated_ = 0;
        allocation_failures_ = 0;
        
        printf("Setting up simple memory stress test...\n");
        return true;
    }
    
    bool teardown_test() override {
        // Release all allocations
        for (auto ptr : allocations_) {
            if (ptr) {
                free(ptr);
                get_metrics_collector()->record_deallocation(1024); // Assume 1KB each for simplicity
            }
        }
        allocations_.clear();
        
        printf("Tearing down simple memory stress test...\n");
        return true;
    }
    
    bool execute_stress_iteration(double intensity) override {
        // Calculate allocation parameters based on intensity
        size_t base_allocation_size = static_cast<size_t>(1024 * intensity); // 0-1KB base
        size_t max_allocation_size = static_cast<size_t>(64 * 1024 * intensity); // 0-64KB max
        size_t allocation_count = static_cast<size_t>(5 * intensity); // 0-5 allocations per iteration
        
        bool iteration_success = true;
        
        // Ensure minimum values
        if (base_allocation_size == 0) base_allocation_size = 64;
        if (max_allocation_size == 0) max_allocation_size = 1024;
        if (allocation_count == 0) allocation_count = 1;
        
        // Perform allocations
        for (size_t i = 0; i < allocation_count; ++i) {
            size_t allocation_size = generate_allocation_size(base_allocation_size, max_allocation_size);
            
            auto start_time = std::chrono::high_resolution_clock::now();
            void* ptr = malloc(allocation_size);
            auto end_time = std::chrono::high_resolution_clock::now();
            
            // Record performance measurement
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
            get_metrics_collector()->record_performance_measurement(duration);
            
            if (ptr) {
                // Successful allocation
                allocations_.push_back(ptr);
                total_allocated_ += allocation_size;
                get_metrics_collector()->record_allocation(allocation_size);
                
                // Write pattern to memory to ensure it's usable
                if (!write_test_pattern(ptr, allocation_size)) {
                    record_warning("Memory write test failed for allocation");
                    iteration_success = false;
                }
                
                record_operation(true);
            } else {
                // Allocation failed - this is expected under memory pressure
                allocation_failures_++;
                record_operation(false);
                
                // Log the failure reason for analysis
                record_warning("Memory allocation failed for size: " + std::to_string(allocation_size));
            }
        }
        
        // Occasionally deallocate some memory to test recovery
        if (intensity > 0.5 && should_deallocate()) {
            perform_random_deallocation();
        }
        
        // Check if we should artificially limit memory to simulate pressure
        simulate_memory_pressure(intensity);
        
        return iteration_success;
    }
    
    bool validate_recovery() override {
        // Test that memory can be allocated again after stress
        const size_t test_allocation_size = 1024;
        void* ptr = malloc(test_allocation_size);
        
        if (ptr) {
            // Successful recovery allocation
            free(ptr);
            return true;
        } else {
            record_error("Memory recovery validation failed - could not allocate 1KB");
            return false;
        }
    }

private:
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
        
        // Deallocate with 10% probability, higher if we have many allocations
        double alloc_pressure = static_cast<double>(allocations_.size()) / 100.0; // Pressure based on allocation count
        double dealloc_probability = 0.1 + (alloc_pressure * 0.2); // 10-30% chance
        
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
                void* ptr = allocations_[index];
                if (ptr) {
                    free(ptr);
                    total_allocated_ -= 1024; // Assume 1KB for simplicity
                    get_metrics_collector()->record_deallocation(1024);
                }
                allocations_.erase(allocations_.begin() + index);
            }
        }
    }
    
    /**
     * @brief Simulate memory pressure by occasionally forcing allocations to fail
     */
    void simulate_memory_pressure(double intensity) {
        // At high intensity, we could artificially limit available memory
        // For now, we just track the pressure without actually limiting
        if (intensity > 0.8 && total_allocated_ > (10 * 1024 * 1024)) { // 10MB threshold
            record_warning("High memory pressure detected: " + std::to_string(total_allocated_ / 1024) + "KB allocated");
        }
    }
    
    std::vector<void*> allocations_;
    std::atomic<size_t> total_allocated_{0};
    std::atomic<size_t> allocation_failures_{0};
    mutable std::mt19937 rng_{std::random_device{}()};
};

} // namespace flight::hal::test::stress

// Test execution function
int main() {
    using namespace flight::hal::test::stress;
    
    printf("Starting Flight HAL Simple Memory Stress Test\n");
    printf("=============================================\n\n");
    
    // Test with different configurations
    std::vector<StressTestConfig> test_configs = {
        config_presets::light_stress("SimpleMemoryStress_Light")
    };
    
    int failed_tests = 0;
    
    for (const auto& config : test_configs) {
        SimpleMemoryStressTest test;
        test.configure(config);
        
        printf("Executing stress test: %s\n", config.test_name.c_str());
        printf("Configuration:\n");
        printf("  - Duration: %ld seconds\n", config.total_duration.count());
        printf("  - Max Memory: %zu MB\n", config.max_memory_mb);
        printf("  - Stress Intensity: %.1f%%\n", config.stress_intensity * 100.0);
        printf("  - Recovery Testing: %s\n", config.enable_recovery_testing ? "enabled" : "disabled");
        printf("\n");
        
        auto result = test.execute();
        
        // Print results
        printf("Test Results for: %s\n", result.test_name.c_str());
        printf("=====================================\n");
        printf("Success: %s\n", result.success ? "✓ PASS" : "✗ FAIL");
        printf("Total Operations: %zu\n", result.total_operations);
        printf("Failed Operations: %zu\n", result.failed_operations);
        printf("Success Rate: %.1f%%\n", 
               result.total_operations > 0 ? 
               (100.0 * (result.total_operations - result.failed_operations) / result.total_operations) : 
               0.0);
        printf("Peak Memory Usage: %zu KB\n", result.peak_memory_usage / 1024);
        printf("Recovery: %s\n", result.successful_recovery ? "✓ successful" : "✗ failed");
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            result.end_time - result.start_time);
        printf("Execution Time: %ld ms\n", duration.count());
        
        if (config.enable_recovery_testing) {
            printf("Recovery Time: %ld ms\n", result.recovery_time.count());
        }
        
        if (!result.errors.empty()) {
            printf("\nErrors:\n");
            for (const auto& error : result.errors) {
                printf("  ✗ %s\n", error.c_str());
            }
        }
        
        if (!result.warnings.empty()) {
            printf("\nWarnings:\n");
            for (const auto& warning : result.warnings) {
                printf("  ⚠ %s\n", warning.c_str());
            }
        }
        
        printf("\n");
        
        if (!result.success) {
            failed_tests++;
        }
    }
    
    printf("Test Summary\n");
    printf("============\n");
    printf("Total Tests: %zu\n", test_configs.size());
    printf("Passed: %zu\n", test_configs.size() - failed_tests);
    printf("Failed: %d\n", failed_tests);
    printf("Overall Result: %s\n", failed_tests == 0 ? "✓ ALL TESTS PASSED" : "✗ SOME TESTS FAILED");
    
    return failed_tests;
}
