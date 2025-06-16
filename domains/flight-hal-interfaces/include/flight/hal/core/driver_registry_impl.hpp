/**
 * @file driver_registry_impl.hpp
 * @brief Flight HAL Driver Registry Template Implementation
 * 
 * Enhanced template implementation supporting multiple drivers per interface,
 * capability-based selection, and fallback management.
 */

#pragma once

#include <algorithm>
#include <mutex>
#include <vector>
#include <string>
#include <typeindex>
#include <memory>

namespace flight::hal {

template<typename InterfaceType>
std::string DriverRegistry::get_interface_key() const {
    return std::type_index(typeid(InterfaceType)).name();
}

template<typename InterfaceType>
bool DriverRegistry::register_driver(std::unique_ptr<InterfaceType> driver, int priority) {
    static_assert(std::is_base_of_v<IHALInterface, InterfaceType>, 
                  "InterfaceType must inherit from IHALInterface");
    
    if (!driver) {
        return false;
    }
    
    // Create DriverInfo from driver properties
    DriverInfo info;
    info.name = driver->get_driver_name();
    info.version = Version{}; // Default version
    info.priority = priority;
    info.performance_tier = driver->get_performance_tier();
    info.capabilities = driver->get_capability_mask();
    info.supported_platforms = 0xFFFFFFFF; // Support all platforms by default
    
    std::unique_lock<std::shared_mutex> lock(registry_mutex_);
    
    std::string type_key = get_interface_key<InterfaceType>();
    
    // Create shared pointer from unique pointer
    auto shared_driver = std::shared_ptr<IHALInterface>(driver.release());
    
    // Create driver entry
    DriverEntry entry(shared_driver, info);
    
    // Add to driver list for this interface type
    drivers_[type_key].emplace_back(std::move(entry));
    
    // Sort drivers by priority (highest first)
    std::sort(drivers_[type_key].begin(), drivers_[type_key].end(),
              [](const DriverEntry& a, const DriverEntry& b) {
                  return a.info.priority > b.info.priority;
              });
    
    // Clear cache for this interface type
    driver_cache_.erase(type_key);
    
    return true;
}

template<typename InterfaceType>
HALResult<void> DriverRegistry::register_driver_with_info(const DriverInfo& info) {
    static_assert(std::is_base_of_v<IHALInterface, InterfaceType>, 
                  "InterfaceType must inherit from IHALInterface");
    
    if (!info.factory) {
        return HALResult<void>::error(errors::invalid_parameter(1, "DriverInfo must contain a factory function"));
    }
    
    std::unique_lock<std::shared_mutex> lock(registry_mutex_);
    
    std::string type_key = get_interface_key<InterfaceType>();
    
    // Create factory-based driver entry
    DriverEntry entry(info.factory, info);
    
    drivers_[type_key].emplace_back(std::move(entry));
    
    // Sort by priority
    std::sort(drivers_[type_key].begin(), drivers_[type_key].end(),
              [](const DriverEntry& a, const DriverEntry& b) {
                  return a.info.priority > b.info.priority;
              });
    
    // Clear cache
    driver_cache_.erase(type_key);
    
    return HALResult<void>::success();
}

template<typename InterfaceType>
HALResult<void> DriverRegistry::register_driver_factory(
    std::function<std::unique_ptr<InterfaceType>()> factory,
    const DriverInfo& info) {
    
    static_assert(std::is_base_of_v<IHALInterface, InterfaceType>, 
                  "InterfaceType must inherit from IHALInterface");
    
    if (!factory) {
        return HALResult<void>::error(errors::invalid_parameter(2, "Factory function cannot be null"));
    }
    
    // Wrap typed factory in generic factory
    auto generic_factory = [factory]() -> std::unique_ptr<IHALInterface> {
        return std::unique_ptr<IHALInterface>(factory());
    };
    
    DriverInfo info_copy = info;
    info_copy.factory = generic_factory;
    
    return register_driver_with_info<InterfaceType>(info_copy);
}

template<typename InterfaceType>
bool DriverRegistry::unregister_driver(std::string_view driver_name) {
    static_assert(std::is_base_of_v<IHALInterface, InterfaceType>, 
                  "InterfaceType must inherit from IHALInterface");
    
    std::unique_lock<std::shared_mutex> lock(registry_mutex_);
    
    std::string type_key = get_interface_key<InterfaceType>();
    auto it = drivers_.find(type_key);
    
    if (it != drivers_.end()) {
        auto& driver_list = it->second;
        auto driver_it = std::find_if(driver_list.begin(), driver_list.end(),
            [driver_name](const DriverEntry& entry) {
                return entry.info.name == driver_name;
            });
        
        if (driver_it != driver_list.end()) {
            // Shutdown driver before removing
            if (driver_it->driver && driver_it->driver->is_active()) {
                driver_it->driver->shutdown();
            }
            driver_list.erase(driver_it);
            
            // Clear cache
            driver_cache_.erase(type_key);
            return true;
        }
    }
    
    return false;
}

template<typename InterfaceType>
size_t DriverRegistry::unregister_all_drivers() {
    static_assert(std::is_base_of_v<IHALInterface, InterfaceType>, 
                  "InterfaceType must inherit from IHALInterface");
    
    std::unique_lock<std::shared_mutex> lock(registry_mutex_);
    
    std::string type_key = get_interface_key<InterfaceType>();
    auto it = drivers_.find(type_key);
    
    if (it != drivers_.end()) {
        size_t count = it->second.size();
        
        // Shutdown all drivers
        for (auto& entry : it->second) {
            if (entry.driver && entry.driver->is_active()) {
                entry.driver->shutdown();
            }
        }
        
        drivers_.erase(it);
        driver_cache_.erase(type_key);
        return count;
    }
    
    return 0;
}

template<typename InterfaceType>
std::shared_ptr<InterfaceType> DriverRegistry::get_interface() {
    static_assert(std::is_base_of_v<IHALInterface, InterfaceType>, 
                  "InterfaceType must inherit from IHALInterface");
    
    std::string type_key = get_interface_key<InterfaceType>();
    
    // Check cache first
    {
        std::shared_lock<std::shared_mutex> lock(registry_mutex_);
        auto cache_it = driver_cache_.find(type_key);
        if (cache_it != driver_cache_.end()) {
            if (auto cached = cache_it->second.lock()) {
                cache_hits_++;
                return std::static_pointer_cast<InterfaceType>(cached);
            } else {
                // Cached driver expired, remove from cache
                driver_cache_.erase(cache_it);
            }
        }
    }
    
    selection_count_++;
    
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    
    auto it = drivers_.find(type_key);
    if (it == drivers_.end() || it->second.empty()) {
        return nullptr;
    }
    
    // Get driver candidates
    std::vector<DriverEntry*> candidates;
    for (auto& entry : it->second) {
        candidates.push_back(&entry);
    }
    
    return select_best_driver<InterfaceType>(candidates);
}

template<typename InterfaceType>
std::shared_ptr<InterfaceType> DriverRegistry::get_interface_with_requirements(
    const CapabilityRequirements& requirements) {
    
    static_assert(std::is_base_of_v<IHALInterface, InterfaceType>, 
                  "InterfaceType must inherit from IHALInterface");
    
    selection_count_++;
    
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    
    std::string type_key = get_interface_key<InterfaceType>();
    auto it = drivers_.find(type_key);
    
    if (it == drivers_.end() || it->second.empty()) {
        return nullptr;
    }
    
    // Filter candidates by requirements
    std::vector<DriverEntry*> candidates;
    for (auto& entry : it->second) {
        if (matches_requirements(entry, requirements)) {
            candidates.push_back(&entry);
        }
    }
    
    if (candidates.empty()) {
        return nullptr;
    }
    
    return select_best_driver<InterfaceType>(candidates, &requirements);
}

template<typename InterfaceType>
std::shared_ptr<InterfaceType> DriverRegistry::get_interface_by_name(std::string_view driver_name) {
    static_assert(std::is_base_of_v<IHALInterface, InterfaceType>, 
                  "InterfaceType must inherit from IHALInterface");
    
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    
    std::string type_key = get_interface_key<InterfaceType>();
    auto it = drivers_.find(type_key);
    
    if (it != drivers_.end()) {
        for (auto& entry : it->second) {
            if (entry.info.name == driver_name) {
                // Ensure driver is created if factory-based
                auto driver = ensure_driver_created(const_cast<DriverEntry&>(entry));
                return std::static_pointer_cast<InterfaceType>(driver);
            }
        }
    }
    
    return nullptr;
}

template<typename InterfaceType>
std::vector<std::shared_ptr<InterfaceType>> DriverRegistry::get_all_interfaces() {
    static_assert(std::is_base_of_v<IHALInterface, InterfaceType>, 
                  "InterfaceType must inherit from IHALInterface");
    
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    
    std::vector<std::shared_ptr<InterfaceType>> result;
    
    std::string type_key = get_interface_key<InterfaceType>();
    auto it = drivers_.find(type_key);
    
    if (it != drivers_.end()) {
        result.reserve(it->second.size());
        
        for (auto& entry : it->second) {
            auto driver = ensure_driver_created(const_cast<DriverEntry&>(entry));
            if (driver) {
                result.push_back(std::static_pointer_cast<InterfaceType>(driver));
            }
        }
    }
    
    return result;
}

template<typename InterfaceType>
bool DriverRegistry::has_interface() const {
    static_assert(std::is_base_of_v<IHALInterface, InterfaceType>, 
                  "InterfaceType must inherit from IHALInterface");
    
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    
    std::string type_key = get_interface_key<InterfaceType>();
    auto it = drivers_.find(type_key);
    
    return it != drivers_.end() && !it->second.empty();
}

template<typename InterfaceType>
bool DriverRegistry::has_interface(std::string_view driver_name) const {
    static_assert(std::is_base_of_v<IHALInterface, InterfaceType>, 
                  "InterfaceType must inherit from IHALInterface");
    
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    
    std::string type_key = get_interface_key<InterfaceType>();
    auto it = drivers_.find(type_key);
    
    if (it != drivers_.end()) {
        return std::any_of(it->second.begin(), it->second.end(),
            [driver_name](const DriverEntry& entry) {
                return entry.info.name == driver_name;
            });
    }
    
    return false;
}

template<typename InterfaceType>
std::vector<DriverInfo> DriverRegistry::get_driver_info() const {
    static_assert(std::is_base_of_v<IHALInterface, InterfaceType>, 
                  "InterfaceType must inherit from IHALInterface");
    
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    
    std::vector<DriverInfo> result;
    
    std::string type_key = get_interface_key<InterfaceType>();
    auto it = drivers_.find(type_key);
    
    if (it != drivers_.end()) {
        result.reserve(it->second.size());
        
        for (const auto& entry : it->second) {
            result.push_back(entry.info);
        }
    }
    
    return result;
}

template<typename InterfaceType>
HALResult<void> DriverRegistry::initialize_interface() {
    static_assert(std::is_base_of_v<IHALInterface, InterfaceType>, 
                  "InterfaceType must inherit from IHALInterface");
    
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    
    std::string type_key = get_interface_key<InterfaceType>();
    auto it = drivers_.find(type_key);
    
    if (it == drivers_.end()) {
        return HALResult<void>::success(); // No drivers to initialize
    }
    
    std::vector<std::string> failed_drivers;
    
    for (auto& entry : it->second) {
        auto driver = ensure_driver_created(const_cast<DriverEntry&>(entry));
        if (driver && !driver->is_active()) {
            auto result = driver->initialize();
            if (!result) {
                failed_drivers.push_back(std::string(entry.info.name));
            }
        }
    }
    
    if (!failed_drivers.empty()) {
        std::string error_msg = "Failed to initialize drivers: ";
        for (size_t i = 0; i < failed_drivers.size(); ++i) {
            if (i > 0) error_msg += ", ";
            error_msg += failed_drivers[i];
        }
        return HALResult<void>::error(errors::internal_error(1, error_msg.c_str()));
    }
    
    return HALResult<void>::success();
}

template<typename InterfaceType>
void DriverRegistry::shutdown_interface() {
    static_assert(std::is_base_of_v<IHALInterface, InterfaceType>, 
                  "InterfaceType must inherit from IHALInterface");
    
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    
    std::string type_key = get_interface_key<InterfaceType>();
    auto it = drivers_.find(type_key);
    
    if (it != drivers_.end()) {
        for (auto& entry : it->second) {
            if (entry.driver && entry.driver->is_active()) {
                entry.driver->shutdown();
            }
        }
    }
    
    // Clear cache for this interface
    driver_cache_.erase(type_key);
}

template<typename InterfaceType>
size_t DriverRegistry::interface_driver_count() const {
    static_assert(std::is_base_of_v<IHALInterface, InterfaceType>, 
                  "InterfaceType must inherit from IHALInterface");
    
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    
    std::string type_key = get_interface_key<InterfaceType>();
    auto it = drivers_.find(type_key);
    
    return it != drivers_.end() ? it->second.size() : 0;
}

template<typename InterfaceType>
std::shared_ptr<InterfaceType> DriverRegistry::select_best_driver(
    const std::vector<DriverEntry*>& candidates,
    const CapabilityRequirements* requirements) {
    
    if (candidates.empty()) {
        return nullptr;
    }
    
    // Create mutable copy for ranking
    std::vector<DriverEntry*> ranked_candidates = candidates;
    rank_drivers(ranked_candidates, requirements);
    
    // Try each driver in order until one succeeds
    for (auto* entry : ranked_candidates) {
        auto driver = ensure_driver_created(*entry);
        if (driver && driver->is_available()) {
            auto typed_driver = std::static_pointer_cast<InterfaceType>(driver);
            
            // Cache the successful driver
            std::string type_key = get_interface_key<InterfaceType>();
            driver_cache_[type_key] = driver;
            
            return typed_driver;
        }
    }
    
    return nullptr;
}

} // namespace flight::hal
