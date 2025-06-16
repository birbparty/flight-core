/**
 * @file hal_config_hierarchy.hpp
 * @brief Flight HAL Hierarchical Configuration System
 * 
 * Provides advanced configuration layering with priority-based overrides,
 * template-based type-safe access, and configuration inheritance.
 */

#pragma once

#include "platform_config.hpp"
#include "hal_result.hpp"
#include "hal_logging.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <any>
#include <functional>
#include <type_traits>
#include <optional>
#include <shared_mutex>

namespace flight::hal {

/**
 * @brief Configuration layer priority levels
 */
enum class ConfigPriority : int {
    System = 0,         ///< System defaults (lowest priority)
    Platform = 100,     ///< Platform-specific settings
    Environment = 200,  ///< Environment variables
    Application = 300,  ///< Application-specific settings
    User = 400,         ///< User overrides
    Runtime = 500,      ///< Runtime overrides (highest priority)
    Override = 1000     ///< Emergency overrides
};

/**
 * @brief Configuration layer information
 */
struct ConfigLayer {
    std::string name;
    ConfigPriority priority;
    std::string description;
    std::unordered_map<std::string, std::any> values;
    bool enabled = true;
    
    ConfigLayer(const std::string& layer_name, ConfigPriority prio, const std::string& desc = "")
        : name(layer_name), priority(prio), description(desc) {}
    
    /**
     * @brief Set a typed value in this layer
     */
    template<typename T>
    void set_value(const std::string& key, const T& value) {
        values[key] = value;
    }
    
    /**
     * @brief Get a typed value from this layer
     */
    template<typename T>
    std::optional<T> get_value(const std::string& key) const {
        auto it = values.find(key);
        if (it != values.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }
    
    /**
     * @brief Check if key exists in this layer
     */
    bool has_key(const std::string& key) const {
        return values.find(key) != values.end();
    }
    
    /**
     * @brief Remove a key from this layer
     */
    void remove_key(const std::string& key) {
        values.erase(key);
    }
    
    /**
     * @brief Clear all values in this layer
     */
    void clear() {
        values.clear();
    }
    
    /**
     * @brief Get all keys in this layer
     */
    std::vector<std::string> get_keys() const;
};

/**
 * @brief Configuration resolution strategy
 */
enum class ConfigResolutionStrategy {
    PriorityBased,      ///< Higher priority layers override lower priority
    Merge,              ///< Merge compatible values from multiple layers
    FirstFound,         ///< Use first found value regardless of priority
    LastFound           ///< Use last found value regardless of priority
};

/**
 * @brief Configuration resolver for handling layer resolution
 */
class ConfigResolver {
public:
    /**
     * @brief Resolve configuration value from layers
     * @tparam T Type of configuration value
     * @param key Configuration key
     * @param layers Ordered list of layers (by priority)
     * @param strategy Resolution strategy
     * @return Resolved value or nullopt if not found
     */
    template<typename T>
    static std::optional<T> resolve_value(const std::string& key,
                                         const std::vector<std::shared_ptr<ConfigLayer>>& layers,
                                         ConfigResolutionStrategy strategy = ConfigResolutionStrategy::PriorityBased);
    
    /**
     * @brief Resolve platform configuration from layers
     * @param layers Configuration layers
     * @param strategy Resolution strategy
     * @return Resolved platform configuration
     */
    static HALResult<PlatformConfig> resolve_platform_config(
        const std::vector<std::shared_ptr<ConfigLayer>>& layers,
        ConfigResolutionStrategy strategy = ConfigResolutionStrategy::PriorityBased);

private:
    template<typename T>
    static std::optional<T> resolve_priority_based(const std::string& key,
                                                   const std::vector<std::shared_ptr<ConfigLayer>>& layers);
    
    template<typename T>
    static std::optional<T> resolve_merge(const std::string& key,
                                         const std::vector<std::shared_ptr<ConfigLayer>>& layers);
};

/**
 * @brief Configuration path for nested access
 */
class ConfigPath {
public:
    /**
     * @brief Constructor from string path (dot-separated)
     * @param path Path string like "memory_budget.total_budget_mb"
     */
    explicit ConfigPath(const std::string& path);
    
