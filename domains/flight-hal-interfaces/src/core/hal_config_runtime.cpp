/**
 * @file hal_config_runtime.cpp
 * @brief Flight HAL Runtime Configuration Management System Implementation
 */

#include "flight/hal/core/hal_config_runtime.hpp"
#include "flight/hal/core/hal_error.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace flight::hal {

// ConfigFileWatcher implementation
ConfigFileWatcher::ConfigFileWatcher() = default;

ConfigFileWatcher::~ConfigFileWatcher() {
    stop();
}

HALResult<void> ConfigFileWatcher::watch_file(const std::string& file_path, FileChangeCallback callback) {
    std::lock_guard<std::mutex> lock(watched_files_mutex_);
    
    if (watched_files_.find(file_path) != watched_files_.end()) {
        return HALResult<void>::error(errors::invalid_parameter(200, ("File already being watched: " + file_path).c_str()));
    }
    
    auto watched_file = std::make_unique<WatchedFile>();
    watched_file->path = file_path;
    watched_file->callback = callback;
    
    // Check if file exists and get initial timestamp
    std::filesystem::path fs_path(file_path);
    if (std::filesystem::exists(fs_path)) {
        watched_file->exists = true;
        watched_file->last_write_time = std::filesystem::last_write_time(fs_path);
    } else {
        watched_file->exists = false;
    }
    
    watched_files_[file_path] = std::move(watched_file);
    return HALResult<void>::success();
}

HALResult<void> ConfigFileWatcher::unwatch_file(const std::string& file_path) {
    std::lock_guard<std::mutex> lock(watched_files_mutex_);
    
    auto it = watched_files_.find(file_path);
    if (it == watched_files_.end()) {
        return HALResult<void>::error(errors::invalid_parameter(201, ("File not being watched: " + file_path).c_str()));
    }
    
    watched_files_.erase(it);
    return HALResult<void>::success();
}

HALResult<void> ConfigFileWatcher::start() {
    if (running_.load()) {
        return HALResult<void>::error(errors::internal_error(202, "File watcher already running"));
    }
    
    running_.store(true);
    watch_thread_ = std::thread(&ConfigFileWatcher::watch_thread, this);
    return HALResult<void>::success();
}

void ConfigFileWatcher::stop() {
    if (running_.load()) {
        running_.store(false);
        if (watch_thread_.joinable()) {
            watch_thread_.join();
        }
    }
}

void ConfigFileWatcher::watch_thread() {
    while (running_.load()) {
        check_file_changes();
        std::this_thread::sleep_for(check_interval_);
    }
}

void ConfigFileWatcher::check_file_changes() {
    std::lock_guard<std::mutex> lock(watched_files_mutex_);
    
    for (auto& [path, watched_file] : watched_files_) {
        std::filesystem::path fs_path(path);
        bool file_exists = std::filesystem::exists(fs_path);
        
        if (!watched_file->exists && file_exists) {
            // File was created
            watched_file->exists = true;
            watched_file->last_write_time = std::filesystem::last_write_time(fs_path);
            watched_file->callback(path);
        } else if (watched_file->exists && !file_exists) {
            // File was deleted
            watched_file->exists = false;
            watched_file->callback(path);
        } else if (watched_file->exists && file_exists) {
            // Check if file was modified
            auto current_write_time = std::filesystem::last_write_time(fs_path);
            if (current_write_time != watched_file->last_write_time) {
                watched_file->last_write_time = current_write_time;
                watched_file->callback(path);
            }
        }
    }
}

// RuntimeConfigManager implementation
RuntimeConfigManager::RuntimeConfigManager(const PlatformConfig& initial_config)
    : current_config_(initial_config), file_watcher_(std::make_unique<ConfigFileWatcher>()) {
    
    // Create initial rollback point
    create_rollback_point("Initial configuration", "system");
}

RuntimeConfigManager::~RuntimeConfigManager() {
    disable_hot_reload();
}

PlatformConfig RuntimeConfigManager::get_config() const {
    std::shared_lock<std::shared_mutex> lock(config_mutex_);
    return current_config_;
}

HALResult<void> RuntimeConfigManager::update_config(const PlatformConfig& new_config,
                                                   const ConfigValidationContext& context,
                                                   bool create_rollback_point) {
    // Validate new configuration
    auto validation_result = validate_config(new_config, context);
    if (!validation_result.is_success()) {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.failed_updates++;
        stats_.total_updates++;
        return validation_result;
    }
    
    ConfigChangeEvent event(ConfigChangeType::Updated, "platform", "", "api");
    return apply_config_change(new_config, event, context, create_rollback_point);
}

HALResult<void> RuntimeConfigManager::merge_config_updates(const PlatformConfig& config_updates,
                                                          const ConfigValidationContext& context) {
    // Get current config and merge with updates
    auto current_config = get_config();
    auto merged_config = PlatformConfigManager::merge_configs(current_config, config_updates);
    
    // Validate merged configuration
    auto validation_result = validate_config(merged_config, context);
    if (!validation_result.is_success()) {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.failed_updates++;
        stats_.total_updates++;
        return validation_result;
    }
    
    ConfigChangeEvent event(ConfigChangeType::Updated, "platform", "", "merge");
    return apply_config_change(merged_config, event, context, true);
}

