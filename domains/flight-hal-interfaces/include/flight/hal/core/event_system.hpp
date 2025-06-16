/**
 * @file event_system.hpp
 * @brief Flight HAL Event System
 * 
 * Comprehensive event system for HAL notifications including hardware events,
 * system events, and driver events with efficient routing and async handling.
 */

#pragma once

#include "hal_result.hpp"
#include "hal_error.hpp"
#include "platform_detection.hpp"
#include "../coordination/cross_driver_messenger.hpp"
#include <cstdint>
#include <string>
#include <memory>
#include <atomic>
#include <functional>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <chrono>
#include <variant>
#include <thread>
#include <condition_variable>

namespace flight::hal::core {

/**
 * @brief Event categories for classification and routing
 */
enum class EventCategory : uint8_t {
    Hardware = 0,    ///< Hardware-related events
    System = 1,      ///< System-level events
    Driver = 2,      ///< Driver-specific events
    Application = 3, ///< Application-level events
    Performance = 4, ///< Performance monitoring events
    Resource = 5,    ///< Resource management events
    Security = 6     ///< Security-related events
};

/**
 * @brief Event severity levels for prioritization
 */
enum class EventSeverity : uint8_t {
    Debug = 0,       ///< Debug information
    Info = 1,        ///< Informational events
    Warning = 2,     ///< Warning conditions
    Error = 3,       ///< Error conditions
    Critical = 4,    ///< Critical system events
    Fatal = 5        ///< Fatal system events
};

/**
 * @brief Hardware event types
 */
enum class HardwareEventType : uint16_t {
    // Device lifecycle
    DeviceConnected = 100,
    DeviceDisconnected = 101,
    DeviceReady = 102,
    DeviceError = 103,
    DeviceRemoved = 104,
    
    // Thermal events
    ThermalThreshold = 200,
    ThermalCritical = 201,
    ThermalShutdown = 202,
    CoolingFailed = 203,
    
    // Power events
    PowerLow = 300,
    PowerCritical = 301,
    PowerStateChanged = 302,
    BatteryLow = 303,
    ACPowerLost = 304,
    
    // Performance events
    PerformanceThrottling = 400,
    ResourceContention = 401,
    BandwidthLimited = 402
};

/**
 * @brief System event types
 */
enum class SystemEventType : uint16_t {
    // Memory events
    MemoryPressure = 500,
    MemoryLow = 501,
    MemoryCritical = 502,
    AllocationFailed = 503,
    
    // Resource events
    ResourceExhausted = 600,
    CPULoadHigh = 601,
    DiskSpaceLow = 602,
    NetworkCongestion = 603,
    
    // Configuration events
    ConfigurationChanged = 700,
    DriverUpdated = 701,
    SystemModeChanged = 702,
    
    // Platform events
    OSNotification = 800,
    UserSessionChanged = 801,
    DisplayModeChanged = 802
};

/**
 * @brief Driver event types
 */
enum class DriverEventType : uint16_t {
    // Lifecycle events
    DriverInitialized = 900,
    DriverShutdown = 901,
    DriverError = 902,
    DriverRecovery = 903,
    
    // State changes
    StateChanged = 1000,
    ModeSwitch = 1001,
    ConfigurationUpdated = 1002,
    
    // Performance events
    LatencySpike = 1100,
    ThroughputDrop = 1101,
    TimeoutOccurred = 1102,
    
    // Resource events
    ResourceAcquired = 1200,
    ResourceReleased = 1201,
    ResourceConflict = 1202
};

/**
 * @brief Event metadata containing common event information
 */
struct EventMetadata {
    uint64_t event_id;                              ///< Unique event identifier
    EventCategory category;                         ///< Event category
    EventSeverity severity;                         ///< Event severity
    std::string source_id;                          ///< Event source identifier
    std::chrono::steady_clock::time_point timestamp; ///< Event timestamp
    std::string description;                        ///< Human-readable description
    std::unordered_map<std::string, std::string> attributes; ///< Custom attributes
    
    EventMetadata()
        : event_id(0)
        , category(EventCategory::System)
        , severity(EventSeverity::Info)
        , timestamp(std::chrono::steady_clock::now())
    {}
};

/**
 * @brief Hardware event data
 */
struct HardwareEventData {
    HardwareEventType type;
    std::string device_id;
    std::string device_name;
    std::variant<double, int64_t, std::string> value;
    std::string unit;
    
    HardwareEventData() : type(HardwareEventType::DeviceReady) {}
};

/**
 * @brief System event data
 */
struct SystemEventData {
    SystemEventType type;
    std::string subsystem;
    std::variant<double, int64_t, std::string> value;
    std::string unit;
    std::unordered_map<std::string, std::string> system_info;
    