    /**
     * @brief Constructor from path components
     * @param components Path components
     */
    explicit ConfigPath(const std::vector<std::string>& components);
    
    /**
     * @brief Get path components
     */
    const std::vector<std::string>& components() const { return components_; }
    
    /**
     * @brief Get path as string
     */
    std::string to_string() const;
    
    /**
     * @brief Get parent path
     */
    ConfigPath parent() const;
    
    /**
     * @brief Get leaf component (last component)
     */
    std::string leaf() const;
    
    /**
     * @brief Check if this is a root path
     */
    bool is_root() const { return components_.empty(); }
    
    /**
     * @brief Append component to path
     */
    ConfigPath append(const std::string& component) const;
    
    /**
     * @brief Equality comparison
     */
    bool operator==(const ConfigPath& other) const;
    
    /**
     * @brief Less than comparison for ordering
     */
    bool operator<(const ConfigPath& other) const;

private:
    std::vector<std::string> components_;
};

/**
 * @brief Type-safe configuration accessor
 */
template<typename T>
class ConfigAccessor {
public:
    /**
     * @brief Constructor
     * @param hierarchy Reference to configuration hierarchy
     * @param path Configuration path
     */
    ConfigAccessor(class HierarchicalConfig& hierarchy, const ConfigPath& path);
    
    /**
     * @brief Get configuration value
     * @return Configuration value or nullopt if not found
     */
    std::optional<T> get() const;
    
    /**
     * @brief Get configuration value with default
     * @param default_value Default value if not found
     * @return Configuration value or default
     */
    T get_or(const T& default_value) const;
    
    /**
     * @brief Set configuration value in highest priority layer
     * @param value Value to set
     * @return HALResult indicating success or failure
     */
    HALResult<void> set(const T& value);
    
    /**
     * @brief Set configuration value in specific layer
     * @param value Value to set
     * @param layer_name Layer name
     * @return HALResult indicating success or failure
     */
    HALResult<void> set_in_layer(const T& value, const std::string& layer_name);
    
    /**
     * @brief Check if configuration value exists
     */
    bool exists() const;
    
    /**
     * @brief Remove configuration value from all layers
     */
    HALResult<void> remove();
    
    /**
     * @brief Remove configuration value from specific layer
     * @param layer_name Layer name
     */
    HALResult<void> remove_from_layer(const std::string& layer_name);

private:
    class HierarchicalConfig& hierarchy_;
    ConfigPath path_;
};

/**
 * @brief Hierarchical configuration manager
 */
class HierarchicalConfig {
public:
    /**
     * @brief Constructor
     */
    HierarchicalConfig();
    
    /**
     * @brief Destructor
     */
    ~HierarchicalConfig();
    
    /**
     * @brief Add configuration layer
     * @param layer Configuration layer
     * @return HALResult indicating success or failure
     */
    HALResult<void> add_layer(std::shared_ptr<ConfigLayer> layer);
    
    /**
     * @brief Remove configuration layer
     * @param layer_name Layer name
     * @return HALResult indicating success or failure
     */
    HALResult<void> remove_layer(const std::string& layer_name);
    
    /**
     * @brief Get configuration layer
     * @param layer_name Layer name
     * @return Shared pointer to layer or nullptr if not found
     */
    std::shared_ptr<ConfigLayer> get_layer(const std::string& layer_name);
    
    /**
     * @brief Enable/disable configuration layer
     * @param layer_name Layer name
     * @param enabled Whether layer should be enabled
     * @return HALResult indicating success or failure
     */
    HALResult<void> set_layer_enabled(const std::string& layer_name, bool enabled);
    
    /**
     * @brief Get all layer names
     */
    std::vector<std::string> get_layer_names() const;
    
    /**
     * @brief Get enabled layers ordered by priority
     */
    std::vector<std::shared_ptr<ConfigLayer>> get_ordered_layers() const;
    