HALResult<void> RuntimeConfigManager::enable_hot_reload(const std::string& config_file_path) {
    if (hot_reload_enabled_.load()) {
        return HALResult<void>::error(errors::internal_error(203, "Hot reload already enabled"));
    }
    
    // Start file watcher if not already running
    if (!file_watcher_->is_running()) {
        auto start_result = file_watcher_->start();
        if (!start_result.is_success()) {
            return start_result;
        }
    }
    
    // Add file to watch list
    auto watch_result = file_watcher_->watch_file(config_file_path,
        [this](const std::string& file_path) {
            this->on_config_file_changed(file_path);
        });
    
    if (!watch_result.is_success()) {
        return watch_result;
    }
    
    watched_config_file_ = config_file_path;
    hot_reload_enabled_.store(true);
    
    return HALResult<void>::success();
}

void RuntimeConfigManager::disable_hot_reload() {
    if (hot_reload_enabled_.load()) {
        if (!watched_config_file_.empty()) {
            file_watcher_->unwatch_file(watched_config_file_);
            watched_config_file_.clear();
        }
        hot_reload_enabled_.store(false);
    }
}

HALResult<void> RuntimeConfigManager::add_listener(std::shared_ptr<IConfigChangeListener> listener) {
    if (!listener) {
        return HALResult<void>::error(errors::invalid_parameter(204, "Listener cannot be null"));
    }
    
    std::lock_guard<std::mutex> lock(listeners_mutex_);
    listeners_.push_back(listener);
    
    // Sort listeners by priority (highest first)
    std::sort(listeners_.begin(), listeners_.end(),
        [](const std::weak_ptr<IConfigChangeListener>& a, const std::weak_ptr<IConfigChangeListener>& b) {
            auto a_locked = a.lock();
            auto b_locked = b.lock();
            if (!a_locked) return false;
            if (!b_locked) return true;
            return a_locked->get_priority() > b_locked->get_priority();
        });
    
    return HALResult<void>::success();
}

HALResult<void> RuntimeConfigManager::remove_listener(std::shared_ptr<IConfigChangeListener> listener) {
    if (!listener) {
        return HALResult<void>::error(errors::invalid_parameter(205, "Listener cannot be null"));
    }
    
    std::lock_guard<std::mutex> lock(listeners_mutex_);
    
    auto it = std::find_if(listeners_.begin(), listeners_.end(),
        [&listener](const std::weak_ptr<IConfigChangeListener>& weak_listener) {
            auto locked = weak_listener.lock();
            return locked && locked.get() == listener.get();
        });
    
    if (it != listeners_.end()) {
        listeners_.erase(it);
        return HALResult<void>::success();
    }
    
    return HALResult<void>::error(errors::invalid_parameter(206, "Listener not found"));
}

HALResult<void> RuntimeConfigManager::create_rollback_point(const std::string& description, const std::string& source) {
    std::lock_guard<std::mutex> lock(rollback_mutex_);
    
    auto current_config = get_config();
    rollback_points_.emplace_back(current_config, description, source);
    
    // Keep only the most recent rollback points
    if (rollback_points_.size() > max_rollback_points_) {
        rollback_points_.erase(rollback_points_.begin());
    }
    
    return HALResult<void>::success();
}

HALResult<void> RuntimeConfigManager::rollback(const ConfigValidationContext& context) {
    return rollback_to(0, context);
}

HALResult<void> RuntimeConfigManager::rollback_to(size_t index, const ConfigValidationContext& context) {
    std::lock_guard<std::mutex> rollback_lock(rollback_mutex_);
    
    if (rollback_points_.empty()) {
        return HALResult<void>::error(errors::internal_error(207, "No rollback points available"));
    }
    
    if (index >= rollback_points_.size()) {
        return HALResult<void>::error(errors::parameter_out_of_range(208, "Invalid rollback point index"));
    }
    
    // Get rollback configuration (most recent is at end of vector)
    const auto& rollback_point = rollback_points_[rollback_points_.size() - 1 - index];
    
    // Validate rollback configuration
    auto validation_result = validate_config(rollback_point.config, context);
    if (!validation_result.is_success()) {
        return validation_result;
    }
    
    ConfigChangeEvent event(ConfigChangeType::Reloaded, "platform", "", "rollback");
    auto result = apply_config_change(rollback_point.config, event, context, false);
    
    if (result.is_success()) {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.rollbacks++;
    }
    
    return result;
}

size_t RuntimeConfigManager::get_rollback_point_count() const {
    std::lock_guard<std::mutex> lock(rollback_mutex_);
    return rollback_points_.size();
}

