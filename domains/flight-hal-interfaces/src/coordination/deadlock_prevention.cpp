/**
 * @file deadlock_prevention.cpp
 * @brief Flight HAL Deadlock Prevention System Implementation
 */

#include "../include/flight/hal/coordination/deadlock_prevention.hpp"
#include "../include/flight/hal/core/hal_error.hpp"
#include <algorithm>
#include <sstream>

namespace flight::hal::coordination {

// DeadlockPrevention implementation
DeadlockPrevention& DeadlockPrevention::instance() {
    static DeadlockPrevention instance;
    return instance;
}

HALResult<void> DeadlockPrevention::initialize() {
    std::lock_guard<std::mutex> lock(system_mutex_);
    
    if (initialized_.load()) {
        return HALResult<void>::success();
    }
    
    // Initialize default resource ordering
    initialize_default_resource_orders();
    
    // Clear any existing state
    owned_resources_.clear();
    resource_owners_.clear();
    dependencies_.clear();
    dependency_graph_.clear();
    
    // Clear statistics
    {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_ = DeadlockStats{};
    }
    
    initialized_.store(true);
    
    return HALResult<void>::success();
}

HALResult<void> DeadlockPrevention::shutdown() {
    std::lock_guard<std::mutex> lock(system_mutex_);
    
    if (!initialized_.load()) {
        return HALResult<void>::success();
    }
    
    // Clear all state
    owned_resources_.clear();
    resource_owners_.clear();
    dependencies_.clear();
    dependency_graph_.clear();
    resource_orders_.clear();
    
    // Clear waiting requests
    while (!waiting_requests_.empty()) {
        waiting_requests_.pop();
    }
    requests_by_requester_.clear();
    
    initialized_.store(false);
    
    return HALResult<void>::success();
}

void DeadlockPrevention::initialize_default_resource_orders() {
    // Define standard resource ordering to prevent deadlocks
    // Lower numbers are acquired first
    resource_orders_[ResourceType::Memory] = ResourceOrder(ResourceType::Memory, 100, "Memory resources");
    resource_orders_[ResourceType::Hardware] = ResourceOrder(ResourceType::Hardware, 200, "Hardware resources");
    resource_orders_[ResourceType::Performance] = ResourceOrder(ResourceType::Performance, 300, "Performance resources");
    resource_orders_[ResourceType::Communication] = ResourceOrder(ResourceType::Communication, 400, "Communication resources");
    resource_orders_[ResourceType::Platform] = ResourceOrder(ResourceType::Platform, 500, "Platform resources");
    resource_orders_[ResourceType::Custom] = ResourceOrder(ResourceType::Custom, 1000, "Custom resources");
}

HALResult<void> DeadlockPrevention::register_resource_order(const ResourceOrder& order) {
    std::lock_guard<std::mutex> lock(system_mutex_);
    
    resource_orders_[order.type] = order;
    
    return HALResult<void>::success();
}

HALResult<bool> DeadlockPrevention::is_acquisition_safe(const ResourceRequest& request) {
    std::lock_guard<std::mutex> lock(system_mutex_);
    
    if (!initialized_.load()) {
        return HALResult<bool>::error(HALError(HALErrorCategory::Internal, 1, 
            "Deadlock prevention not initialized", nullptr));
    }
    
    // Check if resource is already owned by requester
    auto owner_it = resource_owners_.find(request.resource_handle.id());
    if (owner_it != resource_owners_.end() && owner_it->second == request.requester_id) {
        return HALResult<bool>::success(true); // Already owned, safe
    }
    
    // Check resource ordering constraints
    if (!check_resource_ordering(request.requester_id, request.resource_handle)) {
        return HALResult<bool>::success(false); // Would violate ordering
    }
    
    // Simulate the acquisition and check for cycles
    if (owner_it != resource_owners_.end()) {
        // Resource is owned by someone else, would create dependency
        const std::string& current_owner = owner_it->second;
        
        // Temporarily add the dependency edge
        auto temp_graph = dependency_graph_;
        temp_graph[request.requester_id].push_back(current_owner);
        
        // Check for cycles in the modified graph
        std::unordered_set<std::string> visited;
        std::unordered_set<std::string> recursion_stack;
        std::vector<std::string> cycle_path;
        
        for (const auto& [node, _] : temp_graph) {
            if (visited.find(node) == visited.end()) {
                if (dfs_cycle_detection(node, visited, recursion_stack, cycle_path)) {
                    return HALResult<bool>::success(false); // Would create deadlock
                }
            }
        }
    }
    
    return HALResult<bool>::success(true); // Safe to acquire
}

HALResult<void> DeadlockPrevention::request_resource_acquisition(const ResourceRequest& request) {
    std::lock_guard<std::mutex> lock(system_mutex_);
    
    if (!initialized_.load()) {
        return HALResult<void>::error(HALError(HALErrorCategory::Internal, 2, 
            "Deadlock prevention not initialized", nullptr));
    }
    
    {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.requests_processed++;
    }
    
    // Check if acquisition is safe
    auto safety_result = is_acquisition_safe(request);
    if (!safety_result) {
        return HALResult<void>::error(safety_result.error());
    }
    
    if (!safety_result.value()) {
        {
            std::lock_guard<std::mutex> stats_lock(stats_mutex_);
            stats_.requests_denied++;
        }
        
        return HALResult<void>::error(HALError(HALErrorCategory::Resource, 1, 
            "Resource acquisition would cause deadlock", request.resource_handle.name().c_str()));
    }
    
    // Check if resource is available
    auto owner_it = resource_owners_.find(request.resource_handle.id());
    if (owner_it != resource_owners_.end()) {
        // Resource is owned, add to waiting queue
        waiting_requests_.push(request);
        requests_by_requester_[request.requester_id].push_back(request);
        
        // Add dependency edge
        add_dependency(owner_it->second, request.requester_id, request.resource_handle);
        
        return HALResult<void>::error(HALError(HALErrorCategory::Resource, 2, 
            "Resource currently owned, added to waiting queue", nullptr));
    }
    
    // Resource is available, grant access
    resource_owners_[request.resource_handle.id()] = request.requester_id;
    owned_resources_[request.requester_id].push_back(request.resource_handle);
    
    return HALResult<void>::success();
}

HALResult<void> DeadlockPrevention::release_resource(const std::string& requester_id,
                                                    const ResourceHandle& resource_handle) {
    std::lock_guard<std::mutex> lock(system_mutex_);
    
    if (!initialized_.load()) {
        return HALResult<void>::error(HALError(HALErrorCategory::Internal, 3, 
            "Deadlock prevention not initialized", nullptr));
    }
    
    // Check if requester actually owns the resource
    auto owner_it = resource_owners_.find(resource_handle.id());
    if (owner_it == resource_owners_.end() || owner_it->second != requester_id) {
        return HALResult<void>::error(HALError(HALErrorCategory::Configuration, 1, 
            "Requester does not own resource", requester_id.c_str()));
    }
    
    // Remove from ownership tracking
    resource_owners_.erase(owner_it);
    
    auto& owned_list = owned_resources_[requester_id];
    owned_list.erase(std::remove_if(owned_list.begin(), owned_list.end(),
        [&resource_handle](const ResourceHandle& handle) {
            return handle.id() == resource_handle.id();
        }), owned_list.end());
    
    if (owned_list.empty()) {
        owned_resources_.erase(requester_id);
    }
    
    // Remove dependencies where this requester was blocking others
    dependencies_.erase(std::remove_if(dependencies_.begin(), dependencies_.end(),
        [&requester_id, &resource_handle](const ResourceDependency& dep) {
            return dep.from_requester == requester_id && 
                   dep.resource_handle.id() == resource_handle.id();
        }), dependencies_.end());
    
    // Update dependency graph
    dependency_graph_.erase(requester_id);
    for (auto& [node, neighbors] : dependency_graph_) {
        neighbors.erase(std::remove(neighbors.begin(), neighbors.end(), requester_id), 
                       neighbors.end());
    }
    
    // Check if any waiting requests can now be satisfied
    std::queue<ResourceRequest> remaining_requests;
    while (!waiting_requests_.empty()) {
        ResourceRequest request = waiting_requests_.front();
        waiting_requests_.pop();
        
        if (request.resource_handle.id() == resource_handle.id()) {
            // This request was waiting for the released resource
            auto acquisition_result = request_resource_acquisition(request);
            if (!acquisition_result) {
                // Still can't acquire, put back in queue
                remaining_requests.push(request);
            }
        } else {
            remaining_requests.push(request);
        }
    }
    
    waiting_requests_ = remaining_requests;
    
    return HALResult<void>::success();
}

HALResult<DeadlockPrevention::DeadlockInfo> DeadlockPrevention::detect_deadlock() {
    std::lock_guard<std::mutex> lock(system_mutex_);
    
    DeadlockInfo info;
    info.deadlock_detected = false;
    
    if (!initialized_.load()) {
        return HALResult<DeadlockInfo>::error(HALError(HALErrorCategory::Internal, 4, 
            "Deadlock prevention not initialized", nullptr));
    }
    
    // Perform cycle detection using DFS
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> recursion_stack;
    std::vector<std::string> cycle_path;
    
    for (const auto& [node, _] : dependency_graph_) {
        if (visited.find(node) == visited.end()) {
            if (dfs_cycle_detection(node, visited, recursion_stack, cycle_path)) {
                info.deadlock_detected = true;
                info.cycle_participants = cycle_path;
                
                // Find involved resources
                for (const auto& dep : dependencies_) {
                    auto it = std::find(cycle_path.begin(), cycle_path.end(), dep.from_requester);
                    if (it != cycle_path.end()) {
                        info.involved_resources.push_back(dep.resource_handle);
                    }
                }
                
                // Create description
                std::ostringstream desc;
                desc << "Deadlock detected involving requesters: ";
                for (size_t i = 0; i < cycle_path.size(); ++i) {
                    if (i > 0) desc << " -> ";
                    desc << cycle_path[i];
                }
                info.description = desc.str();
                
                break;
            }
        }
    }
    
    if (info.deadlock_detected) {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.deadlocks_detected++;
    }
    
    return HALResult<DeadlockInfo>::success(std::move(info));
}

HALResult<void> DeadlockPrevention::resolve_deadlock(const DeadlockInfo& deadlock_info) {
    std::lock_guard<std::mutex> lock(system_mutex_);
    
    if (!deadlock_info.deadlock_detected) {
        return HALResult<void>::success(); // Nothing to resolve
    }
    
    // Find the requester with lowest priority in the cycle
    std::string victim_requester;
    uint32_t lowest_priority = UINT32_MAX;
    
    for (const auto& participant : deadlock_info.cycle_participants) {
        // Calculate priority based on all resources owned by this requester
        uint32_t total_priority = 0;
        auto owned_it = owned_resources_.find(participant);
        if (owned_it != owned_resources_.end()) {
            for (const auto& resource : owned_it->second) {
                total_priority += calculate_preemption_priority(participant, resource);
            }
        }
        
        if (total_priority < lowest_priority) {
            lowest_priority = total_priority;
            victim_requester = participant;
        }
    }
    
    if (victim_requester.empty()) {
        return HALResult<void>::error(HALError(HALErrorCategory::Internal, 5, 
            "Could not identify victim for preemption", nullptr));
    }
    
    // Preempt the victim's resources
    auto owned_it = owned_resources_.find(victim_requester);
    if (owned_it != owned_resources_.end()) {
        std::vector<ResourceHandle> resources_to_release = owned_it->second;
        
        for (const auto& resource : resources_to_release) {
            auto release_result = release_resource(victim_requester, resource);
            if (!release_result) {
                return HALResult<void>::error(release_result.error());
            }
        }
    }
    
    {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.deadlocks_resolved++;
        stats_.preemptions_performed++;
    }
    
    return HALResult<void>::success();
}

std::unordered_map<std::string, std::vector<ResourceHandle>> 
DeadlockPrevention::get_resource_ownership() const {
    std::lock_guard<std::mutex> lock(system_mutex_);
    return owned_resources_;
}

std::vector<ResourceRequest> DeadlockPrevention::get_waiting_requests() const {
    std::lock_guard<std::mutex> lock(system_mutex_);
    
    std::vector<ResourceRequest> requests;
    std::queue<ResourceRequest> temp_queue = waiting_requests_;
    
    while (!temp_queue.empty()) {
        requests.push_back(temp_queue.front());
        temp_queue.pop();
    }
    
    return requests;
}

std::vector<ResourceDependency> DeadlockPrevention::get_dependencies() const {
    std::lock_guard<std::mutex> lock(system_mutex_);
    return dependencies_;
}

size_t DeadlockPrevention::cleanup_expired_items() {
    std::lock_guard<std::mutex> lock(system_mutex_);
    
    size_t cleaned_count = 0;
    auto now = std::chrono::steady_clock::now();
    
    // Clean expired waiting requests
    std::queue<ResourceRequest> valid_requests;
    while (!waiting_requests_.empty()) {
        ResourceRequest request = waiting_requests_.front();
        waiting_requests_.pop();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - request.request_time);
        
        if (elapsed < request.timeout) {
            valid_requests.push(request);
        } else {
            cleaned_count++;
            {
                std::lock_guard<std::mutex> stats_lock(stats_mutex_);
                stats_.timeouts_occurred++;
            }
        }
    }
    waiting_requests_ = valid_requests;
    
