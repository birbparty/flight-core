/**
 * @file event_system.cpp
 * @brief Flight HAL Event System Implementation
 */

#include "../../include/flight/hal/core/event_system.hpp"
#include "../../include/flight/hal/core/hal_logging.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>

using namespace flight::hal;

namespace flight::hal::core {

namespace {
    // Convert event to message for use with CrossDriverMessenger
    coordination::Message event_to_message(const Event& event) {
        coordination::MessageHeader header;
        header.id = event.metadata().event_id;
        header.type = coordination::MessageType::Event;
        
        // Map severity to message priority
        switch (event.metadata().severity) {
            case EventSeverity::Fatal:
            case EventSeverity::Critical:
                header.priority = coordination::MessagePriority::Critical;
                break;
            case EventSeverity::Error:
                header.priority = coordination::MessagePriority::High;
                break;
            case EventSeverity::Warning:
                header.priority = coordination::MessagePriority::Normal;
                break;
            default:
                header.priority = coordination::MessagePriority::Low;
                break;
        }
        
        header.sender_id = event.metadata().source_id;
        header.timestamp = event.metadata().timestamp;
        
        // Create a simple event payload for the message system
        auto payload = std::make_unique<coordination::PerformancePayload>();
        payload->driver_id = event.metadata().source_id;
        
        return coordination::Message(header, std::move(payload));
    }
}

// Event Implementation
Event::Event(const EventMetadata& metadata, const EventData& data)
    : metadata_(metadata), data_(data) {}

std::chrono::milliseconds Event::get_age() const noexcept {
    auto now = std::chrono::steady_clock::now();
    auto duration = now - metadata_.timestamp;
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration);
}

// EventFilter Implementation
EventFilter::EventFilter(const std::unordered_set<EventCategory>& categories,
                        EventSeverity min_severity)
    : categories_(categories)
    , min_severity_(min_severity)
    , match_all_categories_(categories.empty())
    , match_all_sources_(true) {}

void EventFilter::add_category(EventCategory category) {
    categories_.insert(category);
    match_all_categories_ = false;
}

void EventFilter::remove_category(EventCategory category) {
    categories_.erase(category);
    match_all_categories_ = categories_.empty();
}

void EventFilter::add_source(const std::string& source_id) {
    source_ids_.insert(source_id);
    match_all_sources_ = false;
}

void EventFilter::remove_source(const std::string& source_id) {
    source_ids_.erase(source_id);
    match_all_sources_ = source_ids_.empty();
}

void EventFilter::add_attribute_filter(const std::string& key, const std::string& value) {
    attribute_filters_[key] = value;
}

bool EventFilter::matches(const Event& event) const {
    // Check severity
    if (!event.meets_severity(min_severity_)) {
        return false;
    }
    
    // Check category
    if (!match_all_categories_ && categories_.count(event.metadata().category) == 0) {
        return false;
    }
    
    // Check source
    if (!match_all_sources_ && source_ids_.count(event.metadata().source_id) == 0) {
        return false;
    }
    
    // Check attributes
    for (const auto& [key, value] : attribute_filters_) {
        auto it = event.metadata().attributes.find(key);
        if (it == event.metadata().attributes.end()) {
            return false; // Required attribute not found
        }
        
        // If filter value is empty, just check existence
        if (!value.empty() && it->second != value) {
            return false; // Attribute value doesn't match
        }
    }
    
    return true;
}

bool EventFilter::is_empty() const noexcept {
    return match_all_categories_ && 
           match_all_sources_ && 
           min_severity_ == EventSeverity::Debug &&
           attribute_filters_.empty();
}

// HALEventSystem Implementation
HALEventSystem& HALEventSystem::instance() {
    static HALEventSystem instance;
    return instance;
}

HALResult<void> HALEventSystem::initialize() {
    if (initialized_.load()) {
        return HALResult<void>::success();
    }
    
    try {
        // Clear statistics
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_ = EventSystemStats{};
        }
        
        // Start event processing thread
        shutdown_requested_.store(false);
        processing_thread_ = std::thread(&HALEventSystem::event_processing_thread, this);
        
        // Initialize platform sources
        {
            std::lock_guard<std::mutex> lock(platform_sources_mutex_);
            for (auto& source : platform_sources_) {
                auto result = source->initialize();
                if (!result.is_ok()) {
                    std::string error_msg = "Failed to initialize platform event source: " + 
                                          std::string(result.error().message());
                    HAL_LOG_MESSAGE(LogLevel::Warning, error_msg.c_str());
                }
            }
        }
        
        initialized_.store(true);
        HAL_LOG_MESSAGE(LogLevel::Info, "HAL Event System initialized successfully");
        
        return HALResult<void>::success();
        
    } catch (const std::exception& e) {
        return HALResult<void>::error(errors::initialization_failed(1, e.what()));
    }
}

