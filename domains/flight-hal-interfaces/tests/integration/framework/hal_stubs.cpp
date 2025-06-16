/**
 * @file hal_stubs.cpp
 * @brief Stub implementations for HAL functions to enable standalone integration testing
 */

#include "flight/hal/core/hal_result.hpp"
#include "flight/hal/core/hal_error.hpp"
#include "flight/hal/core/platform_detection.hpp"
#include "flight/hal/coordination/resource_handle.hpp"
#include "flight/hal/coordination/cross_driver_messenger.hpp"
#include "flight/hal/coordination/deadlock_prevention.hpp"
#include <memory>
#include <string>
#include <chrono>

namespace flight::hal::coordination {

// Stub implementations for ResourceRegistry methods
ResourceRegistry& ResourceRegistry::instance() {
    static ResourceRegistry reg;
    return reg;
}

HALResult<ResourceHandle> ResourceRegistry::find_resource(const std::string&) const {
    return HALResult<ResourceHandle>::error(
        HALError(HALErrorCategory::Resource, 1, "Stub implementation", nullptr));
}

// Stub implementations for DeadlockPrevention methods
DeadlockPrevention& DeadlockPrevention::instance() {
    static DeadlockPrevention dp;
    return dp;
}

HALResult<void> DeadlockPrevention::initialize() {
    return HALResult<void>::success();
}

HALResult<void> DeadlockPrevention::shutdown() {
    return HALResult<void>::success();
}

// Stub implementations for CrossDriverMessenger methods
CrossDriverMessenger& CrossDriverMessenger::instance() {
    static CrossDriverMessenger cdm;
    return cdm;
}

HALResult<void> CrossDriverMessenger::initialize() {
    return HALResult<void>::success();
}

HALResult<void> CrossDriverMessenger::shutdown() {
    return HALResult<void>::success();
}

HALResult<void> CrossDriverMessenger::register_handler(const std::string&, std::shared_ptr<IMessageHandler>) {
    return HALResult<void>::success();
}

HALResult<void> CrossDriverMessenger::unregister_handler(const std::string&) {
    return HALResult<void>::success();
}

HALResult<Message> CrossDriverMessenger::send_request(Message&& request, std::chrono::milliseconds) {
    return HALResult<Message>::error(
        HALError(HALErrorCategory::Resource, 1, "Stub implementation", nullptr));
}

CrossDriverMessenger::MessageStats CrossDriverMessenger::get_stats() const {
    return MessageStats{};
}

// Stub implementation for Message constructor
Message::Message(const MessageHeader&, std::unique_ptr<IMessagePayload>) {
    // Stub implementation - just initialize to empty state
}

} // namespace flight::hal::coordination

namespace flight::hal {

// Stub implementation for RuntimePlatformDetector
PlatformInfo RuntimePlatformDetector::detect_platform_info() {
    PlatformInfo info{};
    // Set basic properties that exist in PlatformInfo
    return info;
}

} // namespace flight::hal