    SystemEventData() : type(SystemEventType::OSNotification) {}
};

/**
 * @brief Driver event data
 */
struct DriverEventData {
    DriverEventType type;
    std::string driver_id;
    std::string driver_name;
    std::string old_state;
    std::string new_state;
    std::variant<double, int64_t, std::string> metric_value;
    std::string error_message;
    
    DriverEventData() : type(DriverEventType::DriverInitialized) {}
};

/**
 * @brief Generic event payload for different event types
 */
using EventData = std::variant<HardwareEventData, SystemEventData, DriverEventData>;

/**
 * @brief Complete event with metadata and data
 */
class Event {
public:
    /**
     * @brief Create empty event
     */
    Event() = default;
    
    /**
     * @brief Create event with metadata and data
     * @param metadata Event metadata
     * @param data Event data
     */
    Event(const EventMetadata& metadata, const EventData& data);
    
    /**
     * @brief Get event metadata
     * @return Const reference to metadata
     */
    const EventMetadata& metadata() const noexcept { return metadata_; }
    
    /**
     * @brief Get mutable event metadata
     * @return Reference to metadata
     */
    EventMetadata& metadata() noexcept { return metadata_; }
    
    /**
     * @brief Get event data
     * @return Const reference to event data
     */
    const EventData& data() const noexcept { return data_; }
    
    /**
     * @brief Get typed event data
     * @tparam DataType Type to extract from variant
     * @return Pointer to typed data or nullptr if wrong type
     */
    template<typename DataType>
    const DataType* data_as() const noexcept;
    
    /**
     * @brief Set event data
     * @param data New event data
     */
    void set_data(const EventData& data) { data_ = data; }
    
    /**
     * @brief Check if event matches category
     * @param category Category to check
     * @return true if event matches category
     */
    bool matches_category(EventCategory category) const noexcept {
        return metadata_.category == category;
    }
    
    /**
     * @brief Check if event matches severity level or higher
     * @param min_severity Minimum severity level
     * @return true if event meets severity threshold
     */
    bool meets_severity(EventSeverity min_severity) const noexcept {
        return static_cast<uint8_t>(metadata_.severity) >= static_cast<uint8_t>(min_severity);
    }
    
    /**
     * @brief Get event age
     * @return Duration since event was created
     */
    std::chrono::milliseconds get_age() const noexcept;

private:
    EventMetadata metadata_;
    EventData data_;
};

/**
 * @brief Event filter for subscription-based event routing
 */
class EventFilter {
public:
    /**
     * @brief Create empty filter (matches all events)
     */
    EventFilter() = default;
    
    /**
     * @brief Create filter with category and severity constraints
     * @param categories Set of categories to match
     * @param min_severity Minimum severity level
     */
    EventFilter(const std::unordered_set<EventCategory>& categories,
                EventSeverity min_severity = EventSeverity::Debug);
    
    /**
     * @brief Add category to filter
     * @param category Category to include
     */
    void add_category(EventCategory category);
    
    /**
     * @brief Remove category from filter
     * @param category Category to exclude
     */
    void remove_category(EventCategory category);
    
    /**
     * @brief Set minimum severity level
     * @param severity Minimum severity
     */
    void set_min_severity(EventSeverity severity) { min_severity_ = severity; }
    
    /**
     * @brief Add source ID to filter
     * @param source_id Source to include
     */
    void add_source(const std::string& source_id);
    
    /**
     * @brief Remove source ID from filter
     * @param source_id Source to exclude
     */
    void remove_source(const std::string& source_id);
    
    /**
     * @brief Add attribute filter
     * @param key Attribute key
     * @param value Attribute value (empty means any value)
     */
    void add_attribute_filter(const std::string& key, const std::string& value = "");
    
    /**
     * @brief Check if event matches filter
     * @param event Event to test
     * @return true if event passes filter
     */
    bool matches(const Event& event) const;
    
    /**
     * @brief Check if filter is empty (matches all events)
     * @return true if filter has no constraints
     */
    bool is_empty() const noexcept;

private:
    std::unordered_set<EventCategory> categories_;
    EventSeverity min_severity_ = EventSeverity::Debug;
    std::unordered_set<std::string> source_ids_;
    std::unordered_map<std::string, std::string> attribute_filters_;
    bool match_all_categories_ = true;
    bool match_all_sources_ = true;
};

/**
 * @brief Event subscription interface
 */
class IEventSubscriber {
public:
    virtual ~IEventSubscriber() = default;
    