    // Clean expired dependencies (older than 30 seconds)
    auto max_age = std::chrono::seconds(30);
    dependencies_.erase(std::remove_if(dependencies_.begin(), dependencies_.end(),
        [now, max_age, &cleaned_count](const ResourceDependency& dep) {
            auto age = std::chrono::duration_cast<std::chrono::seconds>(now - dep.created_time);
            if (age > max_age) {
                cleaned_count++;
                return true;
            }
            return false;
        }), dependencies_.end());
    
    return cleaned_count;
}

DeadlockPrevention::DeadlockStats DeadlockPrevention::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void DeadlockPrevention::clear_stats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = DeadlockStats{};
}

bool DeadlockPrevention::check_resource_ordering(const std::string& requester_id,
                                                 const ResourceHandle& new_resource) const {
    auto owned_it = owned_resources_.find(requester_id);
    if (owned_it == owned_resources_.end()) {
        return true; // No existing resources, order is fine
    }
    
    uint32_t new_order = get_resource_order(new_resource.metadata().type);
    
    // Check that new resource order is >= all currently owned resources
    for (const auto& owned_resource : owned_it->second) {
        uint32_t owned_order = get_resource_order(owned_resource.metadata().type);
        if (new_order < owned_order) {
            return false; // Would violate ordering constraint
        }
    }
    
    return true;
}