HALResult<void> HALEventSystem::shutdown() {
    if (!initialized_.load()) {
        return HALResult<void>::success();
    }
    
    try {
        // Signal shutdown
        shutdown_requested_.store(true);
        
        // Stop platform sources
        {
            std::lock_guard<std::mutex> lock(platform_sources_mutex_);
            for (auto& source : platform_sources_) {
                source->shutdown();
            }
        }
        
        // Wait for processing thread to finish
        if (processing_thread_.joinable()) {
            processing_thread_.join();
        }
        
        // Clear subscriptions
        {
            std::lock_guard<std::mutex> lock(subscriptions_mutex_);
            subscriptions_.clear();
        }
        
        initialized_.store(false);
        HAL_LOG_MESSAGE(LogLevel::Info, "HAL Event System shutdown successfully");
        
        return HALResult<void>::success();
        
    } catch (const std::exception& e) {
        return HALResult<void>::error(errors::internal_error(2, e.what()));
    }
}

HALResult<EventSubscription> HALEventSystem::subscribe(
    std::shared_ptr<IEventSubscriber> subscriber,
    const EventFilter& filter) {
    
    if (!subscriber) {
        return HALResult<EventSubscription>::error(errors::invalid_parameter(1, "subscriber"));
    }
    
    if (!initialized_.load()) {
        return HALResult<EventSubscription>::error(errors::invalid_state(1, "not_initialized"));
    }
    
    try {
        auto subscription_id = generate_subscription_id();
        
        SubscriptionInfo info;
        info.subscriber = subscriber;
        info.filter = filter;
        info.id = subscription_id;
        info.created_at = std::chrono::steady_clock::now();
        
        {
            std::lock_guard<std::mutex> lock(subscriptions_mutex_);
            subscriptions_[subscription_id] = std::move(info);
        }
        
        // Update statistics
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.active_subscriptions++;
        }
        
        std::string debug_msg = "Event subscription created: ID=" + std::to_string(subscription_id) +
                               ", Subscriber=" + subscriber->get_subscriber_id();
        HAL_LOG_DEBUG(debug_msg.c_str());
        
        return HALResult<EventSubscription>::success(EventSubscription(subscription_id));
        
    } catch (const std::exception& e) {
        return HALResult<EventSubscription>::error(errors::internal_error(3, e.what()));
    }
}

HALResult<void> HALEventSystem::unsubscribe(const EventSubscription& subscription) {
    if (!subscription.is_valid()) {
        return HALResult<void>::error(errors::invalid_parameter(2, "subscription"));
    }
    
    if (!initialized_.load()) {
        return HALResult<void>::error(errors::invalid_state(2, "not_initialized"));
    }
    
    try {
        {
            std::lock_guard<std::mutex> lock(subscriptions_mutex_);
            auto it = subscriptions_.find(subscription.id());
            if (it != subscriptions_.end()) {
                std::string debug_msg = "Event subscription removed: ID=" + std::to_string(subscription.id()) +
                                       ", Subscriber=" + it->second.subscriber->get_subscriber_id();
                HAL_LOG_DEBUG(debug_msg.c_str());
                subscriptions_.erase(it);
                
                // Update statistics
                {
                    std::lock_guard<std::mutex> stats_lock(stats_mutex_);
                    if (stats_.active_subscriptions > 0) {
                        stats_.active_subscriptions--;
                    }
                }
            }
        }
        
        return HALResult<void>::success();
        
    } catch (const std::exception& e) {
        return HALResult<void>::error(errors::internal_error(4, e.what()));
    }
}

HALResult<void> HALEventSystem::publish_event(const Event& event) {
    if (!initialized_.load()) {
        return HALResult<void>::error(errors::invalid_state(3, "not_initialized"));
    }
    
    try {
        // Convert event to message and enqueue
        auto message = event_to_message(event);
        
        if (!event_queue_.try_enqueue(std::move(message))) {
            // Queue is full, update statistics
            {
                std::lock_guard<std::mutex> lock(stats_mutex_);
                stats_.events_dropped++;
                stats_.queue_overflow_count++;
            }
            
            std::string warning_msg = "Event queue overflow, event dropped: " + event.metadata().description;
            HAL_LOG_MESSAGE(LogLevel::Warning, warning_msg.c_str());
            
            return HALResult<void>::error(errors::resource_exhausted(1, "event_queue"));
        }
        
        // Update statistics
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            stats_.events_generated++;
        }
        
        return HALResult<void>::success();
        
    } catch (const std::exception& e) {
        return HALResult<void>::error(errors::internal_error(5, e.what()));
    }
}

