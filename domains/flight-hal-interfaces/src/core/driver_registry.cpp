/**
 * @file driver_registry.cpp
 * @brief Flight HAL Driver Registry Implementation
 * 
 * Enhanced implementation with multi-driver support, capability matching,
 * and advanced selection algorithms.
 */

#include "flight/hal/core/driver_registry.hpp"
#include <algorithm>
#include <sstream>

namespace flight::hal {

DriverRegistry& DriverRegistry::instance() {
    static DriverRegistry instance_;
    return instance_;
}

HALResult<void> DriverRegistry::initialize_all() {
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    
    std::vector<std::string> failed_drivers;
    
    for (auto& [type_key, driver_list] : drivers_) {
        for (auto& entry : driver_list) {
            auto driver = ensure_driver_created(entry);
            if (driver && !driver->is_active()) {
                auto result = driver->initialize();
                if (!result) {
                    failed_drivers.push_back(std::string(entry.info.name));
                }
            }
        }
    }
    
    if (!failed_drivers.empty()) {
        std::stringstream ss;
        ss << "Failed to initialize drivers: ";
        for (size_t i = 0; i < failed_drivers.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << failed_drivers[i];
        }
        return HALResult<void>::error(errors::internal_error(2, ss.str().c_str()));
    }
    
    return HALResult<void>::success();
}

void DriverRegistry::shutdown_all() {
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    
    for (auto& [type_key, driver_list] : drivers_) {
        for (auto& entry : driver_list) {
            if (entry.driver && entry.driver->is_active()) {
                entry.driver->shutdown();
            }
        }
    }
    
    // Clear all caches
    driver_cache_.clear();
}

size_t DriverRegistry::driver_count() const {
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    
    size_t total = 0;
    for (const auto& [type_key, driver_list] : drivers_) {
        total += driver_list.size();
    }
    
    return total;
}

std::vector<std::string> DriverRegistry::get_registered_interfaces() const {
    std::shared_lock<std::shared_mutex> lock(registry_mutex_);
    
    std::vector<std::string> interfaces;
    interfaces.reserve(drivers_.size());
    
    for (const auto& [type_key, driver_list] : drivers_) {
        if (!driver_list.empty()) {
            // Use the interface name from the first driver in the list
            interfaces.push_back(driver_list[0].driver ? 
                                std::string(driver_list[0].driver->get_interface_name()) :
                                type_key);
        }
    }
    
    std::sort(interfaces.begin(), interfaces.end());
    return interfaces;
}

bool DriverRegistry::matches_requirements(const DriverEntry& entry, 
                                         const CapabilityRequirements& requirements) const {
    // Check required capabilities
    if ((entry.info.capabilities & requirements.required_capabilities) != requirements.required_capabilities) {
        return false;
    }
    
    // Check performance tier
    if (entry.info.performance_tier < requirements.minimum_performance) {
        return false;
    }
    
    // Check memory overhead
    if (entry.info.memory_overhead > requirements.max_memory_overhead) {
        return false;
    }
    
    // Check hot swap requirement
    if (requirements.require_hot_swap && !entry.info.supports_hot_swap) {
        return false;
    }
    
    return true;
}

void DriverRegistry::rank_drivers(std::vector<DriverEntry*>& drivers,
                                 const CapabilityRequirements* requirements) const {
    std::sort(drivers.begin(), drivers.end(),
        [requirements](const DriverEntry* a, const DriverEntry* b) {
            // Primary sort: priority (higher is better)
            if (a->info.priority != b->info.priority) {
                return a->info.priority > b->info.priority;
            }
            
            // Secondary sort: performance tier (higher is better)
            if (a->info.performance_tier != b->info.performance_tier) {
                return a->info.performance_tier > b->info.performance_tier;
            }
            
            // Tertiary sort: memory overhead (lower is better)
            if (a->info.memory_overhead != b->info.memory_overhead) {
                return a->info.memory_overhead < b->info.memory_overhead;
            }
            
            // Quaternary sort: preferred capabilities match (if requirements provided)
            if (requirements) {
                uint32_t a_preferred_match = a->info.capabilities & requirements->preferred_capabilities;
                uint32_t b_preferred_match = b->info.capabilities & requirements->preferred_capabilities;
                
                // Count number of matching preferred capabilities
                auto count_bits = [](uint32_t n) {
                    int count = 0;
                    while (n) {
                        count += n & 1;
                        n >>= 1;
                    }
                    return count;
                };
                
                int a_match_count = count_bits(a_preferred_match);
                int b_match_count = count_bits(b_preferred_match);
                
                if (a_match_count != b_match_count) {
                    return a_match_count > b_match_count;
                }
            }
            
            // Final sort: alphabetical by name for deterministic ordering
            return a->info.name < b->info.name;
        });
}

std::shared_ptr<IHALInterface> DriverRegistry::ensure_driver_created(DriverEntry& entry) {
    if (entry.driver) {
        return entry.driver;
    }
    
    if (entry.is_factory_based && entry.factory) {
        try {
            auto new_driver = entry.factory();
            if (new_driver) {
                entry.driver = std::shared_ptr<IHALInterface>(new_driver.release());
                entry.is_initialized = false; // Will need initialization
                return entry.driver;
            }
        } catch (const std::exception&) {
            // Factory failed, driver remains null
        }
    }
    
    return nullptr;
}

} // namespace flight::hal