    /**
     * @brief Set resolution strategy
     * @param strategy Resolution strategy
     */
    void set_resolution_strategy(ConfigResolutionStrategy strategy);
    
    /**
     * @brief Get resolution strategy
     */
    ConfigResolutionStrategy get_resolution_strategy() const { return resolution_strategy_; }
    
    /**
     * @brief Get type-safe accessor for configuration value
     * @tparam T Configuration value type
     * @param path Configuration path
     * @return Configuration accessor
     */
    template<typename T>
    ConfigAccessor<T> get_accessor(const ConfigPath& path) {
        return ConfigAccessor<T>(*this, path);
    }
    
    /**
     * @brief Get type-safe accessor for configuration value
     * @tparam T Configuration value type
     * @param path Configuration path string
     * @return Configuration accessor
     */
    template<typename T>
    ConfigAccessor<T> get_accessor(const std::string& path) {
        return ConfigAccessor<T>(*this, ConfigPath(path));
    }
    
    /**
     * @brief Resolve configuration value
     * @tparam T Configuration value type
     * @param path Configuration path
     * @return Resolved value or nullopt if not found
     */
    template<typename T>
    std::optional<T> resolve_value(const ConfigPath& path) const;
    
    /**
     * @brief Set configuration value in highest priority layer
     * @tparam T Configuration value type
     * @param path Configuration path
     * @param value Value to set
     * @return HALResult indicating success or failure
     */
    template<typename T>
    HALResult<void> set_value(const ConfigPath& path, const T& value);
    
    /**
     * @brief Set configuration value in specific layer
     * @tparam T Configuration value type
     * @param path Configuration path
     * @param value Value to set
     * @param layer_name Layer name
     * @return HALResult indicating success or failure
     */
    template<typename T>
    HALResult<void> set_value_in_layer(const ConfigPath& path, const T& value, const std::string& layer_name);
    
    /**
     * @brief Resolve complete platform configuration
     * @return Resolved platform configuration
     */
    HALResult<PlatformConfig> resolve_platform_config() const;
    
    /**
     * @brief Load configuration from platform config into layers
     * @param config Platform configuration
     * @param layer_name Target layer name
     * @return HALResult indicating success or failure
     */
    HALResult<void> load_from_platform_config(const PlatformConfig& config, const std::string& layer_name);
    
    /**
     * @brief Create standard configuration layers
     * @return HALResult indicating success or failure
     */
    HALResult<void> create_standard_layers();
    
    /**
     * @brief Validate all layers
     * @return HALResult indicating validity
     */
    HALResult<void> validate() const;
    
    /**
     * @brief Get configuration statistics
     */
    struct HierarchyStats {
        size_t total_layers = 0;
        size_t enabled_layers = 0;
        size_t total_keys = 0;
        std::unordered_map<std::string, size_t> keys_per_layer;
    };
    
    /**
     * @brief Get hierarchy statistics
     */
    HierarchyStats get_stats() const;

private:
    /**
     * @brief Convert configuration path to string key
     */
    std::string path_to_key(const ConfigPath& path) const;
    
    /**
     * @brief Get highest priority layer for writing
     */
    std::shared_ptr<ConfigLayer> get_write_layer();
    
