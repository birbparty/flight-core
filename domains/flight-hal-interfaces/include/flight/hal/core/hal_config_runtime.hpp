/**
 * @file hal_config_runtime.hpp
 * @brief Flight HAL Runtime Configuration Management System
 * 
 * Provides hot reloading, change notification, validation, and rollback
 * capabilities for dynamic configuration updates without restart.
 */

#pragma once

#include "platform_config.hpp"
#include "hal_result.hpp"
#include "hal_logging.hpp"
#include <functional>
#include <memory>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <chrono>
#include <filesystem>

namespace flight::hal {

/**
 * @brief Configuration change event types
 */
enum class ConfigChangeType {
    Updated,    ///< Configuration value updated
    Added,      ///< New configuration value added
    Removed,    ///< Configuration value removed
    Reloaded    ///< Entire configuration reloaded
};

/**
 * @brief Configuration change event information
 */
struct ConfigChangeEvent {
    ConfigChangeType type;
    std::string section;        ///< Configuration section (e.g., "memory_budget", "performance")
    std::string key;            ///< Specific configuration key (empty for section-level changes)
    std::chrono::system_clock::time_point timestamp;
    std::string source;         ///< Source of change (file, API, environment, etc.)
    
    ConfigChangeEvent(ConfigChangeType t, const std::string& sec, 
                     const std::string& k = "", const std::string& src = "unknown")
        : type(t), section(sec), key(k), timestamp(std::chrono::system_clock::now()), source(src) {}
};

/**
 * @brief Configuration change listener interface
 */
class IConfigChangeListener {
public:
    virtual ~IConfigChangeListener() = default;
    
    /**
     * @brief Called when configuration changes
     * @param event Configuration change event details
     * @param old_config Previous configuration (nullptr for additions)
     * @param new_config New configuration (nullptr for removals)
     * @return HALResult indicating if listener handled the change successfully
     */
    virtual HALResult<void> on_config_changed(const ConfigChangeEvent& event,
                                             const PlatformConfig* old_config,
                                             const PlatformConfig* new_config) = 0;
    
    /**
     * @brief Get listener priority (higher values processed first)
     */
    virtual int get_priority() const { return 0; }
    
    /**
     * @brief Get listener name for debugging
     */
    virtual std::string get_name() const = 0;
};

/**
 * @brief File system watcher for configuration hot reloading
 */
class ConfigFileWatcher {
public:
    /**
     * @brief File change callback type
     */
    using FileChangeCallback = std::function<void(const std::string& file_path)>;
    
    /**
     * @brief Constructor
     */
    ConfigFileWatcher();
    
    /**
     * @brief Destructor
     */
    ~ConfigFileWatcher();
    
    /**
     * @brief Start watching a file for changes
     * @param file_path Path to file to watch
     * @param callback Callback to invoke on file changes
     * @return HALResult indicating success or failure
     */
    HALResult<void> watch_file(const std::string& file_path, FileChangeCallback callback);
    
    /**
     * @brief Stop watching a file
     * @param file_path Path to file to stop watching
     * @return HALResult indicating success or failure
     */
    HALResult<void> unwatch_file(const std::string& file_path);
    
    /**
     * @brief Start the file watcher thread
     */
    HALResult<void> start();
    
    /**
     * @brief Stop the file watcher thread
     */
    void stop();
    
    /**
     * @brief Check if watcher is running
     */
    bool is_running() const { return running_.load(); }

private:
    struct WatchedFile {
        std::string path;
        FileChangeCallback callback;
        std::filesystem::file_time_type last_write_time;
        bool exists;
    };
    
    void watch_thread();
    void check_file_changes();
    
    std::atomic<bool> running_{false};
    std::thread watch_thread_;
    std::mutex watched_files_mutex_;
    std::unordered_map<std::string, std::unique_ptr<WatchedFile>> watched_files_;
    std::chrono::milliseconds check_interval_{500}; ///< File check interval
};

/**
 * @brief Configuration validation context
 */
struct ConfigValidationContext {
    std::string platform_name;
    std::vector<std::string> available_drivers;
    std::unordered_map<std::string, std::string> environment_variables;
    bool strict_validation = true;
    
    ConfigValidationContext() = default;
    ConfigValidationContext(const std::string& platform) : platform_name(platform) {}
};

/**
 * @brief Configuration rollback information
 */
struct ConfigRollbackPoint {
    PlatformConfig config;
    std::chrono::system_clock::time_point timestamp;
    std::string description;
    std::string source;
    
    ConfigRollbackPoint(const PlatformConfig& cfg, const std::string& desc, const std::string& src)
        : config(cfg), timestamp(std::chrono::system_clock::now()), description(desc), source(src) {}
};

/**
 * @brief Runtime configuration manager with hot reloading and change notification
 */
class RuntimeConfigManager {
public:
    /**
     * @brief Constructor
     * @param initial_config Initial configuration
     */
    explicit RuntimeConfigManager(const PlatformConfig& initial_config = PlatformConfig::create_default());
    
    /**
     * @brief Destructor
     */
    ~RuntimeConfigManager();
    
    /**
     * @brief Get current configuration (thread-safe)
     * @return Current platform configuration
     */
    PlatformConfig get_config() const;
    
    /**
     * @brief Update configuration with validation and rollback
     * @param new_config New configuration to apply
     * @param context Validation context
     * @param create_rollback_point Whether to create rollback point
     * @return HALResult indicating success or failure
     */
    HALResult<void> update_config(const PlatformConfig& new_config,
                                 const ConfigValidationContext& context = {},
                                 bool create_rollback_point = true);
    
