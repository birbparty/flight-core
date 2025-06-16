/**
 * @file mock_state_tracker.hpp
 * @brief Mock State Tracking and Verification System
 * 
 * Comprehensive state tracking for mock drivers to enable verification
 * of method calls, parameters, state transitions, and resource lifecycle.
 */

#pragma once

#include "../../include/flight/hal/core/hal_result.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include <chrono>
#include <any>
#include <typeinfo>
#include <sstream>
#include <mutex>
#include <thread>
#include <algorithm>

namespace flight::hal::test {

/**
 * @brief Method call record for tracking
 */
struct MethodCall {
    std::string method_name;
    std::chrono::high_resolution_clock::time_point timestamp;
    std::vector<std::any> parameters;
    std::vector<std::string> parameter_names;
    std::vector<std::type_info const*> parameter_types;
    std::any return_value;
    bool success;
    std::string error_message;
    
    /**
     * @brief Add a parameter to the call record
     */
    template<typename T>
    void add_parameter(const std::string& name, const T& value) {
        parameter_names.push_back(name);
        parameters.push_back(std::make_any<T>(value));
        parameter_types.push_back(&typeid(T));
    }
    
    /**
     * @brief Get parameter value by index
     */
    template<typename T>
    T get_parameter(size_t index) const {
        if (index >= parameters.size()) {
            throw std::out_of_range("Parameter index out of range");
        }
        return std::any_cast<T>(parameters[index]);
    }
    
    /**
     * @brief Get parameter value by name
     */
    template<typename T>
    T get_parameter(const std::string& name) const {
        auto it = std::find(parameter_names.begin(), parameter_names.end(), name);
        if (it == parameter_names.end()) {
            throw std::invalid_argument("Parameter name not found");
        }
        size_t index = std::distance(parameter_names.begin(), it);
        return get_parameter<T>(index);
    }
    
    /**
     * @brief Set return value
     */
    template<typename T>
    void set_return_value(const T& value) {
        return_value = std::make_any<T>(value);
    }
    
    /**
     * @brief Get return value
     */
    template<typename T>
    T get_return_value() const {
        return std::any_cast<T>(return_value);
    }
    
    /**
     * @brief Get string representation of the call
     */
    std::string to_string() const {
        std::ostringstream oss;
        oss << method_name << "(";
        for (size_t i = 0; i < parameter_names.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << parameter_names[i] << ": " << parameter_types[i]->name();
        }
        oss << ") -> " << (success ? "SUCCESS" : "FAILURE");
        if (!success && !error_message.empty()) {
            oss << " (" << error_message << ")";
        }
        return oss.str();
    }
};

/**
 * @brief Resource lifecycle event
 */
struct ResourceEvent {
    enum class Type {
        Created,
        Destroyed,
        Modified,
        Accessed
    };
    
    Type type;
    uint32_t resource_id;
    std::string resource_type;
    std::chrono::high_resolution_clock::time_point timestamp;
    size_t size;
    std::string debug_name;
    std::unordered_map<std::string, std::string> metadata;
    
    std::string to_string() const {
        std::ostringstream oss;
        const char* type_str[] = {"Created", "Destroyed", "Modified", "Accessed"};
        oss << type_str[static_cast<int>(type)] << " " << resource_type 
            << " [ID:" << resource_id << ", Size:" << size << "]";
        if (!debug_name.empty()) {
            oss << " '" << debug_name << "'";
        }
        return oss.str();
    }
};

/**
 * @brief State transition record
 */
struct StateTransition {
    std::string state_name;
    std::string from_state;
    std::string to_state;
    std::chrono::high_resolution_clock::time_point timestamp;
    std::string trigger_method;
    std::unordered_map<std::string, std::string> context;
    
    std::string to_string() const {
        std::ostringstream oss;
        oss << state_name << ": " << from_state << " -> " << to_state;
        if (!trigger_method.empty()) {
            oss << " (triggered by " << trigger_method << ")";
        }
        return oss.str();
    }
};

/**
 * @brief Mock state tracker for comprehensive verification
 */
class MockStateTracker {
public:
    MockStateTracker() = default;
    
    /**
     * @brief Record a method call
     */
    void record_call(const MethodCall& call) {
        std::lock_guard<std::mutex> lock(mutex_);
        method_calls_.push_back(call);
        method_call_counts_[call.method_name]++;
    }
    
    /**
     * @brief Begin recording a method call
     */
    MethodCall& begin_call(const std::string& method_name) {
        std::lock_guard<std::mutex> lock(mutex_);
        current_calls_[std::this_thread::get_id()] = MethodCall{};
        auto& call = current_calls_[std::this_thread::get_id()];
        call.method_name = method_name;
        call.timestamp = std::chrono::high_resolution_clock::now();
        call.success = true; // Assume success until proven otherwise
        return call;
    }
    