HALResult<void> HALEventSystem::publish_hardware_event(
    const std::string& source_id,
    const HardwareEventData& event_data,
    EventSeverity severity,
    const std::string& description) {
    
    Event event = create_hardware_event(source_id, event_data.type, severity, description);
    event.set_data(event_data);
    
    return publish_event(event);
}

HALResult<void> HALEventSystem::publish_system_event(
    const std::string& source_id,
    const SystemEventData& event_data,
    EventSeverity severity,
    const std::string& description) {
    
    Event event = create_system_event(source_id, event_data.type, severity, description);
    event.set_data(event_data);
    
    return publish_event(event);
}

HALResult<void> HALEventSystem::publish_driver_event(
    const std::string& source_id,
    const DriverEventData& event_data,
    EventSeverity severity,
    const std::string& description) {
    
    Event event = create_driver_event(source_id, event_data.type, severity, description);
    event.set_data(event_data);
    
    return publish_event(event);
}

HALResult<void> HALEventSystem::register_platform_source(
    std::unique_ptr<IPlatformEventSource> source) {
    
    if (!source) {
        return HALResult<void>::error(errors::invalid_parameter(3, "source"));
    }
    
    try {
        // Set event callback to forward events to our system
        source->set_event_callback([this](const Event& event) {
            auto result = this->publish_event(event);
            if (!result.is_ok()) {
                std::string warning_msg = "Failed to publish platform event: " + 
                                        std::string(result.error().message());
                HAL_LOG_MESSAGE(LogLevel::Warning, warning_msg.c_str());
            }
            
            // Update platform event statistics
            {
                std::lock_guard<std::mutex> lock(stats_mutex_);
                stats_.platform_events++;
            }
        });
        
        // Initialize if system is already initialized
        if (initialized_.load()) {
            auto result = source->initialize();
            if (!result.is_ok()) {
                return result;
            }
        }
        
        {
            std::lock_guard<std::mutex> lock(platform_sources_mutex_);
            platform_sources_.push_back(std::move(source));
        }
        
        HAL_LOG_MESSAGE(LogLevel::Info, "Platform event source registered");
        return HALResult<void>::success();
        
    } catch (const std::exception& e) {
        return HALResult<void>::error(errors::internal_error(6, e.what()));
    }
}

EventSystemStats HALEventSystem::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void HALEventSystem::clear_stats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = EventSystemStats{};
}

void HALEventSystem::set_batching_enabled(bool enabled, size_t batch_size, 
                                         uint32_t batch_timeout_ms) {
    batching_enabled_.store(enabled);
    batch_size_.store(batch_size);
    batch_timeout_ms_.store(batch_timeout_ms);
    
    std::string info_msg = "Event batching " + std::string(enabled ? "enabled" : "disabled") +
                          ", batch_size=" + std::to_string(batch_size) +
                          ", timeout=" + std::to_string(batch_timeout_ms) + "ms";
    HAL_LOG_MESSAGE(LogLevel::Info, info_msg.c_str());
}

void HALEventSystem::event_processing_thread() {
    HAL_LOG_DEBUG("Event processing thread started");
    
    coordination::Message message;
    std::vector<coordination::Message> batch;
    batch.reserve(batch_size_.load());
    
    auto last_batch_time = std::chrono::steady_clock::now();
    
    while (!shutdown_requested_.load()) {
        try {
            bool has_message = event_queue_.try_dequeue(message);
            
            if (has_message) {
                if (batching_enabled_.load()) {
                    batch.push_back(std::move(message));
                    
                    auto now = std::chrono::steady_clock::now();
                    auto time_since_batch = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now - last_batch_time).count();
                    
                    // Process batch if it's full or timeout reached
                    if (batch.size() >= batch_size_.load() || 
                        static_cast<uint32_t>(time_since_batch) >= batch_timeout_ms_.load()) {
                        
                        for (const auto& batch_msg : batch) {
                            // Create event from message and dispatch
                            Event event;  // Convert message back to event
                            dispatch_event(event);
                        }
                        
                        batch.clear();
                        last_batch_time = now;
                    }
                } else {
                    // Process immediately without batching
                    Event event;  // Convert message back to event
                    dispatch_event(event);
                }
            } else {
                // No messages available, process any pending batched events
                if (batching_enabled_.load() && !batch.empty()) {
                    auto now = std::chrono::steady_clock::now();
                    auto time_since_batch = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now - last_batch_time).count();
                    
                    if (static_cast<uint32_t>(time_since_batch) >= batch_timeout_ms_.load()) {
                        for (const auto& batch_msg : batch) {
                            Event event;  // Convert message back to event
                            dispatch_event(event);
                        }
                        
                        batch.clear();
                        last_batch_time = now;
                    }
                }
                
                // Sleep briefly to avoid busy waiting
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            
        } catch (const std::exception& e) {
            std::string error_msg = "Error in event processing thread: " + std::string(e.what());
            HAL_LOG_MESSAGE(LogLevel::Error, error_msg.c_str());
        }
    }
    
    // Process any remaining batched events on shutdown
    if (!batch.empty()) {
        std::string debug_msg = "Processing " + std::to_string(batch.size()) + 
                               " remaining batched events on shutdown";
        HAL_LOG_DEBUG(debug_msg.c_str());
        
        for (const auto& batch_msg : batch) {
            Event event;  // Convert message back to event
            dispatch_event(event);
        }
    }
    
    HAL_LOG_DEBUG("Event processing thread stopped");
}