bool DeadlockPrevention::dfs_cycle_detection(const std::string& start_node,
                                            std::unordered_set<std::string>& visited,
                                            std::unordered_set<std::string>& recursion_stack,
                                            std::vector<std::string>& cycle_path) const {
    visited.insert(start_node);
    recursion_stack.insert(start_node);
    cycle_path.push_back(start_node);
    
    auto neighbors_it = dependency_graph_.find(start_node);
    if (neighbors_it != dependency_graph_.end()) {
        for (const auto& neighbor : neighbors_it->second) {
            if (visited.find(neighbor) == visited.end()) {
                // Unvisited neighbor, recurse
                if (dfs_cycle_detection(neighbor, visited, recursion_stack, cycle_path)) {
                    return true;
                }
            } else if (recursion_stack.find(neighbor) != recursion_stack.end()) {
                // Back edge found - cycle detected
                cycle_path.push_back(neighbor);
                return true;
            }
        }
    }
    
    recursion_stack.erase(start_node);
    cycle_path.pop_back();
    return false;
}

uint32_t DeadlockPrevention::calculate_preemption_priority(const std::string& requester_id,
                                                          const ResourceHandle& resource_handle) const {
    uint32_t priority = resource_utils::calculate_priority_score(
        resource_handle.metadata().priority, resource_handle.metadata().flags);
    
    // Adjust based on how long requester has held the resource
    auto owner_it = resource_owners_.find(resource_handle.id());
    if (owner_it != resource_owners_.end() && owner_it->second == requester_id) {
        // Find the dependency that shows when this was acquired
        for (const auto& dep : dependencies_) {
            if (dep.from_requester == requester_id && 
                dep.resource_handle.id() == resource_handle.id()) {
                auto hold_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - dep.created_time);
                
                // Reduce priority for resources held longer (encourage preemption)
                priority = std::max(priority - static_cast<uint32_t>(hold_time.count() / 100), 1u);
                break;
            }
        }
    }
    
    return priority;
}