    /**
     * @brief End recording a method call
     */
    void end_call() {
        std::lock_guard<std::mutex> lock(mutex_);
        auto thread_id = std::this_thread::get_id();
        auto it = current_calls_.find(thread_id);
        if (it != current_calls_.end()) {
            method_calls_.push_back(it->second);
            method_call_counts_[it->second.method_name]++;
            current_calls_.erase(it);
        }
    }
    
    /**
     * @brief Record a resource event
     */
    void record_resource_event(const ResourceEvent& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        resource_events_.push_back(event);
        
        // Update resource tracking
        switch (event.type) {
            case ResourceEvent::Type::Created:
                active_resources_[event.resource_id] = event;
                break;
            case ResourceEvent::Type::Destroyed:
                active_resources_.erase(event.resource_id);
                break;
            default:
                break;
        }
    }
    
    /**
     * @brief Record a state transition
     */
    void record_state_transition(const StateTransition& transition) {
        std::lock_guard<std::mutex> lock(mutex_);
        state_transitions_.push_back(transition);
        current_states_[transition.state_name] = transition.to_state;
    }
    
    /**
     * @brief Get all recorded method calls
     */
    std::vector<MethodCall> get_method_calls() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return method_calls_;
    }
    
    /**
     * @brief Get method calls for specific method
     */
    std::vector<MethodCall> get_method_calls(const std::string& method_name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<MethodCall> filtered;
        for (const auto& call : method_calls_) {
            if (call.method_name == method_name) {
                filtered.push_back(call);
            }
        }
        return filtered;
    }
    
    /**
     * @brief Get call count for method
     */
    uint32_t get_call_count(const std::string& method_name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = method_call_counts_.find(method_name);
        return (it != method_call_counts_.end()) ? it->second : 0;
    }
    
    /**
     * @brief Get total call count
     */
    uint32_t get_total_call_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return static_cast<uint32_t>(method_calls_.size());
    }
    
    /**
     * @brief Get all resource events
     */
    std::vector<ResourceEvent> get_resource_events() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return resource_events_;
    }
    
    /**
     * @brief Get resource events for specific resource
     */
    std::vector<ResourceEvent> get_resource_events(uint32_t resource_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<ResourceEvent> filtered;
        for (const auto& event : resource_events_) {
            if (event.resource_id == resource_id) {
                filtered.push_back(event);
            }
        }
        return filtered;
    }
    
    /**
     * @brief Get currently active resources
     */
    std::vector<ResourceEvent> get_active_resources() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<ResourceEvent> active;
        for (const auto& pair : active_resources_) {
            active.push_back(pair.second);
        }
        return active;
    }
    
    /**
     * @brief Get state transitions
     */
    std::vector<StateTransition> get_state_transitions() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_transitions_;
    }
    
    /**
     * @brief Get current state for a state machine
     */
    std::string get_current_state(const std::string& state_name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = current_states_.find(state_name);
        return (it != current_states_.end()) ? it->second : "";
    }
    
    /**
     * @brief Check if method was called
     */
    bool was_called(const std::string& method_name) const {
        return get_call_count(method_name) > 0;
    }
    
    /**
     * @brief Check if method was called with specific parameters
     */
    template<typename... Args>
    bool was_called_with(const std::string& method_name, Args... args) const {
        auto calls = get_method_calls(method_name);
        for (const auto& call : calls) {
            if (matches_parameters(call, args...)) {
                return true;
            }
        }
        return false;
    }
    
    /**
     * @brief Verify call sequence
     */
    bool verify_call_sequence(const std::vector<std::string>& expected_sequence) const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (expected_sequence.size() > method_calls_.size()) {
            return false;
        }
        
        for (size_t i = 0; i < expected_sequence.size(); ++i) {
            if (method_calls_[i].method_name != expected_sequence[i]) {
                return false;
            }
        }
        return true;
    }
    
    /**
     * @brief Reset all tracking data
     */
    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        method_calls_.clear();
        method_call_counts_.clear();
        resource_events_.clear();
        active_resources_.clear();
        state_transitions_.clear();
        current_states_.clear();
        current_calls_.clear();
    }
    
    /**
     * @brief Get summary statistics
     */
    struct Statistics {
        uint32_t total_calls = 0;
        uint32_t successful_calls = 0;
        uint32_t failed_calls = 0;
        uint32_t resources_created = 0;
        uint32_t resources_destroyed = 0;
        uint32_t active_resources = 0;
        uint32_t state_transitions = 0;
        std::chrono::microseconds total_duration{0};
        std::chrono::microseconds average_call_duration{0};
    };
    
    Statistics get_statistics() const {
        std::lock_guard<std::mutex> lock(mutex_);
        Statistics stats;
        
        stats.total_calls = static_cast<uint32_t>(method_calls_.size());
        stats.active_resources = static_cast<uint32_t>(active_resources_.size());
        stats.state_transitions = static_cast<uint32_t>(state_transitions_.size());
        
        for (const auto& call : method_calls_) {
            if (call.success) {
                stats.successful_calls++;
            } else {
                stats.failed_calls++;
            }
        }
        
        for (const auto& event : resource_events_) {
            switch (event.type) {
                case ResourceEvent::Type::Created:
                    stats.resources_created++;
                    break;
                case ResourceEvent::Type::Destroyed:
                    stats.resources_destroyed++;
                    break;
                default:
                    break;
            }
        }
        
        if (!method_calls_.empty()) {
            auto start_time = method_calls_.front().timestamp;
            auto end_time = method_calls_.back().timestamp;
            stats.total_duration = std::chrono::duration_cast<std::chrono::microseconds>(
                end_time - start_time);
            stats.average_call_duration = std::chrono::microseconds(
                stats.total_duration.count() / stats.total_calls);
        }
        
        return stats;
    }
    
    /**
     * @brief Generate detailed report
     */
    std::string generate_report() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::ostringstream oss;
        
        auto stats = get_statistics();
        
        oss << "=== Mock Driver State Report ===\n";
        oss << "Total Calls: " << stats.total_calls << "\n";
        oss << "Successful: " << stats.successful_calls << "\n";
        oss << "Failed: " << stats.failed_calls << "\n";
        oss << "Resources Created: " << stats.resources_created << "\n";
        oss << "Resources Destroyed: " << stats.resources_destroyed << "\n";
        oss << "Active Resources: " << stats.active_resources << "\n";
        oss << "State Transitions: " << stats.state_transitions << "\n";
        oss << "Total Duration: " << stats.total_duration.count() << "μs\n";
        oss << "Average Call Duration: " << stats.average_call_duration.count() << "μs\n\n";
        
        oss << "=== Method Call Summary ===\n";
        for (const auto& pair : method_call_counts_) {
            oss << pair.first << ": " << pair.second << " calls\n";
        }
        
        if (!method_calls_.empty()) {
            oss << "\n=== Recent Method Calls ===\n";
            size_t start = method_calls_.size() > 10 ? method_calls_.size() - 10 : 0;
            for (size_t i = start; i < method_calls_.size(); ++i) {
                oss << method_calls_[i].to_string() << "\n";
            }
        }
        
        if (!active_resources_.empty()) {
            oss << "\n=== Active Resources ===\n";
            for (const auto& pair : active_resources_) {
                oss << pair.second.to_string() << "\n";
            }
        }
        
        return oss.str();
    }

