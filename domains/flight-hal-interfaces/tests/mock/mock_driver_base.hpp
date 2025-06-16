/**
 * @file mock_driver_base.hpp
 * @brief Base Mock Driver Implementation
 * 
 * Provides base functionality for all mock HAL drivers including
 * behavior configuration, state tracking, error injection, and timing simulation.
 */

#pragma once

#include "mock_behavior_config.hpp"
#include "mock_state_tracker.hpp"
#include "../../include/flight/hal/core/hal_result.hpp"
#include "../../include/flight/hal/core/hal_capabilities.hpp"
#include "../../include/flight/hal/core/platform_detection.hpp"
#include <chrono>
#include <thread>
#include <string>
#include <memory>
#include <atomic>
#include <mutex>

namespace flight::hal::test {

/**
 * @brief Base class for all mock HAL drivers
 * 
 * Provides comprehensive mock functionality including:
 * - Configurable behavior simulation
 * - Error injection capabilities  
 * - State tracking and verification
 * - Performance simulation
 * - Resource management simulation
 */
template<typename InterfaceType>
class MockDriverBase : public InterfaceType, public ICapabilityProvider {
public:
    MockDriverBase(const std::string& driver_name) 
        : driver_name_(driver_name), initialized_(false), active_(false) {
        
        // Initialize platform info with mock data
        platform_info_.platform_name = "Mock Platform";
        platform_info_.architecture = "Mock-64";
        platform_info_.performance_tier = PerformanceTier::Limited;
        platform_info_.total_memory = 64 * 1024 * 1024; // 64MB
        platform_info_.cpu_cores = 1;
        platform_info_.has_fpu = true;
        platform_info_.has_simd = false;
    }
    
    virtual ~MockDriverBase() = default;
    
    // === Mock Control Interface ===
    
    /**
     * @brief Get behavior configuration for modification
     */
    MockBehaviorConfig& get_behavior_config() { return behavior_config_; }
    
    /**
     * @brief Get state tracker for verification
     */
    const MockStateTracker& get_state_tracker() const { return state_tracker_; }
    MockStateTracker& get_state_tracker() { return state_tracker_; }
    
    /**
     * @brief Reset all mock state
     */
    void reset_mock() {
        behavior_config_.reset();
        state_tracker_.reset();
    }
    
    /**
     * @brief Configure method behavior
     */
    void configure_method(const std::string& method_name, const MethodBehaviorConfig& config) {
        behavior_config_.set_method_behavior(method_name, config);
    }
    
    /**
     * @brief Set default behavior for all methods
     */
    void set_default_behavior(const MethodBehaviorConfig& config) {
        behavior_config_.set_default_behavior(config);
    }
    
    /**
     * @brief Get mock statistics
     */
    MockStateTracker::Statistics get_mock_statistics() const {
        return state_tracker_.get_statistics();
    }
    
    /**
     * @brief Generate mock report
     */
    std::string generate_mock_report() const {
        return state_tracker_.generate_report();
    }
    
    // === HAL Interface Implementation ===
    
    std::string_view get_driver_name() const override {
        return driver_name_;
    }
    
    int get_priority() const override {
        return 10; // Standard mock priority
    }
    
    HALResult<void> initialize() override {
        TRACK_CALL(state_tracker_, "initialize");
        
        if (auto result = execute_with_behavior("initialize"); !result) {
            TRACK_ERROR(result.error().message());
            return result;
        }
        
        initialized_ = true;
        active_ = true;
        
        record_state_transition("driver_state", "uninitialized", "initialized", "initialize");
        
        return HALResult<void>::success();
    }
    
    HALResult<void> shutdown() override {
        TRACK_CALL(state_tracker_, "shutdown");
        
        if (!initialized_) {
            auto error = errors::invalid_state(1, "Driver not initialized");
            TRACK_ERROR(error.message());
            return HALResult<void>::error(error);
        }
        
        if (auto result = execute_with_behavior("shutdown"); !result) {
            TRACK_ERROR(result.error().message());
            return result;
        }
        
        active_ = false;
        initialized_ = false;
        
        record_state_transition("driver_state", "initialized", "shutdown", "shutdown");
        
        return HALResult<void>::success();
    }
    
    bool is_active() const override {
        return active_;
    }
    
    bool is_available() const override {
        return true; // Mock drivers are always available
    }
    
    std::string_view get_version() const override {
        return "1.0.0-mock";
    }
    
    // === ICapabilityProvider Implementation ===
    
    bool supports_capability(HALCapability capability) const override {
        return (capability_mask_ & static_cast<uint32_t>(capability)) != 0;
    }
    
    uint32_t get_capability_mask() const override {
        return capability_mask_;
    }
    
    std::vector<HALCapability> get_capabilities() const override {
        std::vector<HALCapability> caps;
        for (uint32_t i = 0; i < 32; ++i) {
            if (capability_mask_ & (1u << i)) {
                caps.push_back(static_cast<HALCapability>(1u << i));
            }
        }
        return caps;
    }
    
    PerformanceTier get_performance_tier() const override {
        return platform_info_.performance_tier;
    }
    
    const PlatformInfo& get_platform_info() const override {
        return platform_info_;
    }
    
    bool has_fallback(HALCapability capability) const override {
        return true; // Mock drivers can simulate fallbacks
    }

protected:
    /**
     * @brief Execute method with configured behavior (failure injection, timing, etc.)
     */
    HALResult<void> execute_with_behavior(const std::string& method_name) {
        return execute_with_behavior_impl<void>(method_name, []() { return HALResult<void>::success(); });
    }
    