uint32_t DeadlockPrevention::get_resource_order(ResourceType type) const {
    auto order_it = resource_orders_.find(type);
    if (order_it != resource_orders_.end()) {
        return order_it->second.order_value;
    }
    
    // Default order for unknown types
    return 999;
}

void DeadlockPrevention::add_dependency(const std::string& from_requester,
                                       const std::string& to_requester,
                                       const ResourceHandle& resource_handle) {
    dependencies_.emplace_back(from_requester, to_requester, resource_handle);
    dependency_graph_[to_requester].push_back(from_requester);
}

void DeadlockPrevention::remove_dependency(const std::string& from_requester,
                                          const std::string& to_requester,
                                          const ResourceHandle& resource_handle) {
    dependencies_.erase(std::remove_if(dependencies_.begin(), dependencies_.end(),
        [&](const ResourceDependency& dep) {
            return dep.from_requester == from_requester && 
                   dep.to_requester == to_requester &&
                   dep.resource_handle.id() == resource_handle.id();
        }), dependencies_.end());
    
    auto graph_it = dependency_graph_.find(to_requester);
    if (graph_it != dependency_graph_.end()) {
        auto& neighbors = graph_it->second;
        neighbors.erase(std::remove(neighbors.begin(), neighbors.end(), from_requester), 
                       neighbors.end());
        
        if (neighbors.empty()) {
            dependency_graph_.erase(graph_it);
        }
    }
}