void HALEventSystem::dispatch_event(const Event& event) {
    auto start_time = std::chrono::steady_clock::now();
    size_t subscribers_notified = 0;
    size_t events_filtered = 0;
    
    try {
        std::vector<std::shared_ptr<IEventSubscriber>> matching_subscribers;
        
        // Find matching subscribers
        {
            std::lock_guard<std::mutex> lock(subscriptions_mutex_);
            for (const auto& [id, info] : subscriptions_) {
                if (info.filter.matches(event)) {
                    matching_subscribers.push_back(info.subscriber);
                    subscribers_notified++;
                } else {
                    events_filtered++;
                }
            }
        }
        
        // Notify subscribers
        for (auto& subscriber : matching_subscribers) {
            try {
                if (subscriber->wants_async_delivery()) {
                    // Async delivery (could be improved with thread pool)
                    std::thread([subscriber, event]() {
                        try {
                            subscriber->on_event(event);
                        } catch (const std::exception& e) {
                            std::string error_msg = "Exception in async event subscriber '" + 
                                                   subscriber->get_subscriber_id() + "': " + e.what();
                            HAL_LOG_MESSAGE(LogLevel::Error, error_msg.c_str());
                        }
                    }).detach();
                } else {
                    // Synchronous delivery
                    subscriber->on_event(event);
                }
            } catch (const std::exception& e) {
                std::string error_msg = "Exception in event subscriber '" + 
                                       subscriber->get_subscriber_id() + "': " + e.what();
                HAL_LOG_MESSAGE(LogLevel::Error, error_msg.c_str());
            }
        }
        
    } catch (const std::exception& e) {
        std::string error_msg = "Error dispatching event: " + std::string(e.what());
        HAL_LOG_MESSAGE(LogLevel::Error, error_msg.c_str());
    }
    
    // Update statistics
    auto end_time = std::chrono::steady_clock::now();
    auto dispatch_time = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time).count() / 1000.0;
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.events_dispatched++;
        stats_.events_filtered += events_filtered;
        
        // Update timing statistics
        if (dispatch_time > stats_.peak_dispatch_time_ms) {
            stats_.peak_dispatch_time_ms = dispatch_time;
        }
        
        // Running average
        stats_.average_dispatch_time_ms = 
            (stats_.average_dispatch_time_ms * (stats_.events_dispatched - 1) + dispatch_time) / 
            stats_.events_dispatched;
    }
}

// Helper Functions
Event create_hardware_event(const std::string& source_id,
                           HardwareEventType type,
                           EventSeverity severity,
                           const std::string& description) {
    EventMetadata metadata;
    metadata.event_id = 0; // Will be assigned by event system
    metadata.category = EventCategory::Hardware;
    metadata.severity = severity;
    metadata.source_id = source_id;
    metadata.description = description;
    
    HardwareEventData data;
    data.type = type;
    
    return Event(metadata, data);
}

Event create_system_event(const std::string& source_id,
                         SystemEventType type,
                         EventSeverity severity,
                         const std::string& description) {
    EventMetadata metadata;
    metadata.event_id = 0; // Will be assigned by event system
    metadata.category = EventCategory::System;
    metadata.severity = severity;
    metadata.source_id = source_id;
    metadata.description = description;
    
    SystemEventData data;
    data.type = type;
    
    return Event(metadata, data);
}

Event create_driver_event(const std::string& source_id,
                         DriverEventType type,
                         EventSeverity severity,
                         const std::string& description) {
    EventMetadata metadata;
    metadata.event_id = 0; // Will be assigned by event system
    metadata.category = EventCategory::Driver;
    metadata.severity = severity;
    metadata.source_id = source_id;
    metadata.description = description;
    
    DriverEventData data;
    data.type = type;
    
    return Event(metadata, data);
}

} // namespace flight::hal::core
