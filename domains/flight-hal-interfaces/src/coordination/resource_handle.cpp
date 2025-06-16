/**
 * @file resource_handle.cpp
 * @brief Flight HAL Resource Handle System Implementation
 */

#include "flight/hal/coordination/resource_handle.hpp"
#include "flight/hal/core/hal_error.hpp"
#include <mutex>
#include <algorithm>

namespace flight::hal::coordination {

// Static member initialization
std::atomic<uint64_t> ResourceHandle::next_id_{1};

ResourceHandle::ResourceHandle(const std::string& name, const ResourceMetadata& metadata)
    : id_(next_id_.fetch_add(1, std::memory_order_relaxed))
    , version_(1)
    , name_(name)
    , metadata_(metadata)
{
}

HALResult<void> ResourceHandle::update_metadata(const ResourceMetadata& new_metadata) {
    metadata_ = new_metadata;
    increment_version();
    return HALResult<void>::success();
}

// ResourceRegistry implementation
ResourceRegistry& ResourceRegistry::instance() {
    static ResourceRegistry instance;
    return instance;
}

HALResult<ResourceHandle> ResourceRegistry::register_resource(const std::string& name,
                                                             const ResourceMetadata& metadata) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    // Check if resource name already exists
    auto name_it = resources_by_name_.find(name);
    if (name_it != resources_by_name_.end()) {
        return HALResult<ResourceHandle>::error(
            HALError(HALErrorCategory::Resource, 1, "Resource name already exists", name.c_str()));
    }
    
    // Create new resource handle
    ResourceHandle handle(name, metadata);
    uint64_t id = handle.id();
    
    // Add to registries
    resources_by_id_[id] = handle;
    resources_by_name_[name] = id;
    resources_by_type_[metadata.type].push_back(id);
    
    return HALResult<ResourceHandle>::success(std::move(handle));
}

HALResult<void> ResourceRegistry::unregister_resource(const ResourceHandle& handle) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    uint64_t id = handle.id();
    
    // Find and remove from ID registry
    auto id_it = resources_by_id_.find(id);
    if (id_it == resources_by_id_.end()) {
        return HALResult<void>::error(
            HALError(HALErrorCategory::Resource, 2, "Resource not found", nullptr));
    }
    
    const ResourceHandle& stored_handle = id_it->second;
    
    // Remove from name registry
    resources_by_name_.erase(stored_handle.name());
    
    // Remove from type registry
    auto type_it = resources_by_type_.find(stored_handle.metadata().type);
    if (type_it != resources_by_type_.end()) {
        auto& type_vec = type_it->second;
        type_vec.erase(std::remove(type_vec.begin(), type_vec.end(), id), type_vec.end());
        
        // Remove empty type entries
        if (type_vec.empty()) {
            resources_by_type_.erase(type_it);
        }
    }
    
    // Remove from ID registry
    resources_by_id_.erase(id_it);
    
    return HALResult<void>::success();
}

HALResult<ResourceHandle> ResourceRegistry::find_resource(const std::string& name) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    auto name_it = resources_by_name_.find(name);
    if (name_it == resources_by_name_.end()) {
        return HALResult<ResourceHandle>::error(
            HALError(HALErrorCategory::Resource, 3, "Resource not found by name", name.c_str()));
    }
    
    uint64_t id = name_it->second;
    auto id_it = resources_by_id_.find(id);
    
    if (id_it == resources_by_id_.end()) {
        // This should not happen, but handle it gracefully
        return HALResult<ResourceHandle>::error(
            HALError(HALErrorCategory::Internal, 1, "Registry inconsistency", name.c_str()));
    }
    
    return HALResult<ResourceHandle>::success(id_it->second);
}

std::vector<ResourceHandle> ResourceRegistry::get_resources_by_type(ResourceType type) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    std::vector<ResourceHandle> result;
    
    auto type_it = resources_by_type_.find(type);
    if (type_it != resources_by_type_.end()) {
        const auto& id_vec = type_it->second;
        result.reserve(id_vec.size());
        
        for (uint64_t id : id_vec) {
            auto id_it = resources_by_id_.find(id);
            if (id_it != resources_by_id_.end()) {
                result.push_back(id_it->second);
            }
        }
    }
    
    return result;
}

HALResult<ResourceMetadata> ResourceRegistry::get_metadata(const ResourceHandle& handle) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    auto id_it = resources_by_id_.find(handle.id());
    if (id_it == resources_by_id_.end()) {
        return HALResult<ResourceMetadata>::error(
            HALError(HALErrorCategory::Resource, 4, "Resource not found for metadata", nullptr));
    }
    
    return HALResult<ResourceMetadata>::success(id_it->second.metadata());
}

HALResult<void> ResourceRegistry::update_metadata(const ResourceHandle& handle,
                                                 const ResourceMetadata& metadata) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    auto id_it = resources_by_id_.find(handle.id());
    if (id_it == resources_by_id_.end()) {
        return HALResult<void>::error(
            HALError(HALErrorCategory::Resource, 5, "Resource not found for update", nullptr));
    }
    
    // Update the stored handle
    ResourceHandle& stored_handle = id_it->second;
    auto result = stored_handle.update_metadata(metadata);
    
    // If the type changed, update the type registry
    if (stored_handle.metadata().type != metadata.type) {
        // Remove from old type
        auto old_type_it = resources_by_type_.find(stored_handle.metadata().type);
        if (old_type_it != resources_by_type_.end()) {
            auto& type_vec = old_type_it->second;
            type_vec.erase(std::remove(type_vec.begin(), type_vec.end(), handle.id()), type_vec.end());
            
            if (type_vec.empty()) {
                resources_by_type_.erase(old_type_it);
            }
        }
        
        // Add to new type
        resources_by_type_[metadata.type].push_back(handle.id());
    }
    
    return result;
}

size_t ResourceRegistry::get_resource_count() const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    return resources_by_id_.size();
}

HALResult<void> ResourceRegistry::clear_all_resources() {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    resources_by_id_.clear();
    resources_by_name_.clear();
    resources_by_type_.clear();
    
    return HALResult<void>::success();
}

} // namespace flight::hal::coordination