const ConfigRollbackPoint* RuntimeConfigManager::get_rollback_point_info(size_t index) const {
    std::lock_guard<std::mutex> lock(rollback_mutex_);
    
    if (index >= rollback_points_.size()) {
        return nullptr;
    }
    
    // Most recent is at end of vector
    return &rollback_points_[rollback_points_.size() - 1 - index];
}

void RuntimeConfigManager::cleanup_rollback_points(size_t max_points) {
    std::lock_guard<std::mutex> lock(rollback_mutex_);
    
    max_rollback_points_ = max_points;
    
    if (rollback_points_.size() > max_points) {
        size_t points_to_remove = rollback_points_.size() - max_points;
        rollback_points_.erase(rollback_points_.begin(), rollback_points_.begin() + points_to_remove);
    }
}

HALResult<void> RuntimeConfigManager::validate_config(const PlatformConfig& config,
                                                     const ConfigValidationContext& context) {
    // Perform basic validation
    auto basic_validation = config.validate();
    if (!basic_validation.is_success()) {
        return basic_validation;
    }
    
    // Platform-specific validation if context provided
    if (!context.platform_name.empty()) {
        return PlatformConfigManager::validate_for_platform(config, context.platform_name);
    }
    
    return HALResult<void>::success();
}

RuntimeConfigManager::ConfigStats RuntimeConfigManager::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

HALResult<void> RuntimeConfigManager::notify_listeners(const ConfigChangeEvent& event,
                                                      const PlatformConfig* old_config,
                                                      const PlatformConfig* new_config) {
    std::lock_guard<std::mutex> lock(listeners_mutex_);
    
    // Clean up expired listeners
    listeners_.erase(
        std::remove_if(listeners_.begin(), listeners_.end(),
            [](const std::weak_ptr<IConfigChangeListener>& weak_listener) {
                return weak_listener.expired();
            }),
        listeners_.end());
    
    // Notify all valid listeners
    for (const auto& weak_listener : listeners_) {
        auto listener = weak_listener.lock();
        if (listener) {
            auto result = listener->on_config_changed(event, old_config, new_config);
            // Log listener failures but don't fail the entire notification process
            if (!result.is_success()) {
                // In a real implementation, we would log this error
            }
        }
    }
    
    return HALResult<void>::success();
}

void RuntimeConfigManager::on_config_file_changed(const std::string& file_path) {
    // Load new configuration from file
    auto load_result = PlatformConfigManager::load_from_file(file_path);
    if (!load_result.is_success()) {
        // Log error but don't crash
        return;
    }
    
    // Validate and apply new configuration
    ConfigValidationContext context; // Use default context for hot reload
    ConfigChangeEvent event(ConfigChangeType::Reloaded, "platform", "", "hot_reload");
    
    auto apply_result = apply_config_change(load_result.value(), event, context, true);
    if (apply_result.is_success()) {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.hot_reloads++;
    }
}

HALResult<void> RuntimeConfigManager::apply_config_change(const PlatformConfig& new_config,
                                                         const ConfigChangeEvent& event,
                                                         const ConfigValidationContext& context,
                                                         bool create_rollback_point) {
    // Create rollback point before making changes
    if (create_rollback_point) {
        this->create_rollback_point("Before " + event.source + " update", event.source);
    }
    
    // Get old configuration for listeners
    PlatformConfig old_config;
    {
        std::shared_lock<std::shared_mutex> read_lock(config_mutex_);
        old_config = current_config_;
    }
    
    // Apply new configuration
    {
        std::unique_lock<std::shared_mutex> write_lock(config_mutex_);
        current_config_ = new_config;
    }
    
    // Notify listeners
    auto notify_result = notify_listeners(event, &old_config, &new_config);
    
    // Update statistics
    {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.total_updates++;
        stats_.successful_updates++;
        stats_.last_update = std::chrono::system_clock::now();
        stats_.last_successful_update = stats_.last_update;
    }
    
    return HALResult<void>::success();
}

// ScopedConfigUpdate implementation
ScopedConfigUpdate::ScopedConfigUpdate(RuntimeConfigManager& manager, const std::string& description)
    : manager_(manager), description_(description) {
    manager_.create_rollback_point("Scoped update: " + description_, "scoped");
}

ScopedConfigUpdate::~ScopedConfigUpdate() {
    if (has_updates_ && !committed_) {
        // Automatically rollback on destruction if not committed
        rollback();
    }
}

HALResult<void> ScopedConfigUpdate::update(const PlatformConfig& config, const ConfigValidationContext& context) {
    auto result = manager_.update_config(config, context, false); // Don't create additional rollback point
    if (result.is_success()) {
        has_updates_ = true;
    }
    return result;
}

void ScopedConfigUpdate::commit() {
    committed_ = true;
}

HALResult<void> ScopedConfigUpdate::rollback() {
    if (has_updates_) {
        auto result = manager_.rollback();
        if (result.is_success()) {
            has_updates_ = false;
            committed_ = true; // Prevent double rollback
        }
        return result;
    }
    return HALResult<void>::success();
}

} // namespace flight::hal
