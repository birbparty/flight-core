/**
 * @file comprehensive_config_example.cpp
 * @brief Comprehensive Configuration Management Example
 * 
 * Demonstrates the Flight HAL configuration system including:
 * - Hierarchical configuration with priority layers
 * - Runtime hot reloading capabilities
 * - Platform-specific configuration handling
 * - Configuration validation and rollback
 * - Type-safe configuration access
 */

#include "flight/hal/core/hal_config_runtime.hpp"
#include "flight/hal/core/hal_config_hierarchy.hpp"
#include "flight/hal/core/platform_config.hpp"
#include "flight/hal/core/hal_logging.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>

using namespace flight::hal;

/**
 * @brief Example configuration change listener
 */
class ConfigurationWatcher : public IConfigChangeListener {
public:
    explicit ConfigurationWatcher(const std::string& name) : name_(name) {}
    
    HALResult<void> on_config_changed(const ConfigChangeEvent& event,
                                     const PlatformConfig* old_config,
                                     const PlatformConfig* new_config) override {
        std::cout << "[" << name_ << "] Configuration changed: " 
                  << "Type=" << static_cast<int>(event.type)
                  << ", Section=" << event.section
                  << ", Source=" << event.source << std::endl;
        
        if (old_config && new_config) {
            // Log specific changes
            if (old_config->memory_budget.total_budget_mb != new_config->memory_budget.total_budget_mb) {
                std::cout << "  Memory budget changed: " 
                          << old_config->memory_budget.total_budget_mb << " -> " 
                          << new_config->memory_budget.total_budget_mb << " MB" << std::endl;
            }
            
            if (old_config->performance.enable_telemetry != new_config->performance.enable_telemetry) {
                std::cout << "  Telemetry " << (new_config->performance.enable_telemetry ? "enabled" : "disabled") << std::endl;
            }
        }
        
        return HALResult<void>::success();
    }
    
    int get_priority() const override { return 100; }
    std::string get_name() const override { return name_; }

private:
    std::string name_;
};

/**
 * @brief Create sample configuration files for testing
 */
void create_sample_config_files() {
    // Create a sample configuration file for hot reloading
    std::ofstream config_file("sample_config.json");
    config_file << R"({
  "memory_budget": {
    "total_budget_mb": 1024,
    "graphics_budget_mb": 512,
    "audio_budget_mb": 128,
    "network_buffer_mb": 64,
    "file_cache_mb": 256,
    "enforce_budgets": true
  },
  "performance": {
    "enable_telemetry": true,
    "enable_resource_tracking": true,
    "enable_driver_profiling": false,
    "telemetry_interval_ms": 2000,
    "resource_check_interval_ms": 1000
  },
  "debug_level": 2,
  "enable_debug_output": true,
  "enable_validation_checks": true
})";
    config_file.close();
    
    // Create Dreamcast-specific configuration
    std::ofstream dreamcast_config("dreamcast_config.json");
    dreamcast_config << R"({
  "memory_budget": {
    "total_budget_mb": 32,
    "graphics_budget_mb": 16,
    "audio_budget_mb": 4,
    "network_buffer_mb": 2,
    "file_cache_mb": 8,
    "enforce_budgets": true
  },
  "performance": {
    "enable_telemetry": false,
    "enable_resource_tracking": true,
    "enable_driver_profiling": false,
    "telemetry_interval_ms": 5000,
    "resource_check_interval_ms": 2000
  },
  "debug_level": 1,
  "enable_debug_output": false,
  "enable_validation_checks": false
})";
    dreamcast_config.close();
}

/**
 * @brief Demonstrate hierarchical configuration
 */