private:
    /**
     * @brief Check if parameters match
     */
    template<typename... Args>
    bool matches_parameters(const MethodCall& call, Args... args) const {
        if (sizeof...(args) != call.parameters.size()) {
            return false;
        }
        
        return matches_parameters_impl(call, 0, args...);
    }
    
    template<typename T, typename... Rest>
    bool matches_parameters_impl(const MethodCall& call, size_t index, T&& first, Rest&&... rest) const {
        try {
            if (call.get_parameter<T>(index) != first) {
                return false;
            }
            return matches_parameters_impl(call, index + 1, rest...);
        } catch (...) {
            return false;
        }
    }
    
    bool matches_parameters_impl(const MethodCall& call, size_t index) const {
        return true; // Base case
    }
    
    mutable std::mutex mutex_;
    std::vector<MethodCall> method_calls_;
    std::unordered_map<std::string, uint32_t> method_call_counts_;
    std::vector<ResourceEvent> resource_events_;
    std::unordered_map<uint32_t, ResourceEvent> active_resources_;
    std::vector<StateTransition> state_transitions_;
    std::unordered_map<std::string, std::string> current_states_;
    std::unordered_map<std::thread::id, MethodCall> current_calls_;
};

/**
 * @brief RAII helper for automatic call tracking
 */
class CallTracker {
public:
    CallTracker(MockStateTracker& tracker, const std::string& method_name)
        : tracker_(tracker), call_(tracker.begin_call(method_name)) {}
    
    ~CallTracker() {
        tracker_.end_call();
    }
    
    MethodCall& get_call() { return call_; }
    
    template<typename T>
    void add_parameter(const std::string& name, const T& value) {
        call_.add_parameter(name, value);
    }
    
    template<typename T>
    void set_return_value(const T& value) {
        call_.set_return_value(value);
    }
    
    void set_success(bool success) {
        call_.success = success;
    }
    
    void set_error(const std::string& error) {
        call_.success = false;
        call_.error_message = error;
    }

private:
    MockStateTracker& tracker_;
    MethodCall& call_;
};

/**
 * @brief Macro for easy call tracking
 */
#define TRACK_CALL(tracker, method) \
    flight::hal::test::CallTracker _call_tracker(tracker, method)

#define TRACK_PARAM(name, value) \
    _call_tracker.add_parameter(name, value)

#define TRACK_RETURN(value) \
    _call_tracker.set_return_value(value)

#define TRACK_ERROR(error) \
    _call_tracker.set_error(error)

} // namespace flight::hal::test
