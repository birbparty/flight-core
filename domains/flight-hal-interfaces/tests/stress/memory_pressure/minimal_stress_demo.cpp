/**
 * @file minimal_stress_demo.cpp
 * @brief Minimal Stress Test Demo
 * 
 * A minimal demonstration of memory stress testing without complex dependencies.
 * This validates that the basic stress testing concept works.
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <random>
#include <algorithm>
#include <cstdlib>
#include <thread>

namespace flight::hal::test::stress {

/**
 * @brief Simple stress test result
 */
struct SimpleStressResult {
    bool success = false;
    size_t total_operations = 0;
    size_t failed_operations = 0;
    size_t peak_memory_kb = 0;
    std::chrono::milliseconds duration{0};
    std::vector<std::string> warnings;
};

/**
 * @brief Minimal memory stress test
 */
class MinimalMemoryStressTest {
public:
    MinimalMemoryStressTest() : rng_(std::random_device{}()) {}
    
    SimpleStressResult execute() {
        SimpleStressResult result;
        auto start_time = std::chrono::steady_clock::now();
        
        std::cout << "Starting minimal memory stress test...\n";
        std::cout << "Duration: 5 seconds\n";
        std::cout << "Pattern: Gradual allocation increase with random deallocations\n\n";
        
        std::vector<void*> allocations;
        size_t total_allocated = 0;
        size_t peak_allocated = 0;
        
        // Run for 5 seconds
        auto end_time = start_time + std::chrono::seconds{5};
        
        while (std::chrono::steady_clock::now() < end_time) {
            // Calculate intensity based on elapsed time (0.0 to 1.0)
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            double intensity = std::min(1.0, std::chrono::duration<double>(elapsed).count() / 5.0);
            
            // Determine allocation size based on intensity
            size_t base_size = static_cast<size_t>(1024 * intensity);
            size_t max_size = static_cast<size_t>(64 * 1024 * intensity);
            if (base_size == 0) base_size = 512;
            if (max_size < base_size) max_size = base_size * 2;
            
            std::uniform_int_distribution<size_t> size_dist(base_size, max_size);
            size_t alloc_size = size_dist(rng_);
            
            // Try to allocate
            void* ptr = malloc(alloc_size);
            result.total_operations++;
            
            if (ptr) {
                // Successful allocation
                allocations.push_back(ptr);
                total_allocated += alloc_size;
                peak_allocated = std::max(peak_allocated, total_allocated);
                
                // Write pattern to verify memory is usable
                if (alloc_size >= 64) {
                    uint8_t* bytes = static_cast<uint8_t*>(ptr);
                    uint8_t pattern = static_cast<uint8_t>(alloc_size & 0xFF);
                    for (size_t i = 0; i < 64; ++i) {
                        bytes[i] = pattern;
                    }
                    // Verify pattern
                    for (size_t i = 0; i < 64; ++i) {
                        if (bytes[i] != pattern) {
                            result.warnings.push_back("Memory corruption detected!");
                            break;
                        }
                    }
                }
                
                std::cout << "\rProgress: " << std::fixed << std::setprecision(1) 
                         << (intensity * 100.0) << "% | Allocated: " 
                         << (total_allocated / 1024) << " KB | Operations: " 
                         << result.total_operations << std::flush;
            } else {
                // Allocation failed
                result.failed_operations++;
                result.warnings.push_back("Allocation failed for " + std::to_string(alloc_size) + " bytes");
            }
            
            // Occasionally deallocate some memory (10% chance at low intensity, 30% at high)
            if (!allocations.empty()) {
                double dealloc_probability = 0.1 + (intensity * 0.2);
                std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
                
                if (prob_dist(rng_) < dealloc_probability) {
                    // Deallocate a few allocations
                    size_t dealloc_count = std::min(allocations.size(), 
                                                   static_cast<size_t>(allocations.size() * 0.1 + 1));
                    
                    for (size_t i = 0; i < dealloc_count; ++i) {
                        std::uniform_int_distribution<size_t> idx_dist(0, allocations.size() - 1);
                        size_t idx = idx_dist(rng_);
                        
                        if (allocations[idx]) {
                            free(allocations[idx]);
                            total_allocated -= 1024; // Rough estimate
                            allocations.erase(allocations.begin() + idx);
                        }
                    }
                }
            }
            
            // Small delay to prevent overwhelming the system
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
        
        std::cout << "\n\nTest completed. Cleaning up...\n";
        
        // Cleanup remaining allocations
        for (void* ptr : allocations) {
            if (ptr) {
                free(ptr);
            }
        }
        
        // Test recovery by trying a simple allocation
        void* recovery_ptr = malloc(1024);
        bool recovery_success = (recovery_ptr != nullptr);
        if (recovery_ptr) {
            free(recovery_ptr);
        }
        
        auto actual_end_time = std::chrono::steady_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(actual_end_time - start_time);
        result.peak_memory_kb = peak_allocated / 1024;
        result.success = (result.total_operations > 0) && recovery_success;
        
        return result;
    }

private:
    std::mt19937 rng_;
};

} // namespace flight::hal::test::stress

int main() {
    using namespace flight::hal::test::stress;
    
    std::cout << "Flight HAL Minimal Memory Stress Test Demo\n";
    std::cout << "==========================================\n\n";
    
    MinimalMemoryStressTest test;
    auto result = test.execute();
    
    std::cout << "\nTest Results:\n";
    std::cout << "=============\n";
    std::cout << "Success: " << (result.success ? "✓ PASS" : "✗ FAIL") << "\n";
    std::cout << "Total Operations: " << result.total_operations << "\n";
    std::cout << "Failed Operations: " << result.failed_operations << "\n";
    std::cout << "Success Rate: " << std::fixed << std::setprecision(1) 
              << (result.total_operations > 0 ? 
                 (100.0 * (result.total_operations - result.failed_operations) / result.total_operations) : 
                 0.0) << "%\n";
    std::cout << "Peak Memory Usage: " << result.peak_memory_kb << " KB\n";
    std::cout << "Execution Time: " << result.duration.count() << " ms\n";
    
    if (!result.warnings.empty()) {
        std::cout << "\nWarnings:\n";
        for (const auto& warning : result.warnings) {
            std::cout << "  ⚠ " << warning << "\n";
        }
    }
    
    std::cout << "\nThis demonstrates the basic stress testing concept:\n";
    std::cout << "- Gradual intensity increase over time\n";
    std::cout << "- Memory allocation/deallocation patterns\n";
    std::cout << "- Error handling and recovery testing\n";
    std::cout << "- Performance metrics collection\n";
    std::cout << "- Memory corruption detection\n\n";
    
    std::cout << "The full stress testing framework builds on these concepts\n";
    std::cout << "with additional features like cross-platform metrics,\n";
    std::cout << "configurable test phases, and HAL driver integration.\n";
    
    return result.success ? 0 : 1;
}