    /**
     * @brief Execute method with configured behavior and return type
     */
    template<typename T>
    HALResult<T> execute_with_behavior(const std::string& method_name, std::function<HALResult<T>()> impl) {
        return execute_with_behavior_impl<T>(method_name, impl);
    }
    
    /**
     * @brief Record a resource event
     */
    void record_resource_event(ResourceEvent::Type type, uint32_t resource_id, 
                             const std::string& resource_type, size_t size = 0,
                             const std::string& debug_name = "") {
        ResourceEvent event;
        event.type = type;
        event.resource_id = resource_id;
        event.resource_type = resource_type;
        event.timestamp = std::chrono::high_resolution_clock::now();
        event.size = size;
        event.debug_name = debug_name;
        
        state_tracker_.record_resource_event(event);
    }
    
    /**
     * @brief Record a state transition
     */
    void record_state_transition(const std::string& state_name, 
                                const std::string& from_state,
                                const std::string& to_state,
                                const std::string& trigger_method = "") {
        StateTransition transition;
        transition.state_name = state_name;
        transition.from_state = from_state;
        transition.to_state = to_state;
        transition.timestamp = std::chrono::high_resolution_clock::now();
        transition.trigger_method = trigger_method;
        
        state_tracker_.record_state_transition(transition);
    }
    
    /**
     * @brief Set capability mask for this driver
     */
    void set_capability_mask(uint32_t mask) {
        capability_mask_ = mask;
    }
    
    /**
     * @brief Add capability to this driver
     */
    void add_capability(HALCapability capability) {
        capability_mask_ |= static_cast<uint32_t>(capability);
    }
    
    /**
     * @brief Remove capability from this driver
     */
    void remove_capability(HALCapability capability) {
        capability_mask_ &= ~static_cast<uint32_t>(capability);
    }
    
    /**
     * @brief Set platform information
     */
    void set_platform_info(const PlatformInfo& info) {
        platform_info_ = info;
    }
    
    /**
     * @brief Check if driver is initialized (for derived classes)
     */
    bool is_initialized() const { return initialized_; }

protected:
    /**
     * @brief Internal implementation of behavior execution
     */
    template<typename T>
    HALResult<T> execute_with_behavior_impl(const std::string& method_name, 
                                          std::function<HALResult<T>()> impl) {
        
        // Increment call count
        behavior_config_.increment_call_count(method_name);
        uint32_t call_count = behavior_config_.get_call_count(method_name);
        
        // Check resource availability
        size_t resource_usage = behavior_config_.get_resource_usage(method_name, call_count);
        if (resource_usage > 0 && !behavior_config_.has_resources(method_name, resource_usage)) {
            auto error = errors::resource_exhausted(1, "Mock resource limit exceeded");
            return HALResult<T>::error(error);
        }
        
        // Check if should fail
        if (behavior_config_.should_fail(method_name, call_count)) {
            return HALResult<T>::error(behavior_config_.get_error(method_name).error());
        }
        
        // Simulate delay
        auto delay = behavior_config_.get_delay(method_name, call_count);
        if (delay > std::chrono::microseconds{0}) {
            std::this_thread::sleep_for(delay);
        }
        
        // Consume resources
        if (resource_usage > 0) {
            behavior_config_.consume_resources(method_name, resource_usage);
        }
        
        // Execute the actual implementation
        if constexpr (std::is_same_v<T, void>) {
            impl();
            return HALResult<void>::success();
        } else {
            return impl();
        }
    }
    
    std::string driver_name_;
    std::atomic<bool> initialized_;
    std::atomic<bool> active_;
    
    MockBehaviorConfig behavior_config_;
    MockStateTracker state_tracker_;
    
    uint32_t capability_mask_ = 0;
    PlatformInfo platform_info_;
};

/**
 * @brief Helper macros for mock driver implementation
 */
#define MOCK_METHOD_IMPL(method_name, return_type, ...) \
    do { \
        TRACK_CALL(this->state_tracker_, method_name); \
        TRACK_PARAM("this", static_cast<void*>(this)); \
        if (auto result = this->execute_with_behavior(method_name); !result) { \
            TRACK_ERROR(result.error().message()); \
            return HALResult<return_type>::error(result.error()); \
        } \
        __VA_ARGS__ \
    } while(0)

#define MOCK_METHOD_WITH_PARAMS(method_name, return_type, params_code, ...) \
    do { \
        TRACK_CALL(this->state_tracker_, method_name); \
        params_code \
        if (auto result = this->execute_with_behavior(method_name); !result) { \
            TRACK_ERROR(result.error().message()); \
            return HALResult<return_type>::error(result.error()); \
        } \
        __VA_ARGS__ \
    } while(0)

#define MOCK_VOID_METHOD(method_name, ...) \
    MOCK_METHOD_IMPL(method_name, void, __VA_ARGS__)

#define MOCK_RESOURCE_CREATE(type, id, size, name) \
    this->record_resource_event(ResourceEvent::Type::Created, id, type, size, name)

#define MOCK_RESOURCE_DESTROY(type, id) \
    this->record_resource_event(ResourceEvent::Type::Destroyed, id, type)

#define MOCK_STATE_TRANSITION(state_name, from, to) \
    this->record_state_transition(state_name, from, to, __func__)

} // namespace flight::hal::test
