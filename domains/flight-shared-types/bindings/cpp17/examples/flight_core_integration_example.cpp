/**
 * Flight-Core C++17 Integration Example
 * 
 * Demonstrates modern C++17 bindings for Flight Shared Types
 * with zero-cost abstractions and platform adaptation
 */

#include "flight_shared_types.hpp"
#include <iostream>
#include <iomanip>

using namespace flight::shared_types;

void print_separator(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(60, '=') << "\n";
}

void demonstrate_memory_management() {
    print_separator("Memory Management - C++17 Features");
    
    // MemorySize with modern C++17 features
    auto memory_size = memory::MemorySize::from_mb(512);
    std::cout << "Memory size: " << memory_size.bytes << " bytes = " << memory_size.human_readable << "\n";
    
    // Platform-specific memory sizes
    auto dreamcast_mem = memory::MemorySize::dreamcast_total();
    auto v6r_large_mem = memory::MemorySize::v6r_large_total();
    
    std::cout << "Platform Memory Sizes:\n";
    std::cout << "  Dreamcast: " << dreamcast_mem.to_string() << "\n";
    std::cout << "  V6R Large: " << v6r_large_mem.to_string() << "\n";
    
    // V6R Memory Utilities
    auto snapshot = memory::v6r::V6RMemoryUtils::create_snapshot(
        "example-session", "v6r-medium", memory::MemorySize::from_mb(256)
    );
    
    std::cout << "\nV6R Memory Snapshot:\n";
    std::cout << "  Session: " << snapshot.session_id << "\n";
    std::cout << "  Platform: " << snapshot.platform << "\n";
    std::cout << "  Usage: " << std::fixed << std::setprecision(1) 
              << snapshot.usage_percentage() << "%\n";
    std::cout << "  Low Memory: " << (snapshot.is_low_memory() ? "Yes" : "No") << "\n";
    std::cout << "  Fragmented: " << (snapshot.is_fragmented() ? "Yes" : "No") << "\n";
}

void demonstrate_error_handling() {
    print_separator("Error Handling - Exception-Free Design");
    
    // Create various error types
    auto memory_error = error::ErrorOperations::create_simple_error(
        error::ErrorSeverity::Warning,
        error::ErrorCategory::Memory,
        "Memory usage approaching limit",
        "memory_monitor",
        "check_usage"
    );
    
    auto platform_error = error::ErrorOperations::create_platform_error(
        error::PlatformErrorCode::InsufficientPlatformMemory,
        "Dreamcast memory constraint violated",
        "dreamcast",
        "allocate_texture"
    );
    
    std::cout << "Memory Error: " << memory_error.to_string() << "\n";
    std::cout << "Platform Error: " << platform_error.to_string() << "\n";
    
    // Error recovery suggestions
    auto suggestions = error::ErrorOperations::get_recovery_suggestions(memory_error);
    std::cout << "\nRecovery Suggestions:\n";
    for (const auto& suggestion : suggestions) {
        std::cout << "  - " << suggestion.description 
                  << " (Priority: " << suggestion.priority 
                  << ", Auto: " << (suggestion.can_automate ? "Yes" : "No") << ")\n";
    }
    
    // Result type usage with std::variant
    auto memory_result = memory::MemoryOperations::get_memory_snapshot("example-session");
    if (memory::is_ok(memory_result)) {
        const auto& snapshot = memory::unwrap(memory_result);
        std::cout << "\nMemory operation succeeded: " << snapshot.used.to_string() << " used\n";
    } else {
        const auto& error = memory::unwrap_err(memory_result);
        std::cout << "\nMemory operation failed: " << error.to_string() << "\n";
    }
}

void demonstrate_platform_detection() {
    print_separator("Platform Detection - Multi-Platform Support");
    
    // Detect current platform
    auto platform_result = platform::PlatformDetector::detect_current_platform();
    if (error::is_ok(platform_result)) {
        const auto& platform_info = error::unwrap(platform_result);
        
        std::cout << "Current Platform:\n";
        std::cout << "  Name: " << platform_info.name << "\n";
        std::cout << "  ID: " << platform_info.id << "\n";
        std::cout << "  Capability: ";
        
        switch (platform_info.capability) {
            case platform::PlatformCapability::Minimal: std::cout << "Minimal (16MB)"; break;
            case platform::PlatformCapability::Basic: std::cout << "Basic (32-64MB)"; break;
            case platform::PlatformCapability::Standard: std::cout << "Standard (512MB)"; break;
            case platform::PlatformCapability::Enhanced: std::cout << "Enhanced (512MB-1GB)"; break;
            case platform::PlatformCapability::Full: std::cout << "Full (1-2GB)"; break;
            case platform::PlatformCapability::Unlimited: std::cout << "Unlimited (2GB+)"; break;
        }
        std::cout << "\n";
        
        std::cout << "  Memory: " << platform_info.memory.total_memory.to_string() << " total\n";
        std::cout << "  CPU: " << platform_info.cpu.core_count << " cores @ " 
                  << platform_info.cpu.clock_speed_mhz << " MHz\n";
        std::cout << "  Features:\n";
        std::cout << "    Threading: " << (platform_info.supports_threading() ? "Yes" : "No") << "\n";
        std::cout << "    Networking: " << (platform_info.supports_networking() ? "Yes" : "No") << "\n";
        std::cout << "    Cloud Platform: " << (platform_info.is_cloud_platform() ? "Yes" : "No") << "\n";
        std::cout << "    Retro Platform: " << (platform_info.is_retro_platform() ? "Yes" : "No") << "\n";
    } else {
        const auto& error = error::unwrap_err(platform_result);
        std::cout << "Platform detection failed: " << error.to_string() << "\n";
    }
    
    // Show different platform profiles
    std::cout << "\nPlatform Profiles:\n";
    auto dreamcast = platform::PlatformDetector::get_dreamcast_info();
    auto v6r = platform::PlatformDetector::get_v6r_info("large");
    
    std::cout << "  " << dreamcast.name << ": " 
              << dreamcast.memory.total_memory.to_string()
              << (dreamcast.is_constrained() ? " (Constrained)" : "") << "\n";
    std::cout << "  " << v6r.name << ": " 
              << v6r.memory.total_memory.to_string()
              << (v6r.is_cloud_platform() ? " (Cloud)" : "") << "\n";
}

