#include <iostream>
#include <unistd.h>
#include "flight/hal/macos/platform_coordinator.hpp"
#include "flight/hal/macos/process_manager.hpp"
#include "flight/hal/macos/file_system_bridge.hpp"
#include "flight/hal/macos/development_tools.hpp"

using namespace flight::hal::macos;

int main() {
    std::cout << "Flight HAL macOS Integration - Basic Usage Example\n";
    std::cout << "==================================================\n\n";
    
    // Create HAL components
    auto coordinator = PlatformCoordinator::create();
    auto process_manager = ProcessManager::create();
    auto file_system = FileSystemBridge::create();
    auto dev_tools = DevelopmentTools::create();
    
    if (!coordinator || !process_manager || !file_system || !dev_tools) {
        std::cerr << "Failed to create HAL components\n";
        return 1;
    }
    
    // Platform Information
    std::cout << "1. Platform Information:\n";
    std::cout << "   Apple Silicon: " << (coordinator->isAppleSilicon() ? "Yes" : "No") << "\n";
    std::cout << "   M4 Max: " << (coordinator->isM4Max() ? "Yes" : "No") << "\n";
    
    auto system_info = coordinator->getSystemInfo();
    if (system_info) {
        std::cout << "   CPU Model: " << system_info->cpu_model << "\n";
        std::cout << "   Performance Cores: " << system_info->performance_cores << "\n";
        std::cout << "   Efficiency Cores: " << system_info->efficiency_cores << "\n";
        std::cout << "   GPU Cores: " << system_info->gpu_cores << "\n";
        std::cout << "   Memory Size: " << (system_info->unified_memory_size / (1024*1024*1024)) << " GB\n";
        std::cout << "   Memory Bandwidth: " << system_info->memory_bandwidth_gbps << " GB/s\n";
    }
    std::cout << "\n";
    
    // Apple Silicon Optimizations
    std::cout << "2. Apple Silicon Optimizations:\n";
    auto opt_result = coordinator->initializeAppleSiliconOptimizations();
    std::cout << "   Initialization: ";
    switch (opt_result) {
        case CoordinationResult::Success:
            std::cout << "Success\n";
            break;
        case CoordinationResult::NotSupported:
            std::cout << "Not Supported (not on Apple Silicon)\n";
            break;
        default:
            std::cout << "Failed\n";
            break;
    }
    std::cout << "\n";
    
    // Development Tools
    std::cout << "3. Development Tools Detection:\n";
    std::cout << "   Xcode: " << (dev_tools->isToolAvailable(DevelopmentTool::Xcode) ? "Available" : "Not Available") << "\n";
    std::cout << "   Instruments: " << (dev_tools->isToolAvailable(DevelopmentTool::Instruments) ? "Available" : "Not Available") << "\n";
    std::cout << "   Activity Monitor: " << (dev_tools->isToolAvailable(DevelopmentTool::ActivityMonitor) ? "Available" : "Not Available") << "\n";
    std::cout << "\n";
    
    // File System Operations
    std::cout << "4. File System Operations:\n";
    auto cache_dir = file_system->getFlightCacheDirectory();
    std::cout << "   Flight Cache Directory: " << cache_dir.string() << "\n";
    std::cout << "   /tmp exists: " << (file_system->exists("/tmp") ? "Yes" : "No") << "\n";
    std::cout << "\n";
    
    // Performance Monitoring
    std::cout << "5. Performance Monitoring:\n";
    auto cpu_usage = coordinator->getCPUUtilization();
    std::cout << "   CPU Cores: " << cpu_usage.size() << "\n";
    std::cout << "   GPU Utilization: " << coordinator->getGPUUtilization() << "%\n";
    std::cout << "   Memory Pressure: " << coordinator->getMemoryPressure() << "%\n";
    std::cout << "   Thermal State: " << coordinator->getThermalState() << "\n";
    std::cout << "\n";
    
    // Process Management
    std::cout << "6. Process Management:\n";
    int current_pid = getpid();
    std::cout << "   Current PID: " << current_pid << "\n";
    bool priority_set = process_manager->setPriority(current_pid, ProcessPriority::Normal);
    std::cout << "   Priority Setting: " << (priority_set ? "Success" : "Failed") << "\n";
    std::cout << "\n";
    
    std::cout << "Flight HAL Basic Usage Example Complete!\n";
    
    return 0;
}