// ResourceLock implementation
ResourceLock::ResourceLock(const std::string& requester_id,
                          const ResourceHandle& resource_handle,
                          ResourcePriority priority,
                          std::chrono::milliseconds timeout,
                          bool exclusive)
    : requester_id_(requester_id)
    , resource_handle_(resource_handle)
    , locked_(false)
    , result_(HALResult<void>::success())
{
    ResourceRequest request(requester_id, resource_handle, priority, timeout, exclusive);
    result_ = DeadlockPrevention::instance().request_resource_acquisition(request);
    locked_ = result_.is_ok();
}

ResourceLock::~ResourceLock() {
    if (locked_) {
        release();
    }
}

ResourceLock::ResourceLock(ResourceLock&& other) noexcept
    : requester_id_(std::move(other.requester_id_))
    , resource_handle_(std::move(other.resource_handle_))
    , locked_(other.locked_)
    , result_(std::move(other.result_))
{
    other.locked_ = false;
}

ResourceLock& ResourceLock::operator=(ResourceLock&& other) noexcept {
    if (this != &other) {
        if (locked_) {
            release();
        }
        
        requester_id_ = std::move(other.requester_id_);
        resource_handle_ = std::move(other.resource_handle_);
        locked_ = other.locked_;
        result_ = std::move(other.result_);
        
        other.locked_ = false;
    }
    return *this;
}

HALResult<void> ResourceLock::release() {
    if (!locked_) {
        return HALResult<void>::success();
    }
    
    auto result = DeadlockPrevention::instance().release_resource(requester_id_, resource_handle_);
    if (result) {
        locked_ = false;
    }
    
    return result;
}

} // namespace flight::hal::coordination