    /**
     * @brief Handle incoming event
     * @param event The event to handle
     */
    virtual void on_event(const Event& event) = 0;
    
    /**
     * @brief Get subscriber identifier
     * @return Subscriber ID string
     */
    virtual std::string get_subscriber_id() const = 0;
    
    /**
     * @brief Get event filter for this subscriber
     * @return Event filter
     */
    virtual EventFilter get_event_filter() const = 0;
    
    /**
     * @brief Check if subscriber wants async delivery
     * @return true for async delivery, false for sync
     */
    virtual bool wants_async_delivery() const { return true; }
};

/**
 * @brief Event subscription handle for managing subscriptions
 */
class EventSubscription {
public:
    /**
     * @brief Create invalid subscription
     */
    EventSubscription() : id_(0), valid_(false) {}
    
    /**
     * @brief Create valid subscription
     * @param id Subscription ID
     */
    explicit EventSubscription(uint64_t id) : id_(id), valid_(true) {}
    
    /**
     * @brief Check if subscription is valid
     * @return true if subscription is active
     */
    bool is_valid() const noexcept { return valid_; }
    
    /**
     * @brief Get subscription ID
     * @return Subscription identifier
     */
    uint64_t id() const noexcept { return id_; }
    
    /**
     * @brief Invalidate subscription
     */
    void invalidate() noexcept { valid_ = false; }

private:
    uint64_t id_;
    bool valid_;
};

/**
 * @brief Platform-specific event source interface
 */
class IPlatformEventSource {
public:
    virtual ~IPlatformEventSource() = default;
    
    /**
     * @brief Initialize platform event source
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> initialize() = 0;
    
    /**
     * @brief Shutdown platform event source
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> shutdown() = 0;
    
    /**
     * @brief Start monitoring for events
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> start_monitoring() = 0;
    
    /**
     * @brief Stop monitoring for events
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> stop_monitoring() = 0;
    
    /**
     * @brief Get supported event categories
     * @return Set of supported categories
     */
    virtual std::unordered_set<EventCategory> get_supported_categories() const = 0;
    
    /**
     * @brief Set event callback for platform events
     * @param callback Function to call when events occur
     */
    virtual void set_event_callback(std::function<void(const Event&)> callback) = 0;
};

/**
 * @brief Event statistics for monitoring system performance
 */
struct EventSystemStats {
    uint64_t events_generated;          ///< Total events generated
    uint64_t events_dispatched;         ///< Total events dispatched
    uint64_t events_dropped;            ///< Events dropped due to overflow
    uint64_t events_filtered;           ///< Events filtered out
    uint64_t active_subscriptions;      ///< Number of active subscriptions
    uint64_t platform_events;           ///< Events from platform sources
    double average_dispatch_time_ms;    ///< Average dispatch time
    double peak_dispatch_time_ms;       ///< Peak dispatch time
    uint64_t queue_overflow_count;      ///< Number of queue overflows
    
    EventSystemStats() {
        std::memset(this, 0, sizeof(EventSystemStats));
    }
};

/**
 * @brief Main HAL Event System
 */
class HALEventSystem {
public:
    /**
     * @brief Get singleton instance
     * @return Reference to global event system
     */
    static HALEventSystem& instance();
    
    /**
     * @brief Initialize event system
     * @return HALResult indicating success or failure
     */
    HALResult<void> initialize();
    
    /**
     * @brief Shutdown event system
     * @return HALResult indicating success or failure
     */
    HALResult<void> shutdown();
    
    /**
     * @brief Subscribe to events with filter
     * @param subscriber Event subscriber
     * @param filter Event filter (optional)
     * @return HALResult containing subscription handle
     */
    HALResult<EventSubscription> subscribe(std::shared_ptr<IEventSubscriber> subscriber,
                                          const EventFilter& filter = EventFilter{});
    
    /**
     * @brief Unsubscribe from events
     * @param subscription Subscription to cancel
     * @return HALResult indicating success or failure
     */
    HALResult<void> unsubscribe(const EventSubscription& subscription);
    
    /**
     * @brief Publish event to all matching subscribers
     * @param event Event to publish
     * @return HALResult indicating success or failure
     */
    HALResult<void> publish_event(const Event& event);
    
    /**
     * @brief Publish hardware event
     * @param source_id Source identifier
     * @param event_data Hardware event data
     * @param severity Event severity
     * @param description Optional description
     * @return HALResult indicating success or failure
     */
    HALResult<void> publish_hardware_event(const std::string& source_id,
                                          const HardwareEventData& event_data,
                                          EventSeverity severity = EventSeverity::Info,
                                          const std::string& description = "");
    