void demonstrate_hierarchical_config() {
    std::cout << "\n=== Hierarchical Configuration Demo ===" << std::endl;
    
    HierarchicalConfig hierarchy;
    
    // Create standard layers
    auto result = hierarchy.create_standard_layers();
    if (!result.is_success()) {
        std::cout << "Failed to create standard layers: " << result.error().message << std::endl;
        return;
    }
    
    // Show initial configuration
    auto platform_config_result = hierarchy.resolve_platform_config();
    if (platform_config_result.is_success()) {
        auto config = platform_config_result.value();
        std::cout << "Initial configuration:" << std::endl;
        std::cout << "  Memory budget: " << config.memory_budget.total_budget_mb << " MB" << std::endl;
        std::cout << "  Telemetry: " << (config.performance.enable_telemetry ? "enabled" : "disabled") << std::endl;
        std::cout << "  Debug level: " << static_cast<int>(config.debug_level) << std::endl;
    }
    
    // Add platform-specific overrides
    auto platform_layer = hierarchy.get_layer("platform");
    if (platform_layer) {
        platform_layer->set_value("memory_budget.total_budget_mb", size_t(256));
        platform_layer->set_value("performance.enable_telemetry", false);
        
        std::cout << "\nAfter platform overrides:" << std::endl;
        auto updated_config_result = hierarchy.resolve_platform_config();
        if (updated_config_result.is_success()) {
            auto updated_config = updated_config_result.value();
            std::cout << "  Memory budget: " << updated_config.memory_budget.total_budget_mb << " MB" << std::endl;
            std::cout << "  Telemetry: " << (updated_config.performance.enable_telemetry ? "enabled" : "disabled") << std::endl;
        }
    }
    
    // Add user overrides with higher priority
    auto user_layer = hierarchy.get_layer("user");
    if (user_layer) {
        user_layer->set_value("performance.enable_telemetry", true);
        
        std::cout << "\nAfter user overrides:" << std::endl;
        auto final_config_result = hierarchy.resolve_platform_config();
        if (final_config_result.is_success()) {
            auto final_config = final_config_result.value();
            std::cout << "  Memory budget: " << final_config.memory_budget.total_budget_mb << " MB" << std::endl;
            std::cout << "  Telemetry: " << (final_config.performance.enable_telemetry ? "enabled" : "disabled") << std::endl;
        }
    }
    
    // Demonstrate type-safe accessors
    std::cout << "\nType-safe accessors:" << std::endl;
    auto memory_accessor = hierarchy.get_accessor<size_t>("memory_budget.total_budget_mb");
    auto telemetry_accessor = hierarchy.get_accessor<bool>("performance.enable_telemetry");
    
    std::cout << "  Memory budget (accessor): " << memory_accessor.get_or(0) << " MB" << std::endl;
    std::cout << "  Telemetry (accessor): " << (telemetry_accessor.get_or(false) ? "enabled" : "disabled") << std::endl;
    
    // Show hierarchy statistics
    auto stats = hierarchy.get_stats();
    std::cout << "\nHierarchy statistics:" << std::endl;
    std::cout << "  Total layers: " << stats.total_layers << std::endl;
    std::cout << "  Enabled layers: " << stats.enabled_layers << std::endl;
    std::cout << "  Total keys: " << stats.total_keys << std::endl;
}

/**
 * @brief Demonstrate runtime configuration management
 */
void demonstrate_runtime_config() {
    std::cout << "\n=== Runtime Configuration Demo ===" << std::endl;
    
    // Create runtime configuration manager
    auto default_config = PlatformConfig::create_default();
    RuntimeConfigManager runtime_manager(default_config);
    
    // Add configuration change listeners
    auto watcher1 = std::make_shared<ConfigurationWatcher>("MemoryManager");
    auto watcher2 = std::make_shared<ConfigurationWatcher>("PerformanceMonitor");
    
    runtime_manager.add_listener(watcher1);
    runtime_manager.add_listener(watcher2);
    
    // Demonstrate configuration updates
    std::cout << "Initial configuration memory budget: " 
              << runtime_manager.get_config().memory_budget.total_budget_mb << " MB" << std::endl;
    
    // Update configuration
    auto high_perf_config = PlatformConfig::create_high_performance();
    ConfigValidationContext context;
    context.platform_name = "modern";
    
    auto update_result = runtime_manager.update_config(high_perf_config, context);
    if (update_result.is_success()) {
        std::cout << "Updated to high-performance configuration" << std::endl;
        std::cout << "New memory budget: " 
                  << runtime_manager.get_config().memory_budget.total_budget_mb << " MB" << std::endl;
    } else {
        std::cout << "Configuration update failed: " << update_result.error().message << std::endl;
    }
    
    // Demonstrate rollback
    std::cout << "\nDemonstrating rollback..." << std::endl;
    auto rollback_result = runtime_manager.rollback(context);
    if (rollback_result.is_success()) {
        std::cout << "Rolled back to previous configuration" << std::endl;
        std::cout << "Memory budget after rollback: " 
                  << runtime_manager.get_config().memory_budget.total_budget_mb << " MB" << std::endl;
    }
    
    // Show rollback point information
    std::cout << "\nRollback points available: " << runtime_manager.get_rollback_point_count() << std::endl;
    for (size_t i = 0; i < runtime_manager.get_rollback_point_count(); ++i) {
        auto point_info = runtime_manager.get_rollback_point_info(i);
        if (point_info) {
            std::cout << "  Point " << i << ": " << point_info->description 
                      << " (source: " << point_info->source << ")" << std::endl;
        }
    }
    
    // Show statistics
    auto stats = runtime_manager.get_stats();
    std::cout << "\nRuntime manager statistics:" << std::endl;
    std::cout << "  Total updates: " << stats.total_updates << std::endl;
    std::cout << "  Successful updates: " << stats.successful_updates << std::endl;
    std::cout << "  Failed updates: " << stats.failed_updates << std::endl;
    std::cout << "  Rollbacks: " << stats.rollbacks << std::endl;
}