    mutable std::shared_mutex layers_mutex_;
    std::unordered_map<std::string, std::shared_ptr<ConfigLayer>> layers_;
    ConfigResolutionStrategy resolution_strategy_ = ConfigResolutionStrategy::PriorityBased;
};

// Template implementations

template<typename T>
std::optional<T> ConfigResolver::resolve_value(const std::string& key,
                                               const std::vector<std::shared_ptr<ConfigLayer>>& layers,
                                               ConfigResolutionStrategy strategy) {
    switch (strategy) {
        case ConfigResolutionStrategy::PriorityBased:
            return resolve_priority_based<T>(key, layers);
        case ConfigResolutionStrategy::Merge:
            return resolve_merge<T>(key, layers);
        case ConfigResolutionStrategy::FirstFound:
            for (const auto& layer : layers) {
                if (layer->enabled) {
                    auto value = layer->get_value<T>(key);
                    if (value.has_value()) {
                        return value;
                    }
                }
            }
            break;
        case ConfigResolutionStrategy::LastFound:
            for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
                if ((*it)->enabled) {
                    auto value = (*it)->get_value<T>(key);
                    if (value.has_value()) {
                        return value;
                    }
                }
            }
            break;
    }
    return std::nullopt;
}

template<typename T>
std::optional<T> ConfigResolver::resolve_priority_based(const std::string& key,
                                                        const std::vector<std::shared_ptr<ConfigLayer>>& layers) {
    // Layers should be sorted by priority (highest first)
    for (const auto& layer : layers) {
        if (layer->enabled) {
            auto value = layer->get_value<T>(key);
            if (value.has_value()) {
                return value;
            }
        }
    }
    return std::nullopt;
}

template<typename T>
std::optional<T> ConfigResolver::resolve_merge(const std::string& key,
                                              const std::vector<std::shared_ptr<ConfigLayer>>& layers) {
    // For now, merge strategy falls back to priority-based
    // In a full implementation, this would handle merging of compatible types
    return resolve_priority_based<T>(key, layers);
}

template<typename T>
ConfigAccessor<T>::ConfigAccessor(HierarchicalConfig& hierarchy, const ConfigPath& path)
    : hierarchy_(hierarchy), path_(path) {}

template<typename T>
std::optional<T> ConfigAccessor<T>::get() const {
    return hierarchy_.resolve_value<T>(path_);
}

template<typename T>
T ConfigAccessor<T>::get_or(const T& default_value) const {
    auto value = get();
    return value.has_value() ? value.value() : default_value;
}

template<typename T>
HALResult<void> ConfigAccessor<T>::set(const T& value) {
    return hierarchy_.set_value(path_, value);
}

template<typename T>
HALResult<void> ConfigAccessor<T>::set_in_layer(const T& value, const std::string& layer_name) {
    return hierarchy_.set_value_in_layer(path_, value, layer_name);
}

template<typename T>
bool ConfigAccessor<T>::exists() const {
    return get().has_value();
}

template<typename T>
HALResult<void> ConfigAccessor<T>::remove() {
    // Remove from all layers
    auto layer_names = hierarchy_.get_layer_names();
    for (const auto& layer_name : layer_names) {
        remove_from_layer(layer_name);
    }
    return HALResult<void>::success();
}

template<typename T>
HALResult<void> ConfigAccessor<T>::remove_from_layer(const std::string& layer_name) {
    auto layer = hierarchy_.get_layer(layer_name);
    if (layer) {
        std::string key = hierarchy_.path_to_key(path_);
        layer->remove_key(key);
    }
    return HALResult<void>::success();
}

template<typename T>
std::optional<T> HierarchicalConfig::resolve_value(const ConfigPath& path) const {
    std::shared_lock<std::shared_mutex> lock(layers_mutex_);
    auto ordered_layers = get_ordered_layers();
    std::string key = path_to_key(path);
    return ConfigResolver::resolve_value<T>(key, ordered_layers, resolution_strategy_);
}

template<typename T>
HALResult<void> HierarchicalConfig::set_value(const ConfigPath& path, const T& value) {
    auto write_layer = get_write_layer();
    if (!write_layer) {
        return HALResult<void>::error(errors::internal_error(100, "No write layer available"));
    }
    
    std::string key = path_to_key(path);
    write_layer->set_value(key, value);
    return HALResult<void>::success();
}

template<typename T>
HALResult<void> HierarchicalConfig::set_value_in_layer(const ConfigPath& path, const T& value, const std::string& layer_name) {
    auto layer = get_layer(layer_name);
    if (!layer) {
        return HALResult<void>::error(errors::invalid_parameter(101, "Layer not found: " + layer_name));
    }
    
    std::string key = path_to_key(path);
    layer->set_value(key, value);
    return HALResult<void>::success();
}

} // namespace flight::hal