void demonstrate_session_management() {
    print_separator("Session Management - Universal Session Handling");
    
    // Create different session types
    auto system_session_result = session::SessionOperations::create_session(
        session::SessionType::System, "flight", std::nullopt
    );
    
    auto user_session_result = session::SessionOperations::create_session(
        session::SessionType::User, "v6r-medium", std::string("user123")
    );
    
    if (error::is_ok(system_session_result)) {
        const auto& session = error::unwrap(system_session_result);
        std::cout << "System Session Created:\n";
        std::cout << "  ID: " << session.id << "\n";
        std::cout << "  Platform: " << session.platform << "\n";
        std::cout << "  Active: " << (session.is_active() ? "Yes" : "No") << "\n";
        std::cout << "  Age: " << std::fixed << std::setprecision(2) 
                  << session.age().count() << " seconds\n";
    }
    
    if (error::is_ok(user_session_result)) {
        const auto& session = error::unwrap(user_session_result);
        std::cout << "\nUser Session Created:\n";
        std::cout << "  ID: " << session.id << "\n";
        std::cout << "  User: " << (session.user_id ? *session.user_id : "None") << "\n";
        std::cout << "  Platform: " << session.platform << "\n";
        std::cout << "  Healthy: " << (session.is_healthy() ? "Yes" : "No") << "\n";
    }
}

void demonstrate_component_model() {
    print_separator("Component Model - Flight-Core Integration");
    
    // Create Flight-Core components
    auto hal_result = integration::FlightCoreIntegration::create_hal_component("dreamcast");
    auto runtime_result = integration::FlightCoreIntegration::create_runtime_component("v6r-small");
    
    if (error::is_ok(hal_result)) {
        const auto& component = error::unwrap(hal_result);
        std::cout << "HAL Component:\n";
        std::cout << "  ID: " << component.id << "\n";
        std::cout << "  Name: " << component.name << "\n";
        std::cout << "  World: " << component.world << "\n";
        std::cout << "  Running: " << (component.is_running() ? "Yes" : "No") << "\n";
        std::cout << "  Memory: " << component.memory_usage.used.to_string() << "\n";
        std::cout << "  Uptime: " << std::fixed << std::setprecision(2) 
                  << component.uptime().count() << " seconds\n";
    }
    
    if (error::is_ok(runtime_result)) {
        const auto& component = error::unwrap(runtime_result);
        std::cout << "\nRuntime Component:\n";
        std::cout << "  ID: " << component.id << "\n";
        std::cout << "  Platform: " << component.platform << "\n";
        std::cout << "  Healthy: " << (component.is_healthy() ? "Yes" : "No") << "\n";
    }
}

void demonstrate_scoped_resources() {
    print_separator("RAII Resource Management - C++17 Features");
    
    // Demonstrate RAII with std::optional and automatic cleanup
    {
        auto session_result = session::SessionOperations::create_session(
            session::SessionType::Testing, "example-platform", std::nullopt
        );
        
        if (error::is_ok(session_result)) {
            auto session = error::unwrap(session_result);
            std::string session_id = session.id;
            
            std::cout << "Working with scoped session: " << session_id << "\n";
            std::cout << "Session will be automatically managed...\n";
            
            // Demonstrate resource cleanup
            session::SessionOperations::terminate_session(session_id);
            std::cout << "Session terminated: " << session_id << "\n";
            
        } // Automatic cleanup via destructors
    }
    
    std::cout << "RAII resource management demonstrated\n";
}

int main() {
    std::cout << "Flight-Core C++17 Integration Example\n";
    std::cout << "Modern zero-cost abstractions for multi-platform development\n";
    
    try {
        demonstrate_memory_management();
        demonstrate_error_handling();
        demonstrate_platform_detection();
        demonstrate_session_management();
        demonstrate_component_model();
        demonstrate_scoped_resources();
        
        print_separator("Example Completed Successfully");
        std::cout << "All Flight-Core C++17 features demonstrated!\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Example failed with exception: " << e.what() << "\n";
        return 1;
    }
}