    /**
     * @brief Partially update configuration (merge with current)
     * @param config_updates Configuration updates to merge
     * @param context Validation context
     * @return HALResult indicating success or failure
     */
    HALResult<void> merge_config_updates(const PlatformConfig& config_updates,
                                        const ConfigValidationContext& context = {});
    
    /**
     * @brief Enable hot reloading from file
     * @param config_file_path Path to configuration file
     * @return HALResult indicating success or failure
     */
    HALResult<void> enable_hot_reload(const std::string& config_file_path);
    
    /**
     * @brief Disable hot reloading
     */
    void disable_hot_reload();
    
    /**
     * @brief Check if hot reloading is enabled
     */
    bool is_hot_reload_enabled() const { return hot_reload_enabled_.load(); }
    
    /**
     * @brief Add configuration change listener
     * @param listener Shared pointer to listener
     * @return HALResult indicating success or failure
     */
    HALResult<void> add_listener(std::shared_ptr<IConfigChangeListener> listener);
    
    /**
     * @brief Remove configuration change listener
     * @param listener Shared pointer to listener to remove
     * @return HALResult indicating success or failure
     */
    HALResult<void> remove_listener(std::shared_ptr<IConfigChangeListener> listener);
    
    /**
     * @brief Create rollback point
     * @param description Description of the rollback point
     * @param source Source identifier
     * @return HALResult indicating success or failure
     */
    HALResult<void> create_rollback_point(const std::string& description, const std::string& source = "manual");
    
    /**
     * @brief Rollback to previous configuration
     * @param context Validation context for rollback
     * @return HALResult indicating success or failure
     */
    HALResult<void> rollback(const ConfigValidationContext& context = {});
    
    /**
     * @brief Rollback to specific rollback point
     * @param index Index of rollback point (0 = most recent)
     * @param context Validation context
     * @return HALResult indicating success or failure
     */
    HALResult<void> rollback_to(size_t index, const ConfigValidationContext& context = {});
    
    /**
     * @brief Get number of available rollback points
     */
    size_t get_rollback_point_count() const;
    
    /**
     * @brief Get rollback point information
     * @param index Index of rollback point
     * @return Rollback point information or nullptr if invalid index
     */
    const ConfigRollbackPoint* get_rollback_point_info(size_t index) const;
    
    /**
     * @brief Clear old rollback points
     * @param max_points Maximum number of rollback points to keep
     */
    void cleanup_rollback_points(size_t max_points = 10);
    
    /**
     * @brief Validate configuration
     * @param config Configuration to validate
     * @param context Validation context
     * @return HALResult indicating validity
     */
    static HALResult<void> validate_config(const PlatformConfig& config,
                                          const ConfigValidationContext& context = {});
    
    /**
     * @brief Get configuration statistics
     */
    struct ConfigStats {
        size_t total_updates = 0;
        size_t successful_updates = 0;
        size_t failed_updates = 0;
        size_t rollbacks = 0;
        size_t hot_reloads = 0;
        std::chrono::system_clock::time_point last_update;
        std::chrono::system_clock::time_point last_successful_update;
    };
    
    /**
     * @brief Get configuration management statistics
     */
    ConfigStats get_stats() const;

private:
    /**
     * @brief Notify all listeners of configuration change
     */
    HALResult<void> notify_listeners(const ConfigChangeEvent& event,
                                    const PlatformConfig* old_config,
                                    const PlatformConfig* new_config);
    
    /**
     * @brief Handle file change from watcher
     */
    void on_config_file_changed(const std::string& file_path);
    
    /**
     * @brief Apply configuration change with proper synchronization
     */
    HALResult<void> apply_config_change(const PlatformConfig& new_config,
                                       const ConfigChangeEvent& event,
                                       const ConfigValidationContext& context,
                                       bool create_rollback_point);
    
    mutable std::shared_mutex config_mutex_;
    PlatformConfig current_config_;
    
    std::mutex listeners_mutex_;
    std::vector<std::weak_ptr<IConfigChangeListener>> listeners_;
    
    std::atomic<bool> hot_reload_enabled_{false};
    std::unique_ptr<ConfigFileWatcher> file_watcher_;
    std::string watched_config_file_;
    
    mutable std::mutex rollback_mutex_;
    std::vector<ConfigRollbackPoint> rollback_points_;
    size_t max_rollback_points_ = 10;
    
    mutable std::mutex stats_mutex_;
    ConfigStats stats_;
};

/**
 * @brief Helper class for scoped configuration updates
 */
class ScopedConfigUpdate {
public:
    /**
     * @brief Constructor - creates rollback point
     * @param manager Configuration manager
     * @param description Description for rollback point
     */
    ScopedConfigUpdate(RuntimeConfigManager& manager, const std::string& description);
    
    /**
     * @brief Destructor - commits or rolls back based on status
     */
    ~ScopedConfigUpdate();
    
    /**
     * @brief Apply configuration update
     * @param config New configuration
     * @param context Validation context
     * @return HALResult indicating success or failure
     */
    HALResult<void> update(const PlatformConfig& config, const ConfigValidationContext& context = {});
    
    /**
     * @brief Commit the changes (prevent automatic rollback)
     */
    void commit();
    
    /**
     * @brief Explicitly rollback the changes
     */
    HALResult<void> rollback();

private:
    RuntimeConfigManager& manager_;
    std::string description_;
    bool committed_ = false;
    bool has_updates_ = false;
};

} // namespace flight::hal