    /**
     * @brief Publish system event
     * @param source_id Source identifier
     * @param event_data System event data
     * @param severity Event severity
     * @param description Optional description
     * @return HALResult indicating success or failure
     */
    HALResult<void> publish_system_event(const std::string& source_id,
                                        const SystemEventData& event_data,
                                        EventSeverity severity = EventSeverity::Info,
                                        const std::string& description = "");
    
    /**
     * @brief Publish driver event
     * @param source_id Source identifier
     * @param event_data Driver event data
     * @param severity Event severity
     * @param description Optional description
     * @return HALResult indicating success or failure
     */
    HALResult<void> publish_driver_event(const std::string& source_id,
                                        const DriverEventData& event_data,
                                        EventSeverity severity = EventSeverity::Info,
                                        const std::string& description = "");
    
    /**
     * @brief Register platform event source
     * @param source Platform event source
     * @return HALResult indicating success or failure
     */
    HALResult<void> register_platform_source(std::unique_ptr<IPlatformEventSource> source);
    
    /**
     * @brief Get event system statistics
     * @return Current statistics
     */
    EventSystemStats get_stats() const;
    
    /**
     * @brief Clear event system statistics
     */
    void clear_stats();
    
    /**
     * @brief Enable/disable event batching for performance
     * @param enabled true to enable batching
     * @param batch_size Maximum events per batch
     * @param batch_timeout_ms Maximum time to wait for batch
     */
    void set_batching_enabled(bool enabled, size_t batch_size = 10, 
                             uint32_t batch_timeout_ms = 5);

    /**
     * @brief Generate unique event ID (public for helper functions)
     * @return Unique event identifier
     */
    uint64_t generate_event_id() noexcept {
        return next_event_id_.fetch_add(1, std::memory_order_relaxed);
    }

private:
    HALEventSystem() = default;
    ~HALEventSystem() = default;
    HALEventSystem(const HALEventSystem&) = delete;
    HALEventSystem& operator=(const HALEventSystem&) = delete;
    
    /**
     * @brief Event processing thread function
     */
    void event_processing_thread();
    
    /**
     * @brief Dispatch event to matching subscribers
     * @param event Event to dispatch
     */
    void dispatch_event(const Event& event);
    
    /**
     * @brief Generate unique subscription ID
     * @return Unique subscription identifier
     */
    uint64_t generate_subscription_id() noexcept {
        return next_subscription_id_.fetch_add(1, std::memory_order_relaxed);
    }
    
    // System state
    std::atomic<bool> initialized_{false};
    std::atomic<bool> shutdown_requested_{false};
    std::thread processing_thread_;
    
    // Event queue
    static constexpr size_t EVENT_QUEUE_CAPACITY = 2048;
    coordination::SPSCMessageQueue<EVENT_QUEUE_CAPACITY> event_queue_;
    
    // Subscriptions
    struct SubscriptionInfo {
        std::shared_ptr<IEventSubscriber> subscriber;
        EventFilter filter;
        uint64_t id;
        std::chrono::steady_clock::time_point created_at;
    };
    
    mutable std::mutex subscriptions_mutex_;
    std::unordered_map<uint64_t, SubscriptionInfo> subscriptions_;
    
    // Platform event sources
    mutable std::mutex platform_sources_mutex_;
    std::vector<std::unique_ptr<IPlatformEventSource>> platform_sources_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    EventSystemStats stats_;
    
    // Batching configuration
    std::atomic<bool> batching_enabled_{false};
    std::atomic<size_t> batch_size_{10};
    std::atomic<uint32_t> batch_timeout_ms_{5};
    
    // ID generation
    std::atomic<uint64_t> next_event_id_{1};
    std::atomic<uint64_t> next_subscription_id_{1};
};

// Template implementation
template<typename DataType>
const DataType* Event::data_as() const noexcept {
    return std::get_if<DataType>(&data_);
}

/**
 * @brief Helper function to create hardware events
 */
Event create_hardware_event(const std::string& source_id,
                           HardwareEventType type,
                           EventSeverity severity = EventSeverity::Info,
                           const std::string& description = "");

/**
 * @brief Helper function to create system events
 */
Event create_system_event(const std::string& source_id,
                         SystemEventType type,
                         EventSeverity severity = EventSeverity::Info,
                         const std::string& description = "");

/**
 * @brief Helper function to create driver events
 */
Event create_driver_event(const std::string& source_id,
                         DriverEventType type,
                         EventSeverity severity = EventSeverity::Info,
                         const std::string& description = "");

} // namespace flight::hal::core
