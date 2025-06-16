/**
 * @file hal_config_hierarchy.cpp
 * @brief Flight HAL Hierarchical Configuration System Implementation
 */

#include "flight/hal/core/hal_config_hierarchy.hpp"
#include "flight/hal/core/hal_error.hpp"
#include <algorithm>
#include <sstream>

namespace flight::hal {

// ConfigLayer implementation
std::vector<std::string> ConfigLayer::get_keys() const {
    std::vector<std::string> keys;
    keys.reserve(values.size());
    for (const auto& [key, value] : values) {
        keys.push_back(key);
    }
    return keys;
}

// ConfigResolver implementation
HALResult<PlatformConfig> ConfigResolver::resolve_platform_config(
    const std::vector<std::shared_ptr<ConfigLayer>>& layers,
    ConfigResolutionStrategy strategy) {
    
    PlatformConfig config = PlatformConfig::create_default();
    
    // Resolve memory budget settings
    auto total_budget = resolve_value<size_t>("memory_budget.total_budget_mb", layers, strategy);
    if (total_budget.has_value()) {
        config.memory_budget.total_budget_mb = total_budget.value();
    }
    
    auto graphics_budget = resolve_value<size_t>("memory_budget.graphics_budget_mb", layers, strategy);
    if (graphics_budget.has_value()) {
        config.memory_budget.graphics_budget_mb = graphics_budget.value();
    }
    
    auto audio_budget = resolve_value<size_t>("memory_budget.audio_budget_mb", layers, strategy);
    if (audio_budget.has_value()) {
        config.memory_budget.audio_budget_mb = audio_budget.value();
    }
    
    auto network_budget = resolve_value<size_t>("memory_budget.network_buffer_mb", layers, strategy);
    if (network_budget.has_value()) {
        config.memory_budget.network_buffer_mb = network_budget.value();
    }
    
    auto file_cache_budget = resolve_value<size_t>("memory_budget.file_cache_mb", layers, strategy);
    if (file_cache_budget.has_value()) {
        config.memory_budget.file_cache_mb = file_cache_budget.value();
    }
    
    auto enforce_budgets = resolve_value<bool>("memory_budget.enforce_budgets", layers, strategy);
    if (enforce_budgets.has_value()) {
        config.memory_budget.enforce_budgets = enforce_budgets.value();
    }
    
    // Resolve performance settings
    auto enable_telemetry = resolve_value<bool>("performance.enable_telemetry", layers, strategy);
    if (enable_telemetry.has_value()) {
        config.performance.enable_telemetry = enable_telemetry.value();
    }
    
    auto enable_resource_tracking = resolve_value<bool>("performance.enable_resource_tracking", layers, strategy);
    if (enable_resource_tracking.has_value()) {
        config.performance.enable_resource_tracking = enable_resource_tracking.value();
    }
    
    auto enable_driver_profiling = resolve_value<bool>("performance.enable_driver_profiling", layers, strategy);
    if (enable_driver_profiling.has_value()) {
        config.performance.enable_driver_profiling = enable_driver_profiling.value();
    }
    
    auto telemetry_interval = resolve_value<int64_t>("performance.telemetry_interval_ms", layers, strategy);
    if (telemetry_interval.has_value()) {
        config.performance.telemetry_interval = std::chrono::milliseconds(telemetry_interval.value());
    }
    
    auto resource_check_interval = resolve_value<int64_t>("performance.resource_check_interval_ms", layers, strategy);
    if (resource_check_interval.has_value()) {
        config.performance.resource_check_interval = std::chrono::milliseconds(resource_check_interval.value());
    }
    
    // Resolve global settings
    auto debug_level = resolve_value<int>("debug_level", layers, strategy);
    if (debug_level.has_value()) {
        config.debug_level = static_cast<LogLevel>(debug_level.value());
    }
    
    auto enable_debug_output = resolve_value<bool>("enable_debug_output", layers, strategy);
    if (enable_debug_output.has_value()) {
        config.enable_debug_output = enable_debug_output.value();
    }
    
    auto enable_validation_checks = resolve_value<bool>("enable_validation_checks", layers, strategy);
    if (enable_validation_checks.has_value()) {
        config.enable_validation_checks = enable_validation_checks.value();
    }
    
    return HALResult<PlatformConfig>::success(std::move(config));
}

// ConfigPath implementation
ConfigPath::ConfigPath(const std::string& path) {
    if (path.empty()) {
        return;
    }
    
    std::stringstream ss(path);
    std::string component;
    
    while (std::getline(ss, component, '.')) {
        if (!component.empty()) {
            components_.push_back(component);
        }
    }
}

ConfigPath::ConfigPath(const std::vector<std::string>& components)
    : components_(components) {}

std::string ConfigPath::to_string() const {
    if (components_.empty()) {
        return "";
    }
    
    std::ostringstream oss;
    for (size_t i = 0; i < components_.size(); ++i) {
        if (i > 0) {
            oss << ".";
        }
        oss << components_[i];
    }
    return oss.str();
}

ConfigPath ConfigPath::parent() const {
    if (components_.empty()) {
        return ConfigPath(std::vector<std::string>{});
    }
    
    std::vector<std::string> parent_components(components_.begin(), components_.end() - 1);
    return ConfigPath(parent_components);
}

std::string ConfigPath::leaf() const {
    if (components_.empty()) {
        return "";
    }
    return components_.back();
}

ConfigPath ConfigPath::append(const std::string& component) const {
    std::vector<std::string> new_components = components_;
    new_components.push_back(component);
    return ConfigPath(new_components);
}

bool ConfigPath::operator==(const ConfigPath& other) const {
    return components_ == other.components_;
}

bool ConfigPath::operator<(const ConfigPath& other) const {
    return components_ < other.components_;
}

// HierarchicalConfig implementation
HierarchicalConfig::HierarchicalConfig() = default;

HierarchicalConfig::~HierarchicalConfig() = default;

HALResult<void> HierarchicalConfig::add_layer(std::shared_ptr<ConfigLayer> layer) {
    if (!layer) {
        return HALResult<void>::error(errors::invalid_parameter(300, "Layer cannot be null"));
    }
    
    std::unique_lock<std::shared_mutex> lock(layers_mutex_);
    
    if (layers_.find(layer->name) != layers_.end()) {
        return HALResult<void>::error(errors::invalid_parameter(301, ("Layer already exists: " + layer->name).c_str()));
    }
    
    layers_[layer->name] = layer;
    return HALResult<void>::success();
}

HALResult<void> HierarchicalConfig::remove_layer(const std::string& layer_name) {
    std::unique_lock<std::shared_mutex> lock(layers_mutex_);
    
    auto it = layers_.find(layer_name);
    if (it == layers_.end()) {
        return HALResult<void>::error(errors::invalid_parameter(302, ("Layer not found: " + layer_name).c_str()));
    }
    
    layers_.erase(it);
    return HALResult<void>::success();
}

std::shared_ptr<ConfigLayer> HierarchicalConfig::get_layer(const std::string& layer_name) {
    std::shared_lock<std::shared_mutex> lock(layers_mutex_);
    
    auto it = layers_.find(layer_name);
    if (it != layers_.end()) {
        return it->second;
    }
    return nullptr;
}

HALResult<void> HierarchicalConfig::set_layer_enabled(const std::string& layer_name, bool enabled) {
    auto layer = get_layer(layer_name);
    if (!layer) {
        return HALResult<void>::error(errors::invalid_parameter(303, ("Layer not found: " + layer_name).c_str()));
    }
    
    layer->enabled = enabled;
    return HALResult<void>::success();
}

std::vector<std::string> HierarchicalConfig::get_layer_names() const {
    std::shared_lock<std::shared_mutex> lock(layers_mutex_);
    
    std::vector<std::string> names;
    names.reserve(layers_.size());
    for (const auto& [name, layer] : layers_) {
        names.push_back(name);
    }
    return names;
}

std::vector<std::shared_ptr<ConfigLayer>> HierarchicalConfig::get_ordered_layers() const {
    std::shared_lock<std::shared_mutex> lock(layers_mutex_);
    
    std::vector<std::shared_ptr<ConfigLayer>> ordered_layers;
    ordered_layers.reserve(layers_.size());
    
    for (const auto& [name, layer] : layers_) {
        if (layer->enabled) {
            ordered_layers.push_back(layer);
        }
    }
    
    // Sort by priority (highest first)
    std::sort(ordered_layers.begin(), ordered_layers.end(),
        [](const std::shared_ptr<ConfigLayer>& a, const std::shared_ptr<ConfigLayer>& b) {
            return static_cast<int>(a->priority) > static_cast<int>(b->priority);
        });
    
    return ordered_layers;
}

void HierarchicalConfig::set_resolution_strategy(ConfigResolutionStrategy strategy) {
    resolution_strategy_ = strategy;
}

HALResult<PlatformConfig> HierarchicalConfig::resolve_platform_config() const {
    auto ordered_layers = get_ordered_layers();
    return ConfigResolver::resolve_platform_config(ordered_layers, resolution_strategy_);
}

HALResult<void> HierarchicalConfig::load_from_platform_config(const PlatformConfig& config, const std::string& layer_name) {
    auto layer = get_layer(layer_name);
    if (!layer) {
        return HALResult<void>::error(errors::invalid_parameter(304, ("Layer not found: " + layer_name).c_str()));
    }
    
    // Clear existing values
    layer->clear();
    
    // Load memory budget settings
    layer->set_value("memory_budget.total_budget_mb", config.memory_budget.total_budget_mb);
    layer->set_value("memory_budget.graphics_budget_mb", config.memory_budget.graphics_budget_mb);
    layer->set_value("memory_budget.audio_budget_mb", config.memory_budget.audio_budget_mb);
    layer->set_value("memory_budget.network_buffer_mb", config.memory_budget.network_buffer_mb);
    layer->set_value("memory_budget.file_cache_mb", config.memory_budget.file_cache_mb);
    layer->set_value("memory_budget.enforce_budgets", config.memory_budget.enforce_budgets);
    
    // Load performance settings
    layer->set_value("performance.enable_telemetry", config.performance.enable_telemetry);
    layer->set_value("performance.enable_resource_tracking", config.performance.enable_resource_tracking);
    layer->set_value("performance.enable_driver_profiling", config.performance.enable_driver_profiling);
    layer->set_value("performance.telemetry_interval_ms", static_cast<int64_t>(config.performance.telemetry_interval.count()));
    layer->set_value("performance.resource_check_interval_ms", static_cast<int64_t>(config.performance.resource_check_interval.count()));
    
    // Load global settings
    layer->set_value("debug_level", static_cast<int>(config.debug_level));
    layer->set_value("enable_debug_output", config.enable_debug_output);
    layer->set_value("enable_validation_checks", config.enable_validation_checks);
    
    return HALResult<void>::success();
}

HALResult<void> HierarchicalConfig::create_standard_layers() {
    // Create system defaults layer
    auto system_layer = std::make_shared<ConfigLayer>("system", ConfigPriority::System, "System default settings");
    auto result = add_layer(system_layer);
    if (!result.is_success()) {
        return result;
    }
    
    // Load default configuration into system layer
    auto default_config = PlatformConfig::create_default();
    result = load_from_platform_config(default_config, "system");
    if (!result.is_success()) {
        return result;
    }
    
    // Create platform layer
    auto platform_layer = std::make_shared<ConfigLayer>("platform", ConfigPriority::Platform, "Platform-specific settings");
    result = add_layer(platform_layer);
    if (!result.is_success()) {
        return result;
    }
    
    // Create environment layer
    auto env_layer = std::make_shared<ConfigLayer>("environment", ConfigPriority::Environment, "Environment variable settings");
    result = add_layer(env_layer);
    if (!result.is_success()) {
        return result;
    }
    
    // Create application layer
    auto app_layer = std::make_shared<ConfigLayer>("application", ConfigPriority::Application, "Application settings");
    result = add_layer(app_layer);
    if (!result.is_success()) {
        return result;
    }
    
    // Create user layer
    auto user_layer = std::make_shared<ConfigLayer>("user", ConfigPriority::User, "User preferences");
    result = add_layer(user_layer);
    if (!result.is_success()) {
        return result;
    }
    
    // Create runtime layer
    auto runtime_layer = std::make_shared<ConfigLayer>("runtime", ConfigPriority::Runtime, "Runtime overrides");
    result = add_layer(runtime_layer);
    if (!result.is_success()) {
        return result;
    }
    
    return HALResult<void>::success();
}

HALResult<void> HierarchicalConfig::validate() const {
    std::shared_lock<std::shared_mutex> lock(layers_mutex_);
    
    // Validate that we have at least one enabled layer
    bool has_enabled_layer = false;
    for (const auto& [name, layer] : layers_) {
        if (layer->enabled) {
            has_enabled_layer = true;
            break;
        }
    }
    
    if (!has_enabled_layer) {
        return HALResult<void>::error(errors::validation_failed(305, "No enabled configuration layers"));
    }
    
    // Try to resolve platform configuration to validate consistency
    auto config_result = resolve_platform_config();
    if (!config_result.is_success()) {
        return HALResult<void>::error(config_result.error());
    }
    
    // Validate the resolved configuration
    return config_result.value().validate();
}

HierarchicalConfig::HierarchyStats HierarchicalConfig::get_stats() const {
    std::shared_lock<std::shared_mutex> lock(layers_mutex_);
    
    HierarchyStats stats;
    stats.total_layers = layers_.size();
    
    for (const auto& [name, layer] : layers_) {
        if (layer->enabled) {
            stats.enabled_layers++;
        }
        
        size_t layer_keys = layer->values.size();
        stats.keys_per_layer[name] = layer_keys;
        stats.total_keys += layer_keys;
    }
    
    return stats;
}

std::string HierarchicalConfig::path_to_key(const ConfigPath& path) const {
    return path.to_string();
}

std::shared_ptr<ConfigLayer> HierarchicalConfig::get_write_layer() {
    // Find the highest priority enabled layer for writing
    auto ordered_layers = get_ordered_layers();
    if (!ordered_layers.empty()) {
        return ordered_layers.front();
    }
    
    // If no enabled layers, try to find any layer
    std::shared_lock<std::shared_mutex> lock(layers_mutex_);
    if (!layers_.empty()) {
        return layers_.begin()->second;
    }
    
    return nullptr;
}

} // namespace flight::hal
