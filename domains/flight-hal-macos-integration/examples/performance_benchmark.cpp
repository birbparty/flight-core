#include <iostream>
#include <chrono>
#include <unistd.h>
#include "flight/hal/macos/platform_coordinator.hpp"

using namespace flight::hal::macos;

int main() {
    std::cout << "Flight HAL macOS Integration - Performance Benchmark\n";
    std::cout << "===================================================\n\n";
    
    auto coordinator = PlatformCoordinator::create();
    if (!coordinator) {
        std::cerr << "Failed to create platform coordinator\n";
        return 1;
    }
    
    // Benchmark Apple Silicon detection
    std::cout << "1. Apple Silicon Detection Benchmark:\n";
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; ++i) {
        bool is_apple_silicon = coordinator->isAppleSilicon();
        (void)is_apple_silicon; // Suppress unused variable warning
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "   1000 detections in " << duration.count() << " microseconds\n";
    std::cout << "   Average: " << (duration.count() / 1000.0) << " microseconds per detection\n\n";
    
    // Benchmark system info retrieval
    std::cout << "2. System Information Retrieval Benchmark:\n";
    start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 100; ++i) {
        auto system_info = coordinator->getSystemInfo();
        (void)system_info; // Suppress unused variable warning
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "   100 retrievals in " << duration.count() << " microseconds\n";
    std::cout << "   Average: " << (duration.count() / 100.0) << " microseconds per retrieval\n\n";
    
    // Display system capabilities
    auto system_info = coordinator->getSystemInfo();
    if (system_info) {
        std::cout << "3. System Performance Characteristics:\n";
        std::cout << "   CPU Model: " << system_info->cpu_model << "\n";
        std::cout << "   Performance Cores: " << system_info->performance_cores << "\n";
        std::cout << "   Efficiency Cores: " << system_info->efficiency_cores << "\n";
        std::cout << "   Total Cores: " << (system_info->performance_cores + system_info->efficiency_cores) << "\n";
        std::cout << "   GPU Cores: " << system_info->gpu_cores << "\n";
        std::cout << "   Memory Bandwidth: " << system_info->memory_bandwidth_gbps << " GB/s\n";
        std::cout << "   Unified Memory: " << (system_info->unified_memory_size / (1024*1024*1024)) << " GB\n";
    }
    
    std::cout << "\nPerformance Benchmark Complete!\n";
    return 0;
}