/**
 * @brief Demonstrate platform-specific configuration
 */
void demonstrate_platform_configs() {
    std::cout << "\n=== Platform-Specific Configuration Demo ===" << std::endl;
    
    // Modern platform configuration
    auto modern_config = PlatformConfig::create_high_performance();
    std::cout << "Modern platform configuration:" << std::endl;
    std::cout << "  Memory budget: " << modern_config.memory_budget.total_budget_mb << " MB" << std::endl;
    std::cout << "  Graphics budget: " << modern_config.memory_budget.graphics_budget_mb << " MB" << std::endl;
    std::cout << "  Telemetry interval: " << modern_config.performance.telemetry_interval.count() << " ms" << std::endl;
    
    // Dreamcast configuration (minimal)
    auto dreamcast_config = PlatformConfig::create_minimal();
    std::cout << "\nDreamcast configuration:" << std::endl;
    std::cout << "  Memory budget: " << dreamcast_config.memory_budget.total_budget_mb << " MB" << std::endl;
    std::cout << "  Graphics budget: " << dreamcast_config.memory_budget.graphics_budget_mb << " MB" << std::endl;
    std::cout << "  Telemetry interval: " << dreamcast_config.performance.telemetry_interval.count() << " ms" << std::endl;
    
    // Validate for different platforms
    ConfigValidationContext modern_context("modern");
    ConfigValidationContext dreamcast_context("dreamcast");
    
    auto modern_validation = RuntimeConfigManager::validate_config(modern_config, modern_context);
    auto dreamcast_validation = RuntimeConfigManager::validate_config(dreamcast_config, dreamcast_context);
    
    std::cout << "\nValidation results:" << std::endl;
    std::cout << "  Modern config valid: " << (modern_validation.is_success() ? "yes" : "no") << std::endl;
    std::cout << "  Dreamcast config valid: " << (dreamcast_validation.is_success() ? "yes" : "no") << std::endl;
    
    // Try to use modern config on Dreamcast (should fail)
    auto invalid_validation = RuntimeConfigManager::validate_config(modern_config, dreamcast_context);
    std::cout << "  Modern config on Dreamcast: " << (invalid_validation.is_success() ? "valid" : "invalid") << std::endl;
    if (!invalid_validation.is_success()) {
        std::cout << "    Reason: " << invalid_validation.error().message << std::endl;
    }
}

/**
 * @brief Demonstrate hot reloading (simulated)
 */
void demonstrate_hot_reload() {
    std::cout << "\n=== Hot Reload Demo ===" << std::endl;
    
    RuntimeConfigManager runtime_manager;
    auto watcher = std::make_shared<ConfigurationWatcher>("HotReloadWatcher");
    runtime_manager.add_listener(watcher);
    
    // Enable hot reload (would normally watch a real file)
    std::cout << "Enabling hot reload for sample_config.json..." << std::endl;
    auto enable_result = runtime_manager.enable_hot_reload("sample_config.json");
    if (enable_result.is_success()) {
        std::cout << "Hot reload enabled successfully" << std::endl;
        std::cout << "Hot reload status: " << (runtime_manager.is_hot_reload_enabled() ? "enabled" : "disabled") << std::endl;
    } else {
        std::cout << "Failed to enable hot reload: " << enable_result.error().message << std::endl;
    }
    
    // Demonstrate scoped configuration updates
    std::cout << "\nDemonstrating scoped configuration updates..." << std::endl;
    {
        ScopedConfigUpdate scoped_update(runtime_manager, "Temporary performance boost");
        
        auto boost_config = runtime_manager.get_config();
        boost_config.memory_budget.total_budget_mb = 2048;
        boost_config.performance.enable_telemetry = true;
        
        auto update_result = scoped_update.update(boost_config);
        if (update_result.is_success()) {
            std::cout << "Applied temporary performance boost" << std::endl;
            std::cout << "Current memory budget: " 
                      << runtime_manager.get_config().memory_budget.total_budget_mb << " MB" << std::endl;
        }
        
        scoped_update.commit(); // Comment this line to see automatic rollback
    }
    
    std::cout << "After scoped update (committed):" << std::endl;
    std::cout << "Memory budget: " << runtime_manager.get_config().memory_budget.total_budget_mb << " MB" << std::endl;
}

int main() {
    std::cout << "Flight HAL Configuration Management Example" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    // Create sample configuration files
    create_sample_config_files();
    
    try {
        // Demonstrate different aspects of the configuration system
        demonstrate_hierarchical_config();
        demonstrate_runtime_config();
        demonstrate_platform_configs();
        demonstrate_hot_reload();
        
        std::cout << "\n=== Configuration Management Demo Complete ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
